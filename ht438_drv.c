#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include<linux/init.h>
#include<linux/moduleparam.h>
#include<linux/hashtable.h>
#include <linux/ioctl.h>
#include<linux/errno.h>

#define IOCTL_MAGIC                    K
#define HT_IOCTL_BASE                  0x00
#define DUMP_IOCTL									_IOWR(IOCTL_MAGIC, HT_IOCTL_BASE+2, struct DUMP_ARG)



#define DEVICE_NAME                 "ht438_drv"  // device name to be created and registered
#define MAX_DEVICE 2

typedef struct ht_object //provided
{
	int key;
	char data[4];
}ht_object_t;

struct ht_node
{
	ht_object_t node_obj;
	struct hlist_node ht_new_node;
};
//new_node replace by ht_node
// struct dump_arg
// {
// 	int n; // the n-th bucket (in) or n objects retrieved (out)
// 	ht_object_t object_array[8] ; // to retrieve at most 8 objects from the n-th bucket
// };
  

/* per device structure */
struct ht438_dev 
{
	struct cdev cdev;              /* The cdev structure */
	char name[20];                  /* Name of device*/
	DECLARE_HASHTABLE(ht438_tbl, 5);
} *ht438_devp[MAX_DEVICE];

//struct ht438_dev *ht438[2];

static dev_t ht438_dev_number;      /* Allotted device number */
struct class *ht438_dev_class;          /* Tie with the device model */

//static char *user_name = "Dear Kalyani";
int i,ret;

//module_param(user_name,charp,0000);	//to get parameter from load.sh script to greet the user

/*
* Open ht438_drv driver
*/
int ht438_drv_open(struct inode *inode, struct file *file)
{
	//checked no errors
	// printk("MYCHARDEV: Device open\n");
    // return 0;
	struct ht438_dev *ht438_devp;
	//struct ht438_dev *ht438_devp[1];

	printk("\nopening\n");

	/* Get the per-device structure that contains this cdev */
	ht438_devp = container_of(inode->i_cdev,struct ht438_dev, cdev);
	//ht438_devp[1] = container_of(inode->i_cdev, struct ht438_dev, cdev);


	/* Easy access to cmos_devp from rest of the entry points */
	file->private_data = ht438_devp;
	//file->private_data = ht438_devp[1];

	printk("\n%s is openning \n", ht438_devp->name);
	// printk("\n%s is openning \n", ht438_devp->name);

	return 0;
}
/*
 * Write to ht438_drv driver
 */
ssize_t ht438_drv_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{	
	//struct hlist_head *head_node;
	struct ht_node *temp_node = NULL;
	// ht_nodep ht_obj_new_p;
	ht_object_t temp ;

	//char obj[4];
	int bkt;
	int flag = 0;
	int flag2 = 0;

	struct ht438_dev *ht438_devp = file->private_data;


	struct ht_node *ht_obj_new_pp = kmalloc(sizeof(struct ht_node), GFP_KERNEL);
	// {
	// 	printk("Bad Kmalloc\n");
	// 	return -ENOMEM;
	// }
	memset(ht_obj_new_pp,0,sizeof( struct ht_node));
	if(copy_from_user(&temp,buf,count))
	{
		printk("Cannot copy");
		return -1;
	}
	//ht438_devp->ht_obj_node_new_p = ht_obj_new_pp;
	ht_obj_new_pp->node_obj.key = temp.key;
	//ht438_devp->ht_obj_node_new_p;
	//ht_obj_new_pp->node_obj.key=ht_obj_new_pp.
	//ht_obj_new_p->node_obj.key;
	//ht_obj_new_p->node_obj.data;
	for(i=0;i<4;i++)
	{
	 	ht_obj_new_pp->node_obj.data[i] = temp.data[i];
	 	//node_obj.data[i] = temp.data[i];
	}
	printk(" WRITE operation: KEY= %d,  DATA= %s\n",ht_obj_new_pp->node_obj.key, ht_obj_new_pp->node_obj.data);
	
	if(hash_empty(ht438_devp->ht438_tbl))
	{
		hash_add(ht438_devp->ht438_tbl,&ht_obj_new_pp->ht_new_node,ht_obj_new_pp->node_obj.key);
		flag2 = 1;
			//hash_add(ht438_devp->ht438_tbl,&h_node,temp.key);
	}
	else
	{
		hash_for_each(ht438_devp->ht438_tbl,bkt,temp_node,ht_new_node)
		{
			if((ht_obj_new_pp->node_obj.key == temp_node->node_obj.key)&&(ht_obj_new_pp->node_obj.data[0]!= 0))
			{
				hash_del(&temp_node->ht_new_node);
				flag = 1;
				hash_add(ht438_devp->ht438_tbl,&ht_obj_new_pp->ht_new_node,ht_obj_new_pp->node_obj.key);
			}
			else if ((ht_obj_new_pp->node_obj.data[0] == 0 )&&(temp_node->node_obj.key == ht_obj_new_pp->node_obj.key))
			{
				hash_del(&temp_node->ht_new_node);
				flag = 1;
			}
		}
		// hash_del(&n_node->h_node);
		// hash_add(ht438_devp->ht438_tbl,&n_node->h_node,n_node->node_obj.key);
	}
	if((flag2==0)&&(flag == 0)&&(ht_obj_new_pp->node_obj.data != NULL))
	{
		hash_add(ht438_devp->ht438_tbl,&ht_obj_new_pp->ht_new_node,ht_obj_new_pp->node_obj.key);
	}

	
return 0;	
	// struct ht438_dev *ht438_devp = file->private_data;
}
/*
 * Read to gmem driver
 */
