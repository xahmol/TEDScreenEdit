; ====================================================================================
; TED_core_assembly.s
; Core assembly routines for TED_core.c
;
; Credits for code and inspiration:
;
; 6502.org: Practical Memory Move Routines
; http://6502.org/source/general/memory_move.html
;
; =====================================================================================

	.export		_TED_HChar_core
	.export		_TED_VChar_core
	.export		_TED_FillArea_core
	.export		_TED_CopyViewPortToTED_core
	.export		_TED_Scroll_right_core
	.export		_TED_Scroll_left_core
	.export		_TED_Scroll_down_core
	.export		_TED_Scroll_up_core
	.export		_TED_ROM_Peek_core
	.export		_TED_ROM_Memcopy_core
	.export		_TED_addrh
	.export		_TED_addrl
	.export		_TED_desth
	.export		_TED_destl
	.export 	_TED_strideh
	.export		_TED_stridel
	.export		_TED_tmp1
	.export		_TED_tmp2
	.export		_TED_tmp3
	.export		_TED_tmp4

ZP1		= $D8
ZP2		= $D9
ZP3		= $DA
ZP4		= $DB

.segment	"CODE"

_TED_addrh:
	.res	1
_TED_addrl:
	.res	1
_TED_desth:
	.res	1
_TED_destl:
	.res	1
_TED_strideh:
	.res	1
_TED_stridel:
	.res	1
_TED_tmp1:
	.res	1
_TED_tmp2:
	.res	1
_TED_tmp3:
	.res	1
_TED_tmp4:
	.res	1

; Core routines

; ------------------------------------------------------------------------------------------
_TED_HChar_core:
; Function to draw horizontal line with given character (draws from left to right)
; Input:	TED_addrh = high byte of start address in color memory
;			TED_addrl = low byte of start address in color memory
;			TED_tmp1 = character value
;			TED_tmp2 = length value
;			TED_tmp3 = attribute value
; ------------------------------------------------------------------------------------------

	; Hi-byte of the destination address to ZP1
	lda _TED_addrl	        			; Load high byte of start in A
	sta ZP1								; Write to ZP1

	; Lo-byte of the destination address to ZP2
	lda _TED_addrh		        		; Load high byte of start in A
	sta ZP2								; Write to ZP2

	; Initialise color copy
	ldy _TED_tmp2						; Set Y counter at number of chars
	dey									; Decrease counter
	lda _TED_tmp3						; Set color

	; Copy loop color
loophchar1:
	sta (ZP1),Y							; Store character at indexed address
	dey									; Decrease counter
	cpy #$ff							; Check for last char
	bne loophchar1						; Loop until last char
	clc									; Clear carry
	lda ZP2								; Load high byte of destination address
	adc #$04							; Add 4 pages to reach text memory
	sta ZP2								; Store back to ZP2

	; Initialise text copy
	ldy _TED_tmp2						; Set Y counter at number of chars
	dey									; Decrease counter
	lda _TED_tmp1						; Set screencode

	; Copy loop text
loophchar2:
	sta (ZP1),Y							; Store character at indexed address
	dey									; Decrease counter
	cpy #$ff							; Check for last char
	bne loophchar2						; Loop until counter is zero
    rts

; ------------------------------------------------------------------------------------------
_TED_VChar_core:
; Function to draw vertical line with given character (draws from top to bottom)
; Input:	TED_addrh = high byte of start address
;			TED_addrl = low byte of start address
;			TED_tmp1 = character value
;			TED_tmp2 = length value
;			TED_tmp3 = attribute value
; ------------------------------------------------------------------------------------------

loopvchar:
	; Hi-byte of the destination address to ZP1
	lda _TED_addrl	        			; Load high byte of start in A
	sta ZP1								; Write to ZP1

	; Lo-byte of the destination address to ZP2
	lda _TED_addrh		        		; Load high byte of start in A
	sta ZP2								; Write to ZP2

	; Copy color
	ldy #$00							; Set Y index at 0
	lda _TED_tmp3						; Set color
	sta (ZP1),Y							; Store character at indexed address

	; Copy charachter
	clc 								; Clear carry
	lda ZP2								; Load high byte of destination address
	adc #$04							; Add 4 pages to reach text memory
	sta ZP2								; Store back to ZP2
	lda _TED_tmp1						; Set screencode
	sta (ZP1),y							; Store character at indexed address

	; Increase start address with 40 for next line
	clc 								; Clear carry
	lda _TED_addrl	        			; Load low byte of address to A
	adc #$28    						; Add 40 with carry
	sta _TED_addrl			        	; Store result back
	lda _TED_addrh	        			; Load high byte of address to A
	adc #$00    						; Add 0 with carry
	sta _TED_addrh	        			; Store result back

	; Loop until length reaches zero
	dec _TED_tmp2		        		; Decrease length counter
	bne loopvchar		        		; Loop if not zero
    rts

