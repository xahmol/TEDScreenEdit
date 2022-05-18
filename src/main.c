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
#include "overlay1.h"
#include "overlay2.h"
#include "overlay3.h"
#include "overlay4.h"

// Overlay data
unsigned char overlay_active = 0;

//Window data
struct WindowStruct Window[9];
unsigned int windowaddress = WINDOWBASEADDRESS;
unsigned char windownumber = 0;

//Menu data
unsigned char menubaroptions = 4;
unsigned char pulldownmenunumber = 8;
char menubartitles[4][12] = {"screen","file","charset","information"};
unsigned char menubarcoords[4] = {1,8,13,21};
unsigned char pulldownmenuoptions[5] = {6,4,2,2,2};
char pulldownmenutitles[5][6][17] = {
    {"width:       40 ",
     "height:      25 ",
     "background:   0 ",
     "border:       0 ",
     "clear           ",
     "fill            "},
    {"save screen     ",
     "load screen     ",
     "save project    ",
     "load project    "},
    {"load charset    ",
     "save charset    "},
    {"version/credits ",
     "exit program    "},
    {"yes",\
     "no "}
};

// Menucolors
unsigned char mc_mb_normal = COLOR_LIGHTGREEN;
unsigned char mc_mb_select = COLOR_WHITE;
unsigned char mc_pd_normal = COLOR_CYAN;
unsigned char mc_pd_select = COLOR_YELLOW;
unsigned char mc_menupopup = COLOR_WHITE;

// Global variables
unsigned char bootdevice;
char DOSstatus[40];
unsigned char charsetchanged;
unsigned char charsetlowercase;
unsigned char appexit;
unsigned char targetdevice;
char filename[21];
char programmode[11];
unsigned char showbar;

unsigned char screen_col;
unsigned char screen_row;
unsigned int xoffset;
unsigned int yoffset;
unsigned int screenwidth;
unsigned int screenheight;
unsigned int screentotal;
unsigned char screenbackground;
unsigned char screenborder;
unsigned char plotscreencode;
unsigned char plotcolor;
unsigned char plotluminance;
unsigned char plotblink;
unsigned int select_startx, select_starty, select_endx, select_endy, select_width, select_height, select_accept;
unsigned char rowsel = 0;
unsigned char colsel = 0;
unsigned char palettechar;
unsigned char visualmap = 0;
unsigned char favourites[10];

char buffer[81];
char version[22];

// Generic routines
unsigned char dosCommand(const unsigned char lfn, const unsigned char drive, const unsigned char sec_addr, const char *cmd)
{
    // Send DOS command
    // based on version DraCopy 1.0e, then modified.
    // Created 2009 by Sascha Bader.

    int res;
    if (cbm_open(lfn, drive, sec_addr, cmd) != 0)
    {
        return _oserror;
    }

    if (lfn != 15)
    {
        if (cbm_open(15, drive, 15, "") != 0)
        {
            cbm_close(lfn);
            return _oserror;
        }
    }

    DOSstatus[0] = 0;
    res = cbm_read(15, DOSstatus, sizeof(DOSstatus));

    if(lfn != 15)
    {
      cbm_close(15);
    }
    cbm_close(lfn);

    if (res < 1)
    {
        return _oserror;
    }

    return (DOSstatus[0] - 48) * 10 + DOSstatus[1] - 48;
}

unsigned int cmd(const unsigned char device, const char *cmd)
{
    // Prepare DOS command
    // based on version DraCopy 1.0e, then modified.
    // Created 2009 by Sascha Bader.
    
    return dosCommand(15, device, 15, cmd);
}

