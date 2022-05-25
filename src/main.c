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
    {"yes",
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
        cprintf("-%s",pulldownmenutitles[menunumber-1][menuchoice-1]);
        
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

// Help screens
void helpscreen_load(unsigned char screennumber)
{
    // Function to show selected help screen
    // Input: screennumber: 1-Main mode, 2-Character editor, 3-SelectMoveLinebox, 4-Write/colorwrite mode

    // Load system charset if needed
    if(charsetchanged == 1)
    {
        TED_CharsetStandard(charsetlowercase);
    }

    // Set background color to black and switch cursor off
    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);
    cursor(0);

    // Load selected help screen
    sprintf(buffer,"tedse.hsc%u",screennumber);

    if(TED_Load(buffer,bootdevice,COLORMEMORY)<=COLORMEMORY)
    {
        messagepopup("insert application disk.",0);
    }
    
    cgetc();

    // Restore screen
    bgcolor(screenbackground);
    bordercolor(screenborder);
    TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
    if(showbar) { initstatusbar(); }
    if(screennumber!=2)
    {
        gotoxy(screen_col,screen_row);
        TED_Plot(screen_row,screen_col,plotscreencode,TED_Attribute(plotcolor,plotluminance,plotblink));
    }
    cursor(1);

    // Restore custom charset if needed
    if(charsetchanged == 1)
    {
        TED_CharsetCustom(CHARSET);
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

void plot_try()
{
    unsigned char key;

    strcpy(programmode,"try");
    if(showbar) { printstatusbar(); }
    cursor(0);
    key = cgetc();
    if(key==CH_SPACE)
    {
        screenmapplot(screen_row+yoffset,screen_col+xoffset,plotscreencode,TED_Attribute(plotcolor, plotluminance, plotblink));
    }
    strcpy(programmode,"main");
    cursor(1);
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
    if(draworselect ||key!=CH_ESC || key != CH_STOP )
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

int chooseidandfilename(char* headertext, unsigned char maxlen)
{
    // Function to present dialogue to enter device id and filename
    // Input: Headertext to print, maximum length of filename input string

    unsigned char newtargetdevice;
    unsigned char valid = 0;
    char* ptrend;

    windownew(2,5,12,36,0);
    revers(1);
    textcolor(mc_menupopup);
    cputsxy(4,6,headertext);
    do
    {
        cputsxy(4,8,"choose drive id:");
        sprintf(buffer,"%u",targetdevice);
        if(textInput(4,9,buffer,2)==-1) { return -1; }
        newtargetdevice = (unsigned char)strtol(buffer,&ptrend,10);
        if(newtargetdevice > 7 && newtargetdevice<31)
        {
            valid = 1;
            targetdevice=newtargetdevice;
        }
        else{
            cputsxy(4,10,"invalid id. enter valid one.");
        }
    } while (valid==0);
    cputsxy(4,10,"choose filename:            ");
    valid = textInput(4,11,filename,maxlen);
    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));
    return valid;
}

unsigned char checkiffileexists(char* filetocheck, unsigned char id)
{
    // Check if file exists and, if yes, ask confirmation of overwrite
    
    unsigned char proceed = 1;
    unsigned char yesno;
    unsigned char error;

    sprintf(buffer,"r0:%s=%s",filetocheck,filetocheck);
    error = cmd(id,buffer);

    if (error == 63)
    {
        yesno = areyousure("file exists.",0);
        if(yesno==2)
        {
            proceed = 0;
        }
        else
        {
            proceed = 2;
        }
    }

    return proceed;
}

void loadscreenmap()
{
    // Function to load screenmap

    unsigned int lastreadaddress, newwidth, newheight;
    unsigned int maxsize = MEMORYLIMIT - SCREENMAPBASE;
    char* ptrend;
    int escapeflag;
  
    escapeflag = chooseidandfilename("load screen",15);

    if(escapeflag==-1) { windowrestore(0); return; }

    revers(1);
    textcolor(mc_menupopup);

    cputsxy(4,12,"enter screen width:");
    sprintf(buffer,"%i",screenwidth);
    textInput(4,13,buffer,4);
    newwidth = (unsigned int)strtol(buffer,&ptrend,10);

    cputsxy(4,14,"enter screen height:");
    sprintf(buffer,"%i",screenheight);
    textInput(4,15,buffer,4);
    newheight = (unsigned int)strtol(buffer,&ptrend,10);

    if((newwidth*newheight*2) + 24 > maxsize || newwidth<40 || newheight<25)
    {
        cputsxy(4,16,"new size unsupported. Press key.");
        cgetc();
        windowrestore(0);
    }
    else
    {
        windowrestore(0);

        lastreadaddress = TED_Load(filename,targetdevice,SCREENMAPBASE);

        if(lastreadaddress>SCREENMAPBASE)
        {
            windowrestore(0);
            screenwidth = newwidth;
            screenheight = newheight;
            TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
            windowsave(0,1,0);
            menuplacebar();
            if(showbar) { initstatusbar(); }
        }
    }

    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));
}

