
/*
 *  hello-4.c - Demonstrates module documentation.
 */
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kallsyms.h>

/*
 * Get rid of taint message by declaring code as GPL.
 */
MODULE_LICENSE("GPL");

#define DRIVER_AUTHOR "Alex Cater"
#define DRIVER_DESC   "rootkit"

MODULE_AUTHOR(DRIVER_AUTHOR);	/* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */


static unsigned long *sys_call_table;

static int __init init_main(void)
{
	printk(KERN_INFO "Module loaded!\n");
  sys_call_table = (void *)kallsyms_lookup_name("sys_call_table");

  if (sys_call_table == NULL) {
    printk(KERN_ERR "Couldn't look up sys_call_table\n");
    return -1;
  }
	printk(KERN_ERR "Found sys_call_table at %p\n", sys_call_table);
  return 0;
}


static void __exit cleanup(void)
{
	printk(KERN_INFO "Goodbye.\n");
}

module_init(init_main);
module_exit(cleanup);
