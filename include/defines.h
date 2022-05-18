#ifndef __DEFINES_H_
#define __DEFINES_H_

/* Machine code area addresses mapping */
#define MACOSTART           0x1300      // Start of machine code area
#define MACOSIZE            0x0800      // Length of machine code area

/* Memory addresses mapping */
#define PETSCIIMAP          0x0347      // PETSCII map in cassette/RS232 buffer
#define COLORMEMORY         0x0800      // Color memory base address
#define SCREENMEMORY        0x0C00      // Screen memory base address
#define WINDOWBASEADDRESS   0xC000      // Base address for windows system data, 8k reserved
#define CHARSET             0xC800      // Base address for redefined charset
#define SCREENMAPBASE       0xD000      // Base address for screen map
#define MEMORYLIMIT         0xCFFF      // Upper memory limit address for address map

/* Global variables */
extern unsigned char overlay_active;

//Window data
struct WindowStruct
{
    unsigned int address;
    unsigned char ypos;
    unsigned char height;
};
extern struct WindowStruct Window[9];
extern unsigned int windowaddress;
extern unsigned char windownumber;

//Menu data
extern unsigned char menubaroptions;
extern unsigned char pulldownmenunumber;
extern char menubartitles[4][12];
extern unsigned char menubarcoords[4];
extern unsigned char pulldownmenuoptions[5];
extern char pulldownmenutitles[5][6][17];

// Menucolors
extern unsigned char mc_mb_normal;
extern unsigned char mc_mb_select;
extern unsigned char mc_pd_normal;
extern unsigned char mc_pd_select;
extern unsigned char mc_menupopup;

// Global variables
extern unsigned char bootdevice;
extern char DOSstatus[40];
extern unsigned char charsetchanged;
extern unsigned char charsetlowercase;
extern unsigned char appexit;
extern unsigned char targetdevice;
extern char filename[21];
extern char programmode[11];
extern unsigned char showbar;

extern unsigned char screen_col;
extern unsigned char screen_row;
extern unsigned int xoffset;
extern unsigned int yoffset;
extern unsigned int screenwidth;
extern unsigned int screenheight;
extern unsigned int screentotal;
extern unsigned char screenbackground;
extern unsigned char screenborder;
extern unsigned char plotscreencode;
extern unsigned char plotcolor;
extern unsigned char plotluminance;
extern unsigned char plotblink;
extern unsigned int select_startx, select_starty, select_endx, select_endy, select_width, select_height, select_accept;
extern unsigned char rowsel;
extern unsigned char colsel;
extern unsigned char palettechar;
extern unsigned char visualmap;
extern unsigned char favourites[10];

extern char buffer[81];
extern char version[22];

/* Char defines */
#define CH_SPACE            0x20        // Screencode for space
#define CH_INVSPACE         0xA0        // Inverse space
#define CH_MINUS            0x2D        // Screencode for minus
#define CH_BLACK            0x90        // Petscii control code for black           CTRL-1
#define CH_WHITE            0x05        // Petscii control code for white           CTRL-2
#define CH_RED              0x1C        // Petscii control code for red             CTRL-3
#define CH_CYAN             0x9F        // Petscii control code for cyan            CTRL-4
#define CH_PURPLE           0x9C        // Petscii control code for purple          CTRL-5
#define CH_GREEN            0x1E        // Petscii control code for green           CTRL-6
#define CH_BLUE             0x1F        // Petscii control code for blue            CTRL-7
#define CH_YELLOW           0x9E        // Petscii control code for yellow          CTRL-8
#define CH_RVSON            0x12        // Petscii control code for RVS ON          CTRL-9
#define CH_RVSOFF           0x92        // Petscii control code for RVS OFF         CTRL-0
#define CH_ORANGE           0x81        // Petscii control code for orange          C=-1
#define CH_BROWN            0x95        // Petscii control code for brown           C=-2
#define CH_YELGREEN         0x96        // Petscii control code for yellow green    C=-3
#define CH_PINK             0x97        // Petscii control code for pink            C=-4
#define CH_BLUEGREEN        0x98        // Petscii control code for blue green      C=-5
#define CH_LBLUE            0x99        // Petscii control code for light blue      C=-6
#define CH_DBLUE            0x9A        // Petscii control code for dark blue       C=-7
#define CH_LGREEN           0x9B        // Petscii control code for light green     C=-8


/* Declaration global variables as externals */
extern unsigned char bootdevice;

/* Defines for versioning */
/* Version number */
#define VERSION_MAJOR 0
#define VERSION_MINOR 99
/* Build year */
#define BUILD_YEAR_CH0 (__DATE__[ 7])
#define BUILD_YEAR_CH1 (__DATE__[ 8])
#define BUILD_YEAR_CH2 (__DATE__[ 9])
#define BUILD_YEAR_CH3 (__DATE__[10])
/* Build month */
#define BUILD_MONTH_IS_JAN (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_FEB (__DATE__[0] == 'F')
#define BUILD_MONTH_IS_MAR (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')
#define BUILD_MONTH_IS_APR (__DATE__[0] == 'A' && __DATE__[1] == 'p')
#define BUILD_MONTH_IS_MAY (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')
#define BUILD_MONTH_IS_JUN (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')
#define BUILD_MONTH_IS_JUL (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')
#define BUILD_MONTH_IS_AUG (__DATE__[0] == 'A' && __DATE__[1] == 'u')
#define BUILD_MONTH_IS_SEP (__DATE__[0] == 'S')
#define BUILD_MONTH_IS_OCT (__DATE__[0] == 'O')
#define BUILD_MONTH_IS_NOV (__DATE__[0] == 'N')
#define BUILD_MONTH_IS_DEC (__DATE__[0] == 'D')
#define BUILD_MONTH_CH0 \
    ((BUILD_MONTH_IS_OCT || BUILD_MONTH_IS_NOV || BUILD_MONTH_IS_DEC) ? '1' : '0')
#define BUILD_MONTH_CH1 \
    ( \
        (BUILD_MONTH_IS_JAN) ? '1' : \
        (BUILD_MONTH_IS_FEB) ? '2' : \
        (BUILD_MONTH_IS_MAR) ? '3' : \
        (BUILD_MONTH_IS_APR) ? '4' : \
        (BUILD_MONTH_IS_MAY) ? '5' : \
        (BUILD_MONTH_IS_JUN) ? '6' : \
        (BUILD_MONTH_IS_JUL) ? '7' : \
        (BUILD_MONTH_IS_AUG) ? '8' : \
        (BUILD_MONTH_IS_SEP) ? '9' : \
        (BUILD_MONTH_IS_OCT) ? '0' : \
        (BUILD_MONTH_IS_NOV) ? '1' : \
        (BUILD_MONTH_IS_DEC) ? '2' : \
        /* error default */    '?' \
    )
/* Build day */
#define BUILD_DAY_CH0 ((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')
#define BUILD_DAY_CH1 (__DATE__[ 5])
/* Build hour */
#define BUILD_HOUR_CH0 (__TIME__[0])
#define BUILD_HOUR_CH1 (__TIME__[1])
/* Build minute */
#define BUILD_MIN_CH0 (__TIME__[3])
#define BUILD_MIN_CH1 (__TIME__[4])

#endif // __DEFINES_H_