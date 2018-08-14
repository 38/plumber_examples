/*
 * Copyright (C) 2018, Hao Hou
 */

#[macro_use]
extern crate plumber_rs;

use std::io::{Read, Write};

/* Import everything we need from the plumber-rs crate */
use plumber_rs::servlet::{SyncServlet, Unimplemented, ServletFuncResult, Bootstrap, BootstrapResult, success};
use plumber_rs::pipe::{Pipe, PIPE_INPUT, PIPE_OUTPUT};

/* Now let's define a type for the servlet */
struct Greeter {
    input : Pipe<()>,
    output: Pipe<()>
}

/* All the servlet should impelement either async servlet or sync servlet trait, or both 
 * In our case, we only impelemented the sync servlet trait */
impl SyncServlet for Greeter {

    /* This line defines the servlet doesn't use the typed pipe port */
    no_protocol!();

    /* The initialization function, which is similar to init callback to other languages */
    fn init(&mut self, _args:&[&str], _m:&mut Self::ProtocolType) -> ServletFuncResult { success() }

    /* The execution function, which is the main function of the servlet */
    fn exec(&mut self, _i:Self::DataModelType) -> ServletFuncResult 
    {
        let mut line = String::new();
        /* In rust we are able to read/write the pipe just like a file, because Pipe implememnted
         * all the traits for read and write */
        self.input.read_to_string(&mut line).expect("Read failure");
        write!(self.output, "{}hello from Rust\n", line);
        return success();
    }

    /* The cleanup function */
    fn cleanup(&mut self) -> ServletFuncResult { success() }
}

/* Different from other language, we need a bootstrap type, which carries all the servlet metadata
 * we want to export to the Plumber sytsem */
struct BootstrapType;

/* The bootstrap type should implememnt the bootstrap trait */
impl Bootstrap for BootstrapType {
    /* This line indicates that we have implemented a sync servlet */
    type SyncServletType  = Greeter;
    /* This lines indicates the servet doesn't support async model */
    type AsyncServletType = Unimplemented;

    /* This function will be called when the servlet get initialized, and it's resposnible
     * to bootstrap the servlet, which means returns the servlet context */
    fn get(_args:&[&str]) -> BootstrapResult<Self>
    {
        /* The only thing we should do is returns a new instance of the servlet */
        return Self::sync(Greeter {
            input : Pipe::define("input", PIPE_INPUT, None).unwrap(),
            output: Pipe::define("output", PIPE_OUTPUT, None).unwrap()
        });
    }
}

/* Finally just make sure all the information will be exported to the servlet binary */
export_bootstrap!(BootstrapType);