; ------------------------------------------------------------------------------------------
_TED_FillArea_core:
; Function to draw area with given character (draws from topleft to bottomright)
; Input:	TED_addrh = high byte of start address
;			TED_addrl = low byte of start address
;			TED_tmp1 = character value
;			TED_tmp2 = length value
;			TED_tmp3 = attribute value
;			TED_tmp4 = number of lines
; ------------------------------------------------------------------------------------------

loopdrawline:
	jsr _TED_HChar_core					; Draw line

	; Increase start address with 40 for next line
	clc 								; Clear carry
	lda _TED_addrl	        			; Load low byte of address to A
	adc #$28    						; Add 40 with carry
	sta _TED_addrl			        	; Store result back
	lda _TED_addrh	        			; Load high byte of address to A
	adc #$00    						; Add 0 with carry
	sta _TED_addrh	        			; Store result back

	; Decrease line counter and loop until zero
	dec _TED_tmp4						; Decrease line counter
	bne loopdrawline					; Continue until counter is zero
	rts

; ------------------------------------------------------------------------------------------
_TED_CopyViewPortToTED_core:
; Function to copy memory from TED memory to standard memory
; Input:	TED_addrh = high byte of source address
;			TED_addrl = low byte of source address
;			TED_desth = high byte of TED destination address
;			TED_destl = low byte of TED destination address
;			TED_strideh = high byte of characters per line in source
;			TED_stridel = low byte of characters per line in source
;			TED_tmp1 = number lines to copy
;			TED_tmp2 = length per line to copy
; ------------------------------------------------------------------------------------------

	; Set source address pointer in zero-page
	lda _TED_addrl						; Obtain low byte in A
	sta ZP1								; Store low byte in pointer
	lda _TED_addrh						; Obtain high byte in A
	sta ZP2								; Store high byte in pointer

	; Set destination address pointer in zero-page
	lda _TED_destl						; Obtain low byte in A
	sta ZP3								; Store low byte in pointer
	lda _TED_desth						; Obtain high byte in A
	sta ZP4								; Store high byte in pointer

	; Start of copy loop
outerloopvp:							; Start of outer loop
	ldy _TED_tmp2						; Load length of line
	dey									; Decrease counter

	; Read value and store at TED address
copyloopvp:								; Start of copy loop
	lda (ZP1),Y							; Load source data
	sta (ZP3),Y    						; Save at destination

	; Decrese line counter
	dey									; Decrease line counter
	cpy #$ff							; Check for last character
	bne copyloopvp						; Continue loop if not yet last char

	; Add stride to addresses for next line
	clc									; Clear carry
	lda	ZP1								; Load low byte of source address
	adc _TED_stridel					; Add low byte of stride
	sta ZP1								; Store low byte of source
	lda ZP2								; Load high byte of source address
	adc _TED_strideh					; Add high byte of stride
	sta ZP2								; Store high byte of source address
	clc									; Clear carry
	lda ZP3								; Load low byte of TED destination
	adc #$28							; Add 40 characters for next line
	sta ZP3								; Store low byte of TED destination
	lda ZP4								; Load high byte of TED destination
	adc #$00							; Add 0 with carry
	sta ZP4								; Store high byte of TED destination
	dec _TED_tmp1						; Decrease counter number of lines
	bne outerloopvp						; Continue outer loop if not yet below zero
    rts

