#![allow(unused)]

use num_enum::TryFromPrimitive;


#[repr(C)]
#[derive(Debug, Default)]
pub struct DeviceInfo {
    pub vendor_id: u16,
    pub product_id: u16,
    pub hw_rev: u32,
    pub fw_rev: u32,
    pub frame_size: u32,
    pub feedback_size: u32,
    pub sensor_mode: u32,
    pub max_touch_points: u8,
    pub spi_frequency: u8,
    pub spi_io_mode: u8,
    pub reserved0: u8,
    pub sensor_minor_eds_rev: u8,
    pub sensor_major_eds_rev: u8,
    pub sensor_eds_intf_rev: u8,
    pub me_eds_intf_rev: u8,
    pub kernel_compat_ver: u8,
    pub reserved1: u8,
    pub reserved2: [u32; 2],
}


#[repr(C)]
#[derive(Debug)]
pub struct TouchRawDataHeader {
	pub data_type: u32,
	pub data_size: u32,
    pub buffer_id: u32,
    pub protocol_ver: u32,
    pub kernel_compat_id: u8,
    pub reserved: [u8; 15],
    pub hid_private_data: TouchHidPrivateData,
}

#[repr(C)]
#[derive(Debug)]
pub struct TouchHidPrivateData {
    pub transaction_id: u32,
    pub reserved: [u8; 28],
}

#[repr(u32)]
#[derive(Debug, Clone, Copy, TryFromPrimitive)]
pub enum TouchDataType {
    Frame = 0,
    Error,
    VendorData,
    HidReport,
    GetFeatures,
}

#[repr(u16)]
#[derive(Debug, Clone, Copy, TryFromPrimitive)]
pub enum TouchFrameType {
    Stylus = 6,
    Touch = 8,
}


#[repr(C)]
#[derive(Debug)]
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

pub const STYLUS_REPORT_MODE_PROXIMITY: u16 = 1;
pub const STYLUS_REPORT_MODE_TOUCH:     u16 = 2;
pub const STYLUS_REPORT_MODE_BUTTON:    u16 = 4;
pub const STYLUS_REPORT_MODE_RUBBER:    u16 = 8;


pub mod ioctl {
    use nix::{ioctl_none, ioctl_read};
    use super::DeviceInfo;

    ioctl_read!(ipts_get_device_info, 0x86, 0x01, DeviceInfo);
    ioctl_none!(ipts_start, 0x86, 0x02);
    ioctl_none!(ipts_stop,  0x86, 0x03);
}


#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn check_type_sizes() {
        assert_eq!(std::mem::size_of::<TouchRawDataHeader>(), 64);
        assert_eq!(std::mem::size_of::<TouchHidPrivateData>(), 32);
        assert_eq!(std::mem::size_of::<DeviceInfo>(), 44);

        assert_eq!(std::mem::size_of::<StylusData>(), 16);
    }
}
