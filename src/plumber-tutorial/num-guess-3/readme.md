# Number Guessing Game - 3

This is the demo that supports multiple user using the server at the same time and daemonization of dataflow graph.

## How to Compile

Make sure you are under the sandbox environment and run

```bash
make
```

## How to Run

```bash
pscript telnet-guess.pss    #This should start the telnet server foreground
pscript telnet-guess-daemon.pss #This should start the daemon
```

## How to manage the Plumber Daemons

- To list all the running server, use `plumber-daemons`
- To check if the server is still alive, use `plumber-daemon-ping num-guess-server`
- To stop the running daemon, use `plumber-daemon-stop num-guess-server`
