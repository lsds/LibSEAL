all: libzlib.a

ZLIBFLAGS=-O3 -D_LARGEFILE64_SOURCE=1 -DHAVE_HIDDEN

ZFILES= \
		  adler32.o \
		  crc32.o \
		  deflate.o \
		  infback.o \
		  inffast.o \
		  inflate.o \
		  inftrees.o \
		  trees.o  \
		  zutil.o \
		  compress.o \
		  uncompr.o \
		  gzclose.o  \
		  gzlib.o \
		  gzread.o  \
		  gzwrite.o

%.o: %.c
	$(CC) -fPIC -DPIC -c $(CFLAGS) $(ZLIBFLAGS) $< -o $@

zfiles: $(ZFILES)

libzlib.a: zfiles
	$(AR) rc $@ $(ZFILES)

clean:
	rm -f *.o
	rm -f libzlib.a
