#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cpu pentium3 -monitor stdio -cdrom LEANOS.iso

