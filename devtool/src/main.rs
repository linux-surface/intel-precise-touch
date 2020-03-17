use std::io::Read;
use std::sync::Arc;
use std::sync::Mutex;
use std::convert::TryFrom;

use gtk::prelude::*;
use gdk::prelude::*;
use gio::prelude::*;

use cairo::ImageSurface;
use cairo::Format;
use gtk::ApplicationWindow;
use gtk::DrawingArea;
use gdk::WindowState;

use serde::Serialize;

mod interface;
use interface::RawDataHeader;
use interface::RawDataType;
use interface::PayloadHeader;
use interface::PayloadFrameHeader;
use interface::PayloadFrameType;
use interface::mem::PackedDataStruct;
use interface::mem::FrameHeader;

mod device;
use device::Device;


#[derive(Serialize)]
struct DataRecord<'a> {
    ty: u16,
    buf: &'a[u8],
}


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


fn handle_touch_payload(tx: &TxState, data: &[u8]) {
    // Note: This could be merged with handle_stylus_payload, only supported ChunkTypes differ.
    use interface::{ChunkType, ChunkHeader, TouchHeatmapDim};

    let mut width = 0;
    let mut height = 0;
    let mut heatmap = None;

    let mut offset = 0;
    while offset < data.len() {
        let (frame_hdr, frame_data, frame_len) = ChunkHeader::parse(&data[offset..]);
        offset += frame_len;

        match ChunkType::try_from(frame_hdr.ty) {
            Ok(ChunkType::TouchHeatmapDim) => {
                let dim = TouchHeatmapDim::ref_from_bytes(frame_data).unwrap();
                height = dim.height;
                width = dim.width;
            },
            Ok(ChunkType::TouchHeatmap) => {
                heatmap = Some(frame_data);
            },
            Ok(ty) => eprintln!("warning: unsupported touch chunk type {:?}", ty),
            Err(ty) => eprintln!("warning: unknown touch chunk type 0x{:04x}", ty.number),
        }
    }

    if let Some(heatmap) = heatmap {
        if height as usize * width as usize != heatmap.len() {
            eprintln!("error: touch data sizes do not match");
            return;
        }

        tx.push_touch_update(width, height, heatmap);
    }
}

fn emit_stylus_report_gen1(tx: &TxState, stylus: &interface::StylusStylusReportGen1Data) {
    tx.push_stylus_update(StylusData {
        x: u16::from_le_bytes(stylus.x),
        y: u16::from_le_bytes(stylus.y),
        pressure: u16::from_le_bytes(stylus.pressure),
        proximity: (stylus.mode as u16 & interface::STYLUS_REPORT_MODE_PROXIMITY) != 0,
    });
}

fn emit_stylus_report_gen2(tx: &TxState, stylus: &interface::StylusStylusReportGen2Data) {
    tx.push_stylus_update(StylusData {
        x: stylus.x,
        y: stylus.y,
        pressure: stylus.pressure,
        proximity: (stylus.mode & interface::STYLUS_REPORT_MODE_PROXIMITY) != 0,
    });
}

fn handle_stylus_report_gen1(tx: &TxState, data: &[u8]) {
    use interface::{StylusStylusReportGen1Data, StylusReportHeaderU};

    let (hdr, data) = data.split_at(std::mem::size_of::<StylusReportHeaderU>());
    let hdr = StylusReportHeaderU::ref_from_bytes(hdr).unwrap();

    let data = &data[4..];
    let len = std::mem::size_of::<StylusStylusReportGen1Data>();
    for i in 0..hdr.num_reports as usize {
        let report = StylusStylusReportGen1Data::ref_from_bytes(&data[i*len..(i+1)*len]).unwrap();
        emit_stylus_report_gen1(tx, report);
    }
}

fn handle_stylus_report_gen2_u(tx: &TxState, data: &[u8]) {
    use interface::{StylusStylusReportGen2Data, StylusReportHeaderU};

    let (hdr, data) = data.split_at(std::mem::size_of::<StylusReportHeaderU>());
    let hdr = StylusReportHeaderU::ref_from_bytes(hdr).unwrap();

    let len = std::mem::size_of::<StylusStylusReportGen2Data>();
    for i in 0..hdr.num_reports as usize {
        let report = StylusStylusReportGen2Data::ref_from_bytes(&data[i*len..(i+1)*len]).unwrap();
        emit_stylus_report_gen2(tx, report);
    }
}

fn handle_stylus_report_gen2_p(tx: &TxState, data: &[u8]) {
    use interface::{StylusStylusReportGen2Data, StylusReportHeaderP};

    let (hdr, data) = data.split_at(std::mem::size_of::<StylusReportHeaderP>());
    let hdr = StylusReportHeaderP::ref_from_bytes(hdr).unwrap();

    let len = std::mem::size_of::<StylusStylusReportGen2Data>();
    for i in 0..hdr.num_reports as usize {
        let report = StylusStylusReportGen2Data::ref_from_bytes(&data[i*len..(i+1)*len]).unwrap();
        emit_stylus_report_gen2(tx, report);
    }
}

