

//use ovr::{SensorCapabilities, Ovr};

extern crate debug;
extern crate libc;

use libc::{c_uint, c_int, c_float, c_char, c_void, c_double, c_short};

#[cfg(target_os = "macos")]
#[link(name="ovr")]
#[link(name="stdc++")]
#[link(name = "Cocoa", kind = "framework")]
#[link(name = "IOKit", kind = "framework")]
#[link(name = "CoreFoundation", kind = "framework")]
#[link(name = "OpenGL", kind = "framework")]
extern {}

 #[deriving(Clone, Default)]
#[repr(C)]
pub struct Sizei {
    pub x: c_int,
    pub y: c_int
}

#[deriving(Clone, Default)]
#[repr(C)]
pub struct Vector2i {
    pub x: c_int,
    pub y: c_int
}


#[deriving(Clone, Default)]
#[repr(C)]
pub struct FovPort {
    pub up_tan: c_float,
    pub down_tan: c_float,
    pub left_tan: c_float,
    pub right_tan: c_float
}

#[deriving(Clone, Default)]
#[repr(C)]
pub struct Vector3f {
    pub x: c_float,
    pub y: c_float,
    pub z: c_float
}

pub enum HmdStruct { }

#[repr(C)]
pub struct Hmd {
    pub handle: *const HmdStruct,
    pub hmd_type: c_int,
    pub product_name: *const c_char,
    pub manufacturer: *const c_char,
    pub hmd_capabilities: c_uint,
    pub sensor_capabilities: c_uint,
    pub distortion_capabilities: c_uint,
    pub resolution: Sizei,
    pub window_position: Vector2i,
    pub default_eye_fov: [FovPort, ..2],
    pub max_eye_fov: [FovPort, ..2],
    pub eye_render_order: [c_uint, ..2],
    pub display_device_name: *const c_char,
    pub display_id: c_int
}


extern "C" {
    pub fn ovr_Initialize() -> bool;
    pub fn ovrHmd_Create(index: c_int) -> *const Hmd;
}



fn main() {
    unsafe {
        if ovr_Initialize() {
            println!("Initialized.");
        } else {
            println!("Initialization failed.");
        }
        let hmd_ptr = ovrHmd_Create(0);
        let hmd = match hmd_ptr.as_ref() {
            Some(hmd) => hmd,
            None => {
                println!("Could not create HMD.");
                return;
            }
        };
        println!("Resolution: {:?}", hmd.resolution);
        println!("Display Device Name: {:?}", hmd.display_device_name);

    }

}