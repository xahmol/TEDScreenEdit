// ====================================================================================
// ted_core.c
// Functions and definitions which make working with the Commodore 128's TED easier
//
// Credits for code and inspiration:
//
// 6502.org: Practical Memory Move Routines
// http://6502.org/source/general/memory_move.html
//
// =====================================================================================

#include <stdio.h>
#include <string.h>
#include <peekpoke.h>
#include <cbm.h>
#include <conio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <plus4.h>
#include <device.h>
#include "defines.h"
#include "ted_core.h"

void TED_HChar(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char attribute)
{
	// Function to draw horizontal line with given character (draws from left to right)
	// Input: row and column of start position (left end of line), screencode of character to draw line with,
	//		  length in number of character positions, attribute color value

	unsigned int startaddress = TED_RowColToAddress(row,col);
	TED_addrh = (startaddress>>8) & 0xff;	// Obtain high byte of start address
	TED_addrl = startaddress & 0xff;		// Obtain low byte of start address
	TED_tmp1 = character;					// Obtain character value
	TED_tmp2 = length;					    // Obtain length value
	TED_tmp3 = attribute;					// Ontain attribute value

	TED_HChar_core();
}

void TED_VChar(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char attribute)
{
	// Function to draw vertical line with given character (draws from top to bottom)
	// Input: row and column of start position (top end of line), screencode of character to draw line with,
	//		  length in number of character positions, attribute color value

	unsigned int startaddress = TED_RowColToAddress(row,col);
	TED_addrh = (startaddress>>8) & 0xff;	// Obtain high byte of start address
	TED_addrl = startaddress & 0xff;		// Obtain low byte of start address
	TED_tmp1 = character;					// Obtain character value
	TED_tmp2 = length;						// Obtain length value
	TED_tmp3 = attribute;					// Ontain attribute value

	TED_VChar_core();
}

void TED_FillArea(unsigned char row, unsigned char col, unsigned char character, unsigned char length, unsigned char height, unsigned char attribute)
{
	// Function to draw area with given character (draws from topleft to bottomright)
	// Input: row and column of start position (topleft), screencode of character to draw line with,
	//		  length and height in number of character positions, attribute color value

	unsigned int startaddress = TED_RowColToAddress(row,col);
	TED_addrh = (startaddress>>8) & 0xff;	// Obtain high byte of start address
	TED_addrl = startaddress & 0xff;		// Obtain low byte of start address
	TED_tmp1 = character;					// Obtain character value
	TED_tmp2 = length;						// Obtain length value
	TED_tmp3 = attribute;					// Ontain attribute value
	TED_tmp4 = height;						// Obtain number of lines

	TED_FillArea_core();
}

void TED_Init(void)
{
	unsigned int r = 0;

	// Set fast mode
	fast();

	// Disable shift/commodore
	POKE(0x547,0x80);

	// Init screen
    bordercolor(COLOR_BLACK);
    bgcolor(COLOR_BLACK);
    textcolor(COLOR_WHITE);
    clrscr();
}

void TED_Exit(void)
{
	slow();						         	// Switch back to 1Mhz mode for safe exit

	// Enable shift/commodore
	POKE(0x547,0x00);

	clrscr();
}

unsigned char TED_PetsciiToScreenCode(unsigned char p)
{
	/* Convert Petscii values to screen code values */
	if(p <32)	p = p + 128;
	else if (p > 63  && p < 96 ) p = p - 64;
	else if (p > 95  && p < 128) p = p - 32;
	else if (p > 127 && p < 160) p = p + 64;
	else if (p > 159 && p < 192) p = p - 64;
	else if (p > 191 && p < 255) p = p - 128;
	else if (p == 255) p = 94;
	
	return p;
}

unsigned char TED_PetsciiToScreenCodeRvs(unsigned char p)
{
	/* Convert Petscii values to screen code values */
	if(p <32)	p = p + 128;
	else if (p > 31  && p < 64 ) p = p + 128;
	else if (p > 63  && p < 96 ) p = p + 64;
	else if (p > 95  && p < 128) p = p + 96;
	else if (p > 127 && p < 160) p = p -128;
	else if (p > 159 && p < 192) p = p + 64;
	else if (p > 191 && p < 255) p = p;
	else if (p == 255) p = 94;

	return p;
}

