#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>                 // kmalloc()
#include <linux/uaccess.h>              // copy_to/from_user()
#include <linux/debugfs.h>

#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/pci.h>

#define BUF_SIZE 1024

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Stab linux module for operating system's lab");
MODULE_VERSION("1.0");

static int pid = 1;
static int struct_id = 1;

static int filevalue; 
static struct dentry *parent;

/*
** Function Prototypes
*/
static int      __init lab_driver_init(void);
static void     __exit lab_driver_exit(void);


/***************** Procfs Functions *******************/
static ssize_t  read_debug(struct file *filp, char __user *buffer, size_t length,loff_t * offset);
static ssize_t  write_debug(struct file *filp, const char *buff, size_t len, loff_t * off);

/*
** procfs operation sturcture
*/
static struct file_operations  proc_fops = {
        .read = read_debug,
        .write = write_debug,
};

// net_device
static size_t write_pci_device_struct(char __user *ubuf){
    char buf[BUF_SIZE];
    size_t len = 0;

    static struct pci_dev* dev;

    //read_lock(&dev_base_lock);
    while((dev = pci_get_device(PCI_ANY_ID, PCI_ANY_ID, dev))){
        len += sprintf(buf+len, "pci found [%d]\n",     dev->device);
        printk(KERN_INFO "pci found [%d]\n", dev->device);
    }


    if (copy_to_user(ubuf, buf, len)){
        return -EFAULT;
    }

    return len;
}

// signal_struct
static size_t write_fpu_state_struct(char __user *ubuf, struct task_struct *task_struct_ref){
    char buf[BUF_SIZE];
    size_t len = 0;

    struct fpstate* fpu_state = task_struct_ref->thread.fpu.fpstate;

    //len += sprintf(buf,     "live = %d\n",                  atomic_read(&(signalStruct->live)));
    len += sprintf(buf+len, "size = %d\n", fpu_state->size);
    //TODO: finish

    if (copy_to_user(ubuf, buf, len)){
        return -EFAULT;
    }

    return len;
}


/*
** Эта фануция будет вызвана, когда мы ПРОЧИТАЕМ файл procfs
*/
static ssize_t read_debug(struct file *filp, char __user *ubuf, size_t count, loff_t *ppos) {

    char buf[BUF_SIZE];
    int len = 0;
    struct task_struct *task_struct_ref = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
    
    printk(KERN_INFO "proc file read.....\n");
    if (*ppos > 0 || count < BUF_SIZE){
        return 0;
    }

    if (task_struct_ref == NULL){
        len += sprintf(buf,"task_struct for pid %d is NULL. Can not get any information\n",pid);

        if (copy_to_user(ubuf, buf, len)){
            return -EFAULT;
        }
        *ppos = len;
        return len;
    }

    switch(struct_id){
        default:
        case 0:
            len = write_pci_device_struct(ubuf);
            break;
        case 1:
            len = write_fpu_state_struct(ubuf, task_struct_ref);
            break;
    }

    *ppos = len;
    return len;
}

/*
** Эта фануция будет вызвана, когда мы ЗАПИШЕМ в файл procfs
*/
static ssize_t write_debug(struct file *filp, const char __user *ubuf, size_t count, loff_t *ppos) {

    int num_of_read_digits, c, a, b;
    char buf[BUF_SIZE];
    
    printk(KERN_INFO "proc file wrote.....\n");

    if (*ppos > 0 || count > BUF_SIZE){
        return -EFAULT;
    }

    if( copy_from_user(buf, ubuf, count) ) {
        return -EFAULT;
    }

    num_of_read_digits = sscanf(buf, "%d %d", &a, &b);
    if (num_of_read_digits != 2){
        return -EFAULT;
    }

    struct_id = a;
    pid = b;

    c = strlen(buf);
    *ppos = c;
    return c;
}

/*
** Функция инициализации Модуля
*/
static int __init lab_driver_init(void) {
    /* Создание директории процесса. Она будет создана в файловой системе "/proc" */
    parent = debugfs_create_dir("lab",NULL);

    if( parent == NULL )
    {
        printk("Error creating proc entry");
        return -1;
    }

    /* Создание записи процесса в разделе "/proc/lab/" */
    debugfs_create_file("struct_info", 0666, parent, &filevalue, &proc_fops);

    printk("Device Driver Insert...Done!!!\n");
    return 0;
}

/*
** Функция выхода из Модуля
*/
static void __exit lab_driver_exit(void)
{
    /* Удаляет 1 запись процесса */
    //remove_proc_entry("lab/struct_info", parent);

    /* Удяление полностью /proc/lab */
    debugfs_remove_recursive(parent);
    
    printk("Device Driver Remove...Done!!!\n");
}

module_init(lab_driver_init);
module_exit(lab_driver_exit);
