mount -r -t ext4 LABEL=boot /boot
kexec -l /boot/vmlinuz --initrd=/boot/initramfs.img --command-line="root=LABEL=root rootflags=subvol=root"
kexec -e
