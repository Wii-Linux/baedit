# ?= doesn't work on CC, since Make has it built in.
# If origin is default, it was not overriden by the user.
ifeq ($(origin CC),default)
CC = gcc
endif
CFLAGS ?= -O2


all: baedit
baedit.o: baedit.c
	$(CC) $(CFLAGS) -c -o baedit.o baedit.c

baedit: baedit.o
	$(CC) $(LDFLAGS) -o baedit baedit.o
