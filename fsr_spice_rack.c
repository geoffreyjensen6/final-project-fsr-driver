/**
 * @file fsr_spice_rack.c
 * @FSR network driver implementation for ECEA5307 Final Project
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code and aesdchar driver from Assignments in ECEA5306
 *
 * @author Geoffrey Jensen
 * @date 05-26-2023
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/kdev_t.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include "fsr_spice_rack.h"

MODULE_AUTHOR("Geoffrey Jensen");
MODULE_LICENSE("Dual BSD/GPL");

int fsr_major =   0; // use dynamic major
int fsr_minor =   0;

struct fsr_dev fsr_device;
static struct class *dev_class;

int fsr_open(struct inode *inode, struct file *filp)
{
	struct fsr_dev *new_device;
	PDEBUG("In fsr_open\n");
   	new_device = container_of(inode->i_cdev, struct fsr_dev, cdev);
    	filp->private_data = new_device;
    	return 0;
}

int fsr_release(struct inode *inode, struct file *filp)
{
    	PDEBUG("In fsr_release\n");
    	return 0;
}

ssize_t fsr_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	int temp;
        unsigned char output_str;
	struct fsr_dev *dev = filp->private_data;

	temp = gpio_get_value(dev->spice_rack_dev.spice1.gpio_number);
	temp = temp + (gpio_get_value(dev->spice_rack_dev.spice2.gpio_number)<<1);
	temp = temp + (gpio_get_value(dev->spice_rack_dev.spice3.gpio_number)<<2);
	output_str = temp;

	if(copy_to_user(buf, &output_str, count)){
		return -EFAULT;
	}

	return count;
}

static struct file_operations fsr_fops = 
{
	.owner =    THIS_MODULE,
	.read =     fsr_read,
	.open =     fsr_open,
	.release =  fsr_release,
};

static int fsr_drv_remove(struct platform_device *pdev){
	dev_t devno = MKDEV(fsr_major, fsr_minor);
	cdev_del(&fsr_device.cdev);
	device_destroy(dev_class, devno);
	class_destroy(dev_class);
	unregister_chrdev_region(devno, 1);
	return 0;
}

static int fsr_setup_cdev(struct fsr_dev *dev)
{
	int err, devno = MKDEV(fsr_major, fsr_minor);
	PDEBUG("In fsr_setup_cdev\n");
	dev_class = class_create(THIS_MODULE, "fsr_gpio");
	if(dev_class == NULL){
		printk(KERN_ERR "Failed to create class\n");
		return -1;
	}
	PDEBUG("Successfully created class\n");
	if((device_create(dev_class, NULL, devno, NULL, "fsr_gpio_%d", 0)) == NULL){
		printk(KERN_ERR "Failed to create device\n");
		return -1;
	}
	PDEBUG("Successfully created device\n");
	cdev_init(&dev->cdev, &fsr_fops);
	err = cdev_add (&dev->cdev, devno, 1);
	if(err){
		printk(KERN_ERR "Error %d adding fsr cdev\n", err);
	}
	return err;
}

static int fsr_drv_probe(struct platform_device *pdev){	
	static dev_t dev;
	int result;
	struct device_node *np = pdev->dev.of_node;
	PDEBUG("In fsr_drv_probe\n");
	memset(&fsr_device,0,sizeof(struct fsr_dev));

	//Find the GPIO numbers of defined in the device tree
	fsr_device.spice_rack_dev.spice1.name = "spice1-gpio";
	fsr_device.spice_rack_dev.spice2.name = "spice2-gpio";
	fsr_device.spice_rack_dev.spice3.name = "spice3-gpio";


	fsr_device.spice_rack_dev.spice1.gpio_number = of_get_named_gpio(np, fsr_device.spice_rack_dev.spice1.name, 0);
	PDEBUG("GPIO Number is %i for Spice1", fsr_device.spice_rack_dev.spice1.gpio_number);
	if(fsr_device.spice_rack_dev.spice1.gpio_number < 0){
		printk(KERN_ERR "Unable to find GPIO for spice1-gpio in device tree");
		result = fsr_device.spice_rack_dev.spice1.gpio_number;
		return result;
	}
	fsr_device.spice_rack_dev.spice2.gpio_number = of_get_named_gpio(np, fsr_device.spice_rack_dev.spice2.name, 0);
        PDEBUG("GPIO Number is %i for Spice2", fsr_device.spice_rack_dev.spice2.gpio_number);
	if(fsr_device.spice_rack_dev.spice2.gpio_number < 0){
		printk(KERN_ERR "Unable to find GPIO for spice2-gpio in device tree");
		result = fsr_device.spice_rack_dev.spice2.gpio_number;
		return result;
	}
	fsr_device.spice_rack_dev.spice3.gpio_number = of_get_named_gpio(np, fsr_device.spice_rack_dev.spice3.name, 0);
        PDEBUG("GPIO Number is %i for Spice3", fsr_device.spice_rack_dev.spice3.gpio_number);
	if(fsr_device.spice_rack_dev.spice3.gpio_number < 0){
		printk(KERN_ERR "Unable to find GPIO for spice3-gpio in device tree");
		result = fsr_device.spice_rack_dev.spice3.gpio_number;
		return result;
	}

	//Char Dev Registration
	PDEBUG("Beginning Character Device Registration\n");
	result = alloc_chrdev_region(&dev, fsr_minor, 1, "fsr_spice_rack");
	fsr_major = MAJOR(dev);
	PDEBUG("FSR major is %d and FSR minor is %d\n", fsr_major, fsr_minor);
	if (result < 0) {
		printk(KERN_WARNING "Can't get major %d\n", fsr_major);
		return result;
	}
	
	result = fsr_setup_cdev(&fsr_device);
	PDEBUG("fsr_setup_cdev returned %i\n", result);
	if(result){
		unregister_chrdev_region(dev, 1);
	}

	return result;
}

static const struct of_device_id fsr_match[] = {
	{ .compatible = "geoffreyjensen,fsr", },
	{}
};

MODULE_DEVICE_TABLE(of,fsr_match);
static struct platform_driver fsr_driver = {
	.probe = fsr_drv_probe,
	.remove = fsr_drv_remove,
	.driver = {
		.name = "fsr_spice_rack",
		.of_match_table = of_match_ptr(fsr_match),
	},
};

static int __init fsr_driver_init(void){
	return platform_driver_register(&fsr_driver);
}

static void __exit fsr_driver_exit(void){
	platform_driver_unregister(&fsr_driver);
}

module_init(fsr_driver_init);
module_exit(fsr_driver_exit);

