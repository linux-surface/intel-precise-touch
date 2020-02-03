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


#[repr(C)]
#[derive(Debug)]
pub struct PayloadHeader {
    pub counter: u32,
    pub num_frames: u32,
    pub reserved: [u8; 4],
}

#[repr(C)]
#[derive(Debug)]
pub struct PayloadFrameHeader {
    pub num: u16,
    pub ty: u16,
    pub payload_len: u32,
    pub reserved: [u8; 8],
}

#[repr(u16)]
#[derive(Debug, Clone, Copy, TryFromPrimitive)]
pub enum PayloadFrameType {
    Stylus = 6,
    Touch = 8,
}


#[repr(C)]
#[derive(Debug)]
pub struct StylusFrameHeader {
    pub ty: u16,
    pub payload_len: u16,
}

#[repr(u16)]
#[derive(Debug, Clone, Copy, TryFromPrimitive)]
pub enum StylusFrameType {
    ReportU = 0x0460,
    ReportP = 0x0461,
}

#[repr(C)]
#[derive(Debug)]
pub struct StylusReportHeaderU {
    pub num_reports: u8,
    pub reserved0: u8,
    pub reserved1: [u8; 2],
    pub stylus_uuid: [u8; 4],
}

#[repr(C)]
#[derive(Debug)]
pub struct StylusReportHeaderP {
    pub num_reports: u8,
    pub reserved0: u8,
    pub reserved1: [u8; 2],
}

#[repr(C)]
#[derive(Debug)]
pub struct StylusReportData {
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


unsafe impl mem::PackedDataStruct for DeviceInfo {}

unsafe impl mem::PackedDataStruct for TouchRawDataHeader {}
unsafe impl mem::PackedDataStruct for TouchHidPrivateData {}
unsafe impl mem::PackedDataStruct for PayloadHeader {}
unsafe impl mem::PackedDataStruct for PayloadFrameHeader {}

unsafe impl mem::PackedDataStruct for StylusFrameHeader {}
unsafe impl mem::PackedDataStruct for StylusReportHeaderU {}
unsafe impl mem::PackedDataStruct for StylusReportHeaderP {}
unsafe impl mem::PackedDataStruct for StylusReportData {}


pub mod ioctl {
    use nix::{ioctl_none, ioctl_read};
    use super::DeviceInfo;

    ioctl_read!(ipts_get_device_info, 0x86, 0x01, DeviceInfo);
    ioctl_none!(ipts_start, 0x86, 0x02);
    ioctl_none!(ipts_stop,  0x86, 0x03);
}

pub mod mem {
    pub struct TransmuteError;

    impl std::fmt::Display for TransmuteError {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            write!(f, "transmutation with invalid length")
        }
    }

    impl std::fmt::Debug for TransmuteError {
        fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
            write!(f, "transmutation with invalid length")
        }
    }

    impl std::error::Error for TransmuteError {}


    pub unsafe fn transmute_ref_from_bytes<'a, T>(buf: &'a [u8]) -> Result<&'a T, TransmuteError> {
        if buf.len() == std::mem::size_of::<T>() {
            let ptr: *const T = std::mem::transmute(buf.as_ptr());
            Ok(&*ptr)
        } else {
            Err(TransmuteError)
        }
    }

    pub unsafe trait PackedDataStruct: Sized {
        fn ref_from_bytes<'a>(buf: &'a [u8]) -> Result<&'a Self, TransmuteError> {
            unsafe { transmute_ref_from_bytes(buf) }
        }
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn check_type_sizes() {
        assert_eq!(std::mem::size_of::<DeviceInfo>(), 44);

        assert_eq!(std::mem::size_of::<TouchRawDataHeader>(), 64);
        assert_eq!(std::mem::size_of::<TouchHidPrivateData>(), 32);
        assert_eq!(std::mem::size_of::<PayloadHeader>(), 12);
        assert_eq!(std::mem::size_of::<PayloadFrameHeader>(), 16);

        assert_eq!(std::mem::size_of::<StylusFrameHeader>(), 4);
        assert_eq!(std::mem::size_of::<StylusReportHeaderU>(), 8);
        assert_eq!(std::mem::size_of::<StylusReportHeaderP>(), 4);
        assert_eq!(std::mem::size_of::<StylusReportData>(), 16);
    }
}
