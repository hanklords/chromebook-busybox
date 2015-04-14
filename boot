#!/bin/sh

device=/dev/mmcblk0p3

mount -o subvol=boot,ro $device /boot
kexec -l /boot/vmlinuz-linux --initrd=/boot/initramfs-linux.img --command-line="root=$device rootflags=subvol=root"
kexec -e

