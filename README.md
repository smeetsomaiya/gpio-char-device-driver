# GPIO Char Device driver to control an RGB LED

ABOUT:
The objective of the project is to develop a char device driver to control  RGB led display. It demonstrates the usage of GPIO pins in kernel in order to interface with RGBLed and implementation of gpio multiplexing on Galileo Gen 2 board. The char device driver controls the sequence of the RGBLed which switches after every 0.5s. On the occurrence of mouse event, pattern starts from the initial state.

Code files:
		RGBLed.c
		main.c
		main.h
		input.txt
		Makefile

PREREQUISITES:
Linux machine is used as host and galileo Gen2 board as target.
Linux Kernel of minimum v2.6.19 is required.
SDK:iot-devkit-glibc-x86_64-image-full-i586-toolchain-1.7.2.sh
GCC used i586-poky-linux-gcc
USB mouse
Bread-board, wires and Red-Green-Blue LED Module.
STEPS TO SETUP AND COMPILE:
Set up Galileo board by connecting RGB Led module using wires. Connect  to Red, Blue, Green and GND  pin. Connect the USB mouse.

		Note: The module only supports GPIOs connected to the Quark controller 
		i.e only Linux gpio8 to gpio15 which map to IO0, IO1, IO10, IO12

STEPS TO COMPILE:
To cross-compile for the galileo:
Open the make file and change the path of SROOT. Point it to your i586-poky-linux-gcc
Also, change the IP address to that of the board in the “scp” block.
Navigate to the path where all the files are stored and open the terminal and type  

		make galileo 
This will generate the executables (.ko and .o files), next:

		make scp
This will transfer the required files to the board and take you to the Galileo shell, where the module can be installed.

STEPS TO EXECUTE:
Once in the Galileo shell:

Export the mouse event type:
            export MOUSE_DEV=‘/dev/input/event2’

Select the path 
		cd /media/realroot

To install the RGBLed driver type: 
		insmod RGBLed.ko

To execute the program, after installing the driver(RGBLed.ko) type: 
		./main.o

To capture the kernel log use: 
		cat /dev/kmsg
OR
		dmesg

EXPECTED OUTPUT:
On execution, RGB Leds glow in a sequence as specified ( R-G-B-RG-RB-GB-RGB) with a duty cycle of 50% and each pattern runs for 0.5s . On the occurrence of any mouse event, the current executing  step in (lightup_thread) sequence is terminated  and the cycle starts from the initial stage.
Invalid inputs are handled by generating error EINVAL.