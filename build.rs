extern crate bindgen;

fn main() {
    println!("cargo:rustc-link-lib=dacloud");
    let bind = bindgen::Builder::default().header("dacloud.h").generate().unwrap();
    bind.write_to_file("examples/dacloudbind.rs").unwrap();
}