int textInput(unsigned char xpos, unsigned char ypos, char* str, unsigned char size)
{

    /**
    * input/modify a string.
    * based on version DraCopy 1.0e, then modified.
    * Created 2009 by Sascha Bader.
    * @param[in] xpos screen x where input starts.
    * @param[in] ypos screen y where input starts.
    * @param[in,out] str string that is edited, it can have content and must have at least @p size + 1 bytes. Maximum size     if 255 bytes.
    * @param[in] size maximum length of @p str in bytes.
    * @return -1 if input was aborted.
    * @return >= 0 length of edited string @p str.
    */

    register unsigned char c;
    register unsigned char idx = strlen(str);

    textcolor(mc_menupopup);
    cursor(1);
    cputsxy(xpos,ypos,str);
    
    while(1)
    {
        c = cgetc();
        switch (c)
        {
        case CH_ESC:
        case CH_STOP:
            cursor(0);
            return -1;

        case CH_ENTER:
            idx = strlen(str);
            str[idx] = 0;
            cursor(0);
            return idx;

        case CH_DEL:
            if (idx)
            {
                --idx;
                cputcxy(xpos + idx,ypos,CH_SPACE);
                for(c = idx; 1; ++c)
                {
                    unsigned char b = str[c+1];
                    str[c] = b;
                    cputcxy(xpos+c,ypos,b? b : CH_SPACE);
                    if (b == 0) { break; }
                }
                gotoxy(xpos+idx,ypos);
            }
            break;

        case CH_INS:
            c = strlen(str);
            if (c < size && c > 0 && idx < c)
            {
                ++c;
                while(c >= idx)
                {
                    str[c+1] = str[c];
                    if (c == 0) { break; }
                    --c;
                }
                str[idx] = ' ';
                cputsxy(xpos+idx,ypos,str);
            }
            break;

        case CH_CURS_LEFT:
            if (idx)
            {
                --idx;
                gotoxy(xpos+idx,ypos);
            }
            break;

        case CH_CURS_RIGHT:
            if (idx < strlen(str) && idx < size)
            {
                ++idx;
                gotoxy(xpos+idx,ypos);
            }
            break;

        default:
            if (isprint(c) && idx < size)
            {
                unsigned char flag = (str[idx] == 0);
                str[idx] = c;
                cputcxy(xpos+idx++,ypos,c);
                if (flag) { str[idx+1] = 0; }
                break;
            }
            break;
        }
    }
    return 0;
}

/* General screen functions */
void cspaces(unsigned char number)
{
    /* Function to print specified number of spaces, cursor set by conio.h functions */

    unsigned char x;

    for(x=0;x<number;x++) { cputc(CH_SPACE); }
}

void printcentered(char* text, unsigned char xpos, unsigned char ypos, unsigned char width)
{
    /* Function to print a text centered
       Input:
       - Text:  Text to be printed
       - Color: Color for text to be printed
       - Width: Width of window to align to    */

    gotoxy(xpos,ypos);
    cspaces(width);
    gotoxy(xpos,ypos);
    if(strlen(text)<width)
    {
        cspaces((width-strlen(text))/2-1);
    }
    cputs(text);
}

// Status bar functions

void printstatusbar()
{
    if(screen_row==24) { return; }

    revers(1);
    textcolor(mc_menupopup);

    sprintf(buffer,"%-10s",programmode);
    cputsxy(0,24,buffer);
    sprintf(buffer,"%3u,%3u",screen_col+xoffset,screen_row+yoffset);
    cputsxy(14,24,buffer);
    sprintf(buffer,"%2x",plotscreencode);
    cputsxy(25,24,buffer);
    TED_Plot(24,27,plotscreencode,mc_menupopup);
    sprintf(buffer,"%2u",plotcolor);
    cputsxy(31,24,buffer);
    cputcxy(36,24,0x30+plotluminance);
    TED_Plot(24,37,CH_INVSPACE,TED_Attribute(plotcolor,plotluminance,plotblink));
    if(charsetlowercase)
    {
        cputsxy(38,24,"L");
    }
    else
    {
        cputsxy(38,24," ");
    }
    if(plotblink)
    {
        cputsxy(39,24,"b");
    }
    else
    {
        cputsxy(39,24," ");
    }

    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));
    gotoxy(screen_col,screen_row);
}

void initstatusbar()
{
    if(screen_row==24) { return; }

    revers(1);
    textcolor(mc_menupopup);

    TED_FillArea(24,0,CH_INVSPACE,40,1,mc_menupopup);
    cputsxy(11,24,"xy:");
    cputsxy(22,24,"sc:");
    cputsxy(29,24,"c:");
    cputsxy(34,24,"l:");
    printstatusbar();
}

void hidestatusbar()
{
    TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset+24,0,24,40,1);
}

void togglestatusbar()
{
    if(screen_row==24) { return; }

    if(showbar)
    {
        showbar=0;
        hidestatusbar();
    }
    else
    {
        showbar=1;
        initstatusbar();
    }
}

