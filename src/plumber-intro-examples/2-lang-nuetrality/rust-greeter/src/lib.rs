/*
 * Copyright (C) 2018, Hao Hou
 */

#[macro_use]
extern crate plumber_rs;
extern crate libc;

use std::io::{Read, Write};

use plumber_rs::servlet::{SyncServlet, Unimplemented, ServletMode, ServletFuncResult, Bootstrap};
use plumber_rs::pipe::{Pipe, PIPE_INPUT, PIPE_OUTPUT};

struct Greeter {
    input : Pipe<()>,
    output: Pipe<()>
}

impl SyncServlet for Greeter {
    fn init(&mut self, _args:&[&str]) -> ServletFuncResult { Ok(()) }
    fn exec(&mut self) -> ServletFuncResult 
    {
        let mut line = String::new();
        self.input.read_to_string(&mut line).expect("Read failure");
        write!(self.output, "{}hello from Rust\n", line);
        return Ok(());
    }
    fn cleanup(&mut self) -> ServletFuncResult { Ok(()) }
}

struct BootstrapType;

impl Bootstrap for BootstrapType {
    type SyncServletType  = Greeter;
    type AsyncServletType = Unimplemented;

    fn get(_args:&[&str]) -> Result<ServletMode<Self::AsyncServletType, Self::SyncServletType>, ()>
    {
        return Ok(ServletMode::SyncMode(Greeter {
            input : Pipe::define("input", PIPE_INPUT, None).unwrap(),
            output: Pipe::define("output", PIPE_OUTPUT, None).unwrap()
        }));
    }
}

export_bootstrap!(BootstrapType);
