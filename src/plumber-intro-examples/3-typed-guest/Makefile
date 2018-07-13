SERVLETS=input-parser adder
CFLAGS=-g -Wall -DLOG_LEVEL=0 -O0

all: default
	DYLD_LIBRARY_PATH=$(ENVROOT)/lib protoman -u -y adder.ptype

include ../../../shared.mk
