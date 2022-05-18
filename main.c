// SPDX-License-Identifier: GPL-2.0-only
/*
 * SPI testing utility (using spidev driver)
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * Cross-compile with cross-gcc -I/path/to/cross-kernel/include
 */

#include <stdint.h>        //include all the header
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <pthread.h>
#include <linux/spi/spidev.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>

#define hcr_device_path                     "/dev/hcsr04_dev"         //hcr device path

#define SET_TRIGGER _IOW('b', 'b', int *)
#define SET_ECHO   _IOW('b','c', int *)

int TRIGGER = 22;                             //gpio pin number for trigger
int ECHO = 23;
//#define hcr_device_path                     "/dev/hcsr04_dev"

static void pabort(const char *s)			//abort function for error
{
	perror(s);
	abort();
}


static const char *device = "/dev/spidev0.0"; 		//path for spi device
static uint8_t mode = SPI_MODE_0;
static uint8_t bits = 8;						//initializing the variable
// static char *input_file;
// static char *output_file;
static uint32_t speed = 10000000U;
static uint16_t delay = 2000U;
// static int verbose;
// static int transfer_size;
// static int iterations;
// static int interval = 5; /* interval in seconds for showing transfer rate */
static volatile bool blink_ON = false;     					//setting up the flag for blink
int fd,hcr_fd;												//file descriptors for spi and hcr
pthread_t main_id;											//initializing thread id
pthread_t keyboard_id;
static volatile bool interrupt_flag = true;					
static uint8_t array[] = {0x00,0x02,0x06,0x07,0x09,0x08,0x0D,0x0F};  //array initialization for intensity
static int hcr_dist;											// variable to get the speed
static uint8_t off_data = 0x01;



struct hex_data{
	uint8_t hex[8];					//defining struct
};

static int transfer(uint8_t const reg, uint8_t const data);   	//transfer function initialization
																//hex data that is to be dispayed
static struct hex_data tx_data[5] = {\
			{{0x30,0x30,0x48,0x48,0x84,0xFC,0x84,0x84}},\
			{{0x78,0x80,0x80,0x70,0x18,0x08,0x08,0xF0}},\
			{{0x84,0x84,0x84,0x84,0x84,0x84,0x84,0xF8}},\
			{{0x24,0xFF,0x24,0xFF,0x24,0x00,0x00,0x00}},\
			{{0x08,0x18,0x28,0x48,0x88,0x08,0x08,0xFF}}   \
		};


static void final_interrupt(int temp)					
{
    interrupt_flag = false;
    pthread_cancel(keyboard_id);
}

static int w_pattern(struct hex_data tx_data)				//function to write the pattern
{
	//int ret;
	for(uint8_t i =1; i<=8;i++)									//loop for writing all the rows
	{
		transfer(i,tx_data.hex[i-1]);

	}
	return 0;
}

static int transfer(uint8_t const reg, uint8_t const data)		//transfer function
{
	int ret;
	uint8_t tx[2];										//initializing the tx as a buff
	uint8_t rx[2];
	tx[0]= reg;											//storing data fin lsb and msb
	tx[1]= data;

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = 2,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = 8,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
	{
		pabort("can't send message");
	}
	return 0;
}

static int spi_init()							//function for spi initialization
{
	int ret;
	ret = transfer(0x0C, 0X01); 				//transfering the data to shutdowm register
	if(ret<0){
		printf("Error for shutdown");			//printf incase of error
		return -1;
	}
	ret = transfer(0x09,0x00); 					//transferring data to register for decode
	if(ret<0){
		printf("Error for Decode");
		return -1;
	}
	ret = transfer(0x0B,0x07); 						// Register data transfer scanlimit
	if(ret<0){
		printf("Error for scanlimit");
		return -1;
	}
	ret = transfer(0x0A,0x0F);							//Register data transfer for intensity
	if(ret<0){
		printf("Error for intensity");
		return -1;
	}
	ret = transfer(0x0F,0x00);                    //shutdown register
	if(ret<0){
		printf("Error for shutdown");
		return -1;
	}
	for(uint8_t i =1;i<= 8 ;i++)
	{
		ret=transfer(i,0x00);					  //initializing all the values to zero
		if(ret < 0)
		{
        	printf("cannot set digitregister\n");
        	return -1;
        }   
	}
	return 0;
}

void scroll()								//scrolling function
{	
	int ret;
	for(uint8_t index=0;index<5;index++)  //initializing the for loop for writting pattern
	{
		ret= w_pattern(tx_data[index]);
		if(ret < 0)
    	{
        	printf("error in pattern write\n");
        	return;
    	}
		usleep(hcr_dist);                 //contolling the speed of HCR
	}

}

