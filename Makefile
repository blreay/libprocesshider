all: libprocesshider.so

libprocesshider.so: processhider.c
	gcc -Wall -fPIC -shared -o libprocesshider.so processhider.c -ldl
	strip $@

.PHONY clean:
	rm -f libprocesshider.so
