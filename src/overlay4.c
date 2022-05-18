/*
TED Screen Editor
Screen editor for the Commodore Plus/4
Written in 2022 by Xander Mol
Based on VDC Screen Editor for the C128

https://github.com/xahmol/VDCScreenEdit
https://www.idreamtin8bits.com/

Code and resources from others used:

-   CC65 cross compiler:
    https://cc65.github.io/

-   C128 Programmers Reference Guide: For the basic VDC register routines and VDC code inspiration
    http://www.zimmers.net/anonftp/pub/cbm/manuals/c128/C128_Programmers_Reference_Guide.pdf

-   6502.org: Practical Memory Move Routines: Starting point for memory move routines
    http://6502.org/source/general/memory_move.html

-   DraBrowse source code for DOS Command and text input routine
    DraBrowse (db*) is a simple file browser.
    Originally created 2009 by Sascha Bader.
    Used version adapted by Dirk Jagdmann (doj)
    https://github.com/doj/dracopy

-   Bart van Leeuwen: For inspiration and advice while coding.

-   jab / Artline Designs (Jaakko Luoto) for inspiration for Palette mode and PETSCII visual mode

-   Original windowing system code on Commodore 128 by unknown author.
   
-   Tested using real hardware plus VICE.

The code can be used freely as long as you retain
a notice describing original source and author.

THE PROGRAMS ARE DISTRIBUTED IN THE HOPE THAT THEY WILL BE USEFUL,
BUT WITHOUT ANY WARRANTY. USE THEM AT YOUR OWN RISK!
*/

//Includes
#include <stdio.h>
#include <string.h>
#include <peekpoke.h>
#include <cbm.h>
#include <conio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <device.h>
#include <plus4.h>
#include "defines.h"
#include "ted_core.h"
#include "main.h"

#pragma code-name ("OVERLAY4");
#pragma rodata-name ("OVERLAY4");

