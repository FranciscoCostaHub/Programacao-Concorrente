CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lgd -lpthread

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    BREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo /opt/homebrew)
    CFLAGS += -I$(BREW_PREFIX)/include
    LDFLAGS += -L$(BREW_PREFIX)/lib
endif

all: process-photos-parallel-A process-photos-parallel-B

# Parte A
process-photos-parallel-A: process-photos-parallel-A.c image-lib.c image-lib.h
	$(CC) $(CFLAGS) process-photos-parallel-A.c image-lib.c -o process-photos-parallel-A $(LDFLAGS)

# Parte B
process-photos-parallel-B: process-photos-parallel-B.c image-lib.c image-lib.h
	$(CC) $(CFLAGS) process-photos-parallel-B.c image-lib.c -o process-photos-parallel-B $(LDFLAGS)

clean:
	rm -f process-photos-parallel-A process-photos-parallel-B *.o

.PHONY: all clean
