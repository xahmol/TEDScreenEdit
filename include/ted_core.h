// ====================================================================================
// ted_core.h
//
// Screen manipulation assembly routines
//
// =====================================================================================

#ifndef _TED_CORE_
#define _TED_CORE_

// TED addressing
#define COLORMEMORY         0x0800      // Color memory base address
#define SCREENMEMORY        0x0C00      // Screen memory base address

// TED control addresses
#define TED_RAMSELECT       0xff12      // TED data fetch ROM/RAM select (on bit 2)
#define TED_CHARBASE        0xff13      // TED Character data base address (ob bit 2-7)

// Defines for scroll directions
#define SCROLL_LEFT             0x01
#define SCROLL_RIGHT            0x02
#define SCROLL_DOWN             0x04
#define SCROLL_UP               0x08

// Variables in core Functions
extern unsigned char TED_addrh;
extern unsigned char TED_addrl;
extern unsigned char TED_desth;
extern unsigned char TED_destl;
extern unsigned char TED_strideh;
extern unsigned char TED_stridel;
extern unsigned char TED_tmp1;
extern unsigned char TED_tmp2;
extern unsigned char TED_tmp3;
extern unsigned char TED_tmp4;

// Import assembly core Functions
void TED_HChar_core();
void TED_VChar_core();
void TED_FillArea_core();
void TED_CopyViewPortToTED_core();
void TED_ScrollCopy_core();
void TED_Scroll_right_core();
void TED_Scroll_left_core();
void TED_Scroll_down_core();
void TED_Scroll_up_core();
void TED_ROM_Peek_core();
void TED_ROM_Memcopy_core();

// Function Prototypes
void TED_HChar(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char attribute);
void TED_VChar(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char attribute);
void TED_FillArea(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char height, unsigned char attribute);
void TED_Init(void);
void TED_Exit(void);
unsigned char TED_PetsciiToScreenCode(unsigned char p);
unsigned char TED_PetsciiToScreenCodeRvs(unsigned char p);
unsigned int TED_RowColToAddress(unsigned char row, unsigned char col);
unsigned int TED_Load(char* filename, unsigned char deviceid, unsigned int destination);
unsigned char TED_Save(char* filename, unsigned char deviceid, unsigned int source, unsigned int length);
unsigned char TED_Attribute(unsigned char color, unsigned char luminance, unsigned char blink);
void TED_Plot(unsigned char row, unsigned char col, unsigned char screencode, unsigned char attribute);
void TED_PlotString(unsigned char row, unsigned char col, char* plotstring, unsigned char length, unsigned char attribute);
void TED_CopyViewPortToTED(unsigned int sourcebase, unsigned int sourcewidth, unsigned int sourceheight, unsigned int sourcexoffset, unsigned int sourceyoffset, unsigned char xcoord, unsigned char ycoord, unsigned char viewwidth, unsigned char viewheight );
void TED_ScrollCopy(unsigned int sourcebase, unsigned int sourcewidth, unsigned int sourceheight, unsigned int sourcexoffset, unsigned int sourceyoffset, unsigned char xcoord, unsigned char ycoord, unsigned char viewwidth, unsigned char viewheight, unsigned char direction);
void TED_ScrollMove(unsigned char xcoord, unsigned char ycoord, unsigned char viewwidth, unsigned char viewheight, unsigned char direction, unsigned char clear);
void TED_CharsetCustom(unsigned int charsetaddress);
void TED_CharsetStandard(unsigned char lowercaseflag);
unsigned char TED_ROM_Peek(unsigned int address);
void TED_ROM_Memcopy(unsigned int source, unsigned int destination, unsigned char pages);

#endif