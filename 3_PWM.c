#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/pwm.h>


// Module metadata
MODULE_AUTHOR("Mohamed Alhossiny");
MODULE_DESCRIPTION("PWM KLM");
MODULE_LICENSE("GPL");

#define MY_MAJOR    12
#define DRIVERNAME  "My_PWM_Driver"
#define DRIVERCALSS "My_Class3"

static dev_t my_device_num;
static struct class *my_class;
static struct cdev my_device;

/*PWN variables*/
#define PWM_FREQ    1000000
char pwm_duty = 50;//0-100%
struct pwm_device* pwm0;

#define IS_DIGIT(NUM) 	(NUM > '0' || NUM < '9')

int atoi(char* buffer, int count){
    if(count == 1 && IS_DIGIT(buffer[0]))
        return buffer[0] - '0';
    else if(count == 2 && 
                (IS_DIGIT(buffer[0]) && IS_DIGIT(buffer[1])))
        return (buffer[1] -'0') + (buffer[0] - '0')*10;
    else if (count == 3 && 
                (IS_DIGIT(buffer[0]) && IS_DIGIT(buffer[1]) && IS_DIGIT(buffer[2])))
        return (buffer[2] -'0') + (buffer[1] - '0')*10 + (buffer[0] - '0')*100;
    else
        return -1;
}


static int my_open(struct inode* Dev_file, struct file* inst){

    printk("Device file is opened!\n");
    return 0;
}
static ssize_t my_read(struct file* File, char *user_buf, size_t count, loff_t* offs){
    printk("Device file read operation!\n");
    return 0;
}

static ssize_t my_write(struct file* File, const char *user_buf, size_t count, loff_t* offs){
    int to_copy, not_copy;
    char temp[5];
    int duty;
    /*Get amount of items to copy*/
    to_copy = min(sizeof(temp), count);

    /*to copy items to user we can't use memcpy because they are in different address space*/
    not_copy = copy_from_user(temp, user_buf, to_copy);
    temp[to_copy] = '\0';

    if((duty = atoi(temp, to_copy - 1)) == -1){
        printk("Invalid PWM duty, value = %s\n", temp);
        goto Write_Error;
    }

    if(duty > 100){
        printk("PWM Duty is greater than 100, value = %d!\n", duty);
    goto Write_Error;
}
    pwm_duty = duty;
    /*set pwm duty value*/
    pwm_config(pwm0, (PWM_FREQ*(pwm_duty))/100, PWM_FREQ);
    printk("PWM duty is updated to %d\n", pwm_duty);
Write_Error:
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
  
  pwm0 = pwm_request(0, "my-pwm0");
  if(pwm0 == NULL){
    printk("Can't allocate pwm device!\n");
    goto Add_Error;
  }

  pwm_config(pwm0, (PWM_FREQ*pwm_duty)/100, PWM_FREQ);
  pwm_enable(pwm0);

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
    pwm_disable(pwm0);
    pwm_free(pwm0);
    cdev_del(&my_device);
    device_destroy(my_class, my_device_num);
    class_destroy(my_class);
    unregister_chrdev(my_device_num, DRIVERNAME);
    printk(KERN_INFO "Goodbye my friend, I shall miss you dearly...\n");
}


module_init(custom_init);
module_exit(custom_exit);
