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

#pragma code-name ("OVERLAY1");
#pragma rodata-name ("OVERLAY1");

void writemode()
{
    // Write mode with screencodes

    unsigned char key, screencode,newval;
    unsigned char rvs = 0;

    strcpy(programmode,"write");

    do
    {
        if(showbar) { printstatusbar(); }
        key = cgetc();
        newval = plotluminance;

        switch (key)
        {
        // Cursor move
        case CH_CURS_LEFT:
        case CH_CURS_RIGHT:
        case CH_CURS_UP:
        case CH_CURS_DOWN:
            plotmove(key);
            break;

        // Toggle blink
        case CH_F1:
            plotblink = (plotblink==0)? 1:0;
            TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
            break;

        // Toggle upper lower case
        case CH_F4:
            if(!charsetchanged)
            {
                charsetlowercase = (charsetlowercase==0)? 1:0;
                TED_CharsetStandard(charsetlowercase);
            }
            break;

        // Decrease luminance
        case CH_F5:
            if(plotluminance==0) { newval = 7; } else { newval = plotluminance - 1; }
            if(TED_Attribute(plotcolor,newval,plotblink) == screenbackground)
            {
                if(newval==0) { newval = 7; } else { newval--; }
            }
            change_plotluminance(newval);
            break;

        // Increase luminance
        case CH_F2:
            if(plotluminance==7) { newval = 0; } else { newval = plotluminance + 1; }
            if(TED_Attribute(plotcolor,newval,plotblink) == screenbackground)
            {
                if(newval==7) { newval = 0; } else { newval++; }
            }
            change_plotluminance(newval);
            break;

        // Delete present screencode and attributes
        case CH_DEL:
            screenmapplot(screen_row+yoffset,screen_col+xoffset,CH_SPACE,COLOR_WHITE);
            TED_Plot(screen_row,screen_col,CH_SPACE,TED_Attribute(plotcolor,plotluminance,plotblink));
            break;

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        case CH_F8:
            helpscreen_load(4);
            break;

        // Toggle RVS with the RVS ON and RVS OFF keys (control 9 and control 0)
        case CH_RVSON:
            rvs = 1;
            break;
        
        case CH_RVSOFF:
            rvs = 0;
            break;

        // Color control with Control and Commodore keys plus 0-9 key
        case CH_BLACK:
            change_plotcolor(0);
            break;
        
        case CH_WHITE:
            change_plotcolor(1);
            break;
        
        case CH_RED:
            change_plotcolor(2);
            break;

        case CH_CYAN:
            change_plotcolor(3);
            break;

        case CH_PURPLE:
            change_plotcolor(4);
            break;

        case CH_GREEN:
            change_plotcolor(5);
            break;

        case CH_BLUE:
            change_plotcolor(6);
            break;

        case CH_YELLOW:
            change_plotcolor(7);
            break;

        case CH_ORANGE:
            change_plotcolor(8);
            break;

        case CH_BROWN:
            change_plotcolor(9);
            break;
            
        case CH_YELGREEN:
            change_plotcolor(10);
            break;

        case CH_PINK:
            change_plotcolor(11);
            break;

        case CH_BLUEGREEN:
            change_plotcolor(12);
            break;

        case CH_LBLUE:
            change_plotcolor(13);
            break;

        case CH_DBLUE:
            change_plotcolor(14);
            break;

        case CH_LGREEN:
            change_plotcolor(15);
            break;

        // Write printable character                
        default:
            if(isprint(key))
            {
                if(rvs==0) { screencode = TED_PetsciiToScreenCode(key); } else { screencode = TED_PetsciiToScreenCodeRvs(key); }
                screenmapplot(screen_row+yoffset,screen_col+xoffset,screencode,TED_Attribute(plotcolor,plotluminance,plotblink));
                plotmove(CH_CURS_RIGHT);
            }
            break;
        }
    } while (key != CH_ESC && key != CH_STOP);
    strcpy(programmode,"main");
}

void colorwrite()
{
    // Write mode with colors

    unsigned char key, attribute;

    strcpy(programmode,"colorwrite");
    do
    {
        if(showbar) { printstatusbar(); }
        key = cgetc();

        // Get old attribute value
        attribute = PEEK(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth));

        switch (key)
        {

        // Cursor move
        case CH_CURS_LEFT:
        case CH_CURS_RIGHT:
        case CH_CURS_UP:
        case CH_CURS_DOWN:
            plotmove(key);
            break;

        // Toggle blink
        case CH_F1:
            attribute ^= 0x80;           // Toggle bit 7 for blink
            POKE(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth),attribute);
            plotmove(CH_CURS_RIGHT);
            break;

        // Toggle upper lower case
        case CH_F4:
            if(!charsetchanged)
            {
                charsetlowercase = (charsetlowercase==0)? 1:0;
                TED_CharsetStandard(charsetlowercase);
            }
            break;
        
        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        case CH_F8:
            helpscreen_load(4);
            break;            

        default:
            // If keypress is SHIFT+1-8 select luminance
            if(key>32 && key<42)
            {
                attribute &= 0x8f;                  // Erase bits 4-6
                attribute += (key-33)*16;           // Add color 0-9 with key 0-9
                POKE(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth),attribute);
                plotmove(CH_CURS_RIGHT);
            }
            // If keypress is 0-9 or A-F select color
            if(key>47 && key<58)
            {
                attribute &= 0xf0;                  // Erase bits 0-3
                attribute += (key -48);             // Add color 0-9 with key 0-9
                POKE(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth),attribute);
                plotmove(CH_CURS_RIGHT);
            }
            if(key>64 && key<71)
            {
                attribute &= 0xf0;                  // Erase bits 0-3
                attribute += (key -55);             // Add color 10-15 with key A-F
                POKE(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth),attribute);
                plotmove(CH_CURS_RIGHT);
            }
            break;
        }
    } while (key != CH_ESC && key != CH_STOP);
    strcpy(programmode,"main");
}

