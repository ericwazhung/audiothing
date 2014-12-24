/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//cirBuff 1.00-6

//1.00-6 adding CIRBUFF_EMPTY_UNUSED
//       and CIRBUFF_AVAILABLESPACE_UNUSED
//1.00-5 adding to COM_HEADERS when INLINED
//1.00-4 removing % in favor of if...
//       Increased audioThing9's loopCount/sec from 15750 ->120650
//1.00-3 adding INLINEABLE
//1.00-2 verifying that length isn't causing issues with NO_CALLOC
//       and supplied buffer size
//       Seems OK, but:
//       TODO: REVERIFY THIS AND CALLOC TOO
//         why does writePos = 10 in the test-app?!
//1.00-1 adding cirBuff_availableSpace()
//       adding test
//1.00 making the types changeable... uint8 still default
//     for audioThing1
//     TODO: it hasn't all been tested...

//0.99-1 replacing __CIRBUF_H__ with __CIRBUFF_H__
//       adding cirBuff_empty()
//0.99 attempting to remove the necessity for calloc...
//     (threePinIDer35i, uart_in1.10)
//     when CIRBUFF_NO_CALLOC = TRUE
//0.97-1 added to uart_in... some warnings were converted to errors
//       control reaches end of non-void function
//0.97 First version, stolen from midi_out 2.00-7
//      NOT used as a "common library" yet:
//      Developed for T200CS TabletRepeater (currently the only implemetation of cirBuf "libraries")
//      midi_out 2.00-7:
//         2.00-3ish adding buffer-test for sending only if it won't halt
//         1.00-1 buffer length experiments, also maybe buffer-full sei...
//          .94   midi_out Buffered (allows background sending)


// TODO: Add usage...
//       * Does the buffer still need more bytes than can be written?
//         is that now accounted-for?
//       * What about a constantly-full buffer, with read-back of all bytes
//       * What about multiple cirbuffs of different data-types...?
//         (use 1-byte values, and return voids?)


#ifndef __CIRBUFF_H__
#define __CIRBUFF_H__

#if (!defined(CIRBUFF_NO_CALLOC) || !CIRBUFF_NO_CALLOC)
 #include <stdlib.h>
#endif
#include <inttypes.h>



//This can be overridden with e.g. -D'cirBuff_position_t=uint16_t'
#ifndef cirBuff_position_t
#define cirBuff_position_t uint8_t
#endif

#ifndef cirBuff_data_t
   #define cirBuff_data_t     uint8_t
   #define cirBuff_dataRet_t  uint16_t
   #define CIRBUFF_RETURN_NODATA (UINT8_MAX + 1)
#else
   #ifndef cirBuff_dataRet_t
      #error "when defining cirBuff_data_t, make sure to also define cirBuff_dataRet_t"
      #error " If data is float, so could dataRet, if NaN is unused..."
   #endif
   #ifndef CIRBUFF_RETURN_NODATA
      #error "when defining cirBuff_data_t, make sure to also define CIRBUFF_RETURN_NODATA"
      #error "for integer data, this'll be ((cirBuff_data_t)_MAX+1)"
      #error " though I imagine it could be NaN if data is float..."
   #endif
#endif




typedef struct {
   volatile cirBuff_position_t writePosition;  
                        //Where to write, when adding a byte
   volatile cirBuff_position_t readPosition;   
                        //Where to read, when getting a byte
   cirBuff_position_t length;
   cirBuff_data_t *buffer;  //[];
} cirBuff_t;

#if (!defined(CIRBUFF_NO_CALLOC) || !CIRBUFF_NO_CALLOC)
 uint8_t cirBuff_init(cirBuff_t *cirBuff, cirBuff_position_t length);
 uint8_t cirBuff_destroy(cirBuff_t *cirBuff);
#else
 uint8_t cirBuff_init(cirBuff_t *cirBuff, cirBuff_position_t length, 
                                          cirBuff_data_t *array);
#endif

#define DONTBLOCK 1
#define DOBLOCK      0

//Add a datum to the end of the list
// corresponds to writePosition
// NOTE THAT THIS WILL BLOCK IF THE BUFFER IS FULL AND dontBlock==FALSE
//   If there's not an interrupt (or another thread?) doing reads, 
//   it will never exit!
// Returns 1 if the buffer was full and dontBlock == TRUE 
//   (the datum was lost. BE CAREFUL!)
uint8_t cirBuff_add(cirBuff_t *cirBuff, cirBuff_data_t data, 
                                        uint8_t dontBlock);

//Get the first datum in the buffer
// corresponds to readPosition
// Returns CIRBUFF_RETURN_NODATA if no data was in the buffer
cirBuff_dataRet_t cirBuff_get(cirBuff_t *cirBuff);

#if (!defined(CIRBUFF_EMPTY_UNUSED) || !CIRBUFF_EMPTY_UNUSED)
//I *believe* this empties the buffer rather than telling if it is empty
// (must be, since it doesn't return a value)
void cirBuff_empty(cirBuff_t *cirBuff);
#endif

#if(!defined(CIRBUFF_AVAILABLESPACE_UNUSED) || \
                  !CIRBUFF_AVAILABLESPACE_UNUSED)
//Get the amount of available space...
cirBuff_position_t cirBuff_availableSpace(cirBuff_t *cirBuff);
#endif

//Future:
// It might be handy to have the following:
//   isBufferFull()
//   isDataWaiting()
//  (waitForData()?)


/* Not Implemented
 //Return the amount of free space in the buffer... (not tested nor used, IIRC, A/O midi_out 2.00-7)
 uint8_t cirBuff_freeSpace();
 */

#if (defined(_CIRBUFF_INLINE_) && _CIRBUFF_INLINE_)
 #define CIRBUFF_INLINEABLE extern __inline__
 #include "cirBuff.c"
#else
 #define CIRBUFF_INLINEABLE //Nothing Here
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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/cirBuff/1.00/cirBuff.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