void savescreenmap()
{
    // Function to save screenmap

    unsigned char error, overwrite;
    int escapeflag;
  
    escapeflag = chooseidandfilename("save screen",15);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    overwrite = checkiffileexists(filename,targetdevice);

    if(overwrite)
    {
        // Scratch old file
        if(overwrite==2)
        {
            sprintf(buffer,"s:%s",filename);
            cmd(targetdevice,buffer);
        }

        // Set device ID
	    cbm_k_setlfs(0, targetdevice, 0);
    
	    // Set filename
	    cbm_k_setnam(filename);
    
	    // Load from file to memory
	    error = cbm_k_save(SCREENMAPBASE,SCREENMAPBASE+(screenwidth*screenheight*2)+48);
    
        if(error) { fileerrormessage(error,0); }
    }
}

void saveproject()
{
    // Function to save project (screen, charsets and metadata)

    unsigned char error,overwrite;
    char projbuffer[21];
    char tempfilename[21];
    int escapeflag;
  
    escapeflag = chooseidandfilename("save project",10);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    sprintf(tempfilename,"%s.proj",filename);

    overwrite = checkiffileexists(filename,targetdevice);

    if(overwrite)
    {
        // Scratch old files
        if(overwrite==2)
        {
            sprintf(buffer,"s:%s.proj",filename);
            cmd(targetdevice,buffer);
            sprintf(buffer,"s:%s.scrn",filename);
            cmd(targetdevice,buffer);
            sprintf(buffer,"s:%s.chrs",filename);
            cmd(targetdevice,buffer);
            sprintf(buffer,"s:%s.chra",filename);
            cmd(targetdevice,buffer);
        }

        // Store project data to buffer variable
        projbuffer[ 0] = charsetchanged;
        projbuffer[ 1] = charsetlowercase;
        projbuffer[ 2] = screen_col;
        projbuffer[ 3] = screen_row;
        projbuffer[ 4] = (screenwidth>>8) & 0xff;
        projbuffer[ 5] = screenwidth & 0xff;
        projbuffer[ 6] = (screenheight>>8) & 0xff;
        projbuffer[ 7] = screenheight & 0xff;
        projbuffer[ 8] = (screentotal>>8) & 0xff;
        projbuffer[ 9] = screentotal & 0xff;
        projbuffer[10] = screenbackground;
        projbuffer[11] = mc_mb_normal;
        projbuffer[12] = mc_mb_select;
        projbuffer[13] = mc_pd_normal;
        projbuffer[14] = mc_pd_select;
        projbuffer[15] = mc_menupopup;
        projbuffer[16] = plotscreencode;
        projbuffer[17] = plotcolor;
        projbuffer[18] = plotluminance;
        projbuffer[19] = plotblink;
        projbuffer[20] = screenborder;
	    cbm_k_setlfs(0, targetdevice, 0);
        sprintf(buffer,"%s.proj",filename);
	    cbm_k_setnam(buffer);
	    error = cbm_k_save((unsigned int)projbuffer,(unsigned int)projbuffer+21);
        if(error) { fileerrormessage(error,0); }

        // Store screen data
        cbm_k_setlfs(0, targetdevice, 0);
        sprintf(buffer,"%s.scrn",filename);
	    cbm_k_setnam(buffer);
	    error = cbm_k_save(SCREENMAPBASE,SCREENMAPBASE+(screenwidth*screenheight*2)+24);
        if(error) { fileerrormessage(error,0); }

        // Store charset
        if(charsetchanged==1)
        {
            cbm_k_setlfs(0, targetdevice, 0);
            sprintf(buffer,"%s.chrs",filename);
	        cbm_k_setnam(buffer);
	        error = cbm_k_save(CHARSET,CHARSET+128*8);
            if(error) { fileerrormessage(error,0); }
        }   
    }
}

