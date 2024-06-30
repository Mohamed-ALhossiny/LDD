#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>
#include <linux/hrtimer.h>
#include <linux/timer.h>


// Module metadata
MODULE_AUTHOR("Mohamed Alhossiny");
MODULE_DESCRIPTION("GPIO with timer driver");
MODULE_LICENSE("GPL");

#define MY_MAJOR    12
#define DRIVERNAME  "My_GPIO_Driver"
#define DRIVERCALSS "My_Class2"

static dev_t my_device_num;
static struct class *my_class;
static struct cdev my_device;

/*hrtimer objects*/
static struct hrtimer my_hrtimer;
u64 start_time;
static unsigned int TIME_DELAY_MS = 1000;
module_param(TIME_DELAY_MS, uint, S_IRUGO);

MODULE_PARM_DESC(TIME_DELAY_MS, "Timer in ms to fire the callback function");
static enum hrtimer_restart my_hrtimer_callback(struct hrtimer* timer){
    printk("%u ms has passed!\n", jiffies_to_msecs(jiffies - start_time));
    start_time = jiffies;
    return HRTIMER_NORESTART;
}


static int my_open(struct inode* Dev_file, struct file* inst){

    printk("Device file is opened!\n");
    return 0;
}
static ssize_t my_read(struct file* File, char *user_buf, size_t count, loff_t* offs){
    int to_copy, not_copy;


    /*Get amount of items to copy*/
    // to_copy = min(buffer_pos, count);

    /*to copy items to user we can't use memcpy because they are in different address space*/
    // not_copy = copy_to_user(user_buf, buffer, to_copy);

    return to_copy - not_copy;
}

static ssize_t my_write(struct file* File, const char *user_buf, size_t count, loff_t* offs){
    int to_copy, not_copy;

    /*Get amount of items to copy*/
    // to_copy = min(sizeof(buffer), count);

    /*to copy items to user we can't use memcpy because they are in different address space*/
    // not_copy = copy_from_user(buffer, user_buf, to_copy);
    // buffer_pos = to_copy;

    return to_copy - not_copy;
}

static int my_close(struct inode* Dev_file, struct file* inst){
    printk("Device file is closed!\n");
    return 0;
}

static struct file_operations f_ops = {
    .owner = THIS_MODULE,
    .open = my_open, 
    .release = my_close,
    .read = my_read,
    .write = my_write
};



/*Get called when loading the module*/
static int __init custom_init(void) {
    int ret;
    printk(KERN_INFO "Hello world driver loaded.\n");
    /*
        To allocate a device number statically.
        ret = register_chrdev(MY_MAJOR, "My_device_file", &f_ops);*/
     
    if(alloc_chrdev_region(&my_device_num, 0, 1, DRIVERNAME) < 0){
        printk("Unable to allocate a device file!\n");
        return -1;
    }
    printk("My_device_file is allocated!, Major = %d , Minor = %d\n", my_device_num >> 20, my_device_num & 0xfffff);
    /*
     create device class
    */
   if((my_class = class_create(THIS_MODULE, DRIVERCALSS)) == NULL){
        printk("Device class can be created!\n");
        goto Class_Error;
   }
   /*
    create device file.
   */
  if(device_create(my_class, NULL, my_device_num, NULL, DRIVERNAME) == NULL){
    printk("Unable to create device file!\n");
    goto File_Error;
  }
  /*Init device file*/
  cdev_init(&my_device, &f_ops);
  /*register device to kernel*/
  if(cdev_add(&my_device, my_device_num, 1) == -1){
    printk("Unable to register device to kernel!\n");
    goto Add_Error;
  }
  /*hrtimer init*/
  hrtimer_init(&my_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  my_hrtimer.function = my_hrtimer_callback;
  start_time = jiffies;
  hrtimer_start(&my_hrtimer, ms_to_ktime(TIME_DELAY_MS), HRTIMER_MODE_REL);
  /*
         init function returns a value to make sure that module is initialized correctly,
         if any error occurs then return a negative number.
         if not returns 0.
    */
   return 0;

   Add_Error:
        device_destroy(my_class, my_device_num);
   File_Error:
        class_destroy(my_class);
   Class_Error:
        unregister_chrdev(my_device_num, DRIVERNAME);
    return -1;
}
/*Get called when unloading the module*/
static void __exit custom_exit(void) {
    hrtimer_cancel(&my_hrtimer);
    cdev_del(&my_device);
    device_destroy(my_class, my_device_num);
    class_destroy(my_class);
    unregister_chrdev(my_device_num, DRIVERNAME);
    printk(KERN_INFO "Goodbye my friend, I shall miss you dearly...\n");
}


module_init(custom_init);
module_exit(custom_exit);