/* Overlay functions */

void loadoverlay(unsigned char overlay_select)
{
    char modebuffer[11];

    // Load memory overlay with given number

    // Returns if overlay allready active
    if(overlay_select != overlay_active)
    {
        overlay_active = overlay_select;

        // Set status bar
        strcpy(modebuffer,programmode);
        strcpy(programmode,"loading");
        if(showbar) { printstatusbar(); }

        // Compose filename
        sprintf(buffer,"tedse.ovl%u",overlay_select);

        // Load overlay file, exit if not found
        if (cbm_load (buffer, bootdevice, NULL) == 0)
        {
            printf("\nLoading overlay file failed\n");
            exit(1);
        }

        // Restore statusbar
        strcpy(programmode,modebuffer);
        if(showbar) { printstatusbar(); }
    }   
}

// Functions for windowing and menu system

void windowsave(unsigned char ypos, unsigned char height, unsigned char loadsyscharset)
{
    /* Function to save a window
       Input:
       - ypos: startline of window
       - height: height of window    
       - loadsyscharset: load syscharset if userdefined charset is loaded enabled (1) or not (0) */

    unsigned int baseaddress = COLORMEMORY + (ypos*40);
    unsigned int length =height*40;

    Window[windownumber].address = windowaddress;
    Window[windownumber].ypos = ypos;
    Window[windownumber].height = height;

    // Copy attributes
    memcpy((int*)windowaddress,(int*)baseaddress,length);
    windowaddress += length;

    // Copy characters
    baseaddress += (SCREENMEMORY-COLORMEMORY);
    memcpy((int*)windowaddress,(int*)baseaddress,length);
    windowaddress += length;  

    windownumber++;

    // Load system charset if needed
    if(loadsyscharset == 1)
    {
        TED_CharsetStandard(charsetlowercase);
    }
}

void windowrestore(unsigned char restorealtcharset)
{
    /* Function to restore a window
       Input: restorealtcharset: request to restore user defined charset if needed enabled (1) or not (0) */

    unsigned int baseaddress = COLORMEMORY + (Window[--windownumber].ypos*40);
    unsigned int length = Window[windownumber].height*40;

    windowaddress = Window[windownumber].address;

    // Restore attributes
    memcpy((int*)baseaddress,(int*)windowaddress,length);

    // Restore characters
    baseaddress += (SCREENMEMORY-COLORMEMORY);
    memcpy((int*)baseaddress,(int*)(windowaddress+length),length);    

    // Restore custom charset if needed
    if(restorealtcharset == 1 && charsetchanged == 1)
    {
        TED_CharsetCustom(CHARSET);
    }
}

void windownew(unsigned char xpos, unsigned char ypos, unsigned char height, unsigned char width, unsigned char loadsyscharset)
{
    /* Function to make menu border
       Input:
       - xpos: x-coordinate of left upper corner
       - ypos: y-coordinate of right upper corner
       - height: number of rows in window
       - width: window width in characters
        - loadsyscharset: load syscharset if userdefined charset is loaded enabled (1) or not (0) */
 
    windowsave(ypos, height,loadsyscharset);

    TED_FillArea(ypos,xpos,CH_INVSPACE,width,height,mc_menupopup);
}

void menuplacebar()
{
    /* Function to print menu bar */

    unsigned char x;

    revers(1);
    textcolor(mc_mb_normal);
    gotoxy(0,0);
    cspaces(40);
    for(x=0;x<menubaroptions;x++)
    {
        
        cputsxy(menubarcoords[x],0,menubartitles[x]);
    }
    revers(0);
}

