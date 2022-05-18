#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/mutex.h>

#include<linux/init.h>
#include<linux/moduleparam.h>

#define SET_TRIGGER _IOW('b', 'b', int *)
#define SET_ECHO   _IOW('b','c', int *)

#define DEVICE_NAME                 "hcsr04_dev"  // device name to be created and registered
											//initializing the stuct 
typedef struct{
    char name[20];
	struct mutex mutex;
    struct cdev cdev;
	char progress_flag;
	unsigned long dist;
	struct gpio_desc *gpiod_trigger_pin;
	struct gpio_desc *gpiod_echo_pin;
}hcr_dev_t;

hcr_dev_t *hcr_dev;

int E_flag;
ktime_t ti,tf;

static dev_t hcr_dev_number;      /* Allotted device number */
struct class *hcr_dev_class;          /* Tie with the device model */

unsigned int irq;


//int ret;
static irqreturn_t irq_handler(int irq, void *dev_id)
{
	//printk(KERN_ALERT " IRQ Initialization");

	hcr_dev_t *hcr_devp = (hcr_dev_t *)dev_id;
	mutex_lock(&hcr_devp->mutex);               //locking the resource

	if(hcr_devp->progress_flag) {
		// calculation of initial time 
		if(!E_flag) {
			ti = ktime_get();
			E_flag = 1;
		}
		else {
			tf = ktime_get();        	// calculation of final time
			hcr_devp->dist = (unsigned long)(tf - ti);        //storing the distance
			hcr_devp->dist /= 10;
			if(hcr_devp->dist < 10000) {
				hcr_devp->dist = 10000;
			}
			else if(hcr_devp->dist > 1000000) {
				hcr_devp->dist = 1000000;
			}
			E_flag = 0;
			hcr_devp->progress_flag = 0;
		}
	}
	mutex_unlock(&hcr_devp->mutex);           //freeing the resource

	return IRQ_HANDLED;
}

int hcr_drv_open(struct inode *inode, struct file *file)        //open function for driver
{

	//checked no errors
	// printk("MYCHARDEV: Device open\n");
    // return 0;
	hcr_dev_t *hcr_devp;
	//struct ht438_dev *ht438_devp[1];

	printk("\nopening\n");

	/* Get the per-device structure that contains this cdev */
	hcr_devp = container_of(inode->i_cdev, hcr_dev_t, cdev);
	//ht438_devp[1] = container_of(inode->i_cdev, struct ht438_dev, cdev);
	hcr_devp->progress_flag =0;

	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = hcr_devp;
	//file->private_data = ht438_devp[1];

	printk("\n%s is openning \n", hcr_devp->name);
	// printk("\n%s is openning \n", ht438_devp->name);

	return 0;
}

int hcr_drv_release(struct inode *inode, struct file *file)   //release function for driver
{
	hcr_dev_t *hcr_devp = file->private_data;
	if(hcr_devp->gpiod_trigger_pin) 
	{
		gpiod_put(hcr_devp->gpiod_trigger_pin);
	}

	if(hcr_devp->gpiod_echo_pin) 
	{
		gpiod_put(hcr_devp->gpiod_echo_pin);
	}
	
	free_irq(irq, hcr_devp);                     //freeing the irq

    printk("\n%s is closing\n", hcr_devp->name);

    return 0;
}

ssize_t hcr_drv_read(struct file *file, char *buf, size_t count, loff_t *ppos) //read function
{
	//printk(KERN_ALERT "READING DATA \n");
	hcr_dev_t *hcr_devp = file->private_data;
	if(copy_to_user((unsigned long *)buf, &hcr_devp->dist, sizeof(unsigned long))) //copy the distance to user space
	{
		printk("Error in read\n");
		return -1;
	}
	printk(KERN_ALERT "DONE READING \n");

	return 0;

}

int hcr_drv_write(struct file *file, const char *buf, size_t count, loff_t *ppos)  //write function
{
	hcr_dev_t *hcr_devp = file->private_data;
	if(!hcr_devp->progress_flag) 
	{
		mutex_lock(&hcr_devp->mutex);          //locking the resource
		printk(KERN_ALERT "WRITING \n");
		gpiod_set_value(hcr_devp->gpiod_trigger_pin, 1);  //setting the trigger pin to high
		udelay(10);
		gpiod_set_value(hcr_devp->gpiod_trigger_pin, 0);    //setting the trigger pin to low
		hcr_devp->progress_flag = 1;

		mutex_unlock(&hcr_devp->mutex);                      //releasing the mutex
		printk(KERN_ALERT "DONE WRITING \n");

	}
	else 
	{
		printk("Busy\n");
	}

	return count;
}

