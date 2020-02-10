#!/bin/bash

# Author: Pavan Kumar Paluri

# check if the user is sudo or not

#  mkdir -p /mnt/vmdisk
#  cd /mnt/vmdisk
#  echo "Current Directory: $(pwd)"

if [ $(whoami) != "root" ]; then
	echo "Only root user access is allowed"
	exit 1
else
    mkdir -p /mnt/vmdisk
    cd /mnt/vmdisk
    echo "Current Directory: $(pwd)"
    echo "$(ls)"
    dd if=/dev/zero of=./mydisk1 bs=1024M count=32
    if [ $? -eq 0 ]; then
	echo "Records created and copied successfully"
    else
	echo "redo rest of domain configuration steps manually"
    fi
    CHECKER=(file mydisk1)
    echo " mydisk1 has $CHECKER"
    losetup /dev/loop4 ./mydisk1
    #losetup ./mydisk1 /dev/sda1
    #losetup /dev/loop3 ./mydisk1
    pvcreate /dev/loop4
    echo "file mydisk1 now has:$(file mydisk1)"
    #2048 here is the size of the PE (physical extent), which is a basic unit of the memory
    vgcreate myvolumegroup1 /dev/loop4
    lvcreate  -l 100%VG -n mylv1 myvolumegroup1
    #lvcreate -L 4G -n mylv2 myvolumegroup1
    #echo "$(ls /dev/myvolumegroup1)"
    #mkdir -p /var/lib/xen/images/cent-netboot
    #cd /var/lib/xen/images/cent-netboot
    rm ./*
   #############  COMMENTED ON OCT 27, 2019 ##########
   #  wget http://archive.ubuntu.com/ubuntu/dists/precise-updates/main/installer-amd64/current/images/netboot/xen/vmlinuz
   #  wget http://archive.ubuntu.com/ubuntu/dists/precise-updates/main/installer-amd64/current/images/netboot/xen/initrd.gz
   ############ COMMENT ENDS HERE ###################
    # CENT OS Kernel..
    #wget http://vault.centos.org/7.2.1511/os/x86_64/images/pxeboot/vmlinuz
   #wget http://mirror.globo.com/centos/6/os/x86_64/isolinux/vmlinuz
    # CENT OS initrd image ...
    #wget http://vault.centos.org/7.2.1511/os/x86_64/images/pxeboot/initrd.img
   #wget http://mirror.globo.com/centos/6/os/x86_64/isolinux/initrd.img 
   echo "$(ls)"
   # cd /etc/xen
   # echo "All controls checked, Ready to launch the domain NOW!!! - Pavan!!"
   # xl create -c domu1.cfg
    if [ $? -eq 0 ]; then
	echo "vif- Ethernet Connection failure, Probably"
	echo "Dont panic!!, Pavan's script can fix that..yipeee"
        brctl addbr xenbr0
	brctl addif xenbr0 eth0
	ip link set dev xenbr0 up
	#xl create -c domu1.cfg
   else
	echo "Activation begins, sit tight! Show's about to begin!!"
    fi

fi


