#include <linux/kernel.h>//strToInt
#include <linux/device.h>
#include <linux/string.h>//strlen, strchr
#include <linux/module.h>// MODULE_LICENSE
#include <linux/init.h>//module_init, module_exit
#include <linux/fs.h>//register_chrdev_region, alloc_chrdev_region
#include <linux/types.h>// dev_t
#include <linux/cdev.h>//struct cdev,cdevadd
#include <linux/kdev_t.h>//MAJOR,MINOR, MKDEV
#include <linux/uaccess.h>//copytouser,copyfromuser
#include <linux/errno.h>
#define BUFF_SIZE 20

MODULE_LICENSE("Dual BSD/GPL");
dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;
#define num_of_minors 10
int storage[num_of_minors];
int pos = 0;
int endRead = 0;

int storage_open(struct inode *pinode, struct file *pfile);
int storage_close(struct inode *pinode, struct file *pfile);
ssize_t storage_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t storage_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
  .owner = THIS_MODULE,
  .open = storage_open,
  .read = storage_read,
  .write = storage_write,
  .release = storage_close,
};


int storage_open(struct inode *pinode, struct file *pfile) 
{   
  printk(KERN_INFO "Succesfully opened file\n");
  return 0;
}

int storage_close(struct inode *pinode, struct file *pfile) 
{
  printk(KERN_INFO "Succesfully closed file\n");
  return 0;
}

ssize_t storage_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
  int ret;
  char buff[BUFF_SIZE];
  long int len;
  int minor = MINOR(pfile->f_inode->i_rdev);
  if (endRead){
    endRead = 0;    
    printk(KERN_INFO "Succesfully read from file\n");
    return 0;
  }
  len = scnprintf(buff, BUFF_SIZE, "%d\n", storage[minor]);
  ret = copy_to_user(buffer, buff, len);
  if(ret)
    return -EFAULT;
  endRead = 1;  
  return len;
}

ssize_t storage_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
  char buff[BUFF_SIZE];
  int value;
  int ret;
  int minor = MINOR(pfile->f_inode->i_rdev);
  ret = copy_from_user(buff, buffer, length);
  if(ret)
    return -EFAULT;
  buff[length-1] = '\0';
  
  ret = sscanf(buff,"%d",&value);
  storage[minor] = value;
  printk(KERN_INFO "Succesfully wrote value %d at position %d\n", value, minor);       
  return length;
}

static int __init storage_init(void)
{
  int ret = 0;
  int i=0;
  char buff[10];
 
  for (i = 0; i < num_of_minors; i++){
      storage[i] = 0;
  }
  ret = alloc_chrdev_region(&my_dev_id, 0, num_of_minors, "storage");
  if (ret){
    printk(KERN_INFO "failed to register char device\n");
    return ret;
  }
  printk(KERN_INFO "char device region allocated\n");

  my_class = class_create(THIS_MODULE, "storage_class");
  if (my_class == NULL){
    printk(KERN_ERR "failed to create class\n");
    goto fail_0;
  }
  printk(KERN_INFO "class created\n");

  printk(KERN_INFO "creating device\n");
  for (i = 0; i < num_of_minors; i++)
  {
    printk(KERN_INFO "created nod %d\n", i);
    scnprintf(buff, 10, "storage%d", i);
    my_device = device_create(my_class, NULL, MKDEV(MAJOR(my_dev_id), i), NULL, buff);
    if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
    }

  }
  printk(KERN_INFO "device created\n");

  my_cdev = cdev_alloc();	
  my_cdev->ops = &my_fops;
  my_cdev->owner = THIS_MODULE;
  ret = cdev_add(my_cdev, my_dev_id, num_of_minors);
  if (ret)
  {
    printk(KERN_ERR "failed to add cdev\n");
    goto fail_2;
  }
  printk(KERN_INFO "cdev added\n");
  printk(KERN_INFO "Hello world\n");

  return 0;

fail_2:
  device_destroy(my_class, my_dev_id);
fail_1:
  class_destroy(my_class);
fail_0:
  unregister_chrdev_region(my_dev_id, 1);
  return -1;
}

static void __exit storage_exit(void)
{
  int i = 0;

  cdev_del(my_cdev);
  for (i = 0; i < num_of_minors; i++) // every node made must be destroyed
    device_destroy(my_class, MKDEV(MAJOR(my_dev_id), i));
  class_destroy(my_class);
  
  unregister_chrdev_region(my_dev_id,num_of_minors);
  printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(storage_init);
module_exit(storage_exit);
