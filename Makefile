APP = main.o
SROOT = /opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux/i586-poky-linux-gcc
SRC = /opt/iot-devkit/1.7.2/sysroots/i586-poky-linux/usr/src/kernel
obj-m:= RGBLed.o

galileo:
	make -C $(SRC) M=$(PWD) modules -Wall
	$(SROOT) -pthread -Wall -o $(APP) main.c
clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.symvers
	rm -f modules.order
	rm -f *.mod.c
	rm -rf .tmp_versions
	rm -f *.mod.c
	rm -f *.mod.o
	rm -f \.*.cmd
	rm -f Module.markers
	rm -f $(APP) 

scp:
#Change this IP before using
	scp RGBLed.ko root@192.168.1.6:/media/realroot
	scp main.o root@192.168.1.6:/media/realroot
	scp input.txt root@192.168.1.6:/media/realroot
	ssh root@192.168.1.6
ssh:
	ssh root@192.168.1.6
