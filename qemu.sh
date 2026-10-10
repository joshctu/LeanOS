#!/bin/sh
set -e
. ./iso.sh
. ./drive.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cpu pentium3 -drive file=virt_drive.img,format=raw,if=ide -cdrom LEANOS.iso -boot d -monitor stdio -display gtk

