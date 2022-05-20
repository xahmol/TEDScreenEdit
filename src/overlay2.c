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

#pragma code-name ("OVERLAY2");
#pragma rodata-name ("OVERLAY2");

void plotvisible(unsigned char row, unsigned char col, unsigned char setorrestore)
{
    // Plot or erase part of line or box if in visible viewport
    // Input: row, column, and flag setorrestore to plot new value (1) or restore old value (0)


    if(row>=yoffset && row<=yoffset+24 && col>=xoffset && col<=xoffset+79)
    {
        if(setorrestore==1)
        {
            TED_Plot(row-yoffset, col-xoffset,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
        }
        else
        {
            TED_Plot(row-yoffset, col-xoffset,PEEK(screenmap_screenaddr(row,col,screenwidth,screenheight)),PEEK(screenmap_attraddr(row,col,screenwidth)));
        }
    }
}

void lineandbox(unsigned char draworselect)
{
    // Select line or box from upper left corner using cursor keys, ESC for cancel and ENTER for accept
    // Input: draworselect: Choose select mode (0) or draw mode (1)

    unsigned char key;
    unsigned char x,y;

    select_startx = screen_col + xoffset;
    select_starty = screen_row + yoffset;
    select_endx = select_startx;
    select_endy = select_starty;
    select_accept = 0;

    if(draworselect)
    {
        strcpy(programmode,"line/box");
    }

    do
    {
        if(showbar) { printstatusbar(); }
        key = cgetc();

        switch (key)
        {
        case CH_CURS_RIGHT:
            cursormove(0,1,0,0);
            select_endx = screen_col + xoffset;
            for(y=select_starty;y<select_endy+1;y++)
            {
                plotvisible(y,select_endx,1);
            }
            break;

        case CH_CURS_LEFT:
            if(select_endx>select_startx)
            {
                cursormove(1,0,0,0);
                for(y=select_starty;y<select_endy+1;y++)
                {
                    plotvisible(y,select_endx,0);
                }
                select_endx = screen_col + xoffset;
            }
            break;

        case CH_CURS_UP:
            if(select_endy>select_starty)
            {
                cursormove(0,0,1,0);
                for(x=select_startx;x<select_endx+1;x++)
                {
                    plotvisible(select_endy,x,0);
                }
                select_endy = screen_row + yoffset;
            }
            break;

        case CH_CURS_DOWN:
            cursormove(0,0,0,1);
            select_endy = screen_row + yoffset;
            for(x=select_startx;x<select_endx+1;x++)
            {
                plotvisible(select_endy,x,1);
            }
            break;

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;
        
        case CH_F8:
            if(select_startx==select_endx && select_starty==select_endy) helpscreen_load(3);
            break;
        
        default:
            break;
        }
    } while (key!=CH_ESC && key != CH_STOP && key != CH_ENTER);

    if(key==CH_ENTER)
    {
        select_width = select_endx-select_startx+1;
        select_height = select_endy-select_starty+1;
    }  

    if(key==CH_ENTER && draworselect ==1)
    {
        for(y=select_starty;y<select_endy+1;y++)
        {
            memset((void*)screenmap_screenaddr(y,select_startx,screenwidth,screenheight),plotscreencode,select_width);
            memset((void*)screenmap_attraddr(y,select_startx,screenwidth),TED_Attribute(plotcolor,plotluminance,plotblink),select_width);
        }
        TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
    }
    else
    {
        TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
        if(showbar) { initstatusbar(); }
        if(key==CH_ENTER) { select_accept=1; }
    }
    if(draworselect)
    {
        strcpy(programmode,"main");
    }
}

void movemode()
{
    // Function to move the 80x25 viewport

    unsigned char key,y;
    unsigned char moved = 0;

    strcpy(programmode,"move");

    cursor(0);
    TED_Plot(screen_row,screen_col,PEEK(screenmap_screenaddr(yoffset+screen_row,xoffset+screen_col,screenwidth,screenheight)),PEEK(screenmap_attraddr(yoffset+screen_row,xoffset+screen_col,screenwidth)));
    
    if(showbar) { hidestatusbar(); }

    do
    {
        key = cgetc();

        switch (key)
        {
        case CH_CURS_RIGHT:
            TED_ScrollMove(0,0,40,25,2,1);
            TED_VChar(0,0,CH_SPACE,25,COLOR_WHITE);
            moved=1;
            break;
        
        case CH_CURS_LEFT:
            TED_ScrollMove(0,0,40,25,1,1);
            TED_VChar(0,39,CH_SPACE,25,COLOR_WHITE);
            moved=1;
            break;

        case CH_CURS_UP:
            TED_ScrollMove(0,0,40,25,8,1);
            TED_HChar(24,0,CH_SPACE,40,COLOR_WHITE);
            moved=1;
            break;
        
        case CH_CURS_DOWN:
            TED_ScrollMove(0,0,40,25,4,1);
            TED_HChar(0,0,CH_SPACE,40,COLOR_WHITE);
            moved=1;
            break;

        case CH_F8:
            helpscreen_load(3);
            break;
        
        default:
            break;
        }
    } while (key != CH_ENTER && key != CH_ESC && key != CH_STOP);

    if(moved==1)
    {
        if(key==CH_ENTER)
        {
            for(y=0;y<25;y++)
            {
                memcpy((void*)screenmap_screenaddr(y+yoffset,xoffset,screenwidth,screenheight),(void*)(SCREENMEMORY+(y*40)),40);
                memcpy((void*)screenmap_attraddr(y+yoffset,xoffset,screenwidth),(void*)(COLORMEMORY+(y*40)),40);
            }
        }
        TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
        if(showbar) { initstatusbar(); }
    }

    cursor(1);
    TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
    strcpy(programmode,"main");
    if(showbar) { printstatusbar(); }
}

void selectmode()
{
    // Function to select a screen area to delete, cut, copy or paint

    unsigned char key,movekey,x,y,ycount;

    strcpy(programmode,"select");

    movekey = 0;
    lineandbox(0);
    if(select_accept == 0) { return; }

    strcpy(programmode,"x/c/d/a/p?");

    do
    {
        if(showbar) { printstatusbar(); }
        key=cgetc();

        // Toggle statusbar
        if(key==CH_F6)
        {
            togglestatusbar();
        }

        if(key==CH_F8) { helpscreen_load(3); }

    } while (key !='d' && key !='x' && key !='c' && key != 'p' && key !='a' && key != CH_ESC && key != CH_STOP );

    if(key!=CH_ESC && key != CH_STOP)
    {
        if((key=='x' || key=='c')&&(select_width>4096))
        {
            messagepopup("selection too big.",1);
            return;
        }

        if(key=='x' || key=='c')
        {
            if(key=='x')
            {
                strcpy(programmode,"cut");
            }
            else
            {
                strcpy(programmode,"copy");
            }
            do
            {
                if(showbar) { printstatusbar(); }
                movekey = cgetc();

                switch (movekey)
                {
                // Cursor move
                case CH_CURS_LEFT:
                case CH_CURS_RIGHT:
                case CH_CURS_UP:
                case CH_CURS_DOWN:
                    plotmove(movekey);
                    break;

                case CH_F8:
                    helpscreen_load(3);
                    break;

                default:
                    break;
                }
            } while (movekey != CH_ESC && movekey != CH_STOP && movekey != CH_ENTER);

            if(movekey==CH_ENTER)
            {
                if((screen_col+xoffset+select_width>screenwidth) || (screen_row+yoffset+select_height>screenheight))
                {
                    messagepopup("selection does not fit.",1);
                    return;
                }

                for(ycount=0;ycount<select_height;ycount++)
                {
                    y=(screen_row+yoffset>=select_starty)? select_height-ycount-1 : ycount;
                    memcpy((void*)SCREENMEMORY,(void*)screenmap_attraddr(select_starty+y,select_startx,screenwidth),select_width);
                    if(key=='x') { memset((void*)screenmap_attraddr(select_starty+y,select_startx,screenwidth),COLOR_WHITE,select_width); }
                    memcpy((void*)screenmap_attraddr(screen_row+yoffset+y,screen_col+xoffset,screenwidth),(void*)SCREENMEMORY,select_width);
                    memcpy((void*)SCREENMEMORY,(void*)screenmap_screenaddr(select_starty+y,select_startx,screenwidth,screenheight),select_width);
                    if(key=='x') {memset((void*)screenmap_screenaddr(select_starty+y,select_startx,screenwidth,screenheight),CH_SPACE,select_width); }
                    memcpy((void*)screenmap_screenaddr(screen_row+yoffset+y,screen_col+xoffset,screenwidth,screenheight),(void*)SCREENMEMORY,select_width);
                }
            }
        }

        if( key=='d')
        {
            for(y=0;y<select_height;y++)
            {
                memset((void*)screenmap_screenaddr(select_starty+y,select_startx,screenwidth,screenheight),CH_SPACE,select_width);
                memset((void*)screenmap_attraddr(select_starty+y,select_startx,screenwidth),COLOR_WHITE,select_width);
            }
        }

        if(key=='a')
        {
            for(y=0;y<select_height;y++)
            {
                memset((void*)screenmap_attraddr(select_starty+y,select_startx,screenwidth),TED_Attribute(plotcolor,plotluminance,plotblink),select_width);
            }
        }

        if(key=='p')
        {
            for(y=0;y<select_height;y++)
            {
                for(x=0;x<select_width;x++)
                {
                    POKE(screenmap_attraddr(select_starty+y,select_startx+x,screenwidth),(PEEK(screenmap_attraddr(select_starty+y,select_startx+x,screenwidth)) & 0xf0)+plotcolor);
                }
            }
        }

        TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
        if(showbar) { initstatusbar(); }
        TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
    }
    strcpy(programmode,"main");
}

void resizeheight()
{
    // Function to resize screen camvas height

    unsigned int newheight = screenheight;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    unsigned char areyousure = 0;
    unsigned char sizechanged = 0;
    unsigned char y;
    char* ptrend;

    windownew(2,5,12,36,0);
    revers(1);
    textcolor(mc_menupopup);

    cputsxy(4,6,"resize canvas height");
    cputsxy(4,8,"enter new height:");

    sprintf(buffer,"%i",screenheight);
    textInput(4,9,buffer,4);
    newheight = (unsigned int)strtol(buffer,&ptrend,10);

    if((newheight*screenwidth*2) + 48 > maxsize || newheight < 25)
    {
        cputsxy(4,11,"new size unsupported. press key.");
        cgetc();
    }
    else
    {
        if(newheight < screenheight)
        {
            cputsxy(4,11,"shrinking might delete data.");
            cputsxy(4,12,"are you sure?");
            areyousure = menupulldown(20,13,5,0);
            if(areyousure==1)
            {
                memcpy((void*)screenmap_screenaddr(0,0,screenwidth,newheight),(void*)screenmap_screenaddr(0,0,screenwidth,screenheight),screenheight*screenwidth);
                if(screen_row>newheight-1) { screen_row=newheight-1; }
                sizechanged = 1;
            }
        }
        if(newheight > screenheight)
        {
            for(y=0;y<screenheight;y++)
            {
                memcpy((void*)screenmap_screenaddr(screenheight-y-1,0,screenwidth,newheight),(void*)screenmap_screenaddr(screenheight-y-1,0,screenwidth,screenheight),screenwidth);
            }
            memset((void*)screenmap_screenaddr(screenheight,0,screenwidth,newheight),CH_SPACE,(newheight-screenheight)*screenwidth);
            memset((void*)screenmap_attraddr(screenheight,0,screenwidth),COLOR_WHITE,(newheight-screenheight)*screenwidth);
            sizechanged = 1;
        }
    }

    windowrestore(0);

    if(sizechanged==1)
    {
        screenheight = newheight;
        screentotal = screenwidth * screenheight;
        yoffset=0;
        placesignature();
        TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
        sprintf(pulldownmenutitles[0][1],"height:   %5i ",screenheight);
        menuplacebar();
        if(showbar) { initstatusbar(); }
    }
}