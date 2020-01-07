use std::path::Path;
use std::fs::File;
use std::os::unix::io::AsRawFd;

use crate::interface::ioctl;
use crate::interface::DeviceInfo;

const DEFAULT_EVENT_FILE_PATH: &str = "/dev/ipts";


pub struct Device {
    pub file: File,
}

impl Device {
    pub fn open() -> std::io::Result<Self> {
        Self::open_path(DEFAULT_EVENT_FILE_PATH)
    }

    pub fn open_path<P: AsRef<Path>>(path: P) -> std::io::Result<Self> {
        let file = File::open(path)?;
        Ok(Device { file })
    }

    pub fn get_info(&self) -> nix::Result<DeviceInfo> {
        let mut devinfo = DeviceInfo::default();

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
