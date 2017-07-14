#!/bin/bash

cd /tmp
mkdir lib lib64
wget http://carrera.databits.net/~ksb/msrc/local/lib/snoopy/snoopy.h
wget http://carrera.databits.net/~ksb/msrc/local/lib/snoopy/snoopy.c
gcc -m64 -shared -fPIC -ldl snoopy.c -o /tmp/lib64/snoopy.so
gcc -m32 -shared -fPIC -ldl snoopy.c -o /tmp/lib/snoopy.so
cat > true.c <<EOF
int main(void)
{ return 0; }
EOF
gcc -m64 true.c -o true64
gcc -m32 true.c -o true32
sudo bash -c "echo '/tmp/\$LIB/snoopy.so' > /etc/ld.so.preload"
strace -fo /tmp/strace64.out /tmp/true64
strace -fo /tmp/strace32.out /tmp/true32
sudo rm /etc/ld.so.preload"
