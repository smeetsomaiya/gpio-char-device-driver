#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
//#include <linux/types.h>
#include <linux/slab.h>
//#include <asm/uaccess.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
//#include <linux/jiffies.h>
//#include <linux/timer.h>
#include <linux/ioctl.h>
//#include<linux/init.h>
//#include<linux/moduleparam.h>
//#include<linux/timer.h>
//#include<linux/delay.h>
#include <linux/gpio.h>
#include <stdbool.h>

struct led_config_struct
{
	int duty_cycle;
	int red_led;
	int green_led;
	int blue_led;
	int on_time;
	int off_time;
	int intensity_control_period_ms;
	int step_period_ms;
};


//Macros related to lightup sequence
#define STEP_PERIOD_MS 500
#define INTENSITY_CONTROL_PERIOD_MS 20

#define PIN_COLUMN 0

#define OFF_VALUE 0
#define ON_VALUE 1

#define CONFIG 11
#define RESET 12

int valid_pins[4] = {0, 1, 10, 12}; //IO pins are restricted to IO8 IO15

// Array containing the gpio pin configuration
int gpio_mapping_array[20][4] = {
			                                {11,32,-1,-1},
							{12,28,45,-1},
							{61,-1,77,-1},
 							{62,16,76,64},
							{6,36,-1,-1},
							{0,18,66,-1},
							{1,20,68,-1},
							{38,-1,-1,-1},
							{40,-1,-1,-1},
							{4,22,70,-1},
							{10,26,74,-1},
							{5,24,44,72},
							{15,42,-1,-1},
							{7,30,46,-1},
							{48,-1,-1,-1},
							{50,-1,-1,-1},
							{52,-1,-1,-1},
							{54,-1,-1,-1},
							{56,-1,60,78},
							{7,30,60,79}
};

// Array containing the gpio pin configuration
int gpio_values_array[20][4] = {
			                               {0,0,-1,-1},
							{0,0,0,-1},
							{0,0,0,-1},
 							{0,0,0,0},
							{0,0,-1,-1},
							{0,0,0,-1},
							{0,0,0,-1},
							{0,-1,-1,-1},
							{0,-1,-1,-1},
							{0,0,0,-1},
							{0,0,0,-1},
							{0,0,0,0},
							{0,0,-1,-1},
							{0,0,0,-1},
							{0,-1,-1,-1},
							{0,-1,-1,-1},
							{0,-1,-1,-1},
							{0,-1,-1,-1},
							{0,-1,1,1},
							{0,-1,1,1}
};

int lightup_pattern_array[8];
bool is_gpio_requested = false;

#define ROW_SIZE(arr) ((int) (sizeof (arr) / sizeof (arr)[0]))
#define COLUMN_SIZE(arr) ((int) sizeof ((arr)[0]) / sizeof (int))


#define DEVICE_NAME                 "RGBLed"  // device name to be created and registered

/* per device structure */
struct rgbled_dev {
	struct cdev cdev;               /* The cdev structure */
	char name[20];                  /* Name of device*/
	struct led_config_struct *config_struct;
} *rgbled_devp;

static dev_t rgbled_dev_number;      /* Allotted device number */
struct class *rgbled_dev_class;          /* Tie with the device model */
static struct device *rgbled_dev_device;

/* setup timer */
static struct timer_list myTimer, stepTimer;

//Flags to control led's from timer ISR
static int i,j;

void leds_off(void) {
    gpio_set_value(gpio_mapping_array[rgbled_devp->config_struct->red_led][PIN_COLUMN], OFF_VALUE);
    gpio_set_value(gpio_mapping_array[rgbled_devp->config_struct->green_led][PIN_COLUMN], OFF_VALUE);
    gpio_set_value(gpio_mapping_array[rgbled_devp->config_struct->blue_led][PIN_COLUMN], OFF_VALUE);
}

