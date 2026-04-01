#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/LEANOS.kernel isodir/boot/LEANOS.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "leanOS" {
	multiboot /boot/LEANOS.kernel debug=true loglevel=2
}
EOF
grub-mkrescue -o LEANOS.iso isodir
