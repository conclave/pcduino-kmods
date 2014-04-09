/*
 *  ir-keypad.c -gpio based ir-keypad driver for pcDuino
 *
 *  Copyright (C) 2013 Liaods <dongsheng.liao@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at
 *  your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "remote.h"

#define DEBUG_MSG(...)       printk("[ir-keypad]: "__VA_ARGS__);
struct ir_keypad {
    struct input_dev        *input_dev;
    struct platform_device  *pdev;
    u32 pio_hdle;
    int irq;
    u32 irq_mask;
};

struct ir_gpio {
    u32 port;
    u32 port_num;
    unsigned short irq_num;
};

static int pin = 8;
module_param(pin,int,0644);
MODULE_PARM_DESC(pin, "gpio number(0-4, 7-17, default is 8) where ir-receiver connected");

#define PORT_H ('H' - 'A' + 1)
#define PORT_I ('I' - 'A' + 1)
#define REG_RD(fmt...)  __raw_readl(fmt)
#define REG_WR(fmt...)  __raw_writel(fmt)

#ifdef CONFIG_ARCH_SUN7I
#define PIO_IRQ  60
#else
#define PIO_IRQ  28
#endif
#define PIO_BASE_VA 0xf1c20800
#define INTC_BASE_VA 0xf1c20400

#define PIO_INT_CFGn(eint_num) (0x200 + 4*(eint_num/8))
#define PIO_INT_CTL 0x210
#define PIO_INT_STA 0x214
#define PIO_INT_DEB 0x218

#define PIO_PH_CFGn(num) (0xFC + 4*(num/8))
#define PIO_PH_PULLn(num) (0x118 + 4*(num/16))
#define PIO_PI_CFGn(num) (0x120 + 4*(num/8))
#define PIO_PI_PULLn(num) (0x13c + 4*(num/16))

#define IR_VAL_ZERO  0
#define IR_VAL_ONE   1
#define IR_VAL_START 2

typedef enum {
    EDGE_FALLING = 0,
    EDGE_RISING = 1,
    LEVEL_HIGH = 2,
    LEVEL_LOW = 3,
    EDGE_DOUBLE = 4,
} pio_int_mode_t;

typedef enum {
    PULL_DISABLE = 0,
    PULL_UP = 1,
    PULL_DOWN = 2,
} pio_pull_t;

static struct ir_gpio gpio_key;

static unsigned short rawcode_to_keycode(unsigned long code)
{
    int i;
    for ( i = 0; i < MAX_KEYS; ++i)
    {
        if ( raw_codes[i] == code )
            return keys[i];
    }
    return -1;
}

static unsigned char * rawcode_to_str(unsigned long code)
{
    int i;
    for ( i = 0; i < MAX_KEYS; ++i)
    {
        if ( raw_codes[i] == code )
            return key_str[i];
    }
    return "unknown";
}


static void pio_fun_cfg(int port, int num)
{
    u32 reg;
    if ( port == PORT_H )
    {
        reg = REG_RD(PIO_BASE_VA + PIO_PH_CFGn(num));
        reg &= ~(0x7 << (num%8)*4);
        reg |= 0x6 << ((num%8)*4);
        REG_WR(reg, PIO_BASE_VA + PIO_PH_CFGn(num));
    }
    else
    {
        reg = REG_RD(PIO_BASE_VA + PIO_PI_CFGn(num));
        reg &= ~(0x7 << (num%8)*4);
        reg |= 0x6 << ((num%8)*4);
        REG_WR(reg, PIO_BASE_VA + PIO_PI_CFGn(num));
    }   
}

static void pio_int_cfg(int irq, pio_int_mode_t mode)
{
    u32 reg =   REG_RD(PIO_BASE_VA + PIO_INT_CFGn(irq));
    reg &= ~(0xf << (irq%8)*4);
    reg |= mode << ((irq%8)*4);
    REG_WR(reg, PIO_BASE_VA + PIO_INT_CFGn(irq));
}

static void pio_pull_cfg(int port, int num, pio_pull_t val)
{
    u32 reg;
    if ( port == PORT_H )
    {
        reg = REG_RD(PIO_BASE_VA + PIO_PH_PULLn(num));
        reg &= ~(0x3 << (num%15)*2);
        reg |= val << ((num%15)*2);
        REG_WR(reg, PIO_BASE_VA + PIO_PH_PULLn(num));
    }
    else
    {
        reg = REG_RD(PIO_BASE_VA + PIO_PI_PULLn(num));
        reg &= ~(0x3 << (num%15)*2);
        reg |= val << ((num%15)*2);
        REG_WR(reg, PIO_BASE_VA + PIO_PI_PULLn(num));
    }

}

/* map gpio pin on pcDuino to CPU pio */
static int init_ir_pin(u32 *port, u32 *num)
{
    switch(pin)
    {

        case 0: *port = PORT_I; *num = 19; break;
        case 1: *port = PORT_I; *num = 18; break;
        case 2: *port = PORT_H; *num = 7; break;
        case 3: *port = PORT_H; *num = 6; break;
        case 4: *port = PORT_H; *num = 8; break;
        
        case 7: *port = PORT_H; *num = 9; break;
        case 8: *port = PORT_H; *num = 10; break;
        case 9: *port = PORT_H; *num = 5; break;
        case 10: *port = PORT_I; *num = 10; break;
        case 11: *port = PORT_I; *num = 12; break;
        case 12: *port = PORT_I; *num = 13; break;
        case 13: *port = PORT_I; *num = 11; break;
        case 14: *port = PORT_H; *num = 11; break;
        case 15: *port = PORT_H; *num = 12; break;
        case 16: *port = PORT_H; *num = 13; break;
        case 17: *port = PORT_H; *num = 14; break;
        default:
            printk("invalid pin number, should be in 0-4 or 7-17\n");
            return -EINVAL;
    }
    return 0;
}

