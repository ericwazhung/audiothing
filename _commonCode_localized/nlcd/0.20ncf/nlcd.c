/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#include "nlcd.h"

//at 14x6 characters, we can fit 84 characters
// with a scrolling-display, there's no need to store the top row, it'll
// just be obliterated after it scrolls...
// but why do it line-by-line... extra stuff wouldn't be a bad thing...


//No... 'cause cirBuff is 16bit in main... hmm.... Well, let's use it for
// now, it can be modified later.
// Ahh, but cirBuff is usually used for grabbing one byte at the end...
// more for things of varying sizes, rather than a constant size.

//#define SCREEN_CHARS	84
//Allegedly, and logically, initializing this to non-zero would cause the
// code-size to change... Somewhere it needs to store the values to be
// written, no?
// Instead, ONLY the .data and .bss sections changed in sizes, 
//  appropriately. WEIRD
// Ehm, so, even initializing with different values gives the same result
// suggesting maybe there aren't separate init-functions for .bss and .data
// in which case, .bss and .data are *all* .data
// and every initialized variable's value is stored in an array in program
// memory...
// which I suppose is nice from the respect of knowing code-size won't
// increase when initializing variables,
// but also means that code-size is probably quite a bit larger than it
// needs to be, just from storing a bunch of values in a lookup table
// rather than having two separate for-loops... ponderable.
// NO: .text does NOT INCLUDE .data nor .bss
// the total program-memory size is .text + .data
// But there's still a ponderance here...
//  Shouldn't it be at least a *couple* bytes different???
// I guess it just has an array of data, a start address and an end address
// including *all* variables... which must then be consecutive
uint8_t screenTextBuffer[SCREEN_CHARS]; //={ [0 ... (SCREEN_CHARS-1)] = ' ' };

//={1,2,4,1,2,3,4,5,6,76,7,8,9,9,0,1,2,3,4,5,6,7,8,9,0,0}; 
//={1,2,4}
//Position in the screenTextBuffer where the last character was written
static uint8_t nlcd_lastPosition = 0;
static uint8_t nlcd_lastPositionWhenLastRedrawn;

//This'll return any character on the screen
static __inline__ char nlcd_getChar(uint8_t charNum)
{
	uint8_t bufferPos = charNum+nlcd_lastPosition+1;
	if(bufferPos >= SCREEN_CHARS)
		bufferPos -= SCREEN_CHARS;

	return screenTextBuffer[bufferPos];
}

static __inline__ void nlcd_setBufferPos(uint8_t pos)
{
	nlcd_lastPosition = pos;
	nlcd_lastPositionWhenLastRedrawn = 0xff;
}

//Starts writing at the specified position on the screen, where 0 is UL
// It's not particularly smart... as after text is overwritten, it will
// set the lastPos to the last character written, which might shift
// the whole screen...
// Probably best to save the charNum, then do this, then write, then reset
// (if it's < the old charNum)
// Now returns the original position... for that purpose
static __inline__ uint8_t nlcd_setCharNum(uint8_t charNum)
{
	uint8_t lastPosTemp = nlcd_lastPosition;

	nlcd_lastPosition += charNum;
	if(nlcd_lastPosition >= SCREEN_CHARS)
		nlcd_lastPosition -= SCREEN_CHARS;

	return lastPosTemp;
}



static __inline__ uint8_t nlcd_charactersChanged(void)
{
	return (nlcd_lastPositionWhenLastRedrawn != nlcd_lastPosition);
}

void nlcd_redrawCharacters(void)
{
	uint8_t thisCharacter = nlcd_lastPosition;

	nlcd_lastPositionWhenLastRedrawn = thisCharacter;
	
	uint8_t i;
	for(i = 0; i < 6; i++)
	{
		nlcd_gotoXY(0, i);
		uint8_t j;
		for(j = 0; j<14; j++)
		{
			thisCharacter++;
			if(thisCharacter >= SCREEN_CHARS)
				thisCharacter -= SCREEN_CHARS;

			nlcd_drawChar(screenTextBuffer[thisCharacter]);
		}
	}

}

void nlcd_appendCharacter(char character)
{
	nlcd_lastPosition++;
	if(nlcd_lastPosition >= SCREEN_CHARS)
		nlcd_lastPosition -= SCREEN_CHARS;

	screenTextBuffer[nlcd_lastPosition] = character;
#define NLCD_DISABLE_APPEND_REDRAW TRUE
#if (!defined(NLCD_DISABLE_APPEND_REDRAW) || !NLCD_DISABLE_APPEND_REDRAW)
	nlcd_redrawCharacters();
#endif
}