void leds_on(int *pattern_row) {
	if(*pattern_row < ROW_SIZE(lightup_pattern_array)) {
		int r = ((lightup_pattern_array[*pattern_row] & 0x4) ? ON_VALUE : OFF_VALUE);
		int g = ((lightup_pattern_array[*pattern_row] & 0x2) ? ON_VALUE : OFF_VALUE);
		int b = ((lightup_pattern_array[*pattern_row] & 0x1) ? ON_VALUE : OFF_VALUE);

		gpio_set_value(gpio_mapping_array[rgbled_devp -> config_struct -> red_led][PIN_COLUMN], r);
		gpio_set_value(gpio_mapping_array[rgbled_devp -> config_struct -> green_led][PIN_COLUMN], g);
		gpio_set_value(gpio_mapping_array[rgbled_devp -> config_struct -> blue_led][PIN_COLUMN], b);

		printk(KERN_ALERT "Pattern: %d %d %d Row index %d\n", r, g, b, *pattern_row);
	} else {
		printk(KERN_ALERT "Going out of bounds %d", *pattern_row);
	}
}

void step_timer(unsigned long arg) {
	struct led_config_struct *rgbled_p = (void*) arg;
	int tmp;
	j++;
	tmp = j;
	if(tmp == ROW_SIZE(lightup_pattern_array)) {
		printk("End of sequence\n");
		j = 0;
	}
	mod_timer(&stepTimer, jiffies + msecs_to_jiffies(rgbled_p->step_period_ms));
}

void timerFun(unsigned long arg) {
	struct led_config_struct *rgbled_p = (void*) arg;
	int tmp;
	printk(KERN_ALERT "Inside timerFun\n");
	i++;
	tmp = i;

	printk(KERN_ALERT "After tmp = i\n");
	if(tmp % 2 == 0) {
		int off = rgbled_p->off_time;
		leds_on(&j);
		mod_timer(&myTimer, jiffies + msecs_to_jiffies(off));
	} else {
		int on = rgbled_p->on_time;
		leds_off();
		mod_timer(&myTimer, jiffies + msecs_to_jiffies(on));
	}
}

int setup_mytimer(unsigned long arg)
{
	struct led_config_struct *rgbled_p = (void*) arg;
	i = 0; j = 0;

	setup_timer(&myTimer, timerFun, arg);
	setup_timer(&stepTimer, step_timer, arg);

//	timer_setup(&myTimer, timerFun, 0);
//	timer_setup(&stepTimer, step_timer, 0);

	mod_timer(&myTimer, jiffies + msecs_to_jiffies(0)); //Initial value is 0
	mod_timer(&stepTimer, jiffies + msecs_to_jiffies(rgbled_p->step_period_ms));
	printk (KERN_ALERT "Timer(s) added \n");
	return 0;
}

void cleanup_timer(void) {
	if (!del_timer (&myTimer)) {
		printk (KERN_ALERT "Couldn't remove timer!!\n");
	}
	else {
		printk (KERN_ALERT "timer removed \n");
	}
	if (!del_timer (&stepTimer)) {
		printk (KERN_ALERT "Couldn't remove step timer!!\n");
	}
	else {
		printk (KERN_ALERT "step timer removed \n");
	}
}

void cleanup_gpio(void) {
	if(is_gpio_requested) {
		int k, read_value;
		for (k = 0; k < COLUMN_SIZE(gpio_mapping_array); k++) {
			read_value = gpio_mapping_array[rgbled_devp->config_struct->red_led][k];
		        if(read_value != -1) {
				gpio_free(read_value);
				printk(KERN_ALERT "Gpio free %d", read_value);
			}
			read_value = gpio_mapping_array[rgbled_devp->config_struct->green_led][k];
		        if(read_value != -1) {
				gpio_free(read_value);
				printk(KERN_ALERT "Gpio free %d", read_value);
			}
			read_value = gpio_mapping_array[rgbled_devp->config_struct->blue_led][k];
		        if(read_value != -1) {
				gpio_free(read_value);
				printk(KERN_ALERT "Gpio free %d", read_value);
			}
		}
	}
}