unsigned char menupulldown(unsigned char xpos, unsigned char ypos, unsigned char menunumber, unsigned char escapable)
{
    /* Function for pull down menu
       Input:
       - xpos = x-coordinate of upper left corner
       - ypos = y-coordinate of upper left corner
       - menunumber = 
         number of the menu as defined in pulldownmenuoptions array 
       - espacable: ability to escape with escape key enabled (1) or not (0)  */

    unsigned char x;
    unsigned char key;
    unsigned char exit = 0;
    unsigned char menuchoice = 1;

    windowsave(ypos, pulldownmenuoptions[menunumber-1],0);
    revers(1);
    for(x=0;x<pulldownmenuoptions[menunumber-1];x++)
    {
        gotoxy(xpos,ypos+x);
        textcolor(mc_pd_normal);
        cprintf(" %s ",pulldownmenutitles[menunumber-1][x]);
    }
  
    do
    {
        gotoxy(xpos,ypos+menuchoice-1);
        textcolor(mc_pd_select);
        cprintf("-%s ",pulldownmenutitles[menunumber-1][menuchoice-1]);
        
        do
        {
            key = cgetc();
        } while (key != CH_ENTER && key != CH_CURS_LEFT && key != CH_CURS_RIGHT && key != CH_CURS_UP && key != CH_CURS_DOWN && key != CH_ESC && key != CH_STOP );

        switch (key)
        {
        case CH_ESC:
        case CH_STOP:
            if(escapable == 1) { exit = 1; menuchoice = 0; }
            break;

        case CH_ENTER:
            exit = 1;
            break;
        
        case CH_CURS_LEFT:
            exit = 1;
            menuchoice = 18;
            break;
        
        case CH_CURS_RIGHT:
            exit = 1;
            menuchoice = 19;
            break;

        case CH_CURS_DOWN:
        case CH_CURS_UP:
            gotoxy(xpos,ypos+menuchoice-1);
            textcolor(mc_pd_normal);
            cprintf(" %s ",pulldownmenutitles[menunumber-1][menuchoice-1]);
            if(key==CH_CURS_UP)
            {
                menuchoice--;
                if(menuchoice<1)
                {
                    menuchoice=pulldownmenuoptions[menunumber-1];
                }
            }
            else
            {
                menuchoice++;
                if(menuchoice>pulldownmenuoptions[menunumber-1])
                {
                    menuchoice = 1;
                }
            }
            break;

        default:
            break;
        }
    } while (exit==0);
    revers(0);
    windowrestore(0);    
    return menuchoice;
}

unsigned char menumain()
{
    /* Function for main menu selection */

    unsigned char menubarchoice = 1;
    unsigned char menuoptionchoice = 0;
    unsigned char key;
    unsigned char xpos;

    menuplacebar();

    do
    {
        revers(1);
        do
        {
            gotoxy(menubarcoords[menubarchoice-1]-1,0);
            textcolor(mc_mb_select);
            cprintf(" %s",menubartitles[menubarchoice-1]);
            
            do
            {
                key = cgetc();
            } while (key != CH_ENTER && key != CH_CURS_LEFT && key != CH_CURS_RIGHT && key != CH_ESC && key != CH_STOP);

            gotoxy(menubarcoords[menubarchoice-1]-1,0);
            textcolor(mc_mb_normal);
            cprintf(" %s ",menubartitles[menubarchoice-1]);

            if(key==CH_CURS_LEFT)
            {
                menubarchoice--;
                if(menubarchoice<1)
                {
                    menubarchoice = menubaroptions;
                }
            }
            else if (key==CH_CURS_RIGHT)
            {
                menubarchoice++;
                if(menubarchoice>menubaroptions)
                {
                    menubarchoice = 1;
                }
            }
        } while (key!=CH_ENTER && key != CH_ESC && key != CH_STOP);
        if (key != CH_ESC && key != CH_STOP)
            {
            xpos=menubarcoords[menubarchoice-1]-1;
            if(xpos+strlen(pulldownmenutitles[menubarchoice-1][0])>38)
            {
                xpos=menubarcoords[menubarchoice-1]+strlen(menubartitles[menubarchoice-1])-strlen(pulldownmenutitles  [menubarchoice-1][0]);
            }
            menuoptionchoice = menupulldown(xpos,1,menubarchoice,1);
            if(menuoptionchoice==18)
            {
                menuoptionchoice=0;
                menubarchoice--;
                if(menubarchoice<1)
                {
                    menubarchoice = menubaroptions;
                }
            }
            if(menuoptionchoice==19)
            {
                menuoptionchoice=0;
                menubarchoice++;
                if(menubarchoice>menubaroptions)
                {
                    menubarchoice = 1;
                }
            }
        }
        else
        {
            menuoptionchoice = 99;
        }
    } while (menuoptionchoice==0);

    revers(0);

    return menubarchoice*10+menuoptionchoice;    
}

