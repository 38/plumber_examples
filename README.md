# The Example Repository for Plumber Project

*Main project: https://github.com/38/plumber*

This is the collections of the examples for the Plumber project. To compile
the examples, enter the Plumber Isolated Environment using the ./init
script at the root of the repository.

After you entering the environment, you can change current working directory to
any of the examples under the example directory and build the example.

# Getting Started
This repository provides an easy way to try Plumber in an isolated directory.

	git clone --recursive https://github.com/38/plumber_examples.git
	cd plumber_examples && ./init
	

```bash
git clone --recursive https://github.com/38/plumber_examples.git
```

The minimal required dependencies are 

	- Python 2 (Python 2.7 Recommended)
	- CMake 2.6 or later (CMake 3+ Recommended)
	- libreadline 
	- GCC and G++ (GCC-5 Recommended)
	- GNU Make
	- zsh (For the startup script)
	- pkg-config 
	- OpenSSL (For SSL support)
	
for Ubuntu users, use command

```bash
apt-get install cmake gcc g++ uuid-dev libssl-dev pkg-config python2.7 libpython2.7-dev libreadline-dev zsh
```

for MacOS users, use command

```bash
sudo brew install cmake openssl@1.0 ossp-uuid pkg-config  pkgconfig   readline
```

After installed all the dependencies, use the following command to enter the environment.

```bash
cd plumber_examples
./init 
```

In the environment, you should be able to build and run the examples under `src` directory.