; ------------------------------------------------------------------------------------------
Increase_one_line:
; Increase source pointers by one line
; Input and output: original resp. new address pointers in ZP1/ZP2 and ZP3/ZP4
; ------------------------------------------------------------------------------------------

	; Increase source addresses by one lines
	clc									; Clear carry
	lda ZP1								; Load color low byte
	adc #$28							; Add 40 with carry for one line
	sta ZP1								; Store back
	lda ZP2								; Load color high byte
	adc #$00							; Add zero with carry
	sta ZP2								; Store back
	clc									; Clear carry
	lda ZP3								; Load screen low byte
	adc #$28							; Add 40 with carry for one line
	sta ZP3								; Store back
	lda ZP4								; Load screen high byte
	adc #$00							; Add zero with carry
	sta ZP4								; Store back
	rts

; ------------------------------------------------------------------------------------------
Decrease_one_line:
; Decrease source pointers by one line
; Input and output: original resp. new address pointers in ZP1/ZP2 and ZP3/ZP4
; ------------------------------------------------------------------------------------------

	; Decrease source addresses by one lines
	sec									; Set carry
	lda ZP1								; Load color low byte
	sbc #$28							; Subtract 40 with carry for one line
	sta ZP1								; Store back
	lda ZP2								; Load color high byte
	sbc #$00							; Subtract zero with carry
	sta ZP2								; Store back
	sec									; Clear carry
	lda ZP3								; Load screen low byte
	sbc #$28							; Subtract 40 with carry for one line
	sta ZP3								; Store back
	lda ZP4								; Load screen high byte
	sbc #$00							; Subtract zero with carry
	sta ZP4								; Store back
	rts

; ------------------------------------------------------------------------------------------
Store_address_pointers:
; Store the address pointers of the X,Y start location for color and text
; Input:	TED_addrh = high byte of source address
;			TED_addrl = low byte of source address
; Output:	Address pointer in ZP1/ZP2 for color and ZP3/ZP4 for text
; ------------------------------------------------------------------------------------------

	lda _TED_addrl						; Load low byte of TED color memory address
	sta ZP1								; Store in ZP1
	sta ZP3								; Store in ZP3
	lda _TED_addrh						; Load high byte of TED color memory address
	sta ZP2								; Store in ZP2
	clc									; Clear carry
	adc #$04							; Add 4 to high byte to get to text memory address
	sta ZP4								; Store in ZP4
	rts

; ------------------------------------------------------------------------------------------
_TED_Scroll_right_core:
; Function to scroll TED text screen 1 charachter to the right, no fill
; Input:	TED_addrh = high byte of source address
;			TED_addrl = low byte of source address
;			TED_tmp1 = number of lines to copy
;			TED_tmp2 = length per line to copy			
; ------------------------------------------------------------------------------------------

	; Set source address pointers
	jsr	Store_address_pointers			; Set source address pointers

	; Move one line right
loop_sr_outer:
	ldy _TED_tmp2						; Set Y index for width
	dey									; Decrease by 1 for zero base X coord
	dey

loop_sr_inner:
	lda (ZP1),y							; Load color byte
	iny									; Increase index
	sta (ZP1),Y							; Save color byte at adress plus 1
	dey									; Decrease index agauin to get text source
	lda (ZP3),Y							; Load screen byte
	iny									; Increase index
	sta (ZP3),Y							; Save screen byte at adress minus 1
	dey									; Decrease index again
	dey									; Decrease index again
	cpy #$ff							; Check for last char
	bne loop_sr_inner					; Loop until index is at 0

	; Increase source addresses by one line
	jsr Increase_one_line				; Get next line

	; Decrease line counter
	dec _TED_tmp1						; Decrease line counter
	bne loop_sr_outer					; Loop until counter is zero
	rts

; ------------------------------------------------------------------------------------------
_TED_Scroll_left_core:
; Function to scroll TED text screen 1 charachter to the left, no fill
; Input:	TED_addrh = high byte of source address
;			TED_addrl = low byte of source address
;			TED_tmp1 = number of lines to copy
;			TED_tmp2 = length per line to copy	
; ------------------------------------------------------------------------------------------

	; Set source address pointers
	jsr	Store_address_pointers			; Set source address pointers

	; Move one line left
loop_sl_outer:
	ldy #$01							; Start with index character 1