unsigned char areyousure(char* message, unsigned char syscharset)
{
    /* Pull down menu to verify if player is sure */
    unsigned char choice;

    windownew(5,8,6,30,syscharset);
    revers(1);
    textcolor(mc_menupopup);
    cputsxy(7,9,message);
    cputsxy(7,10,"are you sure?");
    choice = menupulldown(20,11,5,0);
    windowrestore(syscharset);
    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));
    return choice;
}

void fileerrormessage(unsigned char error, unsigned char syscharset)
{
    /* Show message for file error encountered */

    windownew(5,8,6,30,syscharset);
    revers(1);
    textcolor(mc_menupopup);
    cputsxy(7,9,"file error!");
    if(error<255)
    {
        sprintf(buffer,"error nr.: %2X",error);
        cputsxy(7,11,buffer);
    }
    cputsxy(7,13,"press key.");
    cgetc();
    windowrestore(syscharset);   
    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink)); 
}

void messagepopup(char* message, unsigned char syscharset)
{
    // Show popup with a message

    windownew(5,8,6,30,syscharset);
    revers(1);
    textcolor(mc_menupopup);
    cputsxy(7,9,message);
    cputsxy(7,11,"press key.");
    cgetc();
    windowrestore(syscharset); 
    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));  
}

// Generic screen map routines

unsigned int screenmap_screenaddr(unsigned char row, unsigned char col, unsigned int width, unsigned int height)
{
    // Function to calculate screenmap address for the character space
    // Input: row, col, width and height for screenmap

    return SCREENMAPBASE+(row*width)+col+(width*height)+24;
}

unsigned int screenmap_attraddr(unsigned char row, unsigned char col, unsigned int width)
{
    // Function to calculate screenmap address for the attribute space
    // Input: row, col, width and height for screenmap
    return SCREENMAPBASE+(row*width)+col;
}

void screenmapplot(unsigned char row, unsigned char col, unsigned char screencode, unsigned char attribute)
{
    // Function to plot a screencodes at bank 1 memory screen map
	// Input: row and column, screencode to plot, attribute code

    POKE(screenmap_screenaddr(row,col,screenwidth,screenheight),screencode);
    POKE(screenmap_attraddr(row,col,screenwidth),attribute);
}

void placesignature()
{
    // Place signature in screenmap with program version

    char versiontext[25] = "";
    unsigned char x;
    unsigned int address = SCREENMAPBASE + (screenwidth*screenheight);

    sprintf(versiontext," %s ",version);

    for(x=0;x<strlen(versiontext);x++)
    {
        POKE(address+x,versiontext[x]);
    }
}

void screenmapfill(unsigned char screencode, unsigned char attribute)
{
    // Function to fill screen with the screencode and attribute code provided as input

    unsigned int address = SCREENMAPBASE;
    
    memset((void*)address,attribute,screentotal);
    placesignature();
    address += screentotal + 24;
    memset((void*)address,screencode,screentotal);
}

void cursormove(unsigned char left, unsigned char right, unsigned char up, unsigned char down)
{
    // Move cursor and scroll screen if needed
    // Input: flags to enable (1) or disable (0) move in the different directions

    if(left == 1 )
    {
        if(screen_col==0)
        {
            if(xoffset>0)
            {
                gotoxy(screen_col,screen_row);
                TED_ScrollCopy(SCREENMAPBASE,screenwidth,screenheight,xoffset--,yoffset,0,0,40,25,2);
                initstatusbar();
            }
        }
        else
        {
            gotoxy(--screen_col,screen_row);
        }
    }
    if(right == 1 )
    {
        if(screen_col==39)
        {
            if(xoffset+screen_col<screenwidth-1)
            {
                gotoxy(screen_col,screen_row);
                TED_ScrollCopy(SCREENMAPBASE,screenwidth,screenheight,xoffset++,yoffset,0,0,40,25,1);
                initstatusbar();
            }
        }
        else
        {
            gotoxy(++screen_col,screen_row);
        }
    }
    if(up == 1 )
    {
        if(screen_row==0)
        {
            if(yoffset>0)
            {
                gotoxy(screen_col,screen_row);
                TED_ScrollCopy(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset--,0,0,40,25,4);
                initstatusbar();
            }
        }
        else
        {
            gotoxy(screen_col,--screen_row);
            if(showbar && screen_row==23) { initstatusbar(); }
        }
    }
    if(down == 1 )
    {
        if(screen_row==23) { hidestatusbar(); }
        if(screen_row==24)
        {
            if(yoffset+screen_row<screenheight-1)
            {
                gotoxy(screen_col,screen_row);
                TED_ScrollCopy(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset++,0,0,40,25,8);
                initstatusbar();
            }
        }
        else
        {
            gotoxy(screen_col,++screen_row);
        }
    }
}