unsigned int TED_RowColToAddress(unsigned char row, unsigned char col)
{
	/* Function returns a TED color memory address for a given row and column */

	unsigned int addr;
	addr = row * 40 + col;

	if (addr < 1000)
	{
		addr += COLORMEMORY;
		return addr;
	}
	else
	{
		return -1;
	}
}

unsigned int TED_Load(char* filename, unsigned char deviceid, unsigned int destination)
{
	// Function to load memory from disk and save to destination address
	// Input: filename, device id and destination address

	unsigned int lastreadaddress;

	// Set device ID
	cbm_k_setlfs(0, deviceid, 0);

	// Set filename
	cbm_k_setnam(filename);
	
	// Load from file to memory
	lastreadaddress = cbm_k_load(0,destination);

	// Return last read address (if not higher than start address error has occurred)
	return lastreadaddress;
}

unsigned char TED_Save(char* filename, unsigned char deviceid, unsigned int source, unsigned int length)
{
	// Function to save a screen to disk from memory
	// Input: filename, memory address, memory bank
	// Output: error code

	unsigned char error;

	// Set device ID
	cbm_k_setlfs(0, deviceid, 0);

	// Set filename
	cbm_k_setnam(filename);
	
	// Load from file to memory
	error = cbm_k_save(source,source+length);

	return error;
}

unsigned char TED_Attribute(unsigned char color, unsigned char luminance, unsigned char blink)
{
	// Function to calculate color code from color and luminance
	// Input: Color code 0-15 and luminance 0-7

	return blink*128+luminance*16+color;
}


void TED_Plot(unsigned char row, unsigned char col, unsigned char screencode, unsigned char attribute)
{
	// Function to plot a screencodes at TED screen
	// Input: row and column, screencode to plot, attribute code

	unsigned int address = TED_RowColToAddress(row,col);
	POKE(address,attribute);
	POKE(address+0x0400,screencode);
	gotoxy(col,row);
}

void TED_PlotString(unsigned char row, unsigned char col, char* plotstring, unsigned char length, unsigned char attribute)
{
	// Function to plot a string of screencodes at TED screen, no trailing zero needed
	// Input: row and column, string to plot, length to plot, attribute code
	
	unsigned char x;

	for(x=0;x<length;x++)
	{
		TED_Plot(row,col++,plotstring[x],attribute);
	}
}

void TED_CopyViewPortToTED(unsigned int sourcebase, unsigned int sourcewidth, unsigned int sourceheight, unsigned int sourcexoffset, unsigned int sourceyoffset, unsigned char xcoord, unsigned char ycoord, unsigned char viewwidth, unsigned char viewheight )
{
	// Function to copy a viewport on the source screen map to the TED
	// Input:
	// - Source:	sourcebase			= source base address in memory
	//				sourcewidth			= number of characters per line in source screen map
	//				sourceheight		= number of lines in source screen map
	//				sourcexoffset		= horizontal offset on source screen map to start upper left corner of viewpoint
	//				sourceyoffset		= vertical offset on source screen map to start upper left corner of viewpoint
	// - Viewport:	xcoord				= x coordinate of viewport upper left corner
	//				ycoord				= y coordinate of viewport upper left corner
	//				viewwidth			= width of viewport in number of characters
	//				viewheight			= height of viewport in number of lines

	// Colors
	unsigned int stride = sourcewidth;
	unsigned int TEDbase = TED_RowColToAddress(ycoord,xcoord);

	sourcebase += (sourceyoffset * sourcewidth ) + sourcexoffset;

	TED_addrh = (sourcebase>>8) & 0xff;					// Obtain high byte of source address
	TED_addrl = sourcebase & 0xff;						// Obtain low byte of source address
	TED_desth = (TEDbase>>8) & 0xff;					// Obtain high byte of destination address
	TED_destl = TEDbase & 0xff;							// Obtain low byte of destination address
	TED_strideh = (stride>>8) & 0xff;					// Obtain high byte of stride
	TED_stridel = stride & 0xff;						// Obtain low byte of stride
	TED_tmp1 = viewheight;								// Obtain number of lines to copy
	TED_tmp2 = viewwidth;								// Obtain length of lines to copy

	TED_CopyViewPortToTED_core();

	// Characters
	sourcebase += (sourceheight * sourcewidth) + 24;
	TEDbase += 0x0400;

	TED_addrh = (sourcebase>>8) & 0xff;					// Obtain high byte of source address
	TED_addrl = sourcebase & 0xff;						// Obtain low byte of source address
	TED_desth = (TEDbase>>8) & 0xff;					// Obtain high byte of destination address
	TED_destl = TEDbase & 0xff;							// Obtain low byte of destination address
	TED_tmp1 = viewheight;								// Obtain number of lines to copy
	TED_tmp2 = viewwidth;								// Obtain length of lines to copy

	TED_CopyViewPortToTED_core();
}