extern uint8_t eeFont[FONTBYTES] EEMEM;



/*
#define NLCD_Select()	clrpinPORT(NLCD_nCS_pin, NLCD_nCS_PORT)

#define NLCD_Deselect()	setpinPORT(NLCD_nCS_pin, NLCD_nCS_PORT)

#define NLCD_Reset()	\
({ \
 	clrpinPORT(NLCD_nRST_pin, NLCD_nRST_PORT); \
	_delay_ms(100); \
 	setpinPORT(NLCD_nRST_pin, NLCD_nRST_PORT); \
 	{}; \
})

#define NLCD_SetCommandMode() clrpinPORT(NLCD_DnC_pin, NLCD_DnC_PORT)
#define NLCD_SetDataMode()		setpinPORT(NLCD_DnC_pin, NLCD_DnC_PORT)
*/

//Stolen almost directly from 3310_routines.c
void nlcd_writeCommand(uint8_t command)
{
	NLCD_Select();

	NLCD_SetCommandMode();

//	SPDR = command;

//	while(sending);

	spi_transferByteWithTimer(command);

	NLCD_Deselect();

}

void nlcd_writeData(uint8_t data)
{
	NLCD_Select();

	NLCD_SetDataMode();

//	SPDR = command;

//	while(sending);

	spi_transferByteWithTimer(data);

	NLCD_Deselect();

}

void nlcd_gotoXY ( unsigned char x, unsigned char y )
{
   nlcd_writeCommand (0x80 | x);   //column
   nlcd_writeCommand (0x40 | y);   //row
}

/*
#define NLCD_UseExtendedCommands()	nlcd_writeCommand(0x20 | 0x01)
#define NLCD_UseBasicCommands()		nlcd_writeCommand(0x20 | 0x00)

#define NLCD_SetContrast(val)	\
({ \
	NLCD_UseExtendedCommands(); \
	nlcd_writeCommand(0x80 | ((val)&0x7f)); \
	NLCD_UseBasicCommands(); \
	{}; \
})
*/

void nlcd_clear ( void )
{  
    int i,j;
    
    nlcd_gotoXY (0,0);    //start with (0,0) position
 
    for(i=0; i<8; i++)
    {
       for(j=0; j<90; j++)
       {
//#define NLCD_CLEAR_HACK	TRUE
#if (defined(NLCD_CLEAR_HACK) && NLCD_CLEAR_HACK)
			if(j>30)
          nlcd_writeData( 0x07 );
			else
#endif
				nlcd_writeData(0x00);
       }
    }
    nlcd_gotoXY (0,0);   //bring the XY position back to (0,0)
}


void nlcd_init(void)
{
	//This part wasn't in LCD_init(?!)
	setoutPORT(NLCD_nRST_pin, NLCD_nRST_PORT);
	setoutPORT(NLCD_nCS_pin, NLCD_nCS_PORT);
	setoutPORT(NLCD_DnC_pin, NLCD_DnC_PORT);
	//Initial values shouldn't matter, I think....
	//Now onto LCD_init's stuff...

	_delay_ms(100);

	NLCD_Select();

	NLCD_Reset();

	NLCD_Deselect();

	//These are defaults from the other dude's init function...
	// contrast doesn't work with mine...
	nlcd_writeCommand( 0x21 );  // LCD Extended Commands.
	nlcd_writeCommand( 0xE0 );  // Set LCD Vop (Contrast).
	nlcd_writeCommand( 0x04 );  // Set Temp coefficent.
	nlcd_writeCommand( 0x13 );  // LCD bias mode 1:48.
	nlcd_writeCommand( 0x20 );  // LCD Standard Commands, 
										//   Horizontal addressing mode.
	nlcd_writeCommand( 0x0c );  // LCD in normal mode.

	NLCD_SetContrast(70);

	nlcd_clear();
	
//#define NLCD_TEST_CONTRAST	10
#if (defined(NLCD_TEST_CONTRAST) && NLCD_TEST_CONTRAST)
	uint8_t contrast;

	dms4day_t startTime = dmsGetTime();

	while(1)
	{
		for(contrast=0; contrast<=0x7f; contrast+=NLCD_TEST_CONTRAST)
		{
			setoutPORT(HEART_PINNUM, HEART_PINPORT);
			togglepinPORT(HEART_PINNUM, HEART_PINPORT);
			NLCD_SetContrast(contrast);
			
			//set_heartBlink(contrast);			

			while(!dmsIsItTime(&startTime, 10*DMS_SEC))
			{
				//heartUpdate();
			}
		}
	}
#endif
}