ssize_t ht438_drv_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	int bytes_read = 0;
	//int size;
	//int bkt;
	struct ht_node *temp_node = NULL;
	ht_object_t temp; //correct

	struct ht438_dev *ht438_devp = file->private_data;


	if(copy_from_user(&temp, (ht_object_t *)buf, sizeof(ht_object_t))) 
	{
        printk("Cannot copy from user\n");
        return -1;
    }
    printk("Reading Data from key: %d\n", temp.key);

	//hash_for_each_possible(ht438_devp->ht438_tbl,temp_node,ht_new_node);
	hash_for_each_possible(ht438_devp->ht438_tbl,temp_node,ht_new_node,temp.key)
	{
		if(temp_node->node_obj.key == temp.key)
		{
			for(i=0;i<4;i++)
			{
				temp.data[i]= temp_node->node_obj.data[i];
		    }
			bytes_read = 1;
		}
	}
	if(bytes_read)
	{
		printk("Found Data at Key: %d\n", temp.key);
        printk("Data is: %s", temp.data);
		//size = sizeof(data);
		//printk("\nPerforming READ operation with KEY= %d Data= %4s\n",temp.key,temp.data);
		if (copy_to_user((ht_object_t*)buf, &temp, sizeof(ht_object_t))) 
       	{	
			printk("unable to copy to  the user");
			return -EFAULT;
		}
	}
	else
	{
		printk("could not find\n");
		return -1;
	}
	
	return 0;
}
/*
 * Release gmem driver
 */

int ht438_drv_release(struct inode *inode, struct file *file)
{
	struct ht438_dev *ht438_devp = file->private_data;

    printk("\n%s is closing\n", ht438_devp->name);

    return 0;
}
/* File operations structure. Defined in linux/fs.h */
static struct file_operations fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= ht438_drv_open,        /* Open method */
    .release	= ht438_drv_release,     /* Release method */
    .write		= ht438_drv_write,       /* Write method */
    .read		= ht438_drv_read,        /* Read method */
};
/*
 * Driver Initialization
 */
int __init ht438_drv_init(void)
{
	printk(KERN_ALERT " Started Initialization");
	//int ret;
	//int i;

	//int time_since_boot;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&ht438_dev_number, 0, 2, DEVICE_NAME) < 0) 
	{
		printk(KERN_DEBUG "Can't register device\n"); 
		
		return -1;
	}
		/* Populate sysfs entries */
	ht438_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	//ht438_devp = kmalloc(sizeof(struct ht438_dev), GFP_KERNEL);

	//major_nu = MAJOR(ht438_dev_number);
	for(i = 0; i < MAX_DEVICE; i++)
	{
		ht438_devp[i] = kmalloc(sizeof(struct ht438_dev), GFP_KERNEL);
		if(!ht438_devp[i]) 
		{
			printk("Bad Kmalloc\n"); 
			return -ENOMEM;
		}
		cdev_init(&ht438_devp[i]->cdev, &fops);
		ht438_devp[i]->cdev.owner = THIS_MODULE;
		ret = cdev_add(&ht438_devp[i]->cdev,MKDEV(MAJOR(ht438_dev_number),i), 1);
		if(ret) 
		{
			printk("Bad cdev\n");
			return ret;
		}
		//hash_init(ht438_devp->ht438_tbl);
		device_create(ht438_dev_class, NULL, MKDEV(MAJOR(ht438_dev_number), i), NULL, "ht438_dev_%d",i);
		hash_init(ht438_devp[i]->ht438_tbl);

		//hash_init(ht438_devp[i]->ht438_tbl);

		//hash_init(ht438_devp[i].ht438_tbl);
	}
return 0;
}

/* Driver Exit */

void __exit ht438_drv_exit(void)
{
    int i;
	// device_remove_file(gmem_dev_device, &dev_attr_xxx);
	/* Release the major number */
	for(i=0;i< MAX_DEVICE;i++)
	{
	    device_destroy (ht438_dev_class, MKDEV(MAJOR(ht438_dev_number),i));
    	cdev_del(&ht438_devp[i]->cdev);
    }
	
	/* Destroy driver_class */
	class_destroy(ht438_dev_class);
	unregister_chrdev_region((ht438_dev_number), 1);
	printk("DEvice deleted");
	kfree(&ht438_devp);

	printk("ht438_drv driver removed.\n");
}

module_init(ht438_drv_init);
module_exit(ht438_drv_exit);
MODULE_LICENSE("GPL v2");