Accessing kernel hash tables via a device file interface
---------------
Subject - CSE 438
Assignment 3
Author - Kalyani Niwrutti Ingole
ASU Id - 1223133028
---------------

ReadMe

###
system Requirement 

Raspberry pi 4B
gcc Compiler 

###

We have to create a device driver "ht438_drv" and create charater device ht438_dev_0 and ht438_dev_1.
Each device initializes a hashtable ht438_tbl_0 and ht438_tbl_1 respectively.

###

Build 
We need add 4 files two files as input and 2 files as output. 
output files are t1_out and t2_out.
We have made a make file that includes the compiler and other commands that generate any warning needed
Import the header file provided in the main assignment file for compilation.
To check for errors and compilation of the code use 'make all' command

###

Execution
make sure the rpi is powered ON
"sudo insmod ht438_drv.ko"  - To add kernel object ko root
To run the file use 'sudo ./assignment3 t1 t2'
once the the code is exceuted the read and write operation can be seen in the t0_out and t1_out file.
After the termination of code use the following command
"sudo rmmod ht438_drv.ko" - to remove kernel object from root
make clean to clean the project