/*
* Open rgbled driver
*/
int rgbled_driver_open(struct inode *inode, struct file *file)
{

//	struct rgbled_dev *rgbled_devp;

	/* Get the per-device structure that contains this cdev */
	rgbled_devp = container_of(inode->i_cdev, struct rgbled_dev, cdev);


	
	file->private_data = rgbled_devp;
	printk(KERN_ALERT"%s is opening. Address: %p", rgbled_devp->name, rgbled_devp);
	return 0;
}

/*
 * Release rgbled driver
 */
int rgbled_driver_release(struct inode *inode, struct file *file)
{
	struct rgbled_dev *rgbled_devp = file->private_data;
	
	cleanup_timer();
	cleanup_gpio();

	printk("\n%s is closing\n", rgbled_devp->name);
	
	return 0;
}

/*
 * Write to rgbled driver
 */
static ssize_t rgbled_driver_write(struct file *file, const char *buf,
           size_t count, loff_t *ppos)
{
//	struct rgbled_dev *rgbled_devp = file->private_data;
	int l = 0;
	int *lightup_patterns_array = kmalloc(count * sizeof(int),GFP_KERNEL);
	printk("Inside write\n");

	for(l=0;l<count;l++) {
		copy_from_user(lightup_patterns_array+(l*sizeof(int)),(int*)(buf+(l*sizeof(int))),sizeof(int));
//		printk(KERN_ALERT "Lightup pointer: %p %p\n", lightup_patterns_array+(l*sizeof(int)), (int*)(buf+(l*sizeof(int))));
		lightup_pattern_array[l] = *(lightup_patterns_array+(l*sizeof(int)));
		printk(KERN_ALERT "Arr %d\n", lightup_pattern_array[l]);
	}
	kfree(lightup_patterns_array);

	setup_mytimer((unsigned long) rgbled_devp->config_struct);

	printk(KERN_ALERT "Exiting to user space\n");
	return 0;
}

int is_gpio_64_to_79(int gpio) { //Check for gpio's which do not have a direction file
	if(gpio >= 64 && gpio <= 79) {
		return 1; //return true
	}	
	return 0; //return false
}

int mux_gpio_set(int gpio, int value)
{
	if(gpio == -1 || value == -1) {
		printk("-1 found in mux_gpio_set\n");
		return -1;
	}
	printk(KERN_ALERT "Gpio %d Value %d\n", gpio, value);
	gpio_request(gpio, "sysfs");

	if(is_gpio_64_to_79(gpio)) { //No direction file present for these GPIOs
		printk(KERN_ALERT "Gpio is between 64 and 79 %d\n", gpio);
//		gpio_set_value(gpio, value);
	} else {
		gpio_direction_output(gpio, value);
		printk(KERN_ALERT "Not setting direction, gpio %d", gpio);
	}
	return 0;
}


void configure_led(int gpio_array_row) {                         //Configuring R,G,B leds as per level shifter and Mux configuration
    int column;
    int columns_of_gpio_array = 4;

    for(column = 0; column < columns_of_gpio_array; column++) {
        int read_value = gpio_mapping_array[gpio_array_row][column];
        if(read_value != -1) {
		printk("Configure pins for IO %d - %d\n", gpio_array_row, gpio_values_array[gpio_array_row][column]);
		mux_gpio_set(gpio_mapping_array[gpio_array_row][column], gpio_values_array[gpio_array_row][column]);
        }
    }
}

bool is_valid_pin(int pin) {
	int t;
	for(t = 0; t < ROW_SIZE(valid_pins); t++) {
		if(pin == valid_pins[t]) {
			return true;
		}
	}
	return false;
}