static long hcr_drv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)  //ioctl function
{
	int ret = 0;

	hcr_dev_t *hcr_devp = file->private_data;

	unsigned int set = (unsigned int)arg;
	printk(KERN_ALERT "IOCTL Initialized \n");

	switch(cmd)
	{
		case SET_TRIGGER:                                     //setting the trigger
			printk(KERN_ALERT "CHECKING CASES FOR TRIGGER \n");
			hcr_devp->gpiod_trigger_pin = gpio_to_desc(set);      //passing GPIO number of trigger to gpio_to_desc
			if(!hcr_devp->gpiod_trigger_pin) {
				printk(KERN_DEBUG "GPIO not present\n");
				return -1;
			}

			ret = gpiod_direction_output(hcr_devp->gpiod_trigger_pin, 0);        //setting the direction of trigger pin
			if(ret) {
				printk(KERN_ALERT "Error setting direction for GPIO \n");
				return -1;
			}
			break;

		case SET_ECHO:                       //setting the echo
			hcr_devp->gpiod_echo_pin = gpio_to_desc(set);    //passing GPIO number of echo to gpio_to_desc
			if(!hcr_devp->gpiod_echo_pin) {
				printk(KERN_DEBUG "GPIO not present\n");
				return -1;
			}

			ret = gpiod_direction_input(hcr_devp->gpiod_echo_pin);  //setting the direction
			if(ret) {
				printk(KERN_DEBUG "Direction error");
				return -1;
			}

			irq = gpiod_to_irq(hcr_devp->gpiod_echo_pin);
			if(irq < 0) {
				printk(KERN_DEBUG "GPIO not requested\n");
				return -1;
			}

			ret = request_irq(irq, irq_handler, (IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING), "Dist IRQ", hcr_devp);
			break;

		default:
			printk(KERN_DEBUG "Invalid Command\n");
			return -1;
	}

	return 0;
}
static struct file_operations fops = {
    .owner		     = THIS_MODULE,           /* Owner */
    .open		     = hcr_drv_open,        /* Open method */
    .release	     = hcr_drv_release,     /* Release method */
	.read		     = hcr_drv_read,        /* Read method */
    .write		     = hcr_drv_write,       /* Write method */
	.unlocked_ioctl  = hcr_drv_ioctl,		/*ioctl method*/
};

int __init hcr_drv_init(void)                  //initializing driver
{
	int ret;										//variable 
	printk(KERN_ALERT " Started Initialization");

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&hcr_dev_number, 0, 1, DEVICE_NAME)) 
	{
		printk(KERN_DEBUG "Can't register device\n"); 
		
		return -1;
	}

	printk(KERN_ALERT " Done Initialization");

		/* Populate sysfs entries */
	// hcr_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	// sprintf(hcr_dev->name,DEVICE_NAME);
	/* Allocate memory for the per-device structure */
	//ht438_devp = kmalloc(sizeof(struct ht438_dev), GFP_KERNEL);

	hcr_dev = kmalloc(sizeof(hcr_dev_t), GFP_KERNEL);
	if(!hcr_dev) 
	{
		printk("Bad Kmalloc\n"); 
		return -ENOMEM;
	}
	hcr_dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	sprintf(hcr_dev->name,DEVICE_NAME);

	cdev_init(&hcr_dev->cdev, &fops);
	hcr_dev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&hcr_dev->cdev,MKDEV(MAJOR(hcr_dev_number),0), 1);
	if(ret) 
	{
		printk("Bad cdev\n");
		return ret;
	}
		//hash_init(ht438_devp->ht438_tbl);
	device_create(hcr_dev_class, NULL, MKDEV(MAJOR(hcr_dev_number), 0), NULL, hcr_dev->name);	
	mutex_init(&hcr_dev->mutex);

    printk("Device Initialized %s\n", hcr_dev->name);

	return ret;
}

void __exit hcr_drv_exit(void)        //exit function for module
{
	// device_remove_file(gmem_dev_device, &dev_attr_xxx);
	unregister_chrdev_region((hcr_dev_number), 1);  //unregister the region

    device_destroy (hcr_dev_class, MKDEV(MAJOR(hcr_dev_number),0));  //destroy the class
	cdev_del(&hcr_dev->cdev);   //delete the device
	kfree(&hcr_dev);            //free the device

	
	/* Destroy driver_class */
	class_destroy(hcr_dev_class);   
	printk("Device deleted");

	printk("hcr_drv driver removed.\n");
}

module_init(hcr_drv_init);
module_exit(hcr_drv_exit);
MODULE_LICENSE("GPL v2");
