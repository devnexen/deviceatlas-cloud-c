use std::ffi::*;
use std::mem;
use std::env;
use std::process;

include!("dacloudbind.rs");

#[allow(non_upper_case_globals)]
fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        process::exit(-1);
    }
    unsafe {
        let mut config: da_cloud_config = mem::zeroed();
        let path = CString::new(&*args[1]).unwrap();

        match da_cloud_init(&mut config, path.as_ptr()) {
            0 => {
                let mut header: da_cloud_header_head = mem::zeroed();
                let mut prop: da_cloud_property_head = mem::zeroed();
                let uastr = CString::new("iPhone").unwrap();
                da_cloud_header_init(&mut header);
                da_cloud_useragent_add(&mut header, uastr.as_ptr() as *const c_char);
                da_cloud_detect(&mut config, &mut header, &mut prop);
                da_cloud_properties_free(&mut prop);
                da_cloud_header_free(&mut header);
                da_cloud_fini(&mut config);
            },
            _ => {
                println!("initialisation failed");
                process::exit(-1);
            }
        }
        da_cloud_fini(&mut config);
    }
}
