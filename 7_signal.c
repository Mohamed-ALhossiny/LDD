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
#include <linux/sched/signal.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/ioctl.h>
#include "7_macros.h"



// Module metadata
MODULE_AUTHOR("Mohamed Alhossiny");
MODULE_DESCRIPTION("KLM, a userspace register and get notified(signaled) if swich is pressed");
MODULE_LICENSE("GPL");

#define MY_MAJOR    12
#define DRIVERNAME  "My_GPIO_Driver"
#define DRIVERCALSS "My_Class3"

static dev_t my_device_num;
static struct class *my_class;
static struct cdev my_device;

static unsigned GPIO_SWITCH_PIN = 16;
static unsigned GPIO_LED_PIN = 12;
static char led_state = 0;
module_param(GPIO_SWITCH_PIN, uint, S_IRUGO);
module_param(GPIO_LED_PIN, uint, S_IRUGO);
MODULE_PARM_DESC(GPIO_SWITCH_PIN, "The gpio pin connected to a switch");
MODULE_PARM_DESC(GPIO_LED_PIN, "The gpio pin which you want to blink");

static struct task_struct* regitered_task = NULL;
unsigned switch_irq_num;

static irq_handler_t gpio_irq_handler(unsigned int irq, void* dev_id, struct pt_regs* regs){
    struct siginfo sig_info = {0};
    
    printk("Switch pin is triggered!\n");
    if(regitered_task != NULL){
        sig_info.si_signo = SWITCH_SIGNAL;
        sig_info.si_code = SI_QUEUE;
        if(send_sig_info(SWITCH_SIGNAL, (struct kernel_siginfo*) &sig_info, regitered_task) < 0){
            printk("Can't send the signal to App!\n");
        }
        printk("A signal is sent!, toggling the LED...\n");
        led_state = led_state ^ 1;
        gpio_set_value(GPIO_LED_PIN, led_state);

    }
    return (irq_handler_t) IRQ_HANDLED;
}




static long int my_ioctl(struct file* File, unsigned cmd, unsigned long arg){
    switch (cmd)
    {
    case REGISTER_APP:
        regitered_task = get_current();
        printk("A task is registered PID = %u\n", regitered_task->pid);
        break;
    
    default:
        break;
    }
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
    regitered_task = NULL;
    return 0;
}

static struct file_operations f_ops = {
    .owner = THIS_MODULE,
    .open = my_open, 
    .release = my_close,
    .read = my_read,
    .write = my_write,
    .unlocked_ioctl = my_ioctl
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

    /*GPIO init*/
  if(gpio_request(GPIO_SWITCH_PIN, "raspi-switch")){
    printk("Can't request gpio pin!\n");
    goto Add_Error;
  }
  if(gpio_direction_input(GPIO_SWITCH_PIN)){
    printk("Can't set gpio pin to be output!\n");
    goto Switch_Direction_Error;
  }
  gpio_set_debounce(GPIO_SWITCH_PIN, 300);

  if(gpio_request(GPIO_LED_PIN, "raspi-LED")){
    printk("Can't request gpio pin!\n");
    goto Switch_Direction_Error;
  }
  if(gpio_direction_output(GPIO_LED_PIN, 0)){
    printk("Can't set gpio pin to be output!\n");
    goto Led_Direction_Error;
  }
  
  switch_irq_num = gpio_to_irq(GPIO_SWITCH_PIN);
  if(request_irq(switch_irq_num, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_RISING, "my_gpio_irq_switch", NULL) != 0 ){
    printk("Can't reqister an irq for the switch pin\n");
    goto Led_Direction_Error;
  }

    /*
         init function returns a value to make sure that module is initialized correctly,
         if any error occurs then return a negative number.
         if not returns 0.
    */
   return 0;
   Led_Direction_Error:
        gpio_free(GPIO_LED_PIN);
   Switch_Direction_Error:
        gpio_free(GPIO_SWITCH_PIN);
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
    gpio_set_value(GPIO_LED_PIN, 0);
    gpio_free(GPIO_LED_PIN);
    free_irq(switch_irq_num, NULL);
    gpio_free(GPIO_SWITCH_PIN);
    cdev_del(&my_device);
    device_destroy(my_class, my_device_num);
    class_destroy(my_class);
    unregister_chrdev(my_device_num, DRIVERNAME);
    printk(KERN_INFO "Goodbye my friend, I shall miss you dearly...\n");
}


module_init(custom_init);
module_exit(custom_exit);