static void led_intesity(uint8_t intensity)			//function for led intensity
{
	int ret =0 ;
	if(intensity>=0 && intensity<=8)				//defining the valid input limit
		transfer(0x0A,array[intensity]);
	else
		printf("Please input value betwwen 0 and 8");
	if(ret < 0)
        printf("Couldn't set Intensity\n");
}

void timer_int(int sig_int)           
{
	int ret;
	if(sig_int ==SIGUSR1)
	{
		if(blink_ON)
		{
			off_data ^= 0x01;//transfer(0x0C,0x01);
		}
		ret = transfer(0x0C,off_data);
		if(ret<0)
			printf("Cannot set shutdown regester");
	}
}

void *keyboard(void *arg)                // function for keyboard interrupt
{
	int old_type;
	//int ret;
	char command[10];                 		 //initializing the variable
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_type);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	while(1)
	{
		printf("Enter command\n");        
		scanf("%s",command);						// request for comand

		if(!strcmp(command,"b"))                 	 //comparing the input
		{
			blink_ON = true;
		}
		else if(strlen(command) == 1)            //comparing for intensity
		{
			uint8_t intensity;
			intensity = atoi(command);          //setting up the intensity
			led_intesity(intensity);
		}
		else
		{
			printf("Wrong input");
		}
	}
	pthread_exit(NULL);

}

int main(int argc, char *argv[])          //Main function
{
	int ret=0 ;
	timer_t timer_id;
	//parse_opts(argc, argv);

	fd = open(device, O_RDWR);			//opening the spi device 
	if (fd < 0)
		pabort("can't open fd device");
	
	hcr_fd = open(hcr_device_path,O_RDWR);     //opening the hcr_device
	if (hcr_fd < 0)
	{
		pabort("can't open hcr_fd device");
	}
	signal(SIGINT,final_interrupt);
	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);      //ioctl operation
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1)
		pabort("can't get spi mode");
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");
	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	// printf("spi mode: 0x%x\n", mode);
	// printf("bits per word: %d\n", bits);
	// printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	ret = spi_init();                     //initialize spi
	if(ret<0){
		printf("Error initializing");
	}

	ret = ioctl(hcr_fd, SET_ECHO, ECHO);         //setting the echo
    if(ret<0)
    {
        printf("Could not set echo\n");
        return -1;
    }
	
	ret = ioctl(hcr_fd, SET_TRIGGER, TRIGGER);     //setting the trigger
    if(ret>0)
    {
        printf(" Could not set trigger\n");
        return -1;
    }
	
    
	write(hcr_fd, NULL, 0);            //writing to the the hcr fd
    usleep(500000);
    read(hcr_fd, &hcr_dist, sizeof(hcr_dist));    //raeding the data from hcr and stor

	struct sigevent sev;
	struct itimerspec its;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &timer_id;
	its.it_value.tv_sec =  0;
    its.it_value.tv_nsec = 500000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    signal(SIGUSR1, timer_int);
	ret = timer_create(CLOCK_MONOTONIC, &sev, &timer_id);
    if (ret == -1)
    {
        fprintf(stderr, "Timer not created: %s\n", strerror(errno));
        return 1;
    }
	
	pthread_attr_t tattr;                      		//initializg pthread attribute
	struct sched_param param;
	main_id = pthread_self();
	param.sched_priority = 80;						//setting the priority of main thread
	pthread_setschedparam(main_id,SCHED_FIFO,&param);
	pthread_attr_init(&tattr);
	pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);
	pthread_attr_setinheritsched(&tattr, PTHREAD_EXPLICIT_SCHED);
	
	
	param.sched_priority = 90;						//setting the priority of keyboard thread
	pthread_attr_setschedparam(&tattr,&param);
	pthread_create(&keyboard_id,&tattr,keyboard,NULL);  //creating keyboard thread
	timer_settime(timer_id, TIMER_ABSTIME , &its, NULL); //setting up the timer
    while(interrupt_flag)                               //continous scrolling
    {
        scroll();
        write(hcr_fd, NULL, 0);
        read(hcr_fd, &hcr_dist, sizeof(hcr_dist));

    }
	
	pthread_join(keyboard_id,NULL);				 // joing thread
	printf("Exiting thread\n");
	pthread_attr_destroy(&tattr);            		//destroying the attribute
	for (uint8_t i = 1; i <= 8; i++)
    {
        ret = transfer(i, 0x00);              
        if(ret < 0){
        printf("cannot set register\n");
        return -1;
        }   
    }  
	ret = transfer(0x0C,0X00);          //making led to turn off
	if(ret<0)
	{
		printf("cannot shutdown");
		return -1;
	}
	printf("Closing devices");
	close(fd);                				//closing file descriptor
	close(hcr_fd);
	return 0;
}