void loadproject()
{
    // Function to load project (screen, charsets and metadata)

    unsigned int lastreadaddress;
    unsigned char projbuffer[21];
    int escapeflag;
  
    escapeflag = chooseidandfilename("load project",10);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    // Load project variables
    sprintf(buffer,"%s.proj",filename);
	cbm_k_setlfs(0,targetdevice, 0);
	cbm_k_setnam(buffer);
	lastreadaddress = cbm_k_load(0,(unsigned int)projbuffer);
    if(lastreadaddress<=(unsigned int)projbuffer) { return; }
    charsetchanged          = projbuffer[ 0];
    charsetlowercase        = projbuffer[ 1];
    screen_col              = projbuffer[ 2];
    screen_row              = projbuffer[ 3];
    screenwidth             = projbuffer[ 4]*256+projbuffer[ 5];
    sprintf(pulldownmenutitles[0][0],"width:    %5i ",screenwidth);
    screenheight            = projbuffer[ 6]*256+projbuffer [7];
    sprintf(pulldownmenutitles[0][1],"height:   %5i ",screenheight);
    screentotal             = projbuffer[ 8]*256+projbuffer[ 9];
    screenbackground        = projbuffer[10];
    bgcolor(screenbackground);
    sprintf(pulldownmenutitles[0][2],"background: %3i ",screenbackground);
    mc_mb_normal            = projbuffer[11];
    mc_mb_select            = projbuffer[12];
    mc_pd_normal            = projbuffer[13];
    mc_pd_select            = projbuffer[14];
    mc_menupopup            = projbuffer[15];
    plotscreencode          = projbuffer[16];
    plotcolor               = projbuffer[17];
    plotluminance           = projbuffer[18];
    plotblink               = projbuffer[19];
    screenborder            = projbuffer[20];
    sprintf(pulldownmenutitles[0][3],"border:     %3i ",screenborder);

    // Load screen
    sprintf(buffer,"%s.scrn",filename);
    lastreadaddress = TED_Load(buffer,targetdevice,SCREENMAPBASE);
    if(lastreadaddress>SCREENMAPBASE)
    {
        windowrestore(0);
        TED_CopyViewPortToTED(SCREENMAPBASE,screenwidth,screenheight,xoffset,yoffset,0,0,40,25);
        windowsave(0,1,0);
        menuplacebar();
        if(showbar) { initstatusbar(); }
    }

    // Load charset
    if(charsetchanged==1)
    {
        sprintf(buffer,"%s.chrs",filename);
        TED_Load(buffer,targetdevice,CHARSET);
    }
}

void loadcharset()
{
    // Function to load charset
    // Input: stdoralt: standard charset (0) or alternate charset (1)

    unsigned int lastreadaddress;
    int escapeflag;
  
    escapeflag = chooseidandfilename("load character set",15);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    lastreadaddress = TED_Load(filename,targetdevice,CHARSET);

    if(lastreadaddress>CHARSET)
    {
        charsetchanged=1;
    }
}

void savecharset()
{
    // Function to save charset
    // Input: stdoralt: standard charset (0) or alternate charset (1)

    unsigned char error;
    int escapeflag;
  
    escapeflag = chooseidandfilename("save character set",15);

    windowrestore(0);

    if(escapeflag==-1) { return; }

    if(checkiffileexists(filename,targetdevice)==1)
    {
        // Scratch old file
        sprintf(buffer,"s:%s",filename);
        cmd(targetdevice,buffer);

        // Set device ID
	    cbm_k_setlfs(0, targetdevice, 0);

	    // Set filename
	    cbm_k_setnam(filename);
    
	    // Load from file to memory
	    error = cbm_k_save(CHARSET,CHARSET+128*8);


        if(error) { fileerrormessage(error,0); }
    }
}

