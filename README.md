# Chromebook minimal busybox installation

Install a minimal busybox installation on an ASUS C200M Chromebook (Baytrail). This model does not have yet a working SEABIOS version, so we use the chrome normal booting method to boot our own custom kernel.


## ssh keys

Let us ssh into the chromebook in dev mode.

https://chromium.googlesource.com/chromiumos/chromite/+/master/ssh_keys

    ssh -i ~/.ssh/testing_rsa root@chromebook_ip

## dump the partitions

    dd if=/dev/mmcblk0pX of=mmcblk0pX


## vboot_reference

CFLAGS : remove -Werror
make utils futil cgpt

## Generate keys

    openssl genrsa -F4 4096 > key.pem
    openssl req -batch -new -x509 -key key.pem > key.crt
    dumpRSAPublicKey -cert key.crt > key.keyb

    futility vbutil_key --pack key.vbpubk  --key key.keyb --algorithm 8 --version 1
    futility vbutil_key --pack key.vbprivk --key key.pem  --algorithm 8
    futility vbutil_keyblock --pack key.keyblock --datapubkey key.vbpubk --flags 15

## Kernel

    curl -O https://kernel.org/pub/linux/kernel/v4.x/testing/linux-4.0-rc7.tar.xz
    tar xvf linux-4.0-rc7.tar.xz
    cd linux-4.0-rc7
    make mrproper
    cp ../config-kernel .config
    ARCH=x86_64 make nconfig
    ARCH=x86_64 make -j4

### Build the kernel partition

    echo 'root=/dev/mmcblk0p3' > cmdline
    futility vbutil_kernel --pack   kern.bin --keyblock key.keyblock --signprivate key.vbprivk --version 1  --config cmdline --vmlinuz linux-4.0-rc7/arch/x86_64/boot/bzImage
    scp -i ~/.ssh/testing_rsa kern.bin  root@192.168.0.104:/root

### Set partition as bootable

    dd if=kern.bin of=/dev/mmcblk0p2    
    cgpt add -i 2 -S 0 -T 1 -P 5 /dev/mmcblk0

## Sysroot

    mkdir -p sys-dev/{include,lib}
    mkdir -p sys/{bin,dev,etc,lib,mnt,proc,root,sbin,sys,tmp}
    # TODO copy rcS
    # TODO fstab
    # TODO keymap
    # TODO dhcp
    ln -s /tmp/resolv.conf sys/etc/resolv.conf

## Busybox

    curl -O http://busybox.net/downloads/busybox-1.23.2.tar.bz2
    tar xvf busybox-1.23.2.tar.bz2 
    cd busybox-1.23.2
    make menuconfig
    make -j8 CC=musl-gcc
    make CC=musl-gcc CONFIG_PREFIX=../sys install

## kexec-tools

    curl -O https://kernel.org/pub/linux/utils/kernel/kexec/kexec-tools-2.0.9.tar.xz
    tar xvf kexec-tools-2.0.9.tar.xz
    cd kexec-tools-2.0.9
    CC=musl-gcc CFLAGS="-Dloff_t=off_t -Os" LDFLAGS=-static ./configure 
    make -j8
    strip build/sbin/kexec
    cp build/sbin/kexec ../sys/bin

## util-linux

    curl -O https://www.kernel.org/pub/linux/utils/util-linux/v2.26/util-linux-2.26.tar.xz
    tar xvf util-linux-2.26.tar.xz
    cd util-linux-2.26
    CC=musl-gcc CFLAGS=-Os ./configure --disable-all-programs --enable-libuuid --enable-libblkid
    make -j8
    mkdir -p ../sys-dev/include/{uuid,blkid}
    cp .libs/libuuid.a .libs/libblkid.a ../sys-dev/lib
    cp libuuid/src/uuid.h ../sys-dev/include/uuid
    cp libblkid/src/blkid.h ../sys-dev/include/blkid

## zlib

    curl -O http://zlib.net/zlib-1.2.8.tar.gz
    tar xvf zlib-1.2.8.tar.gz
    cd zlib-1.2.8
    CC=musl-gcc ./configure --static
    make -j8
    cp zlib.h zconf.h ../sys-dev/include
    cp libz.a ../sys-dev/lib

## lzo

    curl -O http://www.oberhumer.com/opensource/lzo/download/lzo-2.09.tar.gz
    tar xvf lzo-2.09.tar.gz
    cd lzo-2.09
    CC=musl-gcc CFLAGS=-Os LDFLAGS=-static ./configure
    make -j8
    cp src/.libs/liblzo2.a ../sys-dev/lib
    cp -a include/lzo ../sys-dev/include
    
## btrfs-progs

    git clone git://git.kernel.org/pub/scm/linux/kernel/git/kdave/btrfs-progs.git
    cd btrfs-progs
    git checkout -b v3.19.1 v3.19.1
    ./autogen.sh
    CC=musl-gcc CFLAGS='-I../sys-dev/include -Os' LDFLAGS='-static -L../sys-dev/lib/' ./configure --disable-documentation --disable-convert --disable-backtrace
    # Remove /usr/lib from the generated Makefile
    make -j8
    strip btrfs mkfs.btrfs
    cp btrfs mkfs.btrfs ../sys/bin