void TED_ScrollCopy(unsigned int sourcebase, unsigned int sourcewidth, unsigned int sourceheight, unsigned int sourcexoffset, unsigned int sourceyoffset, unsigned char xcoord, unsigned char ycoord, unsigned char viewwidth, unsigned char viewheight, unsigned char direction)
{
	// Function to scroll a viewport on the source screen map on the TED in the given direction
	// Input:
	// - Source:	sourcebase			= source base address in memory
	//				sourcewidth			= number of characters per line in source screen map
	//				sourceheight		= number of lines in source screen map
	//				sourcexoffset		= horizontal offset on source screen map to start upper left corner of viewpoint
	//				sourceyoffset		= vertical offset on source screen map to start upper left corner of viewpoint
	// - Viewport:	xcoord				= x coordinate of viewport upper left corner
	//				ycoord				= y coordinate of viewport upper left corner
	//				viewwidth			= width of viewport in number of characters
	//				viewheight			= height of viewport in number of lines
	// - Direction:	direction			= Bit pattern for direction of scroll:
	//									  bit 7 set ($01): Left
	//									  bit 6 set ($02): right
	//									  bit 5 set ($04): down
	//									  bit 4 set ($08): up


	// First perform scroll
	TED_ScrollMove(xcoord,ycoord,viewwidth,viewheight,direction,0);

	// Finally add the new line or column
	switch (direction)
	{
	case SCROLL_LEFT:
		sourcexoffset += viewwidth;
		xcoord += --viewwidth;
		viewwidth = 1;
		break;

	case SCROLL_RIGHT:
		sourcexoffset--;
		viewwidth = 1;
		break;

	case SCROLL_DOWN:
		sourceyoffset--;
		viewheight = 1;
		break;

	case SCROLL_UP:
		sourceyoffset += viewheight;
		ycoord += --viewheight;
		viewheight = 1;
		break;
	
	default:
		break;
	}
	TED_CopyViewPortToTED(sourcebase,sourcewidth,sourceheight,sourcexoffset,sourceyoffset,xcoord,ycoord,viewwidth,viewheight);
}

