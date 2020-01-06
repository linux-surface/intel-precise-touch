use std::path::Path;
use std::fs::File;
use std::os::unix::io::AsRawFd;
use std::io::Read;

const DEFAULT_EVENT_FILE_PATH: &str = "/dev/ipts";


mod ioctl {
    use nix::{ioctl_none, ioctl_read};

    #[repr(C)]
    #[derive(Debug, Default)]
    pub struct IptsDeviceInfo {
        pub vendor_id: u16,
        pub product_id: u16,
        pub hw_rev: u32,
        pub fw_rev: u32,
        pub frame_size: u32,
        pub feedback_size: u32,
        pub reserved: [u8; 24],
    }

    ioctl_read!(ipts_get_device_info, 0x86, 0x01, IptsDeviceInfo);
    ioctl_none!(ipts_start, 0x86, 0x02);
    ioctl_none!(ipts_stop,  0x86, 0x03);
}

use ioctl::IptsDeviceInfo;


struct Device {
    file: File,
}

impl Device {
    pub fn open() -> std::io::Result<Self> {
        Self::open_path(DEFAULT_EVENT_FILE_PATH)
    }

    pub fn open_path<P: AsRef<Path>>(path: P) -> std::io::Result<Self> {
        let file = File::open(path)?;
        Ok(Device { file })
    }

    pub fn get_info(&self) -> nix::Result<IptsDeviceInfo> {
        let mut devinfo = IptsDeviceInfo::default();

        unsafe {
            ioctl::ipts_get_device_info(self.file.as_raw_fd(), &mut devinfo as *mut _)?
        };

        Ok(devinfo)
    }

    pub fn start(&self) -> nix::Result<()> {
        unsafe { ioctl::ipts_start(self.file.as_raw_fd())? };
        Ok(())
    }

    #[allow(unused)]
    pub fn stop(&self) -> nix::Result<()> {
        unsafe { ioctl::ipts_stop(self.file.as_raw_fd())? };
        Ok(())
    }
}


fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut device = Device::open()?;
    let devinfo = device.get_info()?;

    println!("IPTS UAPI connected ({:04x}:{:04x})", devinfo.vendor_id, devinfo.product_id);

    device.start()?;

    let mut buf = vec![0; 4096];
    loop {
        let len = device.file.read(&mut buf[..])?;
        println!("received data (size: {})", len);
        println!("  {:02x?}", &buf[0..16]);
        println!("  {:02x?}", &buf[16..32]);
    }
}