## libnl

    curl -O http://www.infradead.org/~tgr/libnl/files/libnl-3.2.25.tar.gz
    tar xvf libnl-3.2.25.tar.gz
    cd libnl-3.2.25
    CC=musl-gcc ./configure --disable-pthreads --disable-debug --disable-shared --disable-cli 
    make -j8
    cp lib/.libs/*.a ../sys-dev/lib/
    cp -a include/netlink ../sys-dev/include/

## iw
    curl -O https://www.kernel.org/pub/software/network/iw/iw-4.0.tar.xz
    tar xvf iw-4.0.tar.xz
    cd iw-4.0
    CC=musl-gcc CFLAGS=-I../sys-dev/include LDFLAGS='-static -L../sys-dev/lib' make -j4
    strip iw
    cp iw ../sys/bin

## wpa_supplicant
    
    curl -O http://w1.fi/releases/wpa_supplicant-2.4.tar.gz
    tar xvf wpa_supplicant-2.4.tar.gz
    cd wpa_supplicant-2.4/wpa_supplicant
    cp ../../wpa_supplicant.config .config
    make -j8
    strip wpa_supplicant wpa_cli wpa_passphrase
    cp wpa_supplicant wpa_cli wpa_passphrase ../../sys/bin

## cgpt

    git clone https://chromium.googlesource.com/chromiumos/platform/vboot_reference
    cd vboot_reference
    # TODO: patch
    CC=musl-gcc CFLAGS=-I../sys-dev/include LDFLAGS='-L../sys-dev/lib -static' make cgpt
    strip build/cgpt/cgpt
    cp build/cgpt/cgpt ../sys/bin

## e2fs

    curl -O https://www.kernel.org/pub/linux/kernel/people/tytso/e2fsprogs/v1.42.12/e2fsprogs-1.42.12.tar.xz
    tar xvf e2fsprogs-1.42.12.tar.xz
    cd e2fsprogs-1.42.12
    CC=musl-gcc CFLAGS="-I$PWD/../sys-dev/include -Os" LDFLAGS="-static -L$PWD/../sys-dev/lib" ./configure --disable-uuidd --disable-libblkid --disable-libuuid
    make -j4
    strip ./misc/mke2fs ./misc/tune2fs ./e2fsck/e2fsck
    cp ./misc/mke2fs ./misc/tune2fs ./e2fsck/e2fsck ../sys/bin

## curl

    curl -O http://curl.haxx.se/download/curl-7.41.0.tar.lzma
    tar xvf curl-7.41.0.tar.lzma
    cd curl-7.41.0
    CC=musl-gcc CFLAGS="-I$PWD/../sys-dev/include -Os -Wl,-static" LDFLAGS="-L$PWD/../sys-dev/lib" ./configure --with-zlib --disable-verbose --disable-libcurl-option --disable-rtsp --disable-shared --disable-file --disable-ldap --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-gopher
    make -j4
    strip src/curl
    cp src/curl ../sys/bin

## openssl
## dropbear

    curl -O https://matt.ucc.asn.au/dropbear/dropbear-2015.67.tar.bz2
    tar xvf dropbear-2015.67.tar.bz2
    cd dropbear-2015.67
    CC=musl-gcc CFLAGS="-I$PWD/../sys-dev/include -Os" LDFLAGS="-L$PWD/../sys-dev/lib" CC=musl-gcc CFLAGS="-I$PWD/../sys-dev/include -Os" LDFLAGS="-static -L$PWD/../sys-dev/lib"  ./configure --disable-lastlog --disable-wtmp --disable-syslog --disable-shadow
    make PROGRAMS="dropbear dbclient scp" MULTI=1 STATIC=1 -j8
    make PROGRAMS=dropbearkey STATIC=1
    make PROGRAMS=dropbearconvert STATIC=1
    strip dropbearmulti dropbearkey dropbearconvert
    cp dropbearmulti ../sys/bin
    ln -s dropbearmulti ../sys/bin/dropbear
    ln -s dropbearmulti ../sys/bin/ssh
    ln -s dropbearmulti ../sys/bin/scp
    ./dropbearkey -t ecdsa -f ../sys/etc/ecdsa_host_key
    cat ~/.ssh/testing_rsa.pub > ../sys/etc/authorized_keys

## flashrom

## create filesystem

    mksquashfs  sys sys.sqsh -comp lz4 -all-root -noappend
    scp -i ~/.ssh/testing_rsa sys.sqsh root@192.168.0.104:/root
    # dd if=sys.sqsh of=/dev/mmcblk0p3

## Misc

### URL

- ~~https://chromium.googlesource.com/chromiumos/third_party/bootstub~~
- https://chromium.googlesource.com/chromiumos/third_party/kernel
- https://chromium.googlesource.com/chromiumos/platform/vboot_reference
- https://chromium.googlesource.com/chromiumos/platform/depthcharge
- http://review.coreboot.org/p/coreboot.git

### Repack an existing partition

    futility vbutil_kernel --repack kern.bin --keyblock key.keyblock --signprivate key.vbprivk --config cmdline --oldblob mmcblk0pX

### Connect to the wifi

wpa_passphrase MYSSID passphrase > /tmp/wpa.conf
wpa_supplicant -B -i interface -c /tmp/wpa.conf
udhcpc -i wlan0


