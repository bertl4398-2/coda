# generated Linux dev-virtual-machine 3.5.0-22-generic #34-Ubuntu SMP Tue Jan 8 21:41:11 UTC 2013
#
#
#
SHEXT=so
SHLIBNAME=libccn.$(SHEXT).1
SHLIBDEPS=
SHARED_LD_FLAGS = -shared --whole-archive -soname=$(SHLIBNAME) -lc
PLATCFLAGS=-fPIC
CWARNFLAGS = -Wall -Wpointer-arith -Wreturn-type -Wstrict-prototypes
CPREFLAGS= -I../include -D_REENTRANT
PCAP_PROGRAMS = ccndumppcap
RESOLV_LIBS = -lresolv
INSTALL_BASE = /usr/local
INSTALL_INCLUDE = $(INSTALL_BASE)/include
INSTALL_LIB = $(INSTALL_BASE)/lib
INSTALL_BIN = $(INSTALL_BASE)/bin
INSTALL = install
RM = rm -f
SH = /bin/sh