void palette_draw()
{
    // Draw window for character palette

    unsigned char attribute = mc_menupopup;
    unsigned char x,y;
    unsigned char counter = 0;
    unsigned int petsciiaddress = PETSCIIMAP;

    windowsave(0,21,0);
    TED_FillArea(0,5,CH_INVSPACE,34,21,attribute);
    textcolor(attribute);

    // Set coordinate of present char if no visual map
    if(visualmap==0)
    {
        rowsel = palettechar/32 + 2;
        colsel = palettechar%32;
    }

    // Favourites palette
    for(x=0;x<10;x++)
    {
        TED_Plot(1,6+x,favourites[x]+128,attribute);
    }

    // Full charsets
    for(y=0;y<8;y++)
    {
        for(x=0;x<32;x++)
        {
            if(visualmap)
            {
                TED_Plot(3+y,6+x,PEEK(petsciiaddress)+128,attribute);
                if(PEEK(petsciiaddress++)==palettechar)
                {
                    rowsel = y+2;
                    colsel = x;
                }
            }
            else
            {
                TED_Plot(3+y,6+x,counter+128,attribute);
            }
            counter++;
        }
    }

    // Color palette
    for(y=0;y<8;y++)
    {
        for(x=0;x<16;x++)
        {
            TED_Plot(12+y,6+x,CH_INVSPACE,TED_Attribute(x,y,0));
        }
    }
}

unsigned char palette_returnscreencode()
{
    // Get screencode from palette position

    unsigned char screencode;

    if(rowsel==0)
    {
        screencode = favourites[colsel];
    }
    if(rowsel>1 && rowsel<10)
    {
        if(visualmap)
        {
            screencode = PEEK(PETSCIIMAP+colsel + (rowsel-2)*32);
        }
        else
        {
            screencode = colsel + (rowsel-2)*32;
        }
    }
}

void palette_statusinfo()
{
    // Show status information in palette mode

    textcolor(mc_menupopup);
    revers(1);
    if(rowsel==0) { strcpy(buffer,"favorites "); }
    if(rowsel>1 && rowsel<10) { strcpy(buffer,"characters"); }
    if(rowsel>10) { strcpy(buffer,"colors    "); }
    cputsxy(23,12,buffer);
    if(rowsel<10)
    {
        gotoxy(23,14);
        cprintf("char:  %2x",palette_returnscreencode());
        cputsxy(23,16,"color:   ");
        cputsxy(23,17,"lum:     ");
    }
    else
    {
        cputsxy(23,14,"char:    ");
        gotoxy(23,16);
        cprintf("color: %2u",colsel);
        gotoxy(23,17);
        cprintf("lum:    %u",rowsel-11);
    }
    revers(0);
}

