#include "io_hook.h"

unsigned long **sys_call_table;
unsigned long original_cr0;

int * file_list = NULL;
int file_list_size = 0;

//store the origional system call.
//do NOT use this module with others that also do this!
//module unloading could get ugly.
asmlinkage int(*original_sys_open) (const char*, int, int);
asmlinkage ssize_t(*original_sys_read) (unsigned int, char *, size_t);

asmlinkage int our_sys_open(const char* file, int flags, int mode) {
	int i;

	//if a file is named unix.txt, let's keep track of it.
	if (strcmp(file,"unix.txt") == 0) {
		printk("A file with the name unix.txt was opened\n");
		if (file_list_size == 0) {
			file_list = kmalloc(sizeof(int) * ++file_list_size,GFP_KERNEL);
		} else {
			int * new_file_list = kmalloc(sizeof(int) * ++file_list_size,GFP_KERNEL);
			for (i = 0; i < file_list_size-1; i++) {
				new_file_list[i] = file_list[i];
			}
			kfree(file_list);
			file_list = new_file_list;
		}

		file_list[file_list_size-1] = original_sys_open(file,flags,mode);
		return file_list[file_list_size-1];
	} else {
		return original_sys_open(file,flags,mode);
	}
	
}

asmlinkage ssize_t our_sys_read (unsigned int fd, char * buf, size_t count) {
	ssize_t length = original_sys_read(fd,buf,count);

	//check if this is a while we should watch
	int i;
	int j;
	bool contained = false;
	char * word_to_find = "the";

	if (file_list_size == 0) return length;

	for (i = 0; i < file_list_size; i++) {
		if (file_list[i] == fd)
			contained = true;
	}

	if (contained == false) return length;

	printk("a program is reading a file named unix.txt\n");
	//swap code
	for (i = 0; i < length - 3; i++) {
		for (j = 0; j < 3; j++) {
			if (buf[i+j] != word_to_find[j]){
				break;
			} else {
				if (j == 2) {
					printk(KERN_INFO "the found in text file\n");
					buf[i] = 's';
				}
			}
		}
	}

	return length;

}

static unsigned long **aquire_sys_call_table(void) {

	unsigned long **sct;
	unsigned long int offset;
	for (offset = PAGE_OFFSET; offset < ULLONG_MAX; offset += sizeof(void *)) {
		sct = (unsigned long **) offset;

		if (sct[__NR_close] == (unsigned long *) sys_close)
			return sct;
	}
	return NULL;

}

int init_module(void) {

	file_list_size = 0;
	file_list = NULL;


	//pure black magic from stackoverflow
	if (!(sys_call_table = aquire_sys_call_table()))
		return -1;
	
	original_cr0 = read_cr0();
	write_cr0(original_cr0 & ~0x00010000);
	original_sys_open = (void *)sys_call_table[__NR_open];
	original_sys_read = (void *)sys_call_table[__NR_read];
	sys_call_table[__NR_open] = (unsigned long *) our_sys_open;
	sys_call_table[__NR_read] = (unsigned long *) our_sys_read;
	write_cr0(original_cr0);
	


	printk(KERN_INFO "read hook in place\n");	
	return 0;
}

void cleanup_module(void) {

	if (!sys_call_table)
		return;

	write_cr0(original_cr0 & ~0x00010000);
	sys_call_table[__NR_open] = (unsigned long *) original_sys_open;
	sys_call_table[__NR_read] = (unsigned long *) original_sys_read;
	write_cr0(original_cr0);

	if (file_list_size > 0)
		kfree(file_list);

	printk(KERN_INFO "read hook removed\n");
}
