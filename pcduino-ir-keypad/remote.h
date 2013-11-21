/*
 *  remote.h - define rawcode and keycode for remote controllers
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

#ifndef _REMOTE_H__
#define _REMOTE_H__

/* 
* TODO: need to change the following values for your remote controller
*/
#define MAX_KEYS 20
static unsigned long raw_codes[MAX_KEYS] = {
    0xba45ff00,             0xb847ff00,
    0xbb44ff00, 0xbf40ff00, 0xbc43ff00,
    0xf807ff00, 0xea15ff00, 0xf609ff00,
    0xe916ff00, 0xe619ff00, 0xf20dff00,
    0xf30cff00, 0xe718ff00, 0xa15eff00,
    0xf708ff00, 0xe31cff00, 0xa55aff00,
    0xbd42ff00, 0xad52ff00, 0xb54aff00,
};

static unsigned short keys[MAX_KEYS] = {
    KEY_POWER,  KEY_MENU,
    KEY_ESC,    KEY_KPPLUS,     KEY_BACK,
    KEY_PREVIOUS,   KEY_PLAY,   KEY_NEXT,
    KEY_0,  KEY_KPMINUS,    KEY_C,
    KEY_1,  KEY_2,  KEY_3,
    KEY_4,  KEY_5,  KEY_6,
    KEY_7,  KEY_8,  KEY_9,
};

static unsigned char key_str[MAX_KEYS][10] = {
    "POWER",                "MENU",
    "TEST(ESC)", "PLUS",    "BACK",
    "PREVIOUS", "PLAY",     "NEXT",
    "0",        "MINUS",    "C",
    "1",        "2",        "3",
    "4",        "5",        "6",
    "7",        "8",        "9",
};

#endif
