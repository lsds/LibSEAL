# You can put your build options here
-include config.mk

all: libjsmn.a 

libjsmn.a: jsmn.o
	$(AR) rc $@ $^

%.o: %.c jsmn.h
	$(CC) -fPIC -DPIC -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f libjsmn.a