loop_sl_inner:
	lda (ZP1),y							; Load color byte
	dey									; Decrease index
	sta (ZP1),Y							; Save color byte at adress minus 1
	iny									; Increase index agauin to get text source
	lda (ZP3),Y							; Load screen byte
	dey									; Decrease index
	sta (ZP3),Y							; Save screen byte at adress minus 1
	iny									; Increase again
	iny									; Increase again
	cpy _TED_tmp2						; Compare with width to check for last char in line
	bne loop_sl_inner					; Loop until index is at last char

	; Increase source addresses by one line
	jsr Increase_one_line				; Get next line

	; Decrease line counter
	dec _TED_tmp1						; Decrease line counter
	bne loop_sl_outer					; Loop until counter is zero
	rts

; ------------------------------------------------------------------------------------------
Copy_line:
; Function to copy one line from source to destination for scroll up and down
; Input:	ZP1/ZP2 for source pointer, ZP3/ZP4 for destination pointer
;			TED_tmp2 = length per line to copy
; ------------------------------------------------------------------------------------------

	ldy _TED_tmp2						; Set Y index for width
	dey									; Decrease by 1 for zero base X coord

loop_cl_inner:
	lda (ZP1),Y							; Load byte from source
	sta (ZP3),Y							; Save byte at destination
	dey									; Decrease index again
	cpy #$ff							; Check for last char
	bne loop_cl_inner					; Loop until index is at 0
	rts

; ------------------------------------------------------------------------------------------
_TED_Scroll_down_core:
; Function to scroll TED text screen 1 charachter down, no fill
; Input:	TED_addrh = high byte of source address
;			TED_addrl = low byte of source address
;			TED_tmp1 = number of lines to copy
;			TED_tmp2 = length per line to copy
; ------------------------------------------------------------------------------------------

	; Set number of lines counter for color
	lda _TED_tmp1						; Load number of lines
	sta _TED_tmp4						; Save as counter
	
	; Set source address pointers color memory
	clc									; Clear carry
	lda _TED_addrl						; Load low byte of TED color memory address
	sta ZP1								; Store in ZP1 for source pointer
	lda _TED_addrh						; Load high byte of TED color memory address
	sta ZP2								; Store in ZP2
	lda ZP1								; Load low byte of TED color memory address
	adc #$28							; Subtract 40 for next line
	sta ZP3								; Store in ZP3 for destination pointer
	lda ZP2								; Load high byte of TED color memory address
	adc #$00							; Subtract carry
	sta ZP4								; Store in ZP4

	; Move one line down
loop_sd_color:
	jsr Copy_line						; Copy line from source to destination

	; Increase source addresses by one lines
	jsr Decrease_one_line				; Get next line

	; Decrease line counter
	dec _TED_tmp4						; Decrease line counter
	bne loop_sd_color					; Loop until counter is zero

	; Reset number of lines counter for text
	lda _TED_tmp1						; Load number of lines
	sta _TED_tmp4						; Save as counter

	; Set source address pointers text memory
	clc									; Clear carry
	lda _TED_addrl						; Load low byte of TED color memory address
	sta ZP1								; Store in ZP1 for source pointer
	lda _TED_addrh						; Load high byte of TED color memory address
	adc #$04							; Add 4 pages
	sta ZP2								; Store in ZP2
	clc									; Clear carry
	lda ZP1								; Load low byte of TED color memory address
	adc #$28							; Subtract 40 for next line
	sta ZP3								; Store in ZP3 for destination pointer
	lda ZP2								; Load high byte of TED color memory address
	adc #$00							; Subtract carry
	sta ZP4								; Store in ZP4

	; Move one line down
loop_sd_text:
	jsr Copy_line						; Copy line from source to destination

	; Increase source addresses by one lines
	jsr Decrease_one_line				; Get next line

	; Decrease line counter
	dec _TED_tmp4						; Decrease line counter
	bne loop_sd_text					; Loop until counter is zero
	rts

; ------------------------------------------------------------------------------------------
_TED_Scroll_up_core:
; Function to scroll TED text screen 1 charachter up, no fill
; Input:	TED_addrh = high byte of source address
;			TED_addrl = low byte of source address
;			TED_tmp1 = number of lines to copy
;			TED_tmp2 = length per line to copy
; ------------------------------------------------------------------------------------------

	; Set number of lines counter for color
	lda _TED_tmp1						; Load number of lines
	sta _TED_tmp4						; Save as counter
	
	; Set source address pointers color memory
	sec									; Clear carry
	lda _TED_addrl						; Load low byte of TED color memory address
	sta ZP1								; Store in ZP1 for source pointer
	lda _TED_addrh						; Load high byte of TED color memory address
	sta ZP2								; Store in ZP2
	lda ZP1								; Load low byte of TED color memory address
	sbc #$28							; Subtract 40 for next line
	sta ZP3								; Store in ZP3 for destination pointer
	lda ZP2								; Load high byte of TED color memory address
	sbc #$00							; Subtract carry
	sta ZP4								; Store in ZP4

	; Move one line up
