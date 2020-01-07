use std::io::Read;
use std::sync::Arc;
use std::sync::Mutex;

use gtk::prelude::*;
use gdk::prelude::*;
use gio::prelude::*;

use gtk::ApplicationWindow;
use gtk::DrawingArea;
use gdk::WindowState;

mod interface;
use interface::TouchDataHeader;

mod device;
use device::Device;


struct StylusData {
    pub x: u16,
    pub y: u16,
    pub proximity: bool,
    pub pressure: u16,
}

enum Message {
    Redraw,
}


#[derive(Clone)]
struct CommonState {
    stylus: Arc<Mutex<StylusData>>,
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
}

struct RxState {
    chan_rx: glib::Receiver<Message>,
    state: CommonState,
}

fn setup_shared_state() -> (TxState, RxState) {
    let (sender, receiver) = glib::MainContext::channel(glib::PRIORITY_DEFAULT);

    let stylus = StylusData { x: 0, y: 0, proximity: false, pressure: 0 };
    let stylus = Arc::new(Mutex::new(stylus));

    let state = CommonState {
        stylus: stylus,
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


unsafe fn as_data_header<'a>(buf: &'a [u8]) -> &'a TouchDataHeader {
    let ptr: *const TouchDataHeader = std::mem::transmute(buf.as_ptr());
    &*ptr
}

unsafe fn as_stylus_report<'a>(buf: &'a [u8]) -> &'a interface::StylusData {
    let ptr: *const interface::StylusData = std::mem::transmute(buf.as_ptr());
    &*ptr
}

unsafe fn as_u16(buf: &[u8]) -> u16 {
    let ptr: *const u16 = std::mem::transmute(buf.as_ptr());
    *ptr
}

fn handle_stylus_report(tx: &TxState, stylus: &interface::StylusData) {
    tx.push_stylus_update(StylusData {
        x: stylus.x,
        y: stylus.y,
        pressure: stylus.pressure,
        proximity: (stylus.mode & interface::IPTS_STYLUS_REPORT_MODE_PROXIMITY) != 0,
    });

}

fn handle_frame(tx: &TxState, header: &TouchDataHeader, data: &[u8]) {
    if header.ty != interface::IPTS_TOUCH_DATA_TYPE_FRAME {
        return;
    }

    // check for stylus report
    if unsafe { as_u16(&data[28..30]) } != 0x460 {
        return;
    }

    for i in 0..data[32] as usize {
        let len = std::mem::size_of::<interface::StylusData>();
        let index = 40 + i * len;
        let report = unsafe { as_stylus_report(&data[index..index+len]) };

        handle_stylus_report(tx, report);
    }
}

fn read_loop(mut device: Device, tx: TxState) -> Result<(), Box<dyn std::error::Error>> {
    device.start()?;

    let hdr_len = std::mem::size_of::<interface::TouchDataHeader>();

    let mut buf = vec![0; 4096];
    let mut received = 0;
    loop {
        let len = device.file.read(&mut buf[received..])?;
        received += len;

        if received >= hdr_len {
            let (buf_hdr, buf_data) = buf.split_at_mut(hdr_len);
            let hdr = unsafe { as_data_header(buf_hdr) };

            received -= hdr_len;
            while received < hdr.size as usize {
                let len = device.file.read(&mut buf_data[received..])?;
                received += len;
            }

            handle_frame(&tx, hdr, &buf_data[..received]);
            received = 0;
        }
    }
}


fn draw(area: &DrawingArea, cr: &cairo::Context, state: &CommonState) {
    let (x, y, prox, pressure) = {
        let stylus = state.stylus.lock().unwrap();
        (stylus.x, stylus.y, stylus.proximity, stylus.pressure)
    };

    let w = area.get_allocated_width() as f64;
    let h = area.get_allocated_height() as f64;
    let x = x as f64 / 9600.0 * w;
    let y = y as f64 / 7200.0 * h;
    let p = pressure as f64 / 4096.0;

    cr.set_source_rgb(1.0, 1.0, 1.0);
    cr.paint();

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