static int s_next = 0;
static int s_bits = 0;
static unsigned char s_data[4];
static s64 delta; /* ns */

static int pushbit(unchar *data, ulong code)
{
    if (code == IR_VAL_ZERO )
        *data &= ~(0x1<<s_bits);
    else if (code == IR_VAL_ONE)
        *data |= (0x1<<s_bits);
    else
    {
        s_bits = 0;
        return 1;
    }

    s_bits++;
    if (s_bits >= 8)
    {
        s_bits = 0;
        s_next++;
    }

    return s_next;
}


static int nec_dec(void)
{
    ktime_t         now;    
    static ktime_t  last_event_time;
    u32 code = 0;
    int len;
    u32 data = 0;
    
    now = ktime_get();
    delta = ktime_to_us(ktime_sub(now, last_event_time));
    //DEBUG_MSG("delta=%lld us\n", delta);
    
    last_event_time = now;

    if (delta >= 1020 && delta <= 1520)        //1.125ms  ---0
        code = IR_VAL_ZERO;
    else if (delta >= 1800 && delta <= 2350)   //2.25ms   ---1
        code = IR_VAL_ONE;
    else if (delta >= 10400 && delta <= 13600) //13.5ms   ---Leader Header
    {
        code = IR_VAL_START;
    }
    else
        return -1;

    if ( code == 2 )
    {
        len = s_next-1;

        if ( len == 1 )
            data = s_data[0];
        else if ( len == 2 )
            data =(s_data[0] << 8 | s_data[1]);
        else if ( len == 3 )
            data = ((s_data[2] << 16) | (s_data[1] << 8)| s_data[0] );
        else if ( len == 4 )
            data = ((s_data[3] << 24) |(s_data[2] << 16) | (s_data[1] << 8)| s_data[0] );
        if ( len > 0 )
        {
            s_next = 0;
            printk("data=0x%.8x len=%d\n", data, len);
            return data;
        }
        else
        s_next = 1;
    }

    if (s_next == 0)
    {
        if (code != IR_VAL_START)
            return -1;

        s_next = 1;
        s_bits = 0;
    }
    else if (s_next == 1)
        s_next = pushbit(s_data+0, code);
    else if (s_next == 2)   
        s_next = pushbit(s_data+1, code);
    else if (s_next == 3)
        s_next = pushbit(s_data+2, code);
    else if (s_next == 4)
    {
        s_next = pushbit(s_data+3, code);
        if (s_next == 5)
        {
            s_next = 0;
			data = ((s_data[3] << 24) |(s_data[2] << 16) | (s_data[1] << 8)| s_data[0] );
            return data;
        }
    }
    
    return -1;
}


static irqreturn_t ir_keypad_irq(int irq, void *dev_id)
{
    struct ir_keypad *keypad = (struct ir_keypad *)dev_id;
    unsigned int status;
    static int code;
    unsigned short key;
    
    status = REG_RD(PIO_BASE_VA + PIO_INT_STA ) ;
    if ( status & (1<<gpio_key.irq_num))
    {
        code = nec_dec();
        if ( code != -1 )
        {   
            key = rawcode_to_keycode(code);
            printk("code=%x, key=%d(%s)\n", code, key , rawcode_to_str(code) );
            if ( key != -1 )
            {
                input_report_key(keypad->input_dev, key, 1);
                input_report_key(keypad->input_dev, key, 0);
                input_sync(keypad->input_dev);
            }
        }
    }
    REG_WR(status & (1<<gpio_key.irq_num), PIO_BASE_VA + PIO_INT_STA);
    return IRQ_HANDLED;
}