//In keeping with the prior memory-shortage, some characters can be added
//without increasing code-size at all...
// e.g. characters that are, in the ASCII table, immediately before/after 
// those already-available merely require changing the ranges in tests in 
// the lookup-function.
// Examples: 65 = 'A'
//           64 = '@'
//           63 = '?'
//           62 = '>'
//           61 = '=' //This is the one I need...
//           60 = '<'
//           59 = ';'
//           58 = ':' //Also this.
// That's 8*5 bytes, which is only 40, we had 190 and have room for 256
// So just extend the test-range for 'A'-'Z' to ':' - 'Z' and place these
// characters' font-data immediately-before that for 'A'
// And, in fact, ':' is immediately-after '9', so we'll actually *save*
// code-space, as separate-testing for 'A'-'Z' and '0'-'9' is unnecessary
#define ADD_9toA_PUNCTUATION	TRUE


//We still have 6 characters' worth of FONT-space left...
//There are probably more valuable characters than these, but some I want,
// and we can keep the code-space down again:
// (48 = '0')
//  47 = '/'
//  46 = '.' //Want
//  45 = '-' //Want
//  44 = ','
//  43 = '+' //Want
//  42 = '*'
#define ADD_PRIOR_PUNCTUATION TRUE

#if(   (defined(ADD_PRIOR_PUNCTUATION) && ADD_PRIOR_PUNCTUATION) \
    && (!defined(ADD_9toA_PUNCTUATION) || !ADD_9toA_PUNCTUATION) )
#warning "ADD_9toA_PUNCTUATION is not TRUE, but codewise needs-to-be, so it's implied"
#define ADD_9toA_PUNCTUATION	TRUE
#endif



//Each character in this font is 5 bytes (five columns, one byte per col)
#define FONT_BYTES_PER_CHAR 5
#if (defined(ADD_PRIOR_PUNCTUATION) && ADD_PRIOR_PUNCTUATION)
 #define ASTRISK_POS 0
 #define ZERO_POS      (ASTRISK_POS+(('/'-'*')+1))
 #define COLON_POS     (ZERO_POS  + (('9'-'0')+1))
 #define A_POS         (COLON_POS + (('@'-':')+1))
 #define SPACE_POS     (A_POS     + (('Z'-'A')+1))
 #define QUESTION_POS  (SPACE_POS + 1)
#elif (defined(ADD_9toA_PUNCTUATION) && (ADD_9toA_PUNCTUATION))
 #define ZERO_POS	0
 #define COLON_POS     (ZERO_POS  + (('9'-'0')+1))
 #define A_POS         (COLON_POS + (('@'-':')+1))
 #define SPACE_POS     (A_POS     + (('Z'-'A')+1))
 #define QUESTION_POS  (SPACE_POS + 1)
#else
 //These are the old positions in the array, prior to ADD_..._PUNCTUATION
 #define ZERO_POS	0
 #define A_POS         (ZERO_POS  + (('9'-'0')+1))
 #define SPACE_POS     (A_POS     + (('Z'-'A')+1))
 #define QUESTION_POS  (SPACE_POS + 1)
#endif
 

//Change '\n' and '\r' to spaces (instead of '?')
#define NEWLINE_TO_SPACE	TRUE

