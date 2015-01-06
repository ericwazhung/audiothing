/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//cirBuff 1.00

#include "cirBuff.h"

//Give a read in the middle of a write-block... to test.
//#define __TESTING__


//This should probably be renamed... som'n like bufferEmpty...
//volatile uint8_t UART_Ready[NUMUARTS] = UARTARRAYINIT;

#if(!defined(CIRBUFF_AVAILABLESPACE_UNUSED) || \
            !CIRBUFF_AVAILABLESPACE_UNUSED)
CIRBUFF_INLINEABLE
cirBuff_position_t cirBuff_availableSpace(cirBuff_t *cirBuff)
{
   cirBuff_position_t delta 
            = cirBuff->writePosition - cirBuff->readPosition;

   if( cirBuff->writePosition < cirBuff->readPosition )
      delta += cirBuff->length;

   //This probably isn't as efficient as it could be...
   delta = cirBuff->length - 1 - delta;

   return delta;
}
#endif


//Returns non-zero if there's an error
#if (!defined(CIRBUFF_NO_CALLOC) || !CIRBUFF_NO_CALLOC)
 uint8_t cirBuff_init(cirBuff_t *cirBuff, cirBuff_position_t length)
#else
CIRBUFF_INLINEABLE
 uint8_t cirBuff_init(cirBuff_t *cirBuff, cirBuff_position_t length, 
                                          cirBuff_data_t *array)
#endif
{
   //length+1 because one byte is unused to indicate full/empty (see below)
#if (!defined(CIRBUFF_NO_CALLOC) || !CIRBUFF_NO_CALLOC)
   cirBuff->buffer = (cirBuff_data_t *)calloc(length+1, 
                                                sizeof(cirBuff_data_t));

   //Check if malloc failed
   if(cirBuff->buffer == NULL)
      return 1;

   cirBuff->length = length + 1;
#else
   cirBuff->buffer = array;
   cirBuff->length = length;
#endif
   
   
   cirBuff->writePosition = 0;
   cirBuff->readPosition = 0;
   
   return 0;   
}

#if (!defined(CIRBUFF_NO_CALLOC) || !CIRBUFF_NO_CALLOC)
//In most cases I'm using this for there's no reason to destroy it...
// But one should never alloc without free...
// Returns 1 if the buffer was already NULL...
uint8_t cirBuff_destroy(cirBuff_t *cirBuff)
{
   if(cirBuff->buffer != NULL)
   {
      free(cirBuff->buffer);
      cirBuff->buffer = NULL;
      return 0;
   }
   else
      return 1;
}
#endif

//This loads the byte into the send buffer 
// (in midi_out the buffer is flushed one-by-one whenever the TX-complete interrupt is called)
// this is the old midiOut_sendByte:
// returns 1 if the buffer was full AND dontBlock was true (i.e. the byte was lost!), 0 otherwise
CIRBUFF_INLINEABLE
uint8_t cirBuff_add(cirBuff_t *cirBuff, cirBuff_data_t Data, 
                                             uint8_t dontBlock)
{   
   cirBuff_position_t nextWritePosition = 
                        (cirBuff->writePosition + 1);//%(cirBuff->length);
   if(nextWritePosition >= cirBuff->length)
      nextWritePosition -= cirBuff->length;

#ifdef __TESTING__
   int i = 0;
#endif
   
   //Wait if the buffer's full
   //   can check this by determining if the writePosition is one less than the sendPosition
   //   (or if the nextWritePosition is the same as the readPosition, makes more sense...)
   while(!dontBlock && (nextWritePosition == cirBuff->readPosition))
   {     
#ifdef __TESTING__
      if(i == 60000000)
         cirBuff_get(cirBuff);
      i++;
#endif
      //This probably isn't necessary... it wasn't in midiOut, but I don't know for certain.
      // I've heard of optimizers optimizing out empty loops (even if a test is volatile?!)
      asm("nop;");
   }
   
   //***This is where the midiOut code checked if the UART was ready to receive another byte to transmit***
   //    It would then load the byte directly to the UART instead of the buffer
   //    This doesn't make a whole lot of sense... why not test this outside and call addByte if it's busy?
   //     I think it was more efficient to have a single function, but this test could occur anywhere in this function
   //     (no sense in all that math above... right? Briefly:
   //if(UART_Ready[uartNum])
   //{
   // UDR = Data;
   // UART_Ready[uartNum] = FALSE;
   //}
   //else //Otherwise load to the buffer
   
   //Note, these values may differ from the ones in the while loop...
   // This *should* be OK, and maybe even handy
   // Note, also, that there is always one byte that's unused... 
   //  so writePosition == readPosition => empty
   if(nextWritePosition != cirBuff->readPosition)
   {
      //Load to the buffer
      cirBuff->buffer[ cirBuff->writePosition ] = Data;
      //Set up the writePosition for the next call
      cirBuff->writePosition = nextWritePosition;
      return 0;
   }
   else  //Indicate that the buffer was full
      return 1;
}