void changebackgroundcolor()
{
    // Function to change background color

    unsigned char key;
    unsigned char newcolor = screenbackground%16;
    unsigned char newluminance = screenbackground/16;
    unsigned char changed = 0;

    windownew(2,5,13,36,0);

    revers(1);
    textcolor(mc_menupopup);

    cputsxy(4,6,"change background color");
    sprintf(buffer,"color: %2i lum: %2i",newcolor,newluminance);
    cputsxy(4,8,buffer);
    cputsxy(4,10,"press:");
    cputsxy(4,11,"+:     increase color number");
    cputsxy(4,12,"-:     decrease color number");
    cputsxy(4,13,".:     increase luminance");
    cputsxy(4,14,".:     decrease luminance");
    cputsxy(4,15,"enter: accept color");
    cputsxy(4,16,"esc:   cancel");

    do
    {
        do
        {
            key = cgetc();
        } while (key != CH_ENTER && key != CH_ESC && key !=CH_STOP && key != '+' && key != '-'  && key != '.' && key != ',');

        switch (key)
        {
        case '+':
            newcolor++;
            if(newcolor>15) { newcolor = 0; }
            changed=1;
            break;

        case '-':
            if(newcolor==0) { newcolor = 15; } else { newcolor--; }
            changed=1;
            break;
        
        case '.':
            newluminance++;
            if(newluminance>7) { newluminance = 0; }
            changed=1;
            break;

        case ',':
            if(newluminance==0) { newluminance = 7; } else { newluminance--; }
            changed=1;
            break;
        
        case CH_ESC:
        case CH_STOP:
            changed=0;
            bgcolor(screenbackground);
            break;

        default:
            break;
        }

        if(changed == 1)
        {
            bgcolor(TED_Attribute(newcolor,newluminance,0));
            sprintf(buffer,"color: %2i lum: %2i",newcolor,newluminance);
            cputsxy(4,8,buffer);
        }
    } while (key != CH_ENTER && key != CH_ESC && key != CH_STOP );
    
    if(changed==1)
    {
        screenbackground=TED_Attribute(newcolor,newluminance,0);

        // Change menu palette based on background color

        // Default palette if black or dark grey background
        if(newcolor==0)
        {
            mc_mb_normal = COLOR_LIGHTGREEN;
            mc_mb_select = COLOR_WHITE;
            mc_pd_normal = COLOR_CYAN;
            mc_pd_select = COLOR_YELLOW;
            mc_menupopup = COLOR_WHITE;
        }
        else
        {
            // Palette for background colors with luminance 4 or higher
            if(newluminance>3)
            {
                mc_mb_normal = COLOR_BLACK;
                mc_mb_select = COLOR_WHITE;
                mc_pd_normal = COLOR_BLACK;
                mc_pd_select = COLOR_WHITE;
                mc_menupopup = COLOR_BLACK;
            }
            // Palette for background colors with luminance 3 or lower
            else
            {
                mc_mb_normal = COLOR_WHITE;
                mc_mb_select = COLOR_BLACK;
                mc_pd_normal = COLOR_WHITE;
                mc_pd_select = COLOR_BLACK;
                mc_menupopup = COLOR_WHITE;
            }
        }
                
        sprintf(pulldownmenutitles[0][2],"background: %3i ",screenbackground);
    }
    
    windowrestore(0);
    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));    
}