static int __devinit ir_keypad_probe(struct platform_device *pdev)
{
    struct ir_keypad *keypad = kzalloc(sizeof(struct ir_keypad), GFP_KERNEL);
    struct input_dev *input_dev;
    int error;
    int i;
    u32 irq_ctl;
    
    input_dev = input_allocate_device();
    if (!keypad || !input_dev) {
        error = -ENOMEM;
        goto err_free_mem;
    }

    if(gpio_key.port == PORT_H){
        irq_ctl =   REG_RD(PIO_BASE_VA + PIO_INT_CTL);
        REG_WR((1 << gpio_key.port_num) | irq_ctl, PIO_BASE_VA + PIO_INT_CTL);
        gpio_key.irq_num = gpio_key.port_num;
    } 
    else
    {
        irq_ctl =   REG_RD(PIO_BASE_VA + PIO_INT_CTL);
        REG_WR((1 << (gpio_key.port_num + 12)) | irq_ctl, PIO_BASE_VA + PIO_INT_CTL);
        gpio_key.irq_num = gpio_key.port_num + 12;
    }
    pio_int_cfg(gpio_key.irq_num, EDGE_RISING);
    pio_fun_cfg(gpio_key.port, gpio_key.port_num);  
    pio_pull_cfg(gpio_key.port, gpio_key.port_num, PULL_UP);
    keypad->input_dev = input_dev;

    input_dev->name = pdev->name;
    input_dev->id.bustype = BUS_HOST;
    input_dev->dev.parent = &pdev->dev;
    input_set_drvdata(input_dev, keypad);

    keypad->irq_mask = 0;
    input_dev->evbit[0] = BIT_MASK(EV_KEY);
    for (i=0; i< sizeof(keys)/sizeof(keys[0]); i++)
    {
        set_bit(keys[i], input_dev->keybit);
    }
    
    keypad->irq = PIO_IRQ;

    error = request_irq(keypad->irq, ir_keypad_irq, IRQF_TRIGGER_RISING | IRQF_SHARED, dev_name(&pdev->dev), keypad);
    if (error) {
        DEBUG_MSG("failed to register keypad interrupt\n");
        goto err_free_mem;
    }
    
    error = input_register_device(keypad->input_dev);
    if (error)
        goto err_free_irq;

    platform_set_drvdata(pdev, keypad);
    keypad->pdev = pdev;
    
    DEBUG_MSG("%s done, irq %d, P%c%.2d, EINT%.2d\n", __FUNCTION__, keypad->irq,
        gpio_key.port + 'A' -1, gpio_key.port_num, gpio_key.irq_num);
    return 0;

err_free_irq:
    free_irq(keypad->irq, keypad);
err_free_mem:
    input_free_device(input_dev);
    kfree(keypad);

    return error;
}

static int __devexit ir_keypad_remove(struct platform_device *pdev)
{
    struct ir_keypad *keypad = platform_get_drvdata(pdev);

    DEBUG_MSG("%s\n", __FUNCTION__);
    
    platform_set_drvdata(pdev, NULL);

    input_unregister_device(keypad->input_dev);
    REG_WR(0x00, PIO_BASE_VA + PIO_INT_CTL);

    free_irq(keypad->irq, keypad);
    kfree(keypad);
    return 0;
}

static void ir_keypad_dev_release(struct device *dev)
{
    DEBUG_MSG("%s %d\n", __FUNCTION__, __LINE__);
}


static struct platform_device ir_keypad_device = {
    .name = "ir-keypad",
    .id = -1,
    .num_resources = 0,
    .dev = {
        .release = ir_keypad_dev_release,
    }   
};

static struct platform_driver ir_keypad_driver = {
    .probe      = ir_keypad_probe,
    .remove     = __devexit_p(ir_keypad_remove),
    .driver     = {
        .name   = "ir-keypad",
        .owner  = THIS_MODULE,
    },
};

static int __init ir_keypad_init(void)
{    
    DEBUG_MSG("%s\n", __FUNCTION__);
    if ( init_ir_pin(&gpio_key.port, &gpio_key.port_num))
        return -EINVAL;
    
    platform_device_register(&ir_keypad_device);
    return platform_driver_register(&ir_keypad_driver);
}

static void __exit ir_keypad_exit(void)
{
    DEBUG_MSG("%s\n", __FUNCTION__);
    platform_driver_unregister(&ir_keypad_driver);
    platform_device_unregister(&ir_keypad_device);
}

module_init(ir_keypad_init);
module_exit(ir_keypad_exit);

MODULE_AUTHOR("liaods <dongsheng.liao@gmail.com>");
MODULE_LICENSE("GPL");

