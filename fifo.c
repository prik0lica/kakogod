#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>



#define BUFF_SIZE 16
#define MAX_STRING_SIZE 64

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;


int fifo[16];

static int read_pos = 0;
static int write_pos = 0;
static int num_of_el_current = 0;

static int endRead = 0;

int FIFO_open(struct inode *pinode, struct file *pfile);
int FIFO_close(struct inode *pinode, struct file *pfile);
ssize_t FIFO_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t FIFO_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops = 
{
	.owner = THIS_MODULE,
	.open = FIFO_open,
	.release = FIFO_close,
	.write = FIFO_write,
	.read = FIFO_read,
};

int FIFO_open(struct inode *pinode, struct file *pfile)
{
	printk(KERN_INFO "OPEN - succes\n");
	return 0;
}


int FIFO_close(struct inode *pinode, struct file *pfile)
{
	printk(KERN_INFO "CLOSE - succes\n");
	return 0;
}


ssize_t FIFO_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	int ret;
	char local_buff[MAX_STRING_SIZE] = {0};
	long int len = 0;
	
	//read check
	if(endRead)
	{
		endRead = 0;
		return 0;
	}
	
	//read one value
	if(num_of_el_current > 0) /* check if fifo is empty */
	{
		
		len = scnprintf(local_buff, strlen(local_buff), "%d", fifo[read_pos]); //put data into local buffer for user space to read
		
		ret = copy_to_user(buffer, local_buff, len); 			       //copy into data buffer from local to se if OK
		if(ret)
			return -EFAULT;
		
		printk(KERN_INFO "READ %d - SUCCESS.\n", fifo[read_pos]);
		if (read_pos == (BUFF_SIZE-1))
			{
				read_pos = 0;
			}
		else
			{
				read_pos++;
			}
			
		endRead = 1;
		num_of_el_current--;
		
		return len;
		
	}	
	else
	{
		printk(KERN_WARNING "FIFO - empty\n");
		return 0;
	}
	
}

ssize_t FIFO_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	int ret;					//for error use
	char local_buff[BUFF_SIZE];	//temp location for read data in char type
	int value;					//temp location for read data in int type
	
	ret = copy_from_user(local_buff, buffer, length);
	if(ret)
		return -EFAULT;
	
	local_buff[length-1] = '\0';
	
	
	if(num_of_el_current < 16)
	{
		//read number parsed from local_buff into value
		//char -> int
		ret = sscanf(local_buff, "%d", &value); 
		
		if(ret == 1)
		{
			//wrote into actual buffer
			fifo[write_pos] = value;
			printk(KERN_INFO "WROTE %d - succes\n", value);
			num_of_el_current++;
			
		}
		else
		{
			printk(KERN_WARNING "ERROR in WRITE\n");
		}
		
		if(write_pos == (BUFF_SIZE-1))
		{
			write_pos = 0;
		}
		else
		{
			write_pos++;
		}
		
		
	}
	else
	{
		printk(KERN_WARNING "FIFO is full\n");
	}

	return length;
}

static int __init FIFO_init(void)
{
	int ret = 0;
	int i = 0;
	
	for (i = 0; i < 16; i++)
		fifo[i] = 0;
	
	ret = alloc_chrdev_region(&my_dev_id, 0, 1, "fifo");
	if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");
   
   my_class = class_create(THIS_MODULE, "fifo_class");
   if(my_class == NULL)
   {
	   printk(KERN_ERR "fail create\n");
	   goto fail_0;
   }
   printk(KERN_INFO "class created\n");
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "fifo");
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

   my_cdev = cdev_alloc();	
   my_cdev->ops = &my_fops;
   my_cdev->owner = THIS_MODULE;
   
   ret = cdev_add(my_cdev, my_dev_id, 1);
   if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   
   printk(KERN_INFO "Hello this is FIFO\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;

}


static void __exit FIFO_exit(void)
{
	cdev_del(my_cdev);
	device_destroy(my_class, my_dev_id);
	class_destroy(my_class);
	unregister_chrdev_region(my_dev_id,1);
	printk(KERN_INFO "There is no more FIFO\n");
}

module_init(FIFO_init);
module_exit(FIFO_exit);
