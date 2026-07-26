#!/bin/sh

set -e

DISK="$HOME/OS/virt_drive.img" 
if [ -f "$DISK" ]; then
	echo "Disk already exists"
else

	qemu-img create -f raw virt_drive.img 50M
	mkfs.fat -F 16 virt_drive.img


	sudo losetup -fP virt_drive.img
	sudo mount /dev/loop0 /mnt/fat

	cd /mnt/fat
	sudo mkdir home
	cd home
	echo "Welcome to LeanOS!" | sudo tee readme.txt > /dev/null
	echo "def addition(a, b):
    return a + b
print(addition)" | sudo tee add.py > /dev/null
	sudo mkdir lean
	sudo mkdir test
	sudo mkdir bin
	sudo mkdir kernel
	sudo mkdir lib

	cd $HOME/OS
	sudo umount /mnt/fat
	sudo losetup -D

fi 
