/* 
** regtool.c, regtool driver, used to read/write memory/registers from user mode
** Copyright (C) 2013 liaods < dongsheng.liao@gmail.com >
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/
#include <linux/module.h>
#include <linux/major.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/fs.h>

#define DRIVER_NAME "regtool"

#define REGTOOL_GET 0x1234
#define REGTOOL_SET 0x5678

#define REGTOOL_MAX_ADDR 0xffffffff
#define REGTOOL_SIZE 1024

#define REGTOOL_MAJOR 280

typedef enum {
    CMD_REGCTL_BYTE = 0,
    CMD_REGCTL_WORD = 1,
    CMD_REGCTL_DWORD = 2,   
} regtool_type_t;

typedef struct {
    u32 addr;
    u32 value;
    regtool_type_t type;
} regtool_data_t;

static long regtool_ioctl(struct file *file, 
    unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    regtool_data_t data;
    void __iomem *virt;

    memset((void *)&data, 0, sizeof(data));

    if( copy_from_user((void *)&data, argp, sizeof(data)) )
        return -EFAULT;

    if( data.addr >= REGTOOL_MAX_ADDR )
        return -EINVAL;

    virt = ioremap_nocache(data.addr, REGTOOL_SIZE);
    if ( virt == NULL )
    {
        printk("remap failed!\n");
        return -ENOMEM;
    }

    switch( cmd )
    {
        case REGTOOL_GET:
            if( data.type == CMD_REGCTL_BYTE )
            {
                data.value = readb(virt);
            }
            else if( data.type == CMD_REGCTL_WORD )
            {
                data.value = readw(virt);
            }           
            else if( data.type == CMD_REGCTL_DWORD )
            {
                data.value = readl(virt);
            }
            iounmap(virt);
            if( copy_to_user(argp, (void *)&data, sizeof(data)))
                return -EFAULT;
            break;

        case REGTOOL_SET:
            if( data.type == CMD_REGCTL_BYTE )
            {
                writeb((u8)data.value, (unsigned char *)virt);
            }
            else if( data.type == CMD_REGCTL_WORD )
            {
                writew((u16)data.value, (unsigned short *)virt);
            }           
            else if( data.type == CMD_REGCTL_DWORD )
            {
                writel((u32)data.value, (unsigned long *)virt);
            }
            iounmap(virt);
            break;
            
        default:
            iounmap(virt);
            return -EINVAL;
    }
    return 0;
}

static int regtool_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int regtool_close(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations regtool_fops = {
    .owner    = THIS_MODULE,
    .unlocked_ioctl = regtool_ioctl,
    .open     = regtool_open,
    .release  = regtool_close,
};


static int __init regtool_init(void)
{
    int ret = register_chrdev(REGTOOL_MAJOR, DRIVER_NAME, &regtool_fops);
    printk("major=%d\n", REGTOOL_MAJOR);
    return ret;
}

static void __exit regtool_exit(void)
{
    unregister_chrdev(REGTOOL_MAJOR, DRIVER_NAME);
}

module_init(regtool_init);
module_exit(regtool_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liaods");

