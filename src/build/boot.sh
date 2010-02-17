#!/bin/sh


#
# environ
#

export PATH=$PATH:/usr/local/bin/



#
# ethernet device
#

setup_ethernet_device() {
    ifconfig eth0 0.0.0.0 promisc up
}

> /dev/null 2>&1 ifconfig eth0 || setup_ethernet_device



#
# ethernet bridge
#

setup_ethernet_bridge() {
    modprobe bridge > /dev/null 2>&1
    brctl addbr br0 > /dev/null 2>&1
    brctl addif br0 eth0 > /dev/null 2>&1
    ifconfig br0 192.168.0.44 up

    [ -c /dev/net/tun ] || ( mkdir /dev/net/ && mknod /dev/net/tun c 10 200 ) > /dev/null 2>&1
    modprobe tun > /dev/null 2>&1
    tunctl -t tap0 > /dev/null 2>&1
    ifconfig tap0 up

    brctl addif br0 tap0 > /dev/null 2>&1

    route add default gw 192.168.0.1 netmask 255.255.255.0
}

> /dev/null 2>&1 ifconfig br0 || setup_ethernet_bridge



#
# boot the kernel
#

# qemu	-nographic -serial stdio				\
qemu	-serial stdio						\
	-boot a -fda install/disk.img -m 32			\
	-net nic,macaddr=2a:2a:2a:2a:2a:2a,model=rtl8139	\
	-net tap,ifname=tap0


#
# remove ethernet bridging
#

delete_ethernet_bridge() {
    ifconfig br0 down > /dev/null 2>&1
    brctl delbr br0 > /dev/null 2>&1
}

# delete_ethernet_bridge
