use std::path::Path;
use std::fs::File;
use std::os::unix::io::AsRawFd;
use std::io::Read;

mod interface;
use interface::ioctl;
use interface::DeviceInfo;

const DEFAULT_EVENT_FILE_PATH: &str = "/dev/ipts";


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


unsafe fn as_data_header<'a>(buf: &'a [u8]) -> &'a interface::TouchDataHeader {
    std::mem::transmute(buf.as_ptr())
}


fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut device = Device::open()?;
    let devinfo = device.get_info()?;

    println!("IPTS UAPI connected ({:04x}:{:04x})", devinfo.vendor_id, devinfo.product_id);

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

            println!("frame received: type: {}, size: {}, bufid: {}", hdr.ty, hdr.size, hdr.buffer);
            received = 0;
        }
    }
}
