# generated Linux suno-9.sophia.grid5000.fr 2.6.32-5-amd64 #1 SMP Sun Sep 23 10:07:46 UTC 2012
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
