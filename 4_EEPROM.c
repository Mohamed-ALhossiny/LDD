#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/version.h>

// Module metadata
MODULE_AUTHOR("Mohamed Alhossiny");
MODULE_DESCRIPTION("EEPROM KLM");
MODULE_LICENSE("GPL");

#define MY_MAJOR    12
#define DRIVERNAME  "My_EEPROM_Driver"
#define DRIVERCALSS "My_Class3"

static dev_t my_device_num;
static struct class *my_class;
static struct cdev my_device;

/*I2C Objects*/
static struct i2c_adapter* eeprom_i2c_adapter; /*adapter to i2c bus*/
struct i2c_client* eeprom_i2c_client; /*i2c slave device*/

#define I2C_BUS                     1
#define EEPROM_NAME              "24AA04"
#define EEPROM_ADDRESS             0x50

static const struct i2c_device_id eeprom_ids[] = {
                            {EEPROM_NAME, 0}
};

static struct i2c_driver EEPROM_i2c_driver ={
    .driver = {
        .name = EEPROM_NAME,
        .owner = THIS_MODULE
    }
};

static struct i2c_board_info eeprom_i2c_board_info = {
    I2C_BOARD_INFO(EEPROM_NAME, EEPROM_ADDRESS)
};




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

    // /*Get amount of items to copy*/
    // to_copy = min(sizeof(temp), count);

    /*to copy items to user we can't use memcpy because they are in different address space*/
    // not_copy = copy_from_user(temp, user_buf, to_copy);

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
    u8 data;
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

  /*I2C initialization*/
  eeprom_i2c_adapter = i2c_get_adapter(I2C_BUS);
  if(eeprom_i2c_adapter == NULL){
    printk("Can't initialize an I2C adapter!\n");
    goto Add_Error;
  }
  eeprom_i2c_client = i2c_new_client_device(eeprom_i2c_adapter, &eeprom_i2c_board_info);
  if(eeprom_i2c_client == NULL){
    printk("Can't add new I2C Client device!\n");
    goto Client_Error;
  }
  
  if(i2c_add_driver(&EEPROM_i2c_driver) == -1){
    printk("Can't add I2C EEPROM Driver!\n");
    goto driver_Error;
  }
  i2c_put_adapter(eeprom_i2c_adapter);

  printk("I2c EEPROM Driver added!\n");
  for(int i = 0; i < 256; i++){
    data = i2c_smbus_read_byte_data(eeprom_i2c_client, i);
    printk("address 0x00:  %x\n", data);
  }

    /*
         init function returns a value to make sure that module is initialized correctly,
         if any error occurs then return a negative number.
         if not returns 0.
    */
   return 0;
   driver_Error:
        i2c_unregister_device(eeprom_i2c_client);
   Client_Error:
        i2c_del_adapter(eeprom_i2c_adapter);
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
    i2c_del_driver(&EEPROM_i2c_driver);
    i2c_unregister_device(eeprom_i2c_client);
    i2c_del_adapter(eeprom_i2c_adapter);
    cdev_del(&my_device);
    device_destroy(my_class, my_device_num);
    class_destroy(my_class);
    unregister_chrdev(my_device_num, DRIVERNAME);
    printk(KERN_INFO "Goodbye my friend, I shall miss you dearly...\n");
}


module_init(custom_init);
module_exit(custom_exit);
