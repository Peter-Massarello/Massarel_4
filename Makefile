CC = gcc
CFLAGS = -g

OSS = oss.o
UPROC = uproc.o

all: oss uproc

oss: $(OSS)
	$(CC) -o $@ $(OSS)

uproc: $(UPROC)
	$(CC) -o $@ $(UPROC)

$(OSS): oss.c
	$(CC) -c -o $@ $<

$(UPROC): uproc.c
	$(CC) -c -o $@ $<

clean:
	rm -f *.o *.txt oss uproc