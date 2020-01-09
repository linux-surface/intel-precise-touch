use std::io::Read;
use std::sync::Arc;
use std::sync::Mutex;
use std::convert::{TryFrom, TryInto};

use gtk::prelude::*;
use gdk::prelude::*;
use gio::prelude::*;

use cairo::ImageSurface;
use cairo::Format;
use gtk::ApplicationWindow;
use gtk::DrawingArea;
use gdk::WindowState;

mod interface;
use interface::TouchRawDataHeader;
use interface::TouchDataType;
use interface::TouchFrameType;
use interface::mem::PackedDataStruct;

mod device;
use device::Device;


struct StylusData {
    pub x: u16,
    pub y: u16,
    pub proximity: bool,
    pub pressure: u16,
}

struct TouchData {
    pub width: u8,
    pub height: u8,
    pub heatmap: Vec<u8>,
}

enum Message {
    Redraw,
}


#[derive(Clone)]
struct CommonState {
    stylus: Arc<Mutex<StylusData>>,
    touch: Arc<Mutex<TouchData>>,
}

struct TxState {
    chan_tx: glib::Sender<Message>,
    state: CommonState,
}

impl TxState {
    fn push_stylus_update(&self, data: StylusData) {
        *self.state.stylus.lock().unwrap() = data;
        let _ = self.chan_tx.send(Message::Redraw);
    }

    fn push_touch_update(&self, width: u8, height: u8, heatmap: &[u8]) {
        {
            let mut touch = self.state.touch.lock().unwrap();
            touch.width = width;
            touch.height = height;
            touch.heatmap.resize(heatmap.len(), 0);
            touch.heatmap.copy_from_slice(heatmap);
        }
        let _ = self.chan_tx.send(Message::Redraw);
    }
}

struct RxState {
    chan_rx: glib::Receiver<Message>,
    state: CommonState,
}

fn setup_shared_state() -> (TxState, RxState) {
    let (sender, receiver) = glib::MainContext::channel(glib::PRIORITY_DEFAULT);

    let stylus = StylusData { x: 0, y: 0, proximity: false, pressure: 0 };

    let touch = TouchData {
        width: 72,
        height: 48,
        heatmap: vec![0x80; 72 * 48],
    };

    let state = CommonState {
        stylus: Arc::new(Mutex::new(stylus)),
        touch: Arc::new(Mutex::new(touch)),
    };

    let tx = TxState {
        chan_tx: sender,
        state: state.clone(),
    };

    let rx = RxState {
        chan_rx: receiver,
        state,
    };

    (tx, rx)
}


fn handle_touch_frame(tx: &TxState, data: &[u8]) {
    let height = data[44];
    let width  = data[45];
    let size =  u16::from_le_bytes(data[154..156].try_into().unwrap());

    if height as u16 * width as u16 != size {
        println!("warning: touch data sizes do not match");
        return;
    }

    if size == 0 {
        println!("warning: zero-sized heatmap");
        return;
    }

    let heatmap = &data[156..(156 + size as usize)];
    tx.push_touch_update(width, height, heatmap);
}

fn handle_stylus_report(tx: &TxState, stylus: &interface::StylusData) {
    tx.push_stylus_update(StylusData {
        x: stylus.x,
        y: stylus.y,
        pressure: stylus.pressure,
        proximity: (stylus.mode & interface::STYLUS_REPORT_MODE_PROXIMITY) != 0,
    });
}

fn handle_stylus_frame(tx: &TxState, data: &[u8]) {
    for i in 0..data[32] as usize {
        let len = std::mem::size_of::<interface::StylusData>();
        let index = 40 + i * len;
        let report = interface::StylusData::ref_from_bytes(&data[index..index+len]).unwrap();

        handle_stylus_report(tx, report);
    }
}

fn handle_touch_data_frame(tx: &TxState, data: &[u8]) {
    use pretty_hex::PrettyHex;

    // Data:
    // xx xx                        u16 (counter)
    // 01 00                        u16
    // 01 00 00 00 / 02 00 00 00    u32
    // 00 00 00 00                  u32
    // 00 00                        u16
    // 06 00 / 07 00 / 08 00        u16

    let ty =  u16::from_le_bytes(data[14..16].try_into().unwrap());
    match TouchFrameType::try_from(ty) {
        Ok(TouchFrameType::Stylus) => handle_stylus_frame(tx, data),
        Ok(TouchFrameType::Touch)  => handle_touch_frame(tx, data),
        Err(x) => {
            println!("warning: unimplemented data frame type: {}", x.number);
            println!("{:?}", data.hex_dump());
        },
    }
}

fn handle_frame(tx: &TxState, header: &TouchRawDataHeader, data: &[u8]) {
    match TouchDataType::try_from(header.data_type) {
        Ok(TouchDataType::Frame) => handle_touch_data_frame(tx, data),
        Ok(ty)  => println!("warning: unimplemented header type: {:?}", ty),
        Err(ty) => println!("error: unknown header type {}", ty.number),
    }
}

