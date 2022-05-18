Assignment 4
---------------
Subject - CSE 438
Assignment 4
Author - Kalyani Niwrutti Ingole
ASU Id - 1223133028
----------------

ReadMe
###
System requirement

gcc Compiler 
Raspberry pi 4B 2GB
SD card 16GB

######

SPI Device Programming and Distance-controlled Text Scrolling

######
We have to create a device driver "hcsr_drv" and create charater device hcsr04_dev in kernel.
SPI based user application for working of MAX7219.
Integrate them to create a application with serier of character whoes scrolling speed can be changed with the Ultra sonic sensor linked through the driver.


######
Build

We have made a make file that includes the compiler and other commands that generate any warning needed
Import the header file provided in the main assignment file for compilation
Few other header like #include <time.h> #include <signal.h> needs to be included.
To check for errors and compilation of the code use 'make all' command

######
Execution
make sure the rpi is powered ON
Do the wiring of the rpi with led matrix and Ultrasonic sensor. 
Connect the trigger pin to GPIO 22, echo to 23 of HCSR sensor.
before the exceution run the following command in the terminal
"sudo insmod hcsr_drv.ko"  - To add kernel object ko root
To run the file use 'sudo ./assignment4'
once the code is exceuted without to get the desired output write the following command
	1] b , led will start blinking
	2] integer from 1 to 8 , for intensity change
	3] crtl C to end the program
	writting this in the terminal will terminate the code.
After the termination of code use the following command
"sudo rmmod hcsr_drv" - to remove kernel object from root
and then make clean. 