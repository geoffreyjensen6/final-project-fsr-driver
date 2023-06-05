/*
 * fsr_spice_rack.h
 *
 *  Created on: 05-25-2023
 *      Author: Geoffrey Jensen
 */

#ifndef FSR_DRIVER_H_
#define FSR_DRIVER_H_

#define FSR_DEBUG 1  //Remove comment on this line to enable debug

#undef PDEBUG             /* undef it, just in case */
#ifdef FSR_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "fsr: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

struct spice {
        char *name;
        int gpio_number;
        int gpio_value;
};

struct spice_rack {
        struct spice spice1;
        struct spice spice2;
	struct spice spice3;
};

struct fsr_dev
{
	struct spice_rack spice_rack_dev;
	struct spice spices;
    	struct mutex lock;
	struct cdev cdev;     /* Char device structure      */
};


#endif /* FSR_DRIVER_H_ */