// Application routines
void plotmove(unsigned char direction)
{
    // Drive cursor move
    // Input: ASCII code of cursor key pressed

    TED_Plot(screen_row,screen_col,PEEK(screenmap_screenaddr(yoffset+screen_row,xoffset+screen_col,screenwidth,screenheight)),PEEK(screenmap_attraddr(yoffset+screen_row,xoffset+screen_col,screenwidth)));

    switch (direction)
    {
    case CH_CURS_LEFT:
        cursormove(1,0,0,0);
        break;
    
    case CH_CURS_RIGHT:
        cursormove(0,1,0,0);
        break;

    case CH_CURS_UP:
        cursormove(0,0,1,0);
        break;

    case CH_CURS_DOWN:
        cursormove(0,0,0,1);
        break;
    
    default:
        break;
    }

    TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor, plotluminance,plotblink));
}

void change_plotcolor(unsigned char newval)
{
    plotcolor=newval;
    textcolor(TED_Attribute(plotcolor, plotluminance,plotblink));
    TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor, plotluminance,plotblink));
}

void change_plotluminance(unsigned char newval)
{
    plotluminance=newval;
    textcolor(TED_Attribute(plotcolor, plotluminance,plotblink));
    TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor, plotluminance,plotblink));
}

void showchareditfield()
{
    // Function to draw char editor background field
    // Input: Flag for which charset is edited, standard (0) or alternate (1)

    windowsave(0,12,0);
    TED_FillArea(0,27,CH_INVSPACE,13,12,mc_menupopup);
}

unsigned int charaddress(unsigned char screencode, unsigned char romorram)
{
    // Function to calculate address of character to edit
    // Input:   screencode to edit, 
    //          flag for ROM (0) or RAM (1) memory address

    unsigned int address;

    if(romorram==0)
    {
        address = (charsetlowercase)?0xd400:0xd000;
    }
    else
    {
        address = CHARSET;
    }
    address += screencode*8;
    return address;
}

void showchareditgrid(unsigned int screencode)
{
    // Function to draw grid with present char to edit

    unsigned char x,y,char_byte,colorbase;
    unsigned int address;

    address = charaddress(screencode,1);
    
    colorbase = mc_menupopup;
    revers(1);
    textcolor(colorbase);
    gotoxy(28,1);
    cprintf("char %2x",screencode);
    revers(0);

    for(y=0;y<8;y++)
    {
        char_byte = PEEK(address+y);
        sprintf(buffer,"%2x",char_byte);
        cputsxy(28,y+3,buffer);
        for(x=0;x<8;x++)
        {
            if(char_byte & (1<<(7-x)))
            {
                TED_Plot(y+3,x+31,CH_INVSPACE,colorbase);
            }
            else
            {
                TED_Plot(y+3,x+31,CH_SPACE,colorbase);
            }
        }
    }
}