// Internal...
uint8_t nlcd_getCharacterCol(char character, uint8_t colNum)
{
   uint8_t charPos = 0;
#if (defined(ADD_PRIOR_PUNCTUATION) && ADD_PRIOR_PUNCTUATION)
	if((character>='*') && (character<='Z'))
		charPos = (character-'*') + ASTRISK_POS;
#elif (defined(ADD_9toA_PUNCTUATION) && (ADD_9toA_PUNCTUATION))
	if((character>='0') && (character<='Z'))
		charPos = (character-'0') + ZERO_POS;
#else
   if((character>='0') && (character<='9'))
      charPos = (character-'0') + ZERO_POS;
   else if((character>='A') && (character<='Z'))
      charPos = (character-'A') + A_POS;
#endif
   else if((character>='a') && (character<='z'))
      charPos = (character-'a') + A_POS;
   else if(   (character == ' ') 
			  || (character == 0)   //For some reason '\0' was intentional
#if (defined(NEWLINE_TO_SPACE) && NEWLINE_TO_SPACE)
			  || (character == '\n')
           || (character == '\r')
#endif
			 )
      charPos = SPACE_POS;
   else
      charPos = QUESTION_POS;
      
      
      
   uint8_t eepAddr = charPos * FONT_BYTES_PER_CHAR;
   
	eepAddr += colNum;
   
   return eeprom_read_byte((uint8_t*)((uint16_t)eepAddr));
   
}  

void nlcd_drawChar(char character)
{
   uint8_t colNum;
   for(colNum=0; colNum<5; colNum++)
      nlcd_writeData(nlcd_getCharacterCol(character, colNum));
   nlcd_writeData(0x00);
}  





// EEMEM:
// section     size      addr
// .eeprom        5   8454144 = 0x810000
//
// .eeprom.font same...
//
//This almost worked, see avrCommon.mk's linking stuff
//uint8_t eeTest[5] __attribute__((section(".eepFont"))) = //EEMEM =

//This font stolen and minimized from 3310_routines.c
uint8_t eeFont[FONTBYTES] EEMEM =
{
#if (defined(ADD_PRIOR_PUNCTUATION) && ADD_PRIOR_PUNCTUATION)
 [ASTRISK_POS*FONT_BYTES_PER_CHAR] =
     0x14, 0x08, 0x3E, 0x08, 0x14,   // '*'=ASCII 42
     0x08, 0x08, 0x3E, 0x08, 0x08,   // +
     0x00, 0x00, 0x50, 0x30, 0x00,   // ,
     0x10, 0x10, 0x10, 0x10, 0x10,   // -
     0x00, 0x60, 0x60, 0x00, 0x00,   // .
     0x20, 0x10, 0x08, 0x04, 0x02,   // '/'=ASCII 47
#endif
 [ZERO_POS*FONT_BYTES_PER_CHAR] = 
   0x3E, 0x51, 0x49, 0x45, 0x3E,   // '0'=ASCII 48
   0x00, 0x42, 0x7F, 0x40, 0x00,   // 1
   0x42, 0x61, 0x51, 0x49, 0x46,   // 2
   0x21, 0x41, 0x45, 0x4B, 0x31,   // 3
   0x18, 0x14, 0x12, 0x7F, 0x10,   // 4
   0x27, 0x45, 0x45, 0x45, 0x39,   // 5
   0x3C, 0x4A, 0x49, 0x49, 0x30,   // 6
   0x01, 0x71, 0x09, 0x05, 0x03,   // 7
   0x36, 0x49, 0x49, 0x49, 0x36,   // 8
   0x06, 0x49, 0x49, 0x29, 0x1E,   // '9'=ASCII 57

#if (defined(ADD_9toA_PUNCTUATION) && (ADD_9toA_PUNCTUATION))
 [COLON_POS*FONT_BYTES_PER_CHAR] =
     0x00, 0x36, 0x36, 0x00, 0x00,   // ':'=ASCII 58
     0x00, 0x56, 0x36, 0x00, 0x00,   // ;
     0x08, 0x14, 0x22, 0x41, 0x00,   // <
     0x14, 0x14, 0x14, 0x14, 0x14,   // =
     0x00, 0x41, 0x22, 0x14, 0x08,   // >
     0x02, 0x01, 0x51, 0x09, 0x06,   // ?
     0x32, 0x49, 0x59, 0x51, 0x3E,   // '@'=ASCII 64
#endif
 [A_POS*FONT_BYTES_PER_CHAR] = 
	0x7E, 0x11, 0x11, 0x11, 0x7E,   // 'A'=ASCII 65
   0x7F, 0x49, 0x49, 0x49, 0x36,   // B
   0x3E, 0x41, 0x41, 0x41, 0x22,   // C
   0x7F, 0x41, 0x41, 0x22, 0x1C,   // D
   0x7F, 0x49, 0x49, 0x49, 0x41,   // E
   0x7F, 0x09, 0x09, 0x09, 0x01,   // F
   0x3E, 0x41, 0x49, 0x49, 0x7A,   // G
   0x7F, 0x08, 0x08, 0x08, 0x7F,   // H
   0x00, 0x41, 0x7F, 0x41, 0x00,   // I
   0x20, 0x40, 0x41, 0x3F, 0x01,   // J
   0x7F, 0x08, 0x14, 0x22, 0x41,   // K
   0x7F, 0x40, 0x40, 0x40, 0x40,   // L
   0x7F, 0x02, 0x0C, 0x02, 0x7F,   // M
   0x7F, 0x04, 0x08, 0x10, 0x7F,   // N
   0x3E, 0x41, 0x41, 0x41, 0x3E,   // O
   0x7F, 0x09, 0x09, 0x09, 0x06,   // P
   0x3E, 0x41, 0x51, 0x21, 0x5E,   // Q
   0x7F, 0x09, 0x19, 0x29, 0x46,   // R
   0x46, 0x49, 0x49, 0x49, 0x31,   // S
   0x01, 0x01, 0x7F, 0x01, 0x01,   // T
   0x3F, 0x40, 0x40, 0x40, 0x3F,   // U
   0x1F, 0x20, 0x40, 0x20, 0x1F,   // V
   0x3F, 0x40, 0x38, 0x40, 0x3F,   // W
   0x63, 0x14, 0x08, 0x14, 0x63,   // X
   0x07, 0x08, 0x70, 0x08, 0x07,   // Y
   0x61, 0x51, 0x49, 0x45, 0x43,   // 'Z'=ASCII 90
 [SPACE_POS*FONT_BYTES_PER_CHAR] = 
   0x00, 0x00, 0x00, 0x00, 0x00,   // ' '=ASCII ???
 [QUESTION_POS*FONT_BYTES_PER_CHAR] = 
   0x02, 0x01, 0x51, 0x09, 0x06   // '?'=ASCII 63
}; 