loop_su_color:
	jsr Copy_line						; Copy line from source to destination

	; Increase source addresses by one lines
	jsr Increase_one_line				; Get next line

	; Decrease line counter
	dec _TED_tmp4						; Decrease line counter
	bne loop_su_color					; Loop until counter is zero

	; Reset number of lines counter for text
	lda _TED_tmp1						; Load number of lines
	sta _TED_tmp4						; Save as counter

	; Set source address pointers text memory
	clc									; Clear carry
	lda _TED_addrl						; Load low byte of TED color memory address
	sta ZP1								; Store in ZP1 for source pointer
	lda _TED_addrh						; Load high byte of TED color memory address
	adc #$04							; Add 4 pages
	sta ZP2								; Store in ZP2
	sec									; Clear carry
	lda ZP1								; Load low byte of TED color memory address
	sbc #$28							; Subtract 40 for next line
	sta ZP3								; Store in ZP3 for destination pointer
	lda ZP2								; Load high byte of TED color memory address
	sbc #$00							; Subtract carry
	sta ZP4								; Store in ZP4

	; Move one line down
loop_su_text:
	jsr Copy_line						; Copy line from source to destination

	; Increase source addresses by one lines
	jsr Increase_one_line				; Get next line

	; Decrease line counter
	dec _TED_tmp4						; Decrease line counter
	bne loop_su_text					; Loop until counter is zero
	rts

; ------------------------------------------------------------------------------------------
_TED_ROM_Peek_core:
; Function to PEEK from ROM memory
; Input:	TED_addrh = high byte of address
;			TED_addrl = low byte of address
; Output:	TED_tmp1 = read value
; ------------------------------------------------------------------------------------------

	lda #$00							; Clear A
	ldy #$00							; Clear Y
	sta $FF3E							; Store to $FF3E to set reading from ROM area
	lda _TED_addrl						; Load high byte of address
	sta ZP1								; Store in ZP pointer
	lda _TED_addrh						; Load high byte of address
	sta ZP2								; Store in ZP pointer
	lda (ZP1),y							; Load value
	sta _TED_tmp1						; Store to output value
	sta $FF3F							; Set system to read from RAM again
	rts

; ------------------------------------------------------------------------------------------
_TED_ROM_Memcopy_core:
; Function to copy from ROM memory to RAM memory.
; Note: Simplified to only copys multitudes of 256 bytes (whole pages)
; Function to copy memory from TED memory to standard memory
; Input:	TED_addrh = high byte of ROM source address
;			TED_addrl = low byte of ROM source address
;			TED_desth = high byte of RAM destination address
;			TED_destl = low byte of RAM destination address
;			TED_tmp1 = number of pages to copy
; ------------------------------------------------------------------------------------------

	lda _TED_addrl						; Load low byte of ROM source address
	sta ZP1								; Store in zeropage pointer
	lda _TED_addrh						; Load high byte of ROM source address
	sta ZP2								; Store in zeropage pointer
	lda _TED_destl						; Load low byte of RAM destination address
	sta ZP3								; Store in zeropage pointer
	lda _TED_desth						; Load high byte of RAM destination address
	sta ZP4								; Store in zeropage pointer
tedrommemcopy_pagesloop:
	ldy #$ff							; Set Y counter to 256 bytes
tedrommemcopy_innerloop:
	sta $FF3E							; Store to $FF3E to set reading from ROM area
	lda (ZP1),Y							; Load from ROM
	sta (ZP3),y							; Store to RAM
	dey									; Decrease Y counter
	cpy #$ff							; Compare to $ff to see if counter went past 0
	bne tedrommemcopy_innerloop			; Loop until past zero
	inc ZP2								; Increase high byte of source
	inc ZP4								; Increase high byte of destination
	dec _TED_tmp1						; Decrease pages counter
	bne tedrommemcopy_pagesloop			; Loop until page counter is zero
	sta $FF3F							; Set system to read from RAM again
	rts