fn read_loop(mut device: Device, tx: TxState) -> Result<(), Box<dyn std::error::Error>> {
    device.start()?;

    let hdr_len = std::mem::size_of::<TouchRawDataHeader>();
    let buf_len = 4096;

    let mut buf = vec![0; buf_len];
    let (mut head, mut tail) = (0, 0);
    loop {
        tail += device.file.read(&mut buf[tail..])?;

        while tail - head > hdr_len {
            let (buf_hdr, buf_data) = buf.split_at_mut(head + hdr_len);
            let hdr = TouchRawDataHeader::ref_from_bytes(&buf_hdr[head..]).unwrap();

            // not enough space at all: bail
            if buf_len < hdr_len + hdr.data_size as usize {
                Err(format!("not enough space for data frame: need {}", hdr.data_size))?;
            }

            // not enough space at end of buffer: relocate and try again
            if buf_data.len() < hdr.data_size as usize {
                drop(hdr);
                drop(buf_hdr);
                drop(buf_data);

                buf.copy_within(head..tail, 0);
                tail -= head;
                head = 0;

                continue;
            }

            // read until full frame available
            head += hdr_len;
            while tail - head < hdr.data_size as usize {
                tail += device.file.read(&mut buf_data[tail-head..])?;
            }

            handle_frame(&tx, hdr, &buf_data[..hdr.data_size as usize]);
            head += hdr.data_size as usize;
        }

        // preemptive relocation
        if head < tail {
            buf.copy_within(head..tail, 0);
        }
        tail -= head;
        head = 0;
    }
}


fn draw(area: &DrawingArea, cr: &cairo::Context, state: &CommonState) {
    let (surface, touch_w, touch_h) = {
        let touch = state.touch.lock().unwrap();

        let mut data = Vec::with_capacity(touch.width as usize * touch.height as usize * 3);
        for x in touch.heatmap.iter().copied() {
            data.push(x);
            data.push(x);
            data.push(x);
            data.push(0);
        }

        let surface = ImageSurface::create_for_data(
            data,
            Format::Rgb24,
            touch.width as i32,
            touch.height as i32,
            touch.width as i32 * 4
        ).unwrap();

        (surface, touch.width, touch.height)
    };

    let (x, y, prox, pressure) = {
        let stylus = state.stylus.lock().unwrap();
        (stylus.x, stylus.y, stylus.proximity, stylus.pressure)
    };

    let w = area.get_allocated_width() as f64;
    let h = area.get_allocated_height() as f64;
    let x = x as f64 / 9600.0 * w;
    let y = y as f64 / 7200.0 * h;
    let p = pressure as f64 / 4096.0;

    cr.set_source_rgb(0.0, 0.0, 0.0);
    cr.paint();

    let m = cr.get_matrix();
    cr.translate(0.0, h);
    cr.scale(w / touch_w as f64, -h / touch_h as f64);
    cr.set_source_surface(&surface, 0.0, 0.0);
    cr.get_source().set_filter(cairo::Filter::Nearest);
    cr.paint();
    cr.set_matrix(m);

    if prox {
        cr.set_source_rgb(1.0, 0.0, 0.0);
        cr.set_line_width(1.0);

        cr.move_to(x, 0.0);
        cr.line_to(x, h);
        cr.stroke();

        cr.move_to(0.0, y);
        cr.line_to(w, y);
        cr.stroke();

        if p > 0.0 {
            cr.arc(x, y, p * 25.0, 0.0, 2.0 * std::f64::consts::PI);
            cr.fill();
            cr.stroke();
        }
    }
}


fn build(app: &gtk::Application, rx: RxState) {
    let window = ApplicationWindow::new(app);
    let area = DrawingArea::new();

    window.add(&area);
    window.set_size_request(1200, 800);

    let app = app.clone();
    window.connect_key_release_event(move |window, evt| {
        match evt.get_keyval() {
            gdk::enums::key::Escape => {
                app.quit();
                Inhibit(true)
            },
            gdk::enums::key::F12 => {
                if let Some(w) = window.get_window() {
                    if w.get_state().intersects(WindowState::FULLSCREEN) {
                        window.unfullscreen();
                    } else {
                        window.fullscreen();
                    }

                    Inhibit(true)
                } else {
                    Inhibit(false)
                }
            },
            _ => Inhibit(false),
        }
    });

    let state = rx.state;
    area.connect_draw(move |area, cr| {
        draw(area, cr, &state);
        Inhibit(false)
    });

    rx.chan_rx.attach(None, move |msg| {
        match msg {
            Message::Redraw => {
                area.queue_draw();
            }
        }

        Continue(true)
    });

    window.show_all();
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let (tx, rx) = setup_shared_state();
    let rx = std::cell::Cell::new(Some(rx));

    let device = Device::open()?;
    let devinfo = device.get_info()?;
    println!("IPTS UAPI connected ({:04x}:{:04x})", devinfo.vendor_id, devinfo.product_id);

    std::thread::spawn(move || {
        if let Err(err) = read_loop(device, tx) {
            println!("Error: {}", err);
            std::process::exit(1);
        }
    });

    let app = gtk::Application::new(Some("github.linux-surface.ipts"), Default::default())?;
    app.connect_activate(move |app| build(app, rx.take().unwrap()));
    app.run(&[]);

    Ok(())
}