/* mehPL:
 *    I would love to believe in a world where licensing shouldn't be
 *    necessary; where people would respect others' work and wishes, 
 *    and give credit where it's due. 
 *    A world where those who find people's work useful would at least 
 *    send positive vibes--if not an email.
 *    A world where we wouldn't have to think about the potential
 *    legal-loopholes that others may take advantage of.
 *
 *    Until that world exists:
 *
 *    This software and associated hardware design is free to use,
 *    modify, and even redistribute, etc. with only a few exceptions
 *    I've thought-up as-yet (this list may be appended-to, hopefully it
 *    doesn't have to be):
 * 
 *    1) Please do not change/remove this licensing info.
 *    2) Please do not change/remove others' credit/licensing/copyright 
 *         info, where noted. 
 *    3) If you find yourself profiting from my work, please send me a
 *         beer, a trinket, or cash is always handy as well.
 *         (Please be considerate. E.G. if you've reposted my work on a
 *          revenue-making (ad-based) website, please think of the
 *          years and years of hard work that went into this!)
 *    4) If you *intend* to profit from my work, you must get my
 *         permission, first. 
 *    5) No permission is given for my work to be used in Military, NSA,
 *         or other creepy-ass purposes. No exceptions. And if there's 
 *         any question in your mind as to whether your project qualifies
 *         under this category, you must get my explicit permission.
 *
 *    The open-sourced project this originated from is ~98% the work of
 *    the original author, except where otherwise noted.
 *    That includes the "commonCode" and makefiles.
 *    Thanks, of course, should be given to those who worked on the tools
 *    I've used: avr-dude, avr-gcc, gnu-make, vim, usb-tiny, and 
 *    I'm certain many others. 
 *    And, as well, to the countless coders who've taken time to post
 *    solutions to issues I couldn't solve, all over the internets.
 *
 *
 *    I'd love to hear of how this is being used, suggestions for
 *    improvements, etc!
 *         
 *    The creator of the original code and original hardware can be
 *    contacted at:
 *
 *        EricWazHung At Gmail Dotcom
 *
 *    This code's origin (and latest versions) can be found at:
 *
 *        https://code.google.com/u/ericwazhung/
 *
 *    The site associated with the original open-sourced project is at:
 *
 *        https://sites.google.com/site/geekattempts/
 *
 *    If any of that ever changes, I will be sure to note it here, 
 *    and add a link at the pages above.
 *
 * This license added to the original file located at:
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/nlcd/0.20ncf/nlcd.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
