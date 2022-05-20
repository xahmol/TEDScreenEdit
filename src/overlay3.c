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

#pragma code-name ("OVERLAY3");
#pragma rodata-name ("OVERLAY3");

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