void changebordercolor()
{
    // Function to change background color

    unsigned char key;
    unsigned char newcolor = screenborder%16;
    unsigned char newluminance = screenborder/16;
    unsigned char changed = 0;

    windownew(2,5,13,36,0);

    revers(1);
    textcolor(mc_menupopup);

    cputsxy(4,6,"change border color");
    sprintf(buffer,"color: %2i lum: %2i",newcolor,newluminance);
    cputsxy(4,8,buffer);
    cputsxy(4,10,"press:");
    cputsxy(4,11,"+:     increase color number");
    cputsxy(4,12,"-:     decrease color number");
    cputsxy(4,13,".:     increase luminance");
    cputsxy(4,14,".:     decrease luminance");
    cputsxy(4,15,"enter: accept color");
    cputsxy(4,16,"esc:   cancel");

    do
    {
        do
        {
            key = cgetc();
        } while (key != CH_ENTER && key != CH_ESC && key !=CH_STOP && key != '+' && key != '-'  && key != '.' && key != ',');

        switch (key)
        {
        case '+':
            newcolor++;
            if(newcolor>15) { newcolor = 0; }
            changed=1;
            break;

        case '-':
            if(newcolor==0) { newcolor = 15; } else { newcolor--; }
            changed=1;
            break;
        
        case '.':
            newluminance++;
            if(newluminance>7) { newluminance = 0; }
            changed=1;
            break;

        case ',':
            if(newluminance==0) { newluminance = 7; } else { newluminance--; }
            changed=1;
            break;
        
        case CH_ESC:
        case CH_STOP:
            changed=0;
            bordercolor(screenborder);
            break;

        default:
            break;
        }

        if(changed == 1)
        {
            bordercolor(TED_Attribute(newcolor,newluminance,0));
            sprintf(buffer,"color: %2i lum: %2i",newcolor,newluminance);
            cputsxy(4,8,buffer);
        }
    } while (key != CH_ENTER && key != CH_ESC && key != CH_STOP );
    
    if(changed=1)
    {
        screenborder=TED_Attribute(newcolor,newluminance,0);                
        sprintf(pulldownmenutitles[0][3],"border:     %3i ",screenborder);
    }
    
    windowrestore(0);
    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));    
}

void versioninfo()
{
    windownew(2,5,15,35,1);
    revers(1);
    textcolor(mc_menupopup);
    cputsxy(4,6,"version information and credits");
    cputsxy(4,8,"ted screen editor");
    cputsxy(4,9,"written in 2022 by xander mol");
    sprintf(buffer,"version: %s",version);
    cputsxy(4,11,buffer);
    cputsxy(4,13,"source, docs and credits at:");
    cputsxy(4,14,"github.com/xahmol/tedscreemedit");
    cputsxy(4,16,"(c) 2022, idreamtin8bits.com");
    cputsxy(4,18,"press a key to continue.");
    cgetc();
    windowrestore(0);
    revers(0);
    textcolor(TED_Attribute(plotcolor,plotluminance,plotblink));
}

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
    if(char_screencode > 127) { char_screencode -= 128; }
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
            if(char_screencode > 127) { char_screencode -= 128; }
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
        case CH_F8:
            windowrestore(0);
            helpscreen_load(2);
            showchareditfield();
            showchareditgrid(char_screencode);
            break;

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
            resizewidth();
            break;

        case 12:
            resizeheight();
            break;
        
        case 13:
            changebackgroundcolor();
            break;

        case 14:
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
            savescreenmap();
            break;

        case 22:
            loadscreenmap();
            break;
        
        case 23:
            saveproject();
            break;
        
        case 24:
            loadproject();
            break;
        
        case 31:
            loadcharset();
            break;
        
        case 32:
            savecharset();
            break;

        case 41:
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
    TED_Load("tedse.tscr",bootdevice,COLORMEMORY);

    // Load visual PETSCII map mapping data
    printcentered("load palette map",10,24,20);
    TED_Load("tedse.petv",bootdevice,PETSCIIMAP);

    // Clear screen map in bank 1 with spaces in text color white
    screenmapfill(CH_SPACE,COLOR_WHITE);
 
    // Wait for key press to start application
    printcentered("press key.",10,24,20);
    cgetc();

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
            chareditor();
            break;

        // Palette for character selection
        case 'p':
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
            writemode();
            break;
        
        // Color mode: type colors
        case 'c':
            colorwrite();
            break;

        // Line and box mode
        case 'l':
            lineandbox(1);
            break;

        // Move mode
        case 'm':
            movemode();
            break;

        // Select mode
        case 's':
            selectmode();
            break;

        // Try
        case 't':
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
        case CH_F8:
            helpscreen_load(1);
            break;
        
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