void palette()
{
    // Show character set palette

    unsigned char attribute = mc_menupopup;
    unsigned char key;

    palettechar = plotscreencode;

    strcpy(programmode,"palette");

    palette_draw();
    gotoxy(6+colsel,1+rowsel);

    // Get key loop
    do
    {
        if(showbar) { printstatusbar(); }
        palette_statusinfo();
        gotoxy(6+colsel,1+rowsel);
        key=cgetc();

        switch (key)
        {
        // Cursor movemeny
        case CH_CURS_RIGHT:       
        case CH_CURS_LEFT:
        case CH_CURS_DOWN:
        case CH_CURS_UP:
            if(key==CH_CURS_RIGHT) { colsel++; }
            if(key==CH_CURS_LEFT)
            {
                if(colsel>0)
                {
                    colsel--;
                }
                else
                {
                    if(rowsel==0)
                    {
                        rowsel=18;
                        colsel=15;
                    }
                    else
                    {
                        if(rowsel==2)
                        {
                            rowsel--;
                            colsel=9;
                        }
                        if(rowsel>2 && rowsel<10)
                        {
                            rowsel--;
                            colsel=31;
                        }
                        if(rowsel==10)
                        {
                            rowsel-=2;
                            colsel=31;
                        }
                        if(rowsel>10)
                        {
                            rowsel--;
                            colsel=15;
                        }
                    }
                }
            }
            if(key==CH_CURS_DOWN) { rowsel++; }
            if(key==CH_CURS_UP)
            {
                if(rowsel>0)
                {
                    rowsel--;
                    if(rowsel==1 || rowsel==10) { rowsel--;}
                }
                else
                {
                    rowsel=18;
                }
            }
            if(colsel>9 && rowsel==0) { colsel=0; rowsel=2; }
            if(colsel>31) { colsel=0; rowsel++; }
            if(colsel>15 && rowsel>9) { colsel=0; rowsel++; }
            if(rowsel>18)
            {
                rowsel=0;
                if(colsel>9) { colsel=9; }
            }
            if(rowsel==1 || rowsel==10) { rowsel++;}
            gotoxy(6+colsel,1+rowsel);
            break;

        // Select character
        case CH_SPACE:
        case CH_ENTER:
            if(rowsel<10)
            {
                palettechar = palette_returnscreencode();
                plotscreencode = palettechar;
            }
            else
            {
                plotcolor=colsel;
                plotluminance=rowsel-11;
            }
            key = CH_ESC;
            break;

        // Toggle visual PETSCII map
        case 'v':
            windowrestore(0);
            palettechar = palette_returnscreencode();
            visualmap = (visualmap)?0:1;
            palette_draw();
            gotoxy(46+colsel,1+rowsel);
            break;

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        // Help screen
        case CH_F8:
            windowrestore(0);
            helpscreen_load(2);
            palette_draw();
            break;
        
        default:
            // Store in favourites
            if(key>47 && key<58 && rowsel>0 && rowsel<10)
            {
                palettechar = palette_returnscreencode();
                favourites[key-48] = palettechar;
                TED_Plot(1,key-42,favourites[key-48]+128,attribute);
            }
            break;
        }
    } while (key != CH_ESC && key != CH_STOP);

    windowrestore(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));
    gotoxy(screen_col,screen_row);
    TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
    strcpy(programmode,"main");
}

void resizewidth()
{
    // Function to resize screen canvas width

    unsigned int newwidth = screenwidth;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    unsigned char areyousure = 0;
    unsigned char sizechanged = 0;
    unsigned int y;
    char* ptrend;

    windownew(2,5,12,36,0);
    revers(1);
    textcolor(mc_menupopup);

    cputsxy(4,6,"resize canvas width");
    cputsxy(4,8,"enter new width:");

    sprintf(buffer,"%i",screenwidth);
    textInput(4,9,buffer,4);
    newwidth = (unsigned int)strtol(buffer,&ptrend,10);

    if((newwidth*screenheight*2) + 24 > maxsize || newwidth<40 )
    {
        cputsxy(4,11,"new size unsupported. press key.");
        cgetc();
    }
    else
    {
        if(newwidth < screenwidth)
        {
            cputsxy(4,11,"shrinking might delete data.");
            cputsxy(4,12,"are you sure?");
            areyousure = menupulldown(20,13,5,0);
            if(areyousure==1)
            {
                for(y=1;y<screenheight;y++)
                {
                    memcpy((void*)SCREENMEMORY,(void*)screenmap_attraddr(y,0,screenwidth),newwidth);
                    memcpy((void*)screenmap_attraddr(y,0,newwidth),(void*)SCREENMEMORY,newwidth);
                }
                for(y=0;y<screenheight;y++)
                {
                    memcpy((void*)SCREENMEMORY,(void*)screenmap_screenaddr(y,0,screenwidth,screenheight),newwidth);
                    memcpy((void*)screenmap_screenaddr(y,0,newwidth,screenheight),(void*)SCREENMEMORY,newwidth);
                }
                if(screen_col>newwidth-1) { screen_col=newwidth-1; }
                sizechanged = 1;
            }
        }
        if(newwidth > screenwidth)
        {
            for(y=0;y<screenheight;y++)
            {
                memcpy((void*)SCREENMEMORY,(void*)screenmap_screenaddr(screenheight-y-1,0,screenwidth,screenheight),screenwidth);
                memcpy((void*)screenmap_screenaddr(screenheight-y-1,0,newwidth,screenheight),(void*)SCREENMEMORY,screenwidth);
                memset((void*)screenmap_screenaddr(screenheight-y-1,screenwidth,newwidth,screenheight),CH_SPACE,newwidth-screenwidth);
            }
            for(y=0;y<screenheight;y++)
            {
                memcpy((void*)SCREENMEMORY,(void*)screenmap_attraddr(screenheight-y-1,0,screenwidth),screenwidth);
                memcpy((void*)screenmap_attraddr(screenheight-y-1,0,newwidth),(void*)SCREENMEMORY,screenwidth);
                memset((void*)screenmap_attraddr(screenheight-y-1,screenwidth,newwidth),COLOR_WHITE,newwidth-screenwidth);
            }
            sizechanged = 1;
        }
    }

    windowrestore(0);

    if(sizechanged==1)
    {
        screenwidth = newwidth;
        screentotal = screenwidth * screenheight;
        xoffset = 0;
        placesignature();
        TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
        sprintf(pulldownmenutitles[0][0],"width:    %5i ",screenwidth);
        menuplacebar();
        if(showbar) { initstatusbar(); }
    }
}
