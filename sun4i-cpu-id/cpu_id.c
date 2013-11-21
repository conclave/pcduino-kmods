/*
** cpu-id.c - get cpu id and version of allwinner sun4i chip
** Copyright (C) 2013 liaods <dongsheng.liao@gmail.com>
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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/io.h>
#include <mach/system.h>

#define ID_REG_BASE 0x01c23800

extern enum sw_ic_ver sw_get_ic_ver(void);

static int cpuid_show(char *page, char **start, off_t off,
                          int count, int *eof, void *data)
{
    unsigned char buf[128];
    void *addr;
    char ver;
    *eof = 1;
    if( off > 0 )       
        return 0;

    addr = ioremap_nocache(ID_REG_BASE, 128);
    if ( addr == NULL )
    {
        printk("remap failed!\n");
        return -ENOMEM;
    }

    memset((void *)&buf, 0, sizeof(buf));

    switch( sw_get_ic_ver() )
    {
    case MAGIC_VER_A:
        ver='A';
        break;
    case MAGIC_VER_B:
        ver='B';
        break;
    case MAGIC_VER_C:
        ver='C';
        break;
    default:
        ver='?';
        break;
    }
    sprintf(buf, "VER:%c\nID:%.8x%.8x%.8x%.8x\n",
        ver, readl(addr), readl(addr+4), readl(addr+8), readl(addr+12));
    iounmap(addr);
    return sprintf(page, "%s", buf);
}

static int __init cpuid_init(void)
{
    create_proc_read_entry("cpuid", 644, NULL, cpuid_show, NULL);
    printk("show cpu id from /proc/cpuid\n");
    return 0;
}

static void __exit cpuid_exit(void)
{
    remove_proc_entry("cpuid", NULL);
}

module_init(cpuid_init);
module_exit(cpuid_exit);

MODULE_LICENSE("GPL");

