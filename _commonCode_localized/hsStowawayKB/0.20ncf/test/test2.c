/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




//Stolen from cTools/hexifyFile.c

#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
#include <string.h>
#include <sys/errno.h>
#include "../hsStowawayKB.h"
#include "../../../__std_wrappers/stdin_nonBlock/0.10/stdin_nonBlock.h"

extern uint8_t keyMapUnshifted[];

int main(int argc, char *argv[])
{
	printf("Call with, e.g. 'thisProgram /dev/cu.usbserial'\n");
	printf("   (sizeof(keymapUnshifted)=%d)\n", sizeof(keyMapUnshifted));

	stdinNB_init();

	char * fileString = argv[1];
	printf("File: '%s'\n",fileString);
	
	FILE *fid;
	fid = fopen(fileString, "rb");
	if ( fid == NULL )
	{
	   printf("Unable to open file\n");
	   return 1;
	}

	uint8_t minChar=0xff;
	uint8_t maxChar=0x00;

	int i;

	for(i=0; i<0x200; i++)
	{
		//Skip Shifts and CapsLock
		if(((i&0xff)==0x018) || ((i&0xff)==0x058) || ((i&0xff)==0x059))
			continue;

		//Send Shift...
		if(i==0x100)
			hsSKB_toChar(0x58);

		char dataChar = hsSKB_toChar(i&0xff);
		if(dataChar && (dataChar < minChar))
			minChar = dataChar;
		if(dataChar > maxChar)
			maxChar = dataChar;
	}

	//Clear Shift
	hsSKB_toChar(0xd8);

	printf("min/max outputs from hsSKB_toChar():\n");
	printf("minChar=0x%"PRIx8" maxChar=0x%"PRIx8"\n", minChar, maxChar);

	uint8_t minData=0xff;
	uint8_t maxData=0x00;

	printf("\nNow receiving KB input, press 'q' and return on this keyboard to exit.\n"
			"Must also press any key on the Stowaway Keyboard, afterwards.\n");

	int quit = 0;

	while(!quit)
	{
		uint8_t data;

		fread(&data, sizeof(uint8_t), 1, fid);

		if(data < minData)
			minData = data;
		if(data > maxData)
			maxData = data;

		char dataChar = hsSKB_toChar(data);
		if(dataChar != 0)
			printf("KB: 0x%"PRIx8" = Key: '%c'\n", data, hsSKB_toChar(data));

		errno_handleError("Unhandled Error.", 0);

		int kbChar = stdinNB_getChar();

		if(kbChar == 'q')
			quit=1;
	}

	printf("min/max inputs from the Stowaway Keyboard:\n");
	printf("minData=0x%"PRIx8" maxData=0x%"PRIx8"\n", minData, maxData);
	return 0;
}

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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/hsStowawayKB/0.20ncf/test/test2.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
