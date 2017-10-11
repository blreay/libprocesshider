all: libprocesshider.so libprocesshider32.so

libprocesshider.so: processhider.c
	gcc -m64 -Wall -fPIC -shared -o libprocesshider.so processhider.c -ldl
	strip $@

libprocesshider32.so: processhider.c
	gcc -m32 -Wall -fPIC -shared -o libprocesshider32.so processhider.c -ldl
	strip $@

.PHONY clean:
	rm -f *.o *.so