static long rgbled_ioctl(struct file *file, unsigned int cmd, unsigned long datap) {

	if(cmd == CONFIG) {
		struct rgbled_dev *rgbled_devp = file->private_data;                           // gets device pointer
		printk("Inside ioctl CONFIG\n");
		rgbled_devp->config_struct = kmalloc(sizeof(struct led_config_struct),GFP_KERNEL);

		//copy parameters from user space
		copy_from_user(rgbled_devp -> config_struct, (struct led_config_struct*)datap, sizeof(struct led_config_struct));

		if(!is_valid_pin(rgbled_devp -> config_struct -> red_led)
			|| !is_valid_pin(rgbled_devp -> config_struct -> green_led)
			|| !is_valid_pin(rgbled_devp -> config_struct -> blue_led)) {
			printk(KERN_ALERT"Invalid pins in the input\n");
			return EINVAL;
		}

		configure_led(rgbled_devp -> config_struct -> red_led);
		configure_led(rgbled_devp -> config_struct -> green_led);
		configure_led(rgbled_devp -> config_struct -> blue_led);

		is_gpio_requested = true; //Led's have been configured

		printk(KERN_ALERT "Step period %d\n", rgbled_devp -> config_struct -> step_period_ms);
		printk(KERN_ALERT "Intensity control period %d\n", rgbled_devp -> config_struct -> intensity_control_period_ms);
		printk(KERN_ALERT "On time %d\n", rgbled_devp -> config_struct -> on_time);
		printk(KERN_ALERT "Off time %d\n", rgbled_devp -> config_struct -> off_time);
	
	} else if(cmd == RESET) {
		//Reset the sequence
		j = 0;
		printk("RESET command received\n");
	} else {
		printk(KERN_ALERT"Only CONFIG and RESET commands allowed\n");
		return EINVAL;
	}
	printk("Return successfully from ioctl");
	return 0;
}

/* File operations structure. Defined in linux/fs.h */
static struct file_operations rgbled_fops = {
    .owner		= THIS_MODULE,           /* Owner */
    .open		= rgbled_driver_open,        /* Open method */
    .release    = rgbled_driver_release,     /* Release method */
    .unlocked_ioctl = rgbled_ioctl,                /* Config method */
    .write		= rgbled_driver_write,        /* Write method */
//.read = rgbled_driver_read
};

/*
 * Driver Initialization
 */
int __init rgbled_driver_init(void)
{
	int ret;

	/* Request dynamic allocation of a device major number */
	if (alloc_chrdev_region(&rgbled_dev_number, 0, 1, DEVICE_NAME) < 0) {
			printk(KERN_DEBUG "Can't register device\n"); return -1;
	}

	/* Populate sysfs entries */
	rgbled_dev_class = class_create(THIS_MODULE, DEVICE_NAME);

	/* Allocate memory for the per-device structure */
	rgbled_devp = kmalloc(sizeof(struct rgbled_dev), GFP_KERNEL);
		
	if (!rgbled_devp) {
		printk("Bad Kmalloc\n"); return -ENOMEM;
	}

	/* Request I/O region */
	sprintf(rgbled_devp->name, DEVICE_NAME);

	/* Connect the file operations with the cdev */
	cdev_init(&rgbled_devp->cdev, &rgbled_fops);
	rgbled_devp->cdev.owner = THIS_MODULE;

	/* Connect the major/minor number to the cdev */
	ret = cdev_add(&rgbled_devp->cdev, (rgbled_dev_number), 1);

	if (ret) {
		printk("Bad cdev\n");
		return ret;
	}

	/* Send uevents to udev, so it'll create /dev nodes */
	rgbled_dev_device = device_create(rgbled_dev_class, NULL, MKDEV(MAJOR(rgbled_dev_number), 0), NULL, DEVICE_NAME);		
	// device_create_file(rgbled_dev_device, &dev_attr_xxx);

	printk("RGBLed driver initialized.\n");
	return 0;
}

/* Driver Exit */
void __exit rgbled_driver_exit(void)
{
	printk("Removing RGBLed driver\n");

	// device_remove_file(rgbled_dev_device, &dev_attr_xxx);
	/* Release the major number */
	unregister_chrdev_region((rgbled_dev_number), 1);

	/* Destroy device */
	device_destroy (rgbled_dev_class, MKDEV(MAJOR(rgbled_dev_number), 0));
	cdev_del(&rgbled_devp->cdev);
	kfree(rgbled_devp);
	
	/* Destroy driver_class */
	class_destroy(rgbled_dev_class);

	printk("RGBLed driver removed.\n");
}

module_init(rgbled_driver_init);
module_exit(rgbled_driver_exit);
MODULE_LICENSE("GPL v2");
