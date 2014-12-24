/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




#ifndef __PRINTER_H__
#define __PRINTER_H__



#ifndef PRINTER
   #error "No printer selected"
#endif


//As-is all these functions are assumed to be blocking...

#if (PRINTER == PRINT_NULL)
   #define printStringBuff()  
   #define print_P(pstr)      
   #define printBuff(buffer)  
   #define printByte(byte)    

#elif (PRINTER == PRINT_PUAT)
   #include _POLLED_UAT_HEADER_
 #if(defined(PUAT_STRINGERS) && PUAT_STRINGERS)
   #define NEEDS_STRINGERS TRUE
   #define printByte(byte)    puat_sendByteBlocking(0, byte)
 #else
   #define printStringBuff()  puat_sendStringBlocking(0, stringBuffer)
   #define print_P(pstr)   puat_sendStringBlocking_P(0, stringBuffer, (pstr))
   #define printBuff(buffer)  puat_sendStringBlocking(0, buffer)
   //But wait, this isn't blocking, and there's no puat_update() elsewhere
   // for quite some time...
   #define printByte(byte)    puat_sendByteBlocking(0, byte)
 #endif
#elif (PRINTER == PRINT_NLCD)
   //Hard to #include nlcd.c via this means... so it's in main.
   #define NEEDS_STRINGERS TRUE
// #define printStringBuff()  ()
// #define print_P(pstr)      ()
// #define printBuff(buffer)  ()
/* #define printByte(byte)    \
   ({ \
      setpinPORT(SD_CS_pin, SD_CS_PORT); \
      nlcd_appendCharacter(byte); \
      clrpinPORT(SD_CS_pin, SD_CS_PORT); \
      {}; \
   })
*/
   #define printByte(byte)    nlcd_appendCharacter(byte)

#else
   #error "Invalid printer selected"
#endif


#if(defined(NEEDS_STRINGERS) && NEEDS_STRINGERS)
 void processString(char string[]);
 void processString_P(PGM_P P_string);
   #define printStringBuff()  processString(stringBuffer)
   #define printBuff(buffer)  processString(buffer)
//SHEESH. This caused all sorts of drama by having 
// processString_P(buffer) instead of (pstr)
   #define print_P(pstr)      processString_P(pstr)
#endif


#endif

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
 * /home/meh/_avrProjects/audioThing/57-heart2/printer.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
