#!/bin/sh

mount -r -t ext4 LABEL=boot /boot
kexec -l /boot/vmlinuz-linux --initrd=/boot/initramfs-linux.img --command-line="root=LABEL=root rootflags=subvol=root"
kexec -e

