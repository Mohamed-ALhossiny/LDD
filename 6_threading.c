#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/ioctl.h>



// Module metadata
MODULE_AUTHOR("Mohamed Alhossiny");
MODULE_DESCRIPTION("Threaded GPIO control KLM");
MODULE_LICENSE("GPL");

#define MY_MAJOR    12
#define DRIVERNAME  "My_GPIO_Driver"
#define DRIVERCALSS "My_Class3"

static dev_t my_device_num;
static struct class *my_class;
static struct cdev my_device;

static unsigned GPIO_PIN = 16;
module_param(GPIO_PIN, uint, S_IRUGO);
MODULE_PARM_DESC(GPIO_PIN, "The gpio pin which you want to blink");

/*Threads info*/
static struct task_struct* kthread_high;
static struct task_struct* kthread_low;
static struct semaphore sema;
static unsigned high_delay = 1, low_delay = 1;


static int Thread_GPIO_High(void* param){

    msleep(1);
    while(!kthread_should_stop()){
        gpio_set_value(GPIO_PIN, 1);
        printk("GPIO is High!\n");
        msleep(high_delay * 1000);
        up(&sema);
        msleep(low_delay * 1000);
    }
    printk("GPIO High Thread is terminated!\n");
    return 0;
}

static int Thread_GPIO_Low(void* param){

    while(!kthread_should_stop()){
        if(down_timeout(&sema, msecs_to_jiffies(3000)) != 0){
            break;
        }
        gpio_set_value(GPIO_PIN, 0);
        printk("GPIO is low!\n");
    }
    printk("GPIO LOW Thread is terminated!\n");
    return 0;
}


static int my_open(struct inode* Dev_file, struct file* inst){
    printk("-----Device file is opened!-----\n");
    return 0;
}
static ssize_t my_read(struct file* File, char *user_buf, size_t count, loff_t* offs){
    printk("-----Device file read operation!-----\n");
    return 0;
}

static ssize_t my_write(struct file* File, const char *user_buf, size_t count, loff_t* offs){
    printk("-----Device file write operation!-----\n");

    return 0;
}

static int my_close(struct inode* Dev_file, struct file* inst){
    printk("-----Device file is closed!-----\n");
    return 0;
}

static struct file_operations f_ops = {
    .owner = THIS_MODULE,
    .open = my_open, 
    .release = my_close,
    .read = my_read,
    .write = my_write,
};



/*Get called when loading the module*/
static int __init custom_init(void) {

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
    /*GPIO and tiemr init*/
  if(gpio_request(16, "raspi-16")){
    printk("Can't request gpio pin!\n");
    goto Add_Error;
  }
  if(gpio_direction_output(16, 0)){
    printk("Can't set gpio pin to be output!\n");
    goto direction_Error;
  }
    /*we can create a thread by kthread_run without need to call the wake up function*/
  kthread_high = kthread_create(Thread_GPIO_High, NULL, "Thread High");
  if(kthread_high == NULL){
    printk("Can't create thread high!\n");
    goto Add_Error;
  }
  kthread_low = kthread_create(Thread_GPIO_Low, NULL, "Thread Low");
  if(kthread_low == NULL){
    printk("Can't create thread low!\n");
    goto Thread_Low_Error;
  }
  sema_init(&sema, 1);
  printk("Thread High and Low get created!\n");

  wake_up_process(kthread_high);
  wake_up_process(kthread_low);
  
    /*
         init function returns a value to make sure that module is initialized correctly,
         if any error occurs then return a negative number.
         if not returns 0.
    */
   return 0;
   Thread_Low_Error:
    kthread_stop(kthread_high);
   direction_Error:
    gpio_free(GPIO_PIN);
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
    printk("Thread stopped!\n");
    kthread_stop(kthread_high);
    kthread_stop(kthread_low);
    gpio_set_value(GPIO_PIN, 0);
    gpio_free(GPIO_PIN);
    cdev_del(&my_device);
    device_destroy(my_class, my_device_num);
    class_destroy(my_class);
    unregister_chrdev(my_device_num, DRIVERNAME);
    printk(KERN_INFO "Goodbye my friend, I shall miss you dearly...\n");
}


module_init(custom_init);
module_exit(custom_exit);