void chareditor()
{
    unsigned char x,y,char_screencode,key;
    unsigned char xpos=0;
    unsigned char ypos=0;
    unsigned char char_present[8];
    unsigned char char_copy[8];
    unsigned char char_undo[8];
    unsigned char char_buffer[8];
    unsigned int char_address;
    unsigned char charchanged = 0;
    char* ptrend;

    if(!charsetchanged) { TED_ROM_Memcopy(charaddress(0,0),charaddress(0,1),4); }

    char_screencode = plotscreencode;
    char_address = charaddress(char_screencode,1);
    charsetchanged=1;
    TED_CharsetCustom(CHARSET);
    strcpy(programmode,"charedit");

    for(y=0;y<8;y++)
    {
        char_present[y]=PEEK(char_address+y);
        char_undo[y]=char_present[y];
    }

    showchareditfield();
    showchareditgrid(char_screencode);
    textcolor(mc_menupopup);
    do
    {
        if(showbar) { printstatusbar(); }
        gotoxy(xpos+31,ypos+3);

        key = cgetc();

        switch (key)
        {
        // Movement
        case CH_CURS_RIGHT:
            if(xpos<7) {xpos++; }
            gotoxy(xpos+31,ypos+3);
            break;
        
        case CH_CURS_LEFT:
            if(xpos>0) {xpos--; }
            gotoxy(xpos+31,ypos+3);
            break;
        
        case CH_CURS_DOWN:
            if(ypos<7) {ypos++; }
            gotoxy(xpos+31,ypos+3);
            break;

        case CH_CURS_UP:
            if(ypos>0) {ypos--; }
            gotoxy(xpos+31,ypos+3);
            break;

        // Next or previous character
        case '+':
        case '-':
            if(key=='+')
            {
                char_screencode++;
            }
            else
            {
                char_screencode--;
            }
            charchanged=1;
            break;

        // Toggle bit
        case CH_SPACE:
            char_present[ypos] ^= 1 << (7-xpos);
            POKE(char_address+ypos,char_present[ypos]);
            showchareditgrid(char_screencode);
            break;

        // Inverse
        case 'i':
            for(y=0;y<8;y++)
            {
                char_present[y] ^= 0xff;
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Delete
        case CH_DEL:
            for(y=0;y<8;y++)
            {
                char_present[y] = 0;
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Undo
        case 'z':
            for(y=0;y<8;y++)
            {
                char_present[y] = char_undo[y];
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Restore from system font
        case 's':
            for(y=0;y<8;y++)
            {
                char_present[y] = TED_ROM_Peek(charaddress(char_screencode,0)+y);
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Copy
        case 'c':
            for(y=0;y<8;y++)
            {
                char_copy[y] = char_present[y];
            }
            break;

        // Paste
        case 'v':
            for(y=0;y<8;y++)
            {
                char_present[y] = char_copy[y];
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Mirror y axis
        case 'y':
            for(y=0;y<8;y++)
            {
                POKE(char_address+y,char_present[7-y]);
            }
            for(y=0;y<8;y++)
            {
                char_present[y]=PEEK(char_address+y);
            }
            showchareditgrid(char_screencode);
            break;

        // Mirror x axis
        case 'x':
            for(y=0;y<8;y++)
            {
                char_present[y] = (char_present[y] & 0xF0) >> 4 | (char_present[y] & 0x0F) << 4;
                char_present[y] = (char_present[y] & 0xCC) >> 2 | (char_present[y] & 0x33) << 2;
                char_present[y] = (char_present[y] & 0xAA) >> 1 | (char_present[y] & 0x55) << 1;
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Rotate clockwise
        case 'o':
            for(y=0;y<8;y++)
            {
                for(x=0;x<8;x++)
                {
                    if(char_present[y] & (1<<(7-x)))
                    {
                        char_buffer[x] |= (1<<y);
                    }
                    else
                    {
                        char_buffer[x] &= ~(1<<y);
                    }
                }
            }
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Scroll up
        case 'u':
            for(y=1;y<8;y++)
            {
                char_buffer[y-1]=char_present[y];
            }
            char_buffer[7]=char_present[0];
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Scroll down
        case 'd':
            for(y=1;y<8;y++)
            {
                char_buffer[y]=char_present[y-1];
            }
            char_buffer[0]=char_present[7];
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Scroll right
        case 'r':
            for(y=0;y<8;y++)
            {
                char_buffer[y]=char_present[y]>>1;
                if(char_present[y]&0x01) { char_buffer[y]+=0x80; }
            }
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;
        
        // Scroll left
        case 'l':
            for(y=0;y<8;y++)
            {
                char_buffer[y]=char_present[y]<<1;
                if(char_present[y]&0x80) { char_buffer[y]+=0x01; }
            }
            for(y=0;y<8;y++)
            {
                char_present[y]=char_buffer[y];
                POKE(char_address+y,char_present[y]);
            }
            showchareditgrid(char_screencode);
            break;

        // Hex edit
        case 'h':
            sprintf(buffer,"%2x",char_present[ypos]);
            revers(1);
            textInput(28,ypos+3,buffer,2);
            char_present[ypos] = (unsigned char)strtol(buffer,&ptrend,16);
            gotoxy(31+xpos,3+ypos);
            cursor(1);
            revers(0);
            POKE(char_address+ypos,char_present[ypos]);
            showchareditgrid(char_screencode);
            break;

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        // Help screen
        //case CH_F8:
        //    windowrestore(0);
        //    helpscreen_load(2);
        //    showchareditfield();
        //    showchareditgrid(char_screencode);
        //    break;

        default:
            // 0-9: Favourites select
            if(key>47 && key<58)
            {
                char_screencode = favourites[key-48];
                charchanged=1;
            }
            // Shift 1-9 or *: Store present character in favourites slot
            if(key>32 && key<43)
            {
                favourites[key-33] = char_screencode;
            }
            break;
        }

        if(charchanged)
        {
            charchanged=0;
            char_address = charaddress(char_screencode,1);
            for(y=0;y<8;y++)
            {
                char_present[y]=PEEK(char_address+y);
                char_undo[y]=char_present[y];
            }
            showchareditgrid(char_screencode);
        }
    } while (key != CH_ESC && key != CH_STOP);

    windowrestore(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));
    plotscreencode = char_screencode;
    gotoxy(screen_col,screen_row);
    TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
    strcpy(programmode,"main");
}