void mainmenuloop()
{
    // Function for main menu selection loop

    unsigned char menuchoice;
    
    windowsave(0,1,1);

    do
    {
        menuchoice = menumain();
      
        switch (menuchoice)
        {
        case 11:
            loadoverlay(1);
            resizewidth();
            break;

        case 12:
            loadoverlay(2);
            resizeheight();
            break;
        
        case 13:
            loadoverlay(3);
            changebackgroundcolor();
            break;

        case 14:
            loadoverlay(3);
            changebordercolor();
            break;

        case 15:
            screenmapfill(CH_SPACE,COLOR_WHITE);
            windowrestore(0);
            TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
            windowsave(0,1,0);
            menuplacebar();
            if(showbar) { initstatusbar(); }
            break;
        
        case 16:
            screenmapfill(plotscreencode,TED_Attribute(plotcolor,plotluminance, plotblink));
            windowrestore(0);
            TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
            windowsave(0,1,0);
            menuplacebar();
            if(showbar) { initstatusbar(); }
            break;

        case 21:
            loadoverlay(3);
            savescreenmap();
            break;

        case 22:
            loadoverlay(3);
            loadscreenmap();
            break;
        
        case 23:
            loadoverlay(3);
            saveproject();
            break;
        
        case 24:
            loadoverlay(3);
            loadproject();
            break;
        
        case 31:
            loadoverlay(3);
            loadcharset();
            break;
        
        case 32:
            loadoverlay(3);
            savecharset();
            break;

        case 41:
            loadoverlay(3);
            versioninfo();
            break;

        case 42:
            appexit = 1;
            menuchoice = 99;
            break;

        default:
            break;
        }
    } while (menuchoice < 99);
    
    windowrestore(1);
}

// Main loop