void TED_ScrollMove(unsigned char xcoord, unsigned char ycoord, unsigned char viewwidth, unsigned char viewheight, unsigned char direction, unsigned char clear)
{
	// Function to scroll a viewport without filling in the emptied row or column
	// Input:
	// - Viewport:	xcoord				= x coordinate of viewport upper left corner
	//				ycoord				= y coordinate of viewport upper left corner
	//				viewwidth			= width of viewport in number of characters
	//				viewheight			= height of viewport in number of lines
	// - Direction:	direction			= Bit pattern for direction of scroll:
	//									  bit 7 set ($01): Left
	//									  bit 6 set ($02): right
	//									  bit 5 set ($04): down
	//									  bit 4 set ($08): up
	// - Clear:							= 1 for clear scrolled out area

	unsigned int sourceaddr = COLORMEMORY + (ycoord*40) + xcoord;

	// Set input for core routines
	TED_tmp1 = viewheight;				// Obtain number of lines to copy
	TED_tmp2 = viewwidth;				// Obtain length of lines to copy

	// Scroll in desired direction
	switch (direction)
	{
	case SCROLL_LEFT:
		TED_addrh = (sourceaddr>>8) & 0xff;		// Obtain high byte of source address
		TED_addrl = sourceaddr & 0xff;			// Obtain low byte of source address
		TED_Scroll_left_core();
		if(clear) { TED_FillArea(ycoord,xcoord+viewwidth-1,CH_SPACE,1,viewheight,COLOR_YELLOW); }
		break;

	case SCROLL_RIGHT:
		TED_addrh = (sourceaddr>>8) & 0xff;		// Obtain high byte of source address
		TED_addrl = sourceaddr & 0xff;			// Obtain low byte of source address
		TED_Scroll_right_core();
		if(clear) { TED_FillArea(ycoord,xcoord,CH_SPACE,1,viewheight,COLOR_YELLOW); }
		break;

	case SCROLL_DOWN:
		sourceaddr += (viewheight-2)*40;
		TED_addrh = (sourceaddr>>8) & 0xff;		// Obtain high byte of source address
		TED_addrl = sourceaddr & 0xff;			// Obtain low byte of source address
		TED_Scroll_down_core();
		if(clear) { TED_FillArea(ycoord,xcoord,CH_SPACE,viewwidth,1,COLOR_YELLOW); }
		break;

	case SCROLL_UP:
		sourceaddr += 40;
		TED_addrh = (sourceaddr>>8) & 0xff;		// Obtain high byte of source address
		TED_addrl = sourceaddr & 0xff;			// Obtain low byte of source address
		TED_Scroll_up_core();
		if(clear) { TED_FillArea(ycoord+viewheight-1,xcoord,CH_SPACE,viewwidth,1,COLOR_YELLOW); }
		break;
	
	default:
		break;
	}
}

void TED_CharsetCustom(unsigned int charsetaddress)
{
	// Function to point charset location to specified RAM address
	// Note: address must be a multiple of 1 kilobyte (1024 bytes)

	unsigned char charset_hb = (charsetaddress>>8) & 0xff;
	
	POKE(TED_CHARBASE,(PEEK(TED_CHARBASE)&0x03)|charset_hb);	// Set charset location at desired address
	POKE(TED_RAMSELECT,PEEK(TED_RAMSELECT)&0xfb);				// Clear bit 2 to look at RAM for charset
}

void TED_CharsetStandard(unsigned char lowercaseflag)
{
	// Function to point charset location to standard ROM address
	// Input: lower case flag: 0 for standard upper case, 1 for lower case

	unsigned char romaddrhb = (lowercaseflag)?0xd4:0xd0;		// Set ROM address for lower resp. upper case based on flag

	POKE(TED_CHARBASE,(PEEK(TED_CHARBASE)&0x03)|romaddrhb);		// Set charset location at standard ROM address
	POKE(TED_RAMSELECT,PEEK(TED_RAMSELECT)|0x04); 				// Set bit 2 to look at ROM for charset
}

unsigned char TED_ROM_Peek(unsigned int address)
{
	// Function to PEEK from ROM from address given as input

	TED_addrh = (address>>8) & 0xff;							// Obtain high byte of source address
	TED_addrl = address & 0xff;									// Obtain low byte of source address
	TED_ROM_Peek_core();
	return TED_tmp1;
}

void TED_ROM_Memcopy(unsigned int source, unsigned int destination, unsigned char pages)
{
	// Function to copy memory from ROM to RAM.
	// Input: source and destination addresses and length in number of pages

	TED_addrh = (source>>8) & 0xff;								// Obtain high byte of source address
	TED_addrl = source & 0xff;									// Obtain low byte of source address
	TED_desth = (destination>>8) & 0xff;						// Obtain high byte of source address
	TED_destl = destination & 0xff;								// Obtain low byte of source address
	TED_tmp1 = pages;
	TED_ROM_Memcopy_core();
}