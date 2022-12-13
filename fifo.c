#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/string.h>



#define BUFF_SIZE 16

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_classl;
static struct device *my_device;
static struct cdev *my_cdev;


int fifo[16];
int pos = 0;
int endRead = 0;

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
	printk(KERN_INFO "Succes OPEN brape\n");
	return 0;
}


int FIFO_close(struct inode *pinode, struct file *pfile)
{
	printk(KERN_INFO "Succes CLOSE brape\n");
	return 0;
}


ssize_t FIFO_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset)
{
	int ret;
	char buff[BUFF_SIZE];
	long int len = 0;
	
	if(endRead)
	{
		endRead = 0;
		return 0;
	}
	
	if(pos > 0)
	{
		pos--;
		len = scnprintf(buff, BUFF_SIZE, "%d", fifo[pos]);
		
		ret = copy_to_user(buffer, buff, len);
		if(ret)
			return -EFAULT;
		
		printk(KERN_INFO "Great success READ\n");
		endRead = 1;
	}	
	
	return len;
	
}

ssize_t FIFO_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	int ret;
	char buff[BUFF_SIZE];
	long value;
	
	ret = copy_from_user(buff, buffer, length);
	if(ret)
		return -EFAULT;
	
	buff[length-1] = '\0';
	
	if(pos < 16)
	{
		ret = sscanf(buff, %ld, &value);
		if(ret == 1)
		{
			printk(KERN_INFO "Great success WROTE %ld", value);
			fifo[pos] = value;
			pos++;
		}
		else
		{
			printk(KERN_WARNING "wrong wrong wrong\n");
		}
		
		
	}
	else
	{
		printk(KERN_WARNING "FIFO full full full\n");
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
   
   my_device = device_create(my_class, NULL, my_dev_id, NULL, "lifo");
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
   
   printk(KERN_INFO "Hello b b b b\n");

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
	printk(KERN_INFO "byebye\n");
}

module_init(FIFO_init);
module_exit(FIFO_exit);