void main()
{
    // Main application initialization, loop and exit
    
    unsigned char key, newval;

    // Reset startvalues global variables
    charsetchanged = 0;
    charsetlowercase = 0;
    appexit = 0;
    screen_col = 0;
    screen_row = 0;
    xoffset = 0;
    yoffset = 0;
    screenwidth = 40;
    screenheight = 25;
    screentotal = screenwidth*screenheight;
    screenbackground = 0;
    screenborder = 0;
    plotscreencode = 0;
    plotcolor = BCOLOR_WHITE;
    plotluminance = 7;
    plotblink = 0;

    sprintf(pulldownmenutitles[0][0],"width:    %5i ",screenwidth);
    sprintf(pulldownmenutitles[0][1],"height:   %5i ",screenheight);
    sprintf(pulldownmenutitles[0][2],"background: %3i ",screenbackground);
    sprintf(pulldownmenutitles[0][3],"border:     %3i ",screenborder);

    // Obtain device number the application was started from
    bootdevice = getcurrentdevice();
    targetdevice = bootdevice;

    // Set version number in string variable
    sprintf(version,
            "v%2i.%2i - %c%c%c%c%c%c%c%c-%c%c%c%c",
            VERSION_MAJOR, VERSION_MINOR,
            BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3, BUILD_MONTH_CH0, BUILD_MONTH_CH1, BUILD_DAY_CH0, BUILD_DAY_CH1,BUILD_HOUR_CH0, BUILD_HOUR_CH1, BUILD_MIN_CH0, BUILD_MIN_CH1);

    // Initialise VDC screen and VDC assembly routines
    TED_Init();
    TED_CharsetStandard(0);

    // Load and show title screen
    printcentered("load title screen",10,24,20);

    // Load visual PETSCII map mapping data
    printcentered("load palette map",10,24,20);
    TED_Load("tedse.petv",bootdevice,PETSCIIMAP);

    // Clear screen map in bank 1 with spaces in text color white
    screenmapfill(CH_SPACE,COLOR_WHITE);
 
    // Wait for key press to start application
    printcentered("press key.",10,24,20);

    // Clear viewport of titlescreen
    clrscr();

    // Main program loop
    TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
    cursor(1);
    gotoxy(screen_col,screen_row);
    strcpy(programmode,"main");
    showbar = 1;

    initstatusbar();

    do
    {
        if(showbar) { printstatusbar(); }
        key = cgetc();

        switch (key)
        {
        // Cursor move
        case CH_CURS_LEFT:
        case CH_CURS_RIGHT:
        case CH_CURS_UP:
        case CH_CURS_DOWN:
            plotmove(key);
            break;
        
        // Increase screencode
        case '+':
            plotscreencode++;
            TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
            break;

        // Decrease screencode
        case '-':
            plotscreencode--;
            TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
            break;
        
        // Decrease color
        case ',':
            if(plotcolor==0) { newval = 15; } else { newval = plotcolor - 1; }
            if(TED_Attribute(newval,plotluminance,plotblink) == screenbackground)
            {
                if(newval==0) { newval = 15; } else { newval--; }
            }
            change_plotcolor(newval);
            break;

        // Increase color
        case '.':
            if(plotcolor==15) { newval = 0; } else { newval = plotcolor + 1; }
            if(TED_Attribute(newval,plotluminance,plotblink) == screenbackground)
            {
                if(newval==15) { newval = 0; } else { newval++; }
            }
            change_plotcolor(newval);
            break;

        // Decrease luminance
        case ':':
            if(plotluminance==0) { newval = 7; } else { newval = plotluminance - 1; }
            if(TED_Attribute(plotcolor,newval,plotblink) == screenbackground)
            {
                if(newval==0) { newval = 7; } else { newval--; }
            }
            change_plotluminance(newval);
            break;

        // Increase luminance
        case ';':
            if(plotluminance==7) { newval = 0; } else { newval = plotluminance + 1; }
            if(TED_Attribute(plotcolor,newval,plotblink) == screenbackground)
            {
                if(newval==7) { newval = 0; } else { newval++; }
            }
            change_plotluminance(newval);
            break;

         // Toggle blink
        case 'b':
            plotblink = (plotblink==0)? 1:0;
            TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
            break;
        
        // Toggle alternate character set
        case 'a':
            if(!charsetchanged)
            {
                charsetlowercase = (charsetlowercase==0)? 1:0;
                TED_CharsetStandard(charsetlowercase);
            }
            break;

        // Character eddit mode
        case 'e':
            loadoverlay(4);
            chareditor();
            break;

        // Palette for character selection
        case 'p':
            loadoverlay(1);
            palette();
            break;

        // Grab underlying character and attributes
        case 'g':
            plotscreencode = PEEK(screenmap_screenaddr(screen_row+yoffset,screen_col+xoffset,screenwidth,screenheight));
            newval = PEEK(screenmap_attraddr(screen_row+yoffset,screen_col+xoffset,screenwidth));
            plotluminance = newval/16;
            plotcolor = newval%16;
            textcolor(newval);
            TED_Plot(screen_row,screen_col,plotscreencode,newval);
            break;

        // Write mode: type in screencodes
        case 'w':
            loadoverlay(1);
            writemode();
            break;
        
        // Color mode: type colors
        case 'c':
            loadoverlay(1);
            colorwrite();
            break;

        // Line and box mode
        case 'l':
            loadoverlay(2);
            lineandbox(1);
            break;

        // Move mode
        case 'm':
            loadoverlay(2);
            movemode();
            break;

        // Select mode
        case 's':
            loadoverlay(2);
            selectmode();
            break;

        // Try
        case 't':
            loadoverlay(3);
            plot_try();
            break;

        // Increase/decrease plot screencode by 128 (toggle 'RVS ON' and 'RVS OFF')
        case 'i':
            plotscreencode += 128;      // Will increase 128 if <128 and decrease by 128 if >128 by overflow
            TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor, plotluminance,plotblink));
            break;        

        // Plot present screencode and attribute
        case CH_SPACE:
            screenmapplot(screen_row+yoffset,screen_col+xoffset,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
            break;

        // Delete present screencode and attributes
        case CH_DEL:
            screenmapplot(screen_row+yoffset,screen_col+xoffset,CH_SPACE,COLOR_WHITE);
            break;

        // Go to upper left corner
        case CH_HOME:
            screen_row = 0;
            screen_col = 0;
            yoffset = 0;
            xoffset = 0;
            TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,screen_col,screen_row,40,25);
            if(showbar) { initstatusbar(); }
            gotoxy(screen_col,screen_row);
            TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor, plotluminance,plotblink));
            break;

        // Go to menu
        case CH_F1:
            cursor(0);
            mainmenuloop();
            TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor, plotluminance,plotblink));
            gotoxy(screen_col,screen_row);
            textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));
            cursor(1);
            break;

        // Toggle statusbar
        case CH_F6:
            togglestatusbar();
            break;

        // Help screen
        //case CH_F8:
        //    helpscreen_load(1);
        //    break;
        
        default:
            // 0-9: Favourites select
            if(key>47 && key<58)
            {
                plotscreencode = favourites[key-48];
                TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor, plotluminance,plotblink));
            }
            // Shift 1-9 or *: Store present character in favourites slot
            if(key>32 && key<43)
            {
                favourites[key-33] = plotscreencode;
            }
            break;
        }
    } while (appexit==0);

    cursor(0);
    textcolor(COLOR_YELLOW);
    TED_Exit();
}