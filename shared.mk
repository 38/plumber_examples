BINARIES=$(foreach serv,$(SERVLETS),bin/lib$(serv).so)

LINKER=gcc
OUTPUT=../bin
LDFLAGS:=$(LDFLAGS) -L$(ENVROOT)/lib -lpstd -lproto
CFLAGS:=$(CFLAGS) -I$(ENVROOT)/include/pstd -I$(ENVROOT)/include/proto

PARAM=LINKER="$(LINKER)" OUTPUT="$(OUTPUT)" LDFLAGS="$(LDFLAGS)" CFLAGS="$(CFLAGS)"

default: $(BINARIES)

$(BINARIES): bin/lib%.so: % __always_build__ __check_environment__
	cd $< && make -f $(ENVROOT)/lib/plumber/servlet.mk $(PARAM)

__always_build__:

__check_environment__:
	@if [ "x$(ENVROOT)" = "x" ]; then \
		echo "You should build the example in the Plumber Isolated Environment, see the init script for details"; \
		exit 1; \
	fi

.PHONY: clean

clean:
	for serv in $(SERVLETS); do \
		cd $${serv}; \
		make -f $(ENVROOT)/lib/plumber/servlet.mk clean $(PARAM); \
		cd ..; \
	done;