//If there's data in the buffer, return it (it's a single unsigned byte, 0-255)
// If there was no data in the buffer, return -1
CIRBUFF_INLINEABLE
cirBuff_dataRet_t cirBuff_get(cirBuff_t *cirBuff)
{
   cirBuff_data_t data;
   
   //There's no data in the buffer
   if(cirBuff->writePosition == cirBuff->readPosition)
      return CIRBUFF_RETURN_NODATA;
   else
   {
      data = cirBuff->buffer[ cirBuff->readPosition ];
      
      (cirBuff->readPosition)++;
      //(cirBuff->readPosition) %= (cirBuff->length);
      if(cirBuff->readPosition >= cirBuff->length)
         cirBuff->readPosition -= cirBuff->length;
      return data;
   }
}

#if(!defined(CIRBUFF_EMPTY_UNUSED) || !CIRBUFF_EMPTY_UNUSED)
CIRBUFF_INLINEABLE
void cirBuff_empty(cirBuff_t *cirBuff)
{
   cirBuff->writePosition = cirBuff->readPosition;
}
#endif

/* More UART examples simplified here originally from midiOut... 
SIGNAL(SIG_USART_TRANS)     // UART0 Transmit Complete Interrupt Function
{
   //Check if we're done sending everything in the buffer...
   if(buffer_writePosition[uartNum] == buffer_readPosition[uartNum])
      UART_Ready[uartNum] = TRUE;
   else
   {
      //Send the next character
      UDR = cirBuff_getByte();

      //Already is... but ahwell (is this even true?)
      UART_Ready[uartNum] = FALSE;
   }
}
*/

/*  I dunno if this ever worked...
//Return an integer indicating the amount of free space in the buffer
// so we can prevent sending data if it's unimportant, so it won't halt.
uint8_t midiOut_bufferFreeSpace(uint8_t uartNum)
{
   // uint8_t nextLoadPosition = (buffer_writePosition[uartNum] + 1)%MIDIOUT_BUFFERLENGTH;
   //buffer_writePosition is the actual position in the array 
   // where the next character will be placed 
   //  (it is currently empty unless the buffer is full)
   //buffer_readPosition is the actual position in the array 
   //  where the /next/ character will be sent from
   //i.e. if buffer_readPosition == bufferLoadPosition, 
   //  the array is empty (?) (according to other code)
   // or full?!
   // I think the intent was to leave one byte free, always...
   //   so if lp is one less than sp it's "full"
   //   and if lp == sp, then it's empty
   
   uint8_t freeSpace;
   int16_t lp = buffer_writePosition[uartNum];
   int16_t sp = buffer_readPosition[uartNum];
   
   if(lp < sp)    //the data to be sent is wrapped-around...
      freeSpace = sp - lp - 1;
   else if (lp == sp)   //there is no data to be sent... the whole array is empty
      freeSpace = MIDIOUT_BUFFERLENGTH - 1;
   else // if (lp > sp) //the empty space is wrapped-around
      freeSpace = MIDIOUT_BUFFERLENGTH - lp + sp - 1;
   
   return freeSpace;
   
}
*/

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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/cirBuff/1.00/cirBuff.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
