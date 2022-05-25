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
#include "prggenerator.h"

// Global variables
char DOSstatus[40];
unsigned char bootdevice;
unsigned char targetdevice;
char filename[21];
char filedest[21];
char buffer[81];
char version[22];
unsigned int screenwidth;
unsigned int screenheight;
unsigned char screenbackground;
unsigned char screenborder;
unsigned char charsetchanged;
unsigned char charsetlowercase;
unsigned int r = 0;
unsigned char x,newtargetdevice,error,key;
unsigned char valid = 0;
unsigned int length;
unsigned int address;
unsigned char projbuffer[22];
char* ptrend;

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

unsigned int load_save_data(char* filename, unsigned char deviceid, unsigned int address, unsigned int size, unsigned char saveflag)
{
    // Function to load or save data.
    // Input: filename, device id, destination/source address, bank (0 or 1), saveflag (0=load, 1=save)
    
    unsigned int error;

    // Set device ID
	cbm_k_setlfs(0, deviceid, 0);

    // Set filename
	cbm_k_setnam(filename);

    if(saveflag)
    {
        // Save data
        error = cbm_k_save(address, size);
    }
    else
    {
        // Load data
        error = cbm_k_load(0, address);
    }

    return error;
}

void main()
{
    // Obtain device number the application was started from
    bootdevice = getcurrentdevice();
    targetdevice = bootdevice;  

    // Set version number in string variable
    sprintf(version,
            "v%2i.%2i - %c%c%c%c%c%c%c%c-%c%c%c%c",
            VERSION_MAJOR, VERSION_MINOR,
            BUILD_YEAR_CH0, BUILD_YEAR_CH1, BUILD_YEAR_CH2, BUILD_YEAR_CH3, BUILD_MONTH_CH0, BUILD_MONTH_CH1, BUILD_DAY_CH0, BUILD_DAY_CH1,BUILD_HOUR_CH0, BUILD_HOUR_CH1, BUILD_MIN_CH0, BUILD_MIN_CH1);

    // Set fast mode
	fast();

	// Init screen
    bordercolor(COLOR_BLACK);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_YELLOW);
    clrscr();

    cputsxy(0,0,"TEDSE - PRG generator\n\r");
    cprintf("Written by Xander Mol\n\r");
    cprintf("Version %s\n\r",version);

        // User input for device ID and filenames
    cputsxy(0,3,"Choose drive ID for project to load:");
    do
    {
        sprintf(buffer,"%u",targetdevice);
        textInput(0,4,buffer,2);
        newtargetdevice = (unsigned char)strtol(buffer,&ptrend,10);
        if(newtargetdevice > 7 && newtargetdevice<31)
        {
            valid = 1;
            targetdevice=newtargetdevice;
        }
        else{
            cputsxy(0,4,"Invalid ID. Enter valid one.");
        }
    } while (valid=0);
    cputsxy(0,5,"Choose filename of project to load: ");
    textInput(0,6,filename,15);

    cputsxy(0,7,"Choose filename of generated program:");
    textInput(0,8,filedest,20);

    // Check if outtput file already exists
    sprintf(buffer,"r0:%s=%s",filedest,filedest);
    error = cmd(targetdevice,buffer);

    if (error == 63)
    {
        cputsxy(0,9,"Output file exists. Are you sure? Y/N ");
        do
        {
            key = cgetc();
        } while (key!='y' && key!='n');
        cputc(key);
        if(key=='y')
        {
            // Scratch old files
            sprintf(buffer,"s:%s",filedest);
            cmd(targetdevice,buffer);
        }
        else
        {
            exit(1);
        }
    }

    cprintf("\n\n\rLoading project meta data.\n\r");

    // Load project variables
    sprintf(buffer,"%s.proj",filename);
    length = load_save_data(buffer,targetdevice,(unsigned int)projbuffer,22,0);
    if(length<=(unsigned int)projbuffer)
    { 
        cprintf("Read error on reading project file.\n\r");
        exit(1);
    }
    charsetchanged          = projbuffer[ 0];
    charsetlowercase        = projbuffer[ 1];
    screenwidth             = projbuffer[ 4]*256+projbuffer[ 5];
    screenheight            = projbuffer[ 6]*256+projbuffer [7];
    screenbackground        = projbuffer[10];
    screenborder            = projbuffer[20];

    if(screenwidth!=40 || screenheight!=25)
    {
        cprintf("Only screen dimension of 40x25 supported.\n\r");
        exit(1);
    }

    cprintf("\nGenerating program file.\n\r");
    
    address=BASEADDRESS;

    cprintf("Loading assembly code at %4X.\n\r",address);

    // Load loader program
    length = load_save_data("tedse2prg.ass",bootdevice,address,ASS_SIZE,0);
    if(length<=BASEADDRESS)
    {
        cprintf("Load error on loading assembly code.");
        exit(1);
    }

    // Poke version string
    cprintf("Poking version string.\n\r");
    for(x=0;x<22;x++)
    {
        POKE(BASEADDRESS+VERSIONADDRESS+x,version[x]);
    }

    // Load screen
    address=SCREENSTART;
    cprintf("Loading screen data at %4X.\n\r",address);
    POKE(BGCOLORADDRESS,screenbackground);                   // Set background color
    POKE(BORDERCOLORADDR,screenborder);                      // Set border color
    POKE(CHARSET_LOWER,charsetlowercase);                    // Set lowercase flag
    sprintf(buffer,"%s.scrn",filename);
    length = load_save_data(buffer,targetdevice,address,SCREEN_SIZE,0);
    if(length<=address)
    {
        cprintf("Load error on loading screen data.");
        exit(1);
    }
    address+=SCREEN_SIZE;

    // Load standard charset if defined
    if(charsetchanged)
    {
        cprintf("Loading harset at %4X.\n\r",address);
        POKE(CHARSET_ADDRESS,address&0xff);                   // Set low byte charset address
        POKE(CHARSET_ADDRESS+1,(address>>8)&0xff);            // Set high byte charset address
        sprintf(buffer,"%s.chrs",filename);
        length = load_save_data(buffer,targetdevice,address,CHAR_SIZE,0);
        if(length<=address)
        {
            cprintf("Load error on loading standard charset data.");
            exit(1);
        }
        address+=CHAR_SIZE;
    }

    // Save complete generated program
    cprintf("Saving from %4X to %4X.\n\r",BASEADDRESS,address);
    if(load_save_data(filedest,targetdevice,BASEADDRESS,address,1))
    {
        cprintf("Save error on writing generated program.");
        exit(1);
    }

    cprintf("\nFinished!\n\r");
    cprintf("Created %s",filedest);

    slow();	
}