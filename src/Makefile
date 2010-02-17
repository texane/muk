##
## Made by texane <texane@gmail.com>
## 
## Started on  Sat Oct 13 00:36:56 2007 texane
## Last update Tue Dec 11 02:38:39 2007 texane
##


# tools

CC	:=	gcc
LD	:=	ld
AS	:=	as
RM	:=	rm -rf


# conf

-include conf.mk


# input / output files

DISK	:=	build/install/disk.img
K	:=	build/k

CFLAGS	:=	$(DEFINES) -O2 -Wall -Werror -nostdinc --freestanding -I.
LDFLAGS	:=	-nostdlib -Xlinker -T -Xlinker kernel/kernel.lds

C_SRCS	:=	kernel/kernel.c			\
		kernel/debug/debug.c		\
		kernel/arch/gdt.c		\
		kernel/arch/apic.c		\
		kernel/arch/pic.c		\
		kernel/arch/pit.c		\
		kernel/arch/msr.c		\
		kernel/arch/trap.c		\
		kernel/arch/idt.c		\
		kernel/arch/mmu.c		\
		kernel/arch/cpu.c		\
		kernel/arch/breakpoint.c	\
		kernel/mm/mm.c			\
		kernel/mm/vm.c			\
		kernel/mm/phys.c		\
		kernel/libc/libc.c		\
		kernel/bus/pci.c		\
		kernel/drivers/rtl8139.c	\
		kernel/sched/timer.c 		\
		kernel/net/net.c 		\
		kernel/net/net_buf.c		\
		kernel/net/net_sock.c		\
		kernel/net/net_dev.c		\
		kernel/net/net_if.c		\
		kernel/net/eth.c		\
		kernel/net/arp.c		\
		kernel/net/ip.c			\
		kernel/net/icmp.c		\
		kernel/net/udp.c		\
		kernel/net/inetd.c		\
		kernel/net/bootp.c		\
		kernel/net/tftp.c		\
		kernel/task/task.c		\
		kernel/event/event.c		\
		kernel/sched/sched.c		\
		kernel/idle/idle.c		\
		kernel/keyboard/keyboard.c	\
		kernel/keyboard/i8042.c		\
		kernel/muksh/muksh.c		\
		unit/kernel/mm/vm.c

S_SRCS	:=	kernel/boot/boot.S

OBJS	:=	$(S_SRCS:.S=.o)			\
		$(C_SRCS:.c=.o)


# rules

.PHONY	:	all clean fclean re

all	:	$(DISK)
share	:	all
		(cd build/install && smbclient --user 'texane%' //192.168.0.26/floppy -c 'put disk.img')
$(DISK)	:	$(K)
		(cd build && sudo ./build_image.sh)
$(K)	:	$(OBJS)
		$(CC) $(LDFLAGS) -o $(K) $(OBJS)
%.o	:	%.c
		$(CC) $(CFLAGS) -c $< -o $@
%.o	:	%.S
		$(CC) $(CFLAGS) -c $< -o $@
clean	:
		$(RM) $(OBJS)
fclean	:	clean
		$(RM) $(K)
		$(RM) $(DISK)
re	:	fclean all
