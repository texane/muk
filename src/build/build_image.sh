#!/bin/sh
## Made by texane <texane@gmail.com>
## 
## Started on  Sat Oct 13 01:07:51 2007 texane
## Last update Sat Oct 13 01:30:52 2007 texane
##


# prepare the install environment
GRUBDIR=
SBIN_GRUB=
DEVICE=
MNT=
KERNEL=
do_environ() {
    # set global variable
    GRUBDIR=grub/
    SBIN_GRUB=/sbin/grub
    DEVICE=disk.img
    MNT=mnt
    KERNEL=./k
}

# clean a previous environ
do_cleanup() {
    if [ -d $MNT ] ; then
	umount $MNT > /dev/null 2>&1
	rm -rf $MNT
    fi
    [ -e $DEVICE ] && rm -f $DEVICE
    [ -e menu.lst ] && rm -f menu.lst
}


# pre build operations
do_prebuild() {
    # create files
    mkdir $MNT
}


# build grub filesystem
do_build() {
    # make the device
    dd if=/dev/zero of=$DEVICE count=1440 bs=1k

    # make fat filesystem
    mkfs -t vfat $DEVICE

    # populate the filesystem
    mount -o loop -t vfat $DEVICE $MNT

    # grub files
    mkdir -p $MNT/boot/grub
    cp grub/stage1 $MNT/boot/grub/
    cp grub/stage2 $MNT/boot/grub/

    # install grub
    $SBIN_GRUB --batch --no-floppy <<EOT 1>/dev/null 2>/dev/null || exit 1
device (fd0) $DEVICE
install (fd0)/boot/grub/stage1 (fd0) (fd0)/boot/grub/stage2 p (fd0)/boot/grub/menu.lst
quit
EOT

    # create menu.lst
    cat << EOF > menu.lst
timeout 0
default 0
title  kernel
root   (fd0)
kernel /kernel/k
EOF
    cp menu.lst $MNT/boot/grub/

    # install the kernel
    mkdir $MNT/kernel
    cp $KERNEL $MNT/kernel/

    # unmount the filesystem
    umount $MNT
}


# install resulting files
do_postbuild() {
    [ -d install ] || mkdir install
    cp $DEVICE install/
    rm $DEVICE
}


# entry point
do_environ
do_cleanup
do_prebuild
do_build
do_postbuild