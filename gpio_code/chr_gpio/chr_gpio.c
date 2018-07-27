#include <linux/module.h>  
#include <linux/types.h>  
#include <linux/fs.h>  
#include <linux/errno.h>  
#include <linux/mm.h>  
#include <linux/sched.h>  
#include <linux/init.h>  
#include <linux/cdev.h>  
#include <linux/timer.h>  
#include <linux/time.h>  
#include <linux/slab.h>  
#include <linux/device.h>  
#include <linux/completion.h>
#include <linux/jiffies.h>


#include <asm/uaccess.h>  
#include <asm/atomic.h>  
#include <asm/system.h>
#include <asm/io.h>

#include "chr_gpio.h"
  
#define CDEVDEMO_MAJOR 255  /*预设cdev_gpio的主设备号*/  
  
static int cdev_gpio_major = CDEVDEMO_MAJOR;  
  
struct cdev_gpio_dev   
{  
    struct cdev cdev;  
};  
struct class *cdev_gpio_class;
static int temp = 0;  
struct cdev_gpio_dev *cdev_gpio_devp; /*设备结构体指针*/  

dev_t devno;
static struct timer_list tm[4];

DECLARE_COMPLETION(tm_coml);
typedef struct tagGpio{
    Uint32 di_value;   
    Uint32 do_value;   
    int do_pin;     
    int output_val; 
    int do_period;  
}GpioS;
GpioS gpio_t; 
int pin,val,time_out;

static unsigned int gpio4_virtual_addr , gpio3_virtual_addr;
static unsigned int mult_gpio_virtual ;
  
int cdev_gpio_open(struct inode *inode, struct file *file)     
{  
    file->private_data = cdev_gpio_devp;
    printk(KERN_NOTICE "======== cdev_gpio_open ");  
    return 0;  
}  
  
int cdev_gpio_release(struct inode *inode, struct file *filp)      
{  
    printk(KERN_NOTICE "======== cdev_gpio_release ");     
    return 0;  
}  
  
void tm_callback(unsigned long arg)
{
    Uint32 gpio3_reg_addr[] ={gpio3_virtual_addr+0x4,gpio3_virtual_addr+0x8,gpio3_virtual_addr+0x10,gpio3_virtual_addr+0x20};

    if(val)
        REG_WRITE(gpio3_reg_addr[arg],0);
    else
        REG_WRITE(gpio3_reg_addr[arg],0xff);

    del_timer(&tm[arg]);
    printk(KERN_NOTICE "close tm[%d]\n ",arg);
    //complete(&tm_coml);
}

long cdev_gpio_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{

   Uint32 gpio3_reg_addr[] ={gpio3_virtual_addr+0x4,gpio3_virtual_addr+0x8,gpio3_virtual_addr+0x10,gpio3_virtual_addr+0x20};
    switch(cmd)
    {
        case CMD_SET_GPIO3_0:
            REG_WRITE(gpio3_virtual_addr+0x4,arg);
            break;

        case CMD_SET_GPIO3_1:
            REG_WRITE(gpio3_virtual_addr+0x8,arg);
            break;

        case CMD_SET_GPIO3_2:
            REG_WRITE(gpio3_virtual_addr+0x10,arg);
            break;

        case CMD_SET_GPIO3_3:
            REG_WRITE(gpio3_virtual_addr+0x20,arg);
            break;

        case CMD_SET_VALUE:
            if (copy_from_user(&gpio_t, (GpioS *)arg, sizeof(gpio_t)))
                printk(KERN_NOTICE "copy gpio_t from user err");

            pin = gpio_t.do_pin;
            val = gpio_t.output_val;
            time_out = gpio_t.do_period;

            if(time_out)
            {
                printk(KERN_NOTICE "pin = %d , val = %d , time_out = %d \r\n ",pin,val,time_out);
                init_timer(&tm[pin]);
                tm[pin].function= tm_callback;
                tm[pin].expires = jiffies + time_out * HZ/1000;
                tm[pin].data = pin;
                REG_WRITE(gpio3_reg_addr[pin],val);
                add_timer(&tm[pin]);
            }
            else
            {
                REG_WRITE(gpio3_reg_addr[pin],val);
            }
            break;

        case CMD_GET_VALUE:
            gpio_t.di_value = REG_READ(gpio4_virtual_addr+0x3FC);
            gpio_t.do_value = REG_READ(gpio3_virtual_addr+0x3FC);

            if (copy_to_user((void *)arg, &gpio_t, sizeof(gpio_t)))
                printk(KERN_NOTICE "copy gpio_t to user err");

            break;

        default:
            break;
    }
    return 0;
}

static const struct file_operations cdev_gpio_fops =  
{  
    .owner = THIS_MODULE,  
    .open = cdev_gpio_open,  
    .release = cdev_gpio_release,  
    .unlocked_ioctl = cdev_gpio_ioctl,
};  
  
