#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <asm/cacheflush.h>
#include <linux/syscalls.h>
#include <asm/paravirt.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/string.h>

asmlinkage int our_sys_open(const char *, int, int);
int set_page_rw(long unsigned int);
int set_page_ro(long unsigned int);

int init_module(void);
void cleanup_module(void);
