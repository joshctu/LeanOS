#!/bin/sh

set -e

DISK="$HOME/OS/virt_drive.img" 
if [ -f "$DISK" ]; then
	echo "Disk already exists"
else

	qemu-img create -f raw virt_drive.img 50M
	mkfs.fat -F 16 virt_drive.img
fi 
