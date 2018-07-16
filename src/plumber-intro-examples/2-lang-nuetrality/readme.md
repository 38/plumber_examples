# The Language Neutrality Example

This is the example that demonstrates Plumber is a language neutral platform. 
It contains four versions implementation of the same component.

What the component does is reading the input, copy the input to the output and append
a greeting message `hello from <programming-language>`

This example demonstrate how we are able to connect component written in differnet programming language in a same dataflow graph.

The example contains 3 dataflow graph definitions.

* `greeting.pss` The example that shows the basic mixed language dataflow graph
* `greeting-subgraph.pss` The example that demonstrate the construction of a subgraph
* `greeting-graph-gen.pss` The example that demonstrate dataflow graph meta-programming

Before the you can try the example yourself, you need to compile the project. Please make sure you are under the sandbox environment, and simply run

```bash
make
```

To try each of the example use the following command

```bash
pscript <pss-filename>
```

# Directory Structure
```
.
├── c-greeter                   ; The greeter component written in C
│   ├── build.mk                ; 
│   └── c-greeter.c             ;
├── cpp-greeter                 ; The greeter component written in C++ 
│   ├── build.mk                ;
│   └── cpp-greeter.cpp         ;
├── py-greeter                  ; The greeter component written in Python 
│   └── py-greeter.py           ;
├── js-greeter                  ; The greeter component written in Javascript
│   └── js-greeter.js           ;
├── greeting-graph-gen.pss      ; The Graph Generator Example
├── greeting.pss                ; The Simple mixed language dataflow graph
├── greeting-subgraph.pss       ; The subgraph example 
└── Makefile                    ; Makefile
```

# Details

For more detailed description, please read the introduction of Plumber at [this link](https://plumberserver.com/).

- For the Language Neutrality Example please read this [section](https://plumberserver.com/index.html#home.main@creating-dataflow-graph-with-mixed-guest-languages)
- For the Subgraph Example, please read this [section](https://plumberserver.com/index.html#home.main@create-a-subgraph)
- For the Graph Metaprogramming example, please read this [section](https://plumberserver.com/index.html#home.main@create-a-dataflow-graph-programmatically)
