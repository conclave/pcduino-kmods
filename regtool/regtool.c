/*
** regtool.c, used to get/set registers from user space
** usage: regtool address < b | w | d > < get | < set value > >
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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <string.h>
#include <unistd.h>

#define REGTOOL_DEV "/dev/regtool"
#define REGTOOL_GET 0x1234
#define REGTOOL_SET 0x5678

#define REGTOOL_MAX_ADDR 0x50b00000

typedef enum {
    CMD_REGCTL_BYTE = 0,
    CMD_REGCTL_WORD = 1,
    CMD_REGCTL_DWORD = 2,	
} regtcl_type_t;

typedef struct {
    unsigned long addr;
    unsigned long value;
    regtcl_type_t type;
} regtool_data_t;



int main(int argc, char **argv)
{
    int i=0, j=0;
    int fd;
    regtool_data_t data;
    int cmd = -1;
    
    memset((void *)&data, 0, sizeof(data));
    
    if( argc != 4 && argc != 5)
    {
        printf("Usage: %s address <b|w|d> <get [length] | <set value> >\n", argv[0]);  
        printf("     b ( 8bit) w( 16bit) d(32 bit) \n\n");
        printf("  example:\n");
        printf("       %s 0x28000000 d get \n", argv[0]); 
        printf("       %s 0x28000000 d get 0x100\n", argv[0]); 
        printf("       %s 0x28000000 d set 0x12345678\n\n", argv[0]); 
        return -1;
    }
    

    
    system("mknod /dev/regtool c 280 0 > /dev/null 2>&1");
    fd = open(REGTOOL_DEV, O_RDWR, 0);

    if ( fd < 0 )
    {
        printf("open %s failed\n", REGTOOL_DEV);
        return -1;
    }
    
    sscanf(argv[1], "%x", (unsigned int *)&data.addr);
    
    if( strcmp("b", argv[2]) == 0 )
        data.type = CMD_REGCTL_BYTE;
    else if ( strcmp("w", argv[2]) == 0 )
        data.type = CMD_REGCTL_WORD;
    else if ( strcmp("d", argv[2]) == 0 )
        data.type = CMD_REGCTL_DWORD;        
    else
    {
        printf("invalid argument, argv[2] must be \"b\" , \"w\" or \"d\" \n");
        close(fd);
        return -1;
    }
        
    if( strcmp("get", argv[3]) == 0 )
        cmd = REGTOOL_GET;
    else if ( strcmp("set", argv[3]) == 0 )
        cmd = REGTOOL_SET;
    else
    {
        printf("invalid argument, argv[3] must be \"get\" or \"set\" \n");
        close(fd);
        return -1;
    }
    
    if( argc == 5 )
    {
        sscanf(argv[4], "%x", (unsigned int *)&data.value);
        if ( cmd == REGTOOL_GET && data.value > 0 )
        {
            i = data.value;
            for( j= 0; j < i; j+= (1<<data.type))
            {
                if( ioctl(fd, cmd, &data) )
                {
                    printf("ioctl failed!\n");
                    close(fd);
                    return -1;
                }
    
                printf("type=%d, address:0x%.8x, value=0x%.8x\n",
                    data.type, (unsigned int)data.addr, (unsigned int)data.value);
                data.addr += (1<<data.type);
            }
            close(fd);
            return 0;
        }
    }
    
    if( ioctl(fd, cmd, &data) )
    {
        printf("ioctl failed!\n");
        close(fd);
        return -1;
    }
    
    printf("address:0x%.8x, type=%d, value=0x%.8x\n", 
        (unsigned int)data.addr, data.type, (unsigned int)data.value);
    close(fd);
    return 0;
}