void init_gpio(void)
{
    Uint8 data;

    gpio4_virtual_addr = (unsigned int)ioremap_nocache(GPIO4_BASE_ADDR,10000);
    gpio3_virtual_addr = (unsigned int)ioremap_nocache(GPIO3_BASE_ADDR,10000);

    mult_gpio_virtual = (unsigned int)ioremap_nocache(MULT_GPIO_BASE_ADDR,10000);

    REG_WRITE(mult_gpio_virtual+0x004,0x1);
    REG_WRITE(mult_gpio_virtual+0x008,0x1);

    if(!gpio4_virtual_addr ||!gpio3_virtual_addr ||!mult_gpio_virtual)
    {
        printk("remap addr filed\r\n");
        
    }

    //gpio4  0,1,2,3,4,5 input
    data = REG_READ(gpio4_virtual_addr+0x400);
    data = data & 0xc0;  //1100 0000
    REG_WRITE(gpio4_virtual_addr+0x400,data);

    //gpio3 0,1,2,3 output
    data = REG_READ(gpio3_virtual_addr+0x400);
    data = data | 0x0f;
    REG_WRITE(gpio3_virtual_addr+0x400,data);

}

static void cdev_gpio_setup_cdev(struct cdev_gpio_dev *dev, int index)  
{  
    int err; 
    printk(KERN_NOTICE "======== cdev_gpio_setup_cdev 1");     
    devno = MKDEV(cdev_gpio_major, index);  
    printk(KERN_NOTICE "======== cdev_gpio_setup_cdev 2");  
  
    cdev_init(&dev->cdev, &cdev_gpio_fops);  
    printk(KERN_NOTICE "======== cdev_gpio_setup_cdev 3");     
    dev->cdev.owner = THIS_MODULE;  
    dev->cdev.ops = &cdev_gpio_fops;  
    printk(KERN_NOTICE "======== cdev_gpio_setup_cdev 4");     
    err = cdev_add(&dev->cdev, devno, 1);  
    printk(KERN_NOTICE "======== cdev_gpio_setup_cdev 5");  
    if(err)  
    {  
        printk(KERN_NOTICE "Error %d add cdev_gpio %d", err, index);   
    }  
}  
  
int cdev_gpio_init(void)  
{  
    int ret;  
    struct device *devdemo;

   // struct proc_dir_entry *entry;
    printk(KERN_NOTICE "======== cdev_gpio_init ");    
    devno = MKDEV(cdev_gpio_major, 0);  
  
    if(cdev_gpio_major)  
    {  
        printk(KERN_NOTICE "======== cdev_gpio_init 1");  
        ret = register_chrdev_region(devno, 1, "cdev_gpio");  
    }else  
    {  
        printk(KERN_NOTICE "======== cdev_gpio_init 2");  
        ret = alloc_chrdev_region(&devno,0,1,"cdev_gpio");  
        cdev_gpio_major = MAJOR(devno);  
    }  
    if(ret < 0)  
    {  
        printk(KERN_NOTICE "======== cdev_gpio_init 3");  
        return ret;  
    }  
    cdev_gpio_devp = kmalloc(sizeof(struct cdev_gpio_dev), GFP_KERNEL);  
    if(!cdev_gpio_devp)  /*申请失败*/  
    {  
        ret = -ENOMEM;  
        printk(KERN_NOTICE "Error add cdev_gpio");     
        goto fail_malloc;  
    }  
  
    memset(cdev_gpio_devp,0,sizeof(struct cdev_gpio_dev));  
    printk(KERN_NOTICE "======== cdev_gpio_init 3");  
    cdev_gpio_setup_cdev(cdev_gpio_devp, 0);  
  
    cdev_gpio_class = class_create(THIS_MODULE, "cdev_gpio");  
    devdemo = device_create(cdev_gpio_class, NULL, MKDEV(cdev_gpio_major, 0), NULL, "cdev_gpio");  
  
    printk(KERN_NOTICE "======== cdev_gpio_init 4");  

    init_gpio();      

    return 0;  
  
    fail_malloc:  
        unregister_chrdev_region(devno,1);  
}  
  
void cdev_gpio_exit(void)    /*模块卸载*/  
{  
    printk(KERN_NOTICE "End cdev_gpio");   
    device_destroy(cdev_gpio_class,MKDEV(cdev_gpio_major,0));
    class_destroy(cdev_gpio_class);

    cdev_del(&cdev_gpio_devp->cdev);  /*注销cdev*/  
    kfree(cdev_gpio_devp);       /*释放设备结构体内存*/  
    unregister_chrdev_region(MKDEV(cdev_gpio_major,0),1);    //释放设备号  

}  
  
MODULE_LICENSE("Dual BSD/GPL");  
module_param(cdev_gpio_major, int, S_IRUGO);  
module_init(cdev_gpio_init);  
module_exit(cdev_gpio_exit);  
