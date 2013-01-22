SRCDIR= $(dir $(firstword $(MAKEFILE_LIST)))
INSTALL=install

all: readversiondef
install: all
	$(INSTALL) -d $(PREFIX)/bin
	$(INSTALL) -t $(PREFIX)/bin readversiondef

readversiondef.o: $(SRCDIR)readversiondef.c
	$(CC) $(CFLAGS) -c -o $@ $^
readversiondef: readversiondef.o
	$(CC) $(LDFLAGS) -o $@ $^
