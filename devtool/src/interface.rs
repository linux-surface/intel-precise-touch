
#[repr(C)]
#[derive(Debug, Default)]
pub struct DeviceInfo {
    pub vendor_id: u16,
    pub product_id: u16,
    pub hw_rev: u32,
    pub fw_rev: u32,
    pub frame_size: u32,
    pub feedback_size: u32,
    pub reserved: [u8; 24],
}

#[repr(C)]
pub struct TouchDataHeader {
	pub ty: u32,
	pub size: u32,
	pub buffer: u32,
	pub reserved: [u8; 52],
}

#[repr(C)]
#[derive(Debug, Default)]
pub struct StylusData {
	pub timestamp: u16,
	pub mode: u16,
	pub x: u16,
	pub y: u16,
	pub pressure: u16,
	pub altitude: u16,
	pub azimuth: u16,
	pub reserved: u16,
}


pub const IPTS_TOUCH_DATA_TYPE_FRAME: u32 = 0;

pub const IPTS_STYLUS_REPORT_MODE_PROXIMITY: u16 = 1;


pub mod ioctl {
    use nix::{ioctl_none, ioctl_read};
    use super::DeviceInfo;

    ioctl_read!(ipts_get_device_info, 0x86, 0x01, DeviceInfo);
    ioctl_none!(ipts_start, 0x86, 0x02);
    ioctl_none!(ipts_stop,  0x86, 0x03);
}