fn handle_stylus_payload(tx: &TxState, data: &[u8]) {
    // Note: This could be merged with handle_touch_payload, only supported ChunkTypes differ.
    use interface::{ChunkType, ChunkHeader};

    let mut offset = 0;
    while offset < data.len() {
        let (frame_hdr, frame_data, frame_len) = ChunkHeader::parse(&data[offset..]);
        offset += frame_len;

        match ChunkType::try_from(frame_hdr.ty) {
            Ok(ChunkType::StylusReportGen1) => handle_stylus_report_gen1(tx, frame_data),
            Ok(ChunkType::StylusReportGen2U) => handle_stylus_report_gen2_u(tx, frame_data),
            Ok(ChunkType::StylusReportGen2P) => handle_stylus_report_gen2_p(tx, frame_data),
            Ok(ty) => eprintln!("warning: unsupported stylus chunk type {:?}", ty),
            Err(ty) => eprintln!("error: unknown stylus chunk type 0x{:04x}", ty.number),
        }
    }
}

fn handle_payload_frame(tx: &TxState, data: &[u8]) {
    let (hdr, data) = data.split_at(std::mem::size_of::<PayloadHeader>());
    let hdr = PayloadHeader::ref_from_bytes(hdr).unwrap();

    let mut offset = 0;
    for _ in 0..hdr.num_frames {
        let (frame_hdr, frame_data, frame_len) = PayloadFrameHeader::parse(&data[offset..]);
        offset += frame_len;

        match PayloadFrameType::try_from(frame_hdr.ty) {
            Ok(PayloadFrameType::Stylus) => handle_stylus_payload(tx, frame_data),
            Ok(PayloadFrameType::Touch)  => handle_touch_payload(tx, frame_data),
            Err(x) => eprintln!("warning: unimplemented data frame type: {}", x.number),
        }
    }
}

fn handle_frame(tx: &TxState, header: &RawDataHeader, data: &[u8]) {
    match RawDataType::try_from(header.data_type) {
        Ok(RawDataType::Payload) => handle_payload_frame(tx, data),
        Ok(ty)  => eprintln!("warning: unimplemented header type: {:?}", ty),
        Err(ty) => eprintln!("error: unknown header type {}", ty.number),
    }
}

fn read_loop(mut device: Device, tx: TxState) -> Result<(), Box<dyn std::error::Error>> {
    device.start()?;

    let hdr_len = std::mem::size_of::<RawDataHeader>();
    let buf_len = 4096;

    let mut buf = vec![0; buf_len];
    let (mut head, mut tail) = (0, 0);
    loop {
        tail += device.file.read(&mut buf[tail..])?;

        while tail - head > hdr_len {
            let (buf_hdr, buf_data) = buf.split_at_mut(head + hdr_len);
            let hdr = RawDataHeader::ref_from_bytes(&buf_hdr[head..]).unwrap();

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


fn get_dominant_touch_point(heatmap: &[u8], width: u8, height: u8) -> (bool, f64, f64) {
    let (min_idx, min, max) = heatmap.iter()
        .enumerate()
        .fold((0, 255, 0), |(min_idx, min, max), (idx, val)| {
            let mut min_idx = min_idx;
            let mut min = min;
            let mut max = max;

            if *val < min {
                min_idx = idx;
                min = *val;
            } else if *val > max {
                max = *val;
            }

            (min_idx, min, max)
        });

    let x = min_idx % width as usize;
    let y = min_idx / width as usize;

    let x = (x as f64 + 0.5) / width as f64;
    let y = 1.0 - (y as f64 + 0.5) / height as f64;

    (max - min > 10, x, y)
}

fn draw(area: &DrawingArea, cr: &cairo::Context, state: &CommonState) {
    let (surface, touch_w, touch_h, finger, tx, ty) = {
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

        let (finger, cx, cy) = get_dominant_touch_point(&touch.heatmap, touch.width, touch.height);

        (surface, touch.width, touch.height, finger, cx, cy)
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

    if finger {
        cr.set_source_rgb(0.0, 1.0, 0.0);
        cr.set_line_width(1.0);

        cr.move_to(tx * w, 0.0);
        cr.line_to(tx * w, h);
        cr.stroke();

        cr.move_to(0.0, ty * h);
        cr.line_to(w, ty * h);
        cr.stroke();
    }

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
    eprintln!("IPTS UAPI connected ({:04x}:{:04x})", devinfo.vendor_id, devinfo.product_id);

    std::thread::spawn(move || {
        if let Err(err) = read_loop(device, tx) {
            eprintln!("Error: {}", err);
            std::process::exit(1);
        }
    });

    let app = gtk::Application::new(Some("github.linux-surface.ipts"), Default::default())?;
    app.connect_activate(move |app| build(app, rx.take().unwrap()));
    app.run(&[]);

    Ok(())
}
