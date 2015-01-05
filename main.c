/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */






//a/o v51-12:
//Quick test, try to write *all* samples in the circular-buffer
//immediately
//Initial test is strange, upon boot once it stalled printing lc/fc for
//several seconds, next boot it worked right away, but stalled later for
//several seconds... 
//In general: LC has dropped to 30600, but fc has dropped to 1-5
//Oddly: Disabling this has a similar effect, a little less stable.
// THE HANDLING has changed, it used to be a state machine within a state
// machine, now it's a simplie if() or while() depending on this setting:
// REVISIT 51-11 to compare functionality!!!
// Actually, a 'break' from the switch() remained inside the while()
// so functionality should've been near identical.
//#define SDWU_WRITE_SEVERAL_SAMPLES TRUE


//a/o v51-11:
//PRINT_LCandFC always reports a full-Count, I'm guessing because the
//printout is BLOCKING.
//This is a quick test to see if that's the case
// Since the interrupts will be disabled during printout, the fullCount
// can't be incremented
//TODO: Ultimately, most of the printout-stuff can be NON BLOCKING
// it just needs to be implemented as such.
// Probably requiring a cirbuff
#define PRINTER_CLI TRUE

//PUAT_STRINGERS overrides the normal PUAT string-related functions with
//those from printer.c
//That's necessary here, since PRINTER_CLI is only implemented in printer.c
#define PUAT_STRINGERS TRUE

//The Status, related to the above:
// LCandFC is being printed
// loopCount is actually quite high, compared to what I expected ~32780...
//   (heart removed)
//   that's how many times the main() while() loops in a second
//   (Considering that most functions are state-machines, that could be too
//    low... e.g. say writing a byte to the SD card requires three
//    transfers, or three states... then the main() loop would have to run
//    at least 3 times faster than the ADC samples, so as not to miss any
//    samples. The ADC is sampling around 19KS/s, so we're actually not
//    even running double that... I guess it's not as fast as I thought)
// fullCount is riding ~2700
//   that's how many times the ADC interrupt has occurred and there was no
//   memory in the buffer for a new sample
//   (how many lost samples occurred in a second)
//
// Oddly, adding PRINTER_CLI had no effect on the fullCount.
// So... did I miss something? Certainly with a BLOCKING printer-function,
// missed samples would've been stacking up. But then puting CLI/SEI around
// the blocking printer-function should've prevented the ADC ISR from
// running, which should've prevented fullCount from incrementing during
// printout. (The samples would still be missed, just not counted)
// Instead, we've got the same full-count
// (was CLI/SEI around puatSendByteBlocking() already? If so, then the
// full-count is actually, roughly, the number of samples missed each
// second *regardless* of whether printout happens.
// 3k in 19 is a bit, but not horrendous... we'll leave it for now and
// reexplore some other issues.






#include "mainConfig.h"


#include "projInfo.h"   //Don't include in main.h 
                        //'cause that's included in other .c's?
#include "main.h"

#include <util/delay.h> //For delay_us...



#if(!defined(PUAR_DISABLED) || !PUAR_DISABLED)
#include _POLLED_UAR_HEADER_
#endif

//Now handled in printer.h...
//#include _POLLED_UAT_HEADER_
#include _CIRBUFF_HEADER_
#include <string.h>
#include <inttypes.h>
#include _ADC_FREE_RUNNING_HEADER_
#include <avr/pgmspace.h>//progmem.h"

//Saving space... this isn't currently used
// It'll have to be reenabled if it's used again.
//#include _HSSKB_HEADER_  //Stowaway Keyboard


#include <avr/eeprom.h>



#include "pinout.h" //pinout.h automatically includes pinoutTrinketPro.h
                    // if necessary



#ifdef __AVR_ATmega328P__
 #include _USART_SPI_HEADER_
#else
 #include _USI_SPI_HEADER_
#endif

#include "sd.h"


#if(PRINTER == PRINT_NLCD)
//#include "nlcd.h"
#include _NLCD_HEADER_
#else
#define FONTBYTES 256   //Needed to determine where to store the usageTable
#endif

#include _ANABUTTONS_HEADER_


#if(defined(AUDIO_OUT_ENABLED) && AUDIO_OUT_ENABLED)
 #ifndef __AVR_ATmega328P__
  #include _TINYPLL_HEADER_
 #endif
#endif





#include "printer.h"


uint8_t availableChunks;
#define UT_UNUSED_CHUNK    0


//In S/s. Currently 19230S/s
#define SAMPLE_RATE (F_CPU/ADC_CLKDIV/ADC_CALC_CYCLES)

//a/o v60: The SD-Card will have a format-header indicating the sample-rate
//  THIS REQUIRES audioThing-desktop v7p20+
//#include _STRINGIFY_HEADER_
//PGM_P sampleRateString = PSTR(STRINGIFY(SAMPLE_RATE));
//STRINGIFY(SAMPLE_RATE) stringifies the values, separated by divisions...
// It might be doable, but I'm drawing a blank.


#define UT_EEPROM_START_BYTE  FONTBYTES
#define eeprom_read_utByte(byteNum) \
            eeprom_read_byte((byteNum) + UT_EEPROM_START_BYTE)
#define eeprom_update_utByte(byteNum, val) \
            eeprom_update_byte(((byteNum) + UT_EEPROM_START_BYTE), (val))
#define eeprom_write_utByte(byteNum, val) \
            eeprom_write_byte(((byteNum) + UT_EEPROM_START_BYTE), (val))




//TODO:
//  Add power-removal detector to finish writing a block
//    possibly add "done" indicator (?)
//  Add SD-swapping ability:
//    e.g. two SD cards, when one's full (or removed?) switch to the other
//  Multiple input-types? for on-the-go vs... keyboard...
//    Detect high on ser-port for kb
//    (and for otg?)
//    switch while-powered?
//  Remove blocking printout...
//  Time...? (RTC?)

//  Log kbInput times (in eeprom?)
//    block out writes to the surrounding locations... (15min prior?)
//    Use first sector to indicate usage...
//    512 bytes -> 512 locations
//    (2GB/512 = 3,906,250B = 101.6sec per location)
//    (Might have to cut this down, due to memory limitations...?)
//    (e.g. writing the first sector all at once, requires having that data
//     all available, to read then rewrite, at once...)
//    each byte indicates the last-stored data at that location...
//    e.g. fresh card, kb pressed @ 200sec, second byte (byte1) in first 
//     sector written 1 (to indicate usage in chunk 1)
//    e.g.2. card has filled and wrapped-around, only byte1 indicates usage
//     the corresponding chunk is unused in next loop
//     kb pressed @300sec from loop2 start... 
//         first 102sec in chunk 0
//          skips used chunk 1
//         second 102sec in chunk 2
//         third 102sec in sector 3 (kbPress)
//             tableSector updated with byte3 = 2 
//                 (chunk 3 used in loop number 2)
//         
//     Readback tableSector (after several loops?):
//      byte 0 = 0         
//      byte 1 = 1         chunk 1 used in first loop
//      byte 2 = 0
//      byte 3 = 2         chunk 3 used in second loop
//      bytes 4-20 = 0
//      byte 21-24 = 5     chunk 24 noted used in fifth loop
//                         indicated via kb to save several minutes prior
//
//      Might do something automatically... notation in 1, so try to keep
//        0 for as long as possible... (upper bits indicate priority?)
//
//        e.g.
//      byte 99 = 0x01     maybe overwrite if running out of space(?)
//      byte 100 = 0x11    attempt to keep surrounding
//      byte 101 = 0xf1    Definitely Keep chunk 101
//      byte 102 = 0x11    attempt to keep surrounding
//
//  If powered-down correctly: Log latest loop number (sector 2?)
//                             and sector-location last-written...
//                             (so we can determine where new data ends)



// A Note On Inline Functions:
//  GCC claims that "extern inline" assures that it won't
//    be compiled into a separate function
// and claims (almost assuredly ?) that "static inline" will
//  which really doesn't make sense, since a static function is one that
//  exists only within the namespace, whereas an extern function is one
//  that might be available (to be called) elsewhere
//
// Either way, "extern inline" gives warnings about static variables
// "static inline" does not.
// which, again, sorta makes sense.
//
// And both seem smart enough (at least in avr-gcc) to only compile if used
//
// Ironically, it's not smart enough to recognize that regular functions
// that're unused needn't be compiled...
// Maybe due to function-pointers being used somewhere in the commonCode
// (dmsWaitfn)
//
// info gcc:
//  When a function is both inline and `static', if all calls to the
//  function are integrated into the caller, and the function's address is
//  never used, then the function's own assembler code is never referenced.
//  In this case, GCC does not actually output assembler code for the
//  function
//  Some calls cannot be integrated for various reasons...
//  ...then the function is compiled to assembler code as usual.
//
//  If you specify both `inline' and `extern' in the function definition,
//  then the definition is used only for inlining.  In *no case* is the
//  function compiled on its own, not even if you refer to its address
//  explicitly.
//  This combination of `inline' and `extern' has almost the effect of a
//  macro.






#if((!defined(HSSKB_TRANSLATE) || !HSSKB_TRANSLATE) && \
    (!defined(NKP_ENABLED) || !NKP_ENABLED))
   #warning "Memo, etc, don't work without HSSKB_TRANSLATE or NKP. (Why shouldn't the heart-button handle it?)"
#endif






#if ((defined(PRINTEVERYBYTE) && PRINTEVERYBYTE))
//Am Thinkink there's no reason to keep this down to 4 samples, with
//PRINTEVERYBYTE, but that it's a remnant of early-early testing where
//there was no cirBuff *unless* we used PRINTEVERYBYTE (or the old
//PRINTOUT)
//used to be called "BUFFSIZE" woot!
#define ADC_CIRBUFF_SIZE  4
#else
#define ADC_CIRBUFF_SIZE  16//96
#endif

//used to be called, simply, "buffer" good one.
uint16_t arrayForADCCirBuff[ADC_CIRBUFF_SIZE];

//used to be called "myCirBuff" wee!
cirBuff_t adcCirBuff;






void haltError(uint8_t errNum);

//Left global so we can keep track of memory usage...
char stringBuffer[40];



static __inline__ 
void printHex(uint8_t hexVal)
{
   uint8_t hv = (hexVal >> 4);

   stringBuffer[0] = '0';
   stringBuffer[1] = 'x';
   stringBuffer[2] = (hv < 10) ? (hv + '0') : (hv + ('a'-10));
   hv = (hexVal & 0x0f);
   stringBuffer[3] = (hv < 10) ? (hv + '0') : (hv + ('a'-10));
   stringBuffer[4] = ' ';
   stringBuffer[5] = '\0';

   //sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), byteIn);
   printStringBuff();
}




//Track the number of samples lost
uint32_t fullCount = 0;


#if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
//This USED TO track whether it was time to cycle back to block-zero
// when reached the end of the SD card...
// Only used in writeUpdate and for printout in main...
// let's keep it for printout purposes
uint32_t blocksWritten = 0;
#endif


/* Two paths:
   1) Started yesterdah:
      Write blocks
         Block Finished:
            continue until curBlock == endBlock
            otherwise Start next
         Start Next:
            find next fragment
               set curBlock to start
               set endBlock to end
   
      Uses blockNum to stop when blockNum == endBlock
   
   2) Started last night...
      Write Blocks:
         Block Finished:
            If @ end of chunk
               check if next block is available
                  NO: Stop Writing
                  YES: continue
         Stop -> Start Writing:
            findNextChunk

      Uses blockNumInChunk counter to stop when == numBlocksInAChunk

   Durn-near identical...
      Now, how to initialize
      Do we allow curBlock (or curChunk) to advance to the next one
         and test *after* or do we test immediately
      Is End == the last available block, or the next (used) block...?

      Heh, and glancing at options 1 and 2, I have them reversed in my mind
      I thought 2) was with fragments, and 1) was with chunks...

   WriteBlocks:
      Start (new block location):
         findNextFragment  //cur could be @ usable, on init
            set cur        //    OR       @ used, after Block Complete
            set end
         writeAtBlock cur
      
      Block Complete:
         cur == end?
            cur++ //WOULD be out of range... either used or end of card
            Start

   Is cur chunks or blocks or...?
   so then we need a chunk counter...
      or a means to determine which chunk the current block resides

   WriteBlocks:
      Init:
         FindNextAvailableChunk_IncludingThis:
            while(!curChunkAvailable())
               curChunk++
         endChunk = findFragmentEnd(curChunk)
         Start
      Start:
         SD_WriteMultiple(startBlockFromChunk(curChunk))
      Write:
         ...
      Bock Complete:
         blockCount_thisChunk++;
         if(blockCount_thisChunk == numBlocksPerChunk)
            curChunk++
            blockCount_thisChunk=0
            FindNextAvailableChunk_AFTERThis(

   HAH! Mixing fragment-method with chunk-method AGAIN!

//thisChunkNum_u8 is the one currently being written
// UNLESS between block-complete and Init
WriteBlocks:
   Init:
      FindNextAvailableChunk_IncludingThis:
         while(!chunkAvailable(thisChunkNum_u8))
            thisChunkNum_u8++
      //endChunk = findFragmentEnd(thisChunkNum_u8)
      //             endChunk = thisChunkNum_u8
      //             while(chunkAvailable(endChunk))
      //                endChunk++
      //             endChunk--
      START
   Start:
      SD_WriteMultiple(startBlockFromChunk(thisChunkNum_u8))
      WRITE
   Write:
      ...
   Block Complete:
      blockNum_inThisChunk++;
      if(blockNum_inThisChunk == numBlocksPerChunk)
         thisChunkNum_u8++
         blockNum_inThisChunk=0
         if(!chunkAvailable(thisChunkNum_u8)) //FindNextAvailableChunk_AFTERThis(
            STOP
         else
            WRITE
   Stop:
      SD_SendStopWriting()
      INIT


*/


//This tracks the cycle/loop number of writing to the SD...
// each time it reaches the end, this is incremented
// Used to fill the usageTable with some timing-info...
// (It should not be incremented past 255, no?)
uint8_t thisLoopNum = 0;

//If this is true, the usage-table has been updated in the eeprom
// and should be written to the SD
uint8_t usageTableUpdateState = 0; //Idle... FALSE;


//See description...
// Basically, this is the usageTable position that is to be marked
// when the "memo" key is pressed...
// Each time it's pressed, this will jump to the previous position in the
// usage table and that position will be marked
// After ten seconds of not-being-pressed it will reset to follow
// thisChunkNum_u8...
uint8_t memoPositionInUT = 0;

//This is the number of chunks that have been requested to be marked 
//  (regardless of whether they were marked already, or if there are
//   enough written chunks to mark)
// in this go-round with the memo-key
// This is used for calculating the number of step-backs necessary 
//   when the key is pressed again (a/o memoMinutes)
uint8_t memoChunks = 0;

//This is the number of minutes that have been marked this go-round
uint8_t memoMinutes = 0;

//Loop Number to be written when Memo is pressed
uint8_t memoLoopNum = 0;

// Don't allow Memo's loopNum to decrease below bootLoopNum
uint8_t bootLoopNum = 0;

//Rather than testing whether the memoPositionInUT in eeprom contains
// memoLoopNum, to determine whether to update the SD, use this instead
// (because memoPositionInUT may not contain a valid position
//  if memo was pressed more times than there is data...)
uint8_t writeMemoToUT = FALSE;



//this needs to be global for comparison upon BLOCK_COMPLETE
// The number of blocks in a chunk...
// Since there are 256 chunks, this is sd_numBlocks / 256
uint32_t numBlocksPerChunk;

// This will track the number of the block within the current chunk
// When == numBlocksPerChunk, it's time to look for the next chunk
// CBN (ChunkBlockNum) is nice, but it could sound like 
//   the blocknum *of* the chunk, rather than *in* the chunk...
uint32_t blockNum_inThisChunk = 0; //chunkBlockNum = 0;

// This tracks which chunk we're in... its value aligns directly with the
// position in the usage-table...
// EXCEPT when a chunk has finished, at which point it will be the next
// expected chunkNum until writeUpdate's BEGIN_WRITING determines it's
// impossible...
// It *will* be wrapped to 0 after chunkNum 255
// though that may not be available, it will be tested
uint8_t thisChunkNum_u8 = 0;


//Even with two calls, it's smaller to inline this...
// what about three?
static __inline__ 
void memoReset(void)
{
   memoLoopNum = thisLoopNum;
   memoPositionInUT = thisChunkNum_u8;
   memoChunks = 0;
   memoMinutes = 0;
}


// Would prefer that the memo key doesn't mark a specific number of
// chunks (namely 1, as-implemented)
// INSTEAD: that it would mark a certain number of *minutes*' worth of
//  chunks.. So it's the same regardless of the SD card

// We have: SAMPLE_RATE (19230 S/s)
// numBlocksPerChunk ( ? Bk/C)
// BLOCKSIZE (512 B/Bk)
// SAMPLE_BYTES (2 B/S)
//
// so 5 min -> numChunks
// 5 min = 5*60 sec
//       = (5*60)*19230 Sec
//       = (5*60)*19230 * 2 Bytes
//       = (5*60)*19230 * 2 / 512 Blocks
//       = (5*60)*19230 * 2 / 512 / numBlocksPerChunk Chunks
// So, ideally, each time the memo key is pressed, 5min is marked-out
// but on the 2G card, each chunk is 3:19... don't want to round-down...
// better to round up. But I'd rather also not have 3 keypresses 
//   = 6:38*3 =~ 19:30 when it should be 15... and 5*3.19 would be closer 
//     (~16.1min)
// So how to do this... an xyTracker, maybe...
//  Or instead of tracking memoChunks, track memoTime... and recalculate
//  *each time* the memo-key is pressed...
//       
//  memoChunks(memoMins) 
//     = memoMins * ((60 * SAMPLE_RATE * 2) / 512) / numBlocksPerChunk
//      would still round-down...
//     = ((memoMins*60) + (chunkSecs-1)) * (SAMPLE_RATE * 2) / 512)
//                                       / numBlocksPerChunk
//
//     or chunkBlocks instead of chunkSecs would be more accurate...
//
//     memoBlocks_notChunkAligned = ((memoMins*60)*19230) * 2 / 512
//
//  memoChunks(memoMins)
//     = (memoBlocks_notChunkAligned + numBlocksPerChunk - 1)
//               / numBlocksPerChunk
//
static __inline__ uint8_t memoChunksFromMinutes(uint8_t memoMinutes)
{
   //Add BLOCKSIZE-1 before /BLOCKSIZE to round up...
   uint32_t memoBlocks_notChunkAligned =
      (((BLOCKSIZE-1) + ((uint32_t)memoMinutes           //Bytes / 512
//    + ((uint32_t)60 * (uint32_t)SAMPLE_RATE * (uint32_t)2))) >> 9 );
      * ((uint32_t)60 * (uint32_t)SAMPLE_RATE * (uint32_t)2))) >> 9 );
//#warning "MATH HAS BEEN MODIFIED FOR SIZE-TESTING"

   uint32_t memoBlocks_ForRounding =
      memoBlocks_notChunkAligned + (numBlocksPerChunk - 1);

   uint32_t tempChunks = memoBlocks_ForRounding / numBlocksPerChunk;
// uint32_t memoChunks = memoBlocks_ForRounding - numBlocksPerChunk;
// No Good: assumes both are *large*
// uint16_t memoChunks = (uint16_t)(memoBlocks_ForRounding>>16) / 
//                      (uint16_t)(numBlocksPerChunk>>16);
   //WTF... commenting out this *increases* codesize from 8->14Bytes
   if(memoChunks > 255)
      return 255;
   else
      return (uint8_t)tempChunks;
}











#if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
uint32_t writeBusyCount = 0;
#endif

ISR(ADC_vect) //, ISR_NOBLOCK)
{
   //See notes in adcFreeRunning.c...
   //Note that optimization can reorder C code, so this should probably be
   //checked:
//Actually, replacing it with "ADC" instead, which should be guaranteed to
//work in the proper order. (a/o v52)
//#error "here?"
//   uint8_t adcLow = ADCL;//Read ADCL first, then ADCH isn't updated until
//   uint8_t adcHigh = ADCH; // it's read.


#if (defined(BUTTON_IN_SAMPLE) && BUTTON_IN_SAMPLE)
   //Tried |=ing the last bit, but it increased code-size!
   cirBuff_data_t adcVal = ADC
                           //(uint16_t)adcLow | (((uint16_t)adcHigh)<<8)
                         | (((uint16_t)anaComp_getButton())<<10);
#else
   cirBuff_data_t adcVal = ADC; 
                           //(uint16_t)adcLow | (((uint16_t)adcHigh)<<8);
#endif

   if(cirBuff_add(&adcCirBuff, adcVal, DONTBLOCK))
      fullCount++;
   else
   {
      //Repeat the output only when the cirbuff isn't full, that way
      //there's some sense of what we're actually recording.
#if(defined(AUDIO_OUT_ENABLED) && AUDIO_OUT_ENABLED)
   #ifdef __AVR_ATmega328P__
      OCR2B = adcVal >> 2;
   #else
#error "This should be changed back to adcVal..."
      //These notes are old, apparently I used adcVal, then switched to
      //adcHigh/Low, now I suggest switching back to adcVal but haven't yet
      //implemented it for this chip (a/o v52)

      //1:1 would be TC1H=(uint8_t)(adcVal>>8); OCR1D=(uint8_t)adcVal;
      TC1H = adcHigh; //(uint8_t)(adcVal>>6);
      OCR1D = adcLow; //(uint8_t)(adcVal<<2);
   #endif
#endif
   }
}

/****** Re: sei() at the end of the ISR: *******

//Below is the disassembly of the last few instructions in the ISR.
// (prior to commenting-out sei() at the end of the function)
//Note that because of optimization (?) the sei occurs *way before*
//exitting. The order of instructions is never guaranteed with
//optimization, but this doesn't make sense, since the position of sei() is
//so important... one'd think the compiler would know that (shouldn't it be
//"volatile" or something? Ahhh... I see the rjmp over the cirBuff stuff). 
//
//Regardless: What *would* make sense is that the sei() would occur before
//the function-return handling; the 'pop's.
//I don't know what I was thinking in putting sei() in there, at all,
//except that there was some confusion long ago about two different types
//of interrupt-handling routines. Was it SIG vs ISR? As I recall:
//One type explicitly did *not* cli/sei, one did. 
//(Or was it that one explicitly sei'd in the entry of the "function" and
//didn't use reti at the end?)
//I had a hard time
//locating that reference a second-time and figured adding my own cli/sei
//was a safe method. It had not occurred to me, at the time, that of course
//'pop's (and possibly more) would've occurred between my explicit sei()
//and the function's reti().
//
//This program has been running this way for months, but it seems
//surprising, running close to full CPU-usage just between the ADC and the
//SD-writing, that e.g. no ADC handling occurred before a previous
//ISR-handler finished... sei() kinda nullifies the purpose of "reti" at
//the end.

//Even more oddly...? the 'out 0x3f, r0' two 'pop's before 'reti'
// that wasn't my doing, but would have the same effect!
//
//So here's the answer, from the Atmega328p documentation, 
// Section 7.7: Reset and interrupt handling:
//  When an interrupt occurs, the Global Interrupt Enable I-bit is cleared
//  and all interrupts are disabled. 
//  ...The I-bit is automatically set when a Return from Interrupt 
//  instruction – RETI – is executed.
//
//So, first, "ISR(SIG_whatever){}" apparently *does* keep interrupts 
//disabled until the end of the "function", an explicit "cli()" is
//unnecessary since the interrupt itself (rather than the interrupt-handler
//"function") clears the I-bit, and the 'ISR()' "function" return is 
//handled with reti, reenabling the I-bit without an explicit "sei()". 
//So, I was overly-cautious and in doing-so managed to open up potential
//for infinitely-nested interrupts.
//Also, the push/pop of the SREG (0x3f) occurs *while the I-bit is
//disabled* and reenables with the reti.

// puar_update(0);
   sei();
     da6:   78 94          sei
     da8:   14 c0          rjmp  .+40        ; 0xdd2 <__vector_21+0x92>
   cirBuff_data_t adcVal = (uint16_t)adcLow | (((uint16_t)adcHigh)<<8);
#endif

   if(cirBuff_add(&adcCirBuff, adcVal, DONTBLOCK))
   {
      fullCount++;
     daa:   80 91 a2 01    lds   r24, 0x01A2
     dae:   90 91 a3 01    lds   r25, 0x01A3
     db2:   a0 91 a4 01    lds   r26, 0x01A4
     db6:   b0 91 a5 01    lds   r27, 0x01A5
     dba:   01 96          adiw  r24, 0x01   ; 1
     dbc:   a1 1d          adc   r26, r1
     dbe:   b1 1d          adc   r27, r1
     dc0:   80 93 a2 01    sts   0x01A2, r24
     dc4:   90 93 a3 01    sts   0x01A3, r25
     dc8:   a0 93 a4 01    sts   0x01A4, r26
     dcc:   b0 93 a5 01    sts   0x01A5, r27
     dd0:   ea cf          rjmp  .-44        ; 0xda6 <__vector_21+0x66>
   }

// tcnter_update();
// puar_update(0);
   sei();
}
     dd2:   ff 91          pop   r31
     dd4:   ef 91          pop   r30
     dd6:   bf 91          pop   r27
     dd8:   af 91          pop   r26
     dda:   9f 91          pop   r25
     ddc:   8f 91          pop   r24
     dde:   5f 91          pop   r21
     de0:   4f 91          pop   r20
     de2:   3f 91          pop   r19
     de4:   2f 91          pop   r18
     de6:   0f 90          pop   r0
     de8:   0f be          out   0x3f, r0 ; 63
     dea:   0f 90          pop   r0
     dec:   1f 90          pop   r1
     dee:   18 95          reti
*/






// v6_2 differences:
//   (see: v6_2-v6_1tcnter+lastSampleDiff.txt)
//   ?   BlockSize setting/verification (NYI-here)
//          CMD16 + checking for non-erroring return
//          Not difficult to reimplement, but some changes
//          (switch statement -> if's due to blockSize variable not const)
//   !   PrintEveryByte (Now Implemented Here...)
//   ?   DataNotReadyCounter (NYIH)
//         + packetCount -> determine the delay between packets
//   ?   PWM8 option (NYIH)
//      (Note re: implications of functioning despite CRC non-grabbing
//      irrelevent due to the packetByte tests not being executed properly)
//   ?   Periodic printouts (blocking) (NYIH)
//   !   int16->uint10 conversions
//           changed from division to >>
//   !   No longer relevent, since using uint16 raw files...
//   ?   CMD9 -> get Card-Specific-Data (NYIH)
//           Didn't handle the data, anyhow.
//   ?   spi_sd_transferByte made extern __inline__ (NYIH)
//           Caused warnings about static versus extern inlining calls


//#include <util/delay.h>  //for delay_us in initializing the PLL...
//#include <stdio.h> //for sprintf


#if !defined(__AVR_ARCH__)
 #error "__AVR_ARCH__ not defined!"
#endif


//void spi_sd_readTable(void);

static __inline__ void sd_writeUpdate(void);

void haltError(uint8_t errNum)
{
   //Disable the SD card...
   // (then insertion while powered should be OK)
   setpinPORT(SD_CS_pin, SD_CS_PORT);
   
   //Reenable the timer interrupt so the heart will update...
#if(defined(_DMSTIMER_HEADER_))
   timer_compareMatchIntSetup(0, OUT_CHANNELA, TRUE);
#endif

   //Thought about disabling the ADC interrupt, but it shouldn't be
   // necessary... so it fills up the cirBuff and once it's full
   // audio passthrough is stopped...

   heart_blink(errNum);
   while(1)
   {
#if(!defined(_DMSTIMER_HEADER_))
         tcnter_update();
#endif
         heart_update();
   }
}

#if 0
//No Longer Used for SPI... 
//#define timer0_clearOutputCompareFlag()   TIFR = (1<<OCF0A)
void pauseIndicate(uint8_t indicateNum)
{
   //The timer's running at about 300kHz...
// uint32_t count;

   heart_blink(indicateNum);

// uint8_t i = 0;

   uint32_t time = (((indicateNum&0x0f) + ((indicateNum&0xf0)>>4)) * 3 * DMS_SEC);

   dmsWait(time);
// while(dmsGetTime() < endTime)
//    heart_update();
/*
   for(count=0; count < countMax; count++)
   {
         //This can be removed for xmission without the timer...
//         timer0_clearOutputCompareFlag();

         //This can be removed for xmission without the timer...
         //Wait for the next output compare
         while(!getbit(OCF0A, TIFR))
         {
            i++;
            //TOO FAST!
            if(i==30)
            {
               i=0;
               heart_update();
            }
         }
   }
*/
   heart_blink(0);
}
#endif

#if(defined(AUDIO_OUT_ENABLED) && AUDIO_OUT_ENABLED)

 void pwmTimer_init(void);
 #ifndef __AVR_ATmega328P__
  void pll_enable(void);
 #endif
#endif



#if 0
 
// THE FOLLOWING IS HOKEY...

// This should be initialized to 0's, right? In the Data section?
//uint8_t usageTable[USAGETABLE_SIZE/8];
// Moving this to EEPROM, keep that RAM for other purposes...
// (Though, surprisingly, dropping CIRBUFF from 96B to 16B doesn't seem
//  to be causing full issues... wasn't that a big deal before?!)
// This way, too, we can load all the data, rather than just used/unused

//This is the number of the last unused block in this fragment
// Or, obviously, the last block on the card... whichever comes first
// "Chunk" refers to the amount of card-space represented by a position
//  in the usage-table
// "fragment" refers to a bunch of consecutive unused chunks...
uint32_t lastBlockInThisFragment = 0;

//This is the current position in the usage-table...
// Could be uint8_t and do uint16_t math internally...
uint16_t usageTablePosition = 0;

//sd_numBLOCKSIZEBlocks is the *number* of blocks
//  the last block is numBlocks-1 (zero-based)
// Then each "chunk" is numBlocks/(numChunks=256) (blocks/chunk)
// Or chunkSize_Blocks = (sd_numBlocks >> 8)
// uint16_t should be plenty: 
//    256(chunks)*65536(maxBlocks/chunk)*512(bytes/block)=8GB...
// erm... not really... 8GB is easy to find...
#if(USAGETABLE_SIZE != 256)
#error "This math'll need adjustment..."
#endif
uint16_t chunkSize_Blocks = 0;



//This whole thing was eluding me...
// Realistically, it makes more sense to just test the next chunk
// when it's time to start writing a new block (in writeUpdate)
// rather than trying to keep track of when the end of the fragment will
// arrive...

//Returns the block number of the next available fragment
// Also writes endBlock and updates usageTablePosition
static __inline__ uint32_t findNextFragment(void)
{
   //Idea:
   // * Find the first empty chunk from current position...
   //   set currentPosition here...
   // * Find the last empty chunk
   //   set stopBlock to the next one...  


   //**Find the first empty chunk...
   // The chunk is full if the value is non-zero
   while( (usageTablePosition < USAGETABLE_SIZE)
          && (0 != eeprom_read_utByte((uint8_t *)usageTablePosition)) ) 
   {
      usageTablePosition++;
   }

   // Should never be >, if I'm smart about it...
   if(usageTablePosition >= USAGETABLE_SIZE)
   {
      usageTablePosition = 0;
   }


   //This could be put in the for-loop...
   // e.g. startBlock += chunkSize_Blocks... ish
   //So the usage-table position should indicate the first block
   // by simple multiplication...
   uint32_t startBlock;
   startBlock = ((uint32_t)chunkSize_Blocks*(uint32_t)usageTablePosition);


   uint16_t ut_nextUsed;

   ut_nextUsed = usageTablePosition;
   stopBlock = startBlock;

   //**Find the last empty chunk (could be this same chunk!)
   // Really, we're looking for the next *used* chunk, then subtracting
   //  one block from its position...
   while( (ut_nextUsed < USAGETABLE_SIZE)
          && (0 == eeprom_read_utByte((uint8_t *)ut_nextUsed)) )
   {
      ut_nextUsed++;
      stopBlock += chunkSize_Blocks;
   }

   // Should never be >, if I'm smart about it...
   if( ut_nextUsed >= USAGETABLE_SIZE)
   {
      //The math below should also handle this...
      // but I suppose this could make the last chunk fill the space
      // in case sd_numBlocks / 256 isn't an integer...
      stopBlock = sd_numBLOCKSIZEBlocks;
   }
   else
   {
      //stopBlock = ((uint32_t)chunkSize_Blocks*(uint32_t)ut_nextUsed - 1;
   }



   return startBlock;
}
#endif //0

#define rt_xfer(a) spi_sd_transferByte(a) //WithTimer(a)

//Write the usageTable from eeprom to the SD card...
// This includes the formatting...
void sd_writeUsageTable(void)
{
   uint16_t i;

   for(i=0; i<16; i++)
   {
         rt_xfer(0xff);
   }

/* //Wait until BUSY is no longer...
   while(rt_xfer(0xff) != 0xff)
   {
         asm("nop;");
   }
*/
   //Everything else is formatting:
   //CMD24: Write single block:
   //                         |--|<----(">=1Byte")
   //                         .  .
   // DI    __________________.__.__________/____________________________
   //        |  CMD24  |      .  | 0xfe | DATA | 2B CRC |
   //        |_________|      .  |______|___/__|________|
   //                         .             /           .
   // DO    __________________._________________________.____         ___
   //                    | R1 |                         | DR | BUSY  |
   //                    |____|                         |____|_______|
   //                                     Data Response --^
   //                                      xxx0 010 1
   //                                           ^^^---Data Accepted

   // Write at block 0:
   spi_sd_sendEmptyCommand(24);
   uint8_t r1Response = spi_sd_getR1response(TRUE);
   //uint16_t i;

//  TRUE above should handle that...
   // Insert a delay ">= 1Byte"
   //for(i=0; i<16; i++)
   //{
   //   rt_xfer(0xff);
   //}


   //cli();

   //WTF, forgot the Data Token?! How'd it *ever* write *anything*?
   // (even something shifted...)
   // (maybe 0xff's sent in delay, followed by 0x00 (first Formatted Byte)
   // ( shifted by 7 bits looks like 0xfe = Data Token )

   // Send the data token:
   rt_xfer(0xfe);

   // Write the "formatted" indicator...
      //a/o v60: We'll store the sample-rate in the format-header
      // How about starting 8 bytes from the end...
      // "19230" is six... (+null)
   for(i=0; i<(BLOCKSIZE-USAGETABLE_SIZE-8); i++)
   {
      rt_xfer(i);
   }

   //Write the sample-rate
   // pad the remainder with '\0'
   uint8_t nullFound = FALSE;
   sprintf_P(stringBuffer, PSTR("%"PRIu16), (uint16_t)SAMPLE_RATE);

   for(i=0; i<8; i++)
   {
      uint8_t character;

      if(!nullFound)
      {
         character = stringBuffer[i];

         if(character == '\0')
            nullFound = TRUE;
      }
      else
         character = '\0';

      rt_xfer(character);
   }

   availableChunks = 0;

   // Write the usage-table stored in the eeprom to the SD card
   for(i=0 ; i<(USAGETABLE_SIZE); i++)
   {
      uint8_t utByte = eeprom_read_utByte((uint8_t *)i);

      if(utByte == UT_UNUSED_CHUNK)
         availableChunks++;

      rt_xfer(utByte);
   }

   //Send pseudo CRC
   rt_xfer(0x00);
   rt_xfer(0x00);

   //Get the Data Response and Busy

   //SD should respond "Immediately" with 0bxxx00101
   // to indicate that the data was accepted
   uint8_t dataResponse = rt_xfer(0xff);
   //Check that the data was accepted
   if((dataResponse & 0x1f) != 0x05)
   {
#if(defined(PRINT_BAD_DATARESPONSE) && PRINT_BAD_DATARESPONSE)
      sprintf_P(stringBuffer,
               PSTR("Bad Data Resp2: 0x%"PRIx8"\n\r"),
               dataResponse);
      printStringBuff();
#endif
#if (defined(HALT_ON_BAD_DATARESPONSE) && HALT_ON_BAD_DATARESPONSE)
      haltError(0x0f);
#endif
   }

   //Wait until BUSY is no longer...
   while(rt_xfer(0xff) != 0xff)
   {
      asm("nop;");
   }

// sei();

}


//Using this instead of findPreviouslyWrittenChunk() saved 16 bytes
// (when only called once)
// Returned the size back to what it was before functionifying
#define fpwc(chunkNum, loopNum, retVal) \
{ \
   retVal = 1; \
   uint8_t ut; \
   do { \
      if(chunkNum == 0x00){ \
         if(loopNum > bootLoopNum) { chunkNum = 0xff;  loopNum--; } \
         else { retVal = FALSE; break; } }\
      else { chunkNum--; }\
      ut = eeprom_read_utByte((uint8_t *)((uint16_t)chunkNum));\
      if(ut == loopNum) { retVal = FALSE; break; }\
   } while( (ut != 0) ); \
}



extern __inline__ 
uint8_t findPreviouslyWrittenChunk(uint8_t *chunkNum, uint8_t *loopNum);


static __inline__ 
uint8_t findNextUsableChunk(uint8_t startingChunkNum);


static __inline__ 
void formatCard(void);

static __inline__ void spi_sd_readTable(void)
{

#if(USAGETABLE_SIZE != 256)
#error "This math needs revision if USAGETABLE_SIZE!=256"
#endif
   // This is sd_numBlocks / (numChunks==256)
   numBlocksPerChunk = (sd_numBLOCKSIZEBlocks>>8);



   //CMD17: Read single block:
   //
   // DI    _____________________________________________________
   //           |  CMD17  |
   //           |_________|
   // DO    ____________________________________/________________
   //                        | R1 |   | 0xfe | DATA | 2B CRC |
   //                        |____|   |______|__/___|________|
   //                                           /
   //  Not 100% certain that 0xfe (Data Token) arrives aligned with 8-clock
   //   cycles... but maybe 75%
   //
   //Send CMD17 to read a single block:
   // Since we're reading block 0, the command is "empty" 
   //   (no non-zero arguments)
   spi_sd_sendEmptyCommand(17);
   uint8_t r1Response = spi_sd_getR1response(FALSE);

   //Can't use TRUE above, here, because it'll start sending actual data

   //Grab the delay, and the data-token
   while( rt_xfer(0xff) != 0xfe)
   {
      asm("nop;");
   }

   //Start processing received data...
   // There's not enough memory to store the entire usage-table
   // Also, it'd be nice to know this card is "formatted" properly...
   // So, for now, the first 256 data bytes are 0-255 in order...
   
   //Verify the card is formatted, and grab the first 256 bytes
   // (If later the usage-table is < 256 bytes, the table will be stored
   //  at the *end* of the block...)

   uint8_t cardFormatted = TRUE; //will be Falsified if appropriate

#if(defined(PRINT_USAGETABLE) && PRINT_USAGETABLE)
   print_P(PSTR("Format: "));
#endif

   //Could use do-while with uint8_t iterator?
   uint16_t i;
   for(i=0; i<(BLOCKSIZE-USAGETABLE_SIZE); i++)
   {
      uint8_t byteIn = rt_xfer(0xff);
#if(defined(PRINT_USAGETABLE) && PRINT_USAGETABLE)
      //sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), byteIn);
      //printStringBuff();
      //WOOT! This saved 8 bytes!
      printHex(byteIn);
#endif

#if(BLOCKSIZE-USAGETABLE_SIZE <= 256)
      //a/o v60:
      //Don't test bytes in the sampleRate portion of the format-header
      // (last 8 bytes)
      // But still print them, in PRINT_USAGETABLE
      if( (i<(BLOCKSIZE-USAGETABLE_SIZE-8)) && (byteIn != i) )
#else
#error "NYI"
      // Later we might want to get more sophisticated...
      // e.g. a 16G card would be ~24min / chunk
      // doesn't make sense to limit it to 256 chunks
      // and there's no reason to believe we'd cycle more than 16 times
      // so could read the usage-table as we go...
      // so each byte could indicate *two* chunks
      // and/or the space used to indicate it could be longer than 256B
      // (Maybe indicate the start of the usage-table on SD by 0 0 <size>?)
      if( byteIn != (i%8) )
#endif
         cardFormatted = FALSE;
   }

#if(defined(PRINT_USAGETABLE) && PRINT_USAGETABLE)
   print_P(PSTR("\n\rUsageTable: "));
#endif

   // Grab the data-table (either way, we have to read the rest of the 
   //  block)
   // 
   for(i=0 ; i<(USAGETABLE_SIZE); i++)
   {
      uint8_t byteIn = rt_xfer(0xff);

#if(defined(PRINT_USAGETABLE) && PRINT_USAGETABLE)
      //sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), byteIn);
      //printStringBuff();
      //WOOT! This second one saved 180-106 Bytes
      // (since sprintf_P needn't be compiled?)
      // No... 'cause that's used by main, etc.... so... wtf.
      printHex(byteIn);
#endif
/*
      if(byteIn)
         usageTable[i/8] |= (1<<(i%8));
*/
      eeprom_update_utByte((uint8_t *)i, byteIn);


      // BEWARE: thisLoopNum may be complete garbage at this point and
      // and garbagified even further, thereafter!!!
      // IF the card isn't formatted.
      // SO BE SURE to set thisLoopNum to 1 if formatting was required
      if(byteIn > thisLoopNum)
         thisLoopNum = byteIn;
   }

   //LoopNum=0 ain't no good for filling a usageTable
   // it looks empty!
   // Also, on reset, this ain't the same loop, so increment
   // Might (maybe) be handy to use the high nibble to indicate rebootNum?
   // 16 loops on a 512MB card is ~24 hours... with no usage
   // how 'bout 31 loops and 8 boots...
   //
   // With all this coding... reboots come quite often, and long-runs are
   // are relatively few... Maybe I should give more boot-counts and fewer 
   // loops... even more than 16x16... but nibbles are also easier to view
#define BOOTCOUNT_1           0x01 //0x20 //0010 0000 -> 001 00000
#define BOOTCOUNT_MASK        0xf0 //0xe0 //111 00000
#define BOOTCOUNT_SHIFTRIGHT  4 //5
#define GETBOOTCOUNT(val)     ((val)>>BOOTCOUNT_SHIFTRIGHT)
#define BOOTCOUNT_MAX         0x07
#define SETBOOTCOUNT(val)     ((val)<<BOOTCOUNT_SHIFTRIGHT)
   
   uint8_t bootCount = GETBOOTCOUNT(thisLoopNum);

   //First usage of this card... or nothing stored in previous uses
   // value 0 indicates empty... so we must start with 1
   if(thisLoopNum == 0)
      thisLoopNum = 1;
   //Less than 7 boots (including 0)
   // so increment the bootCount...
   
//main.c:1190: warning: assuming signed overflow does not occur when changing X +- C1 cmp C2 to X cmp C1 +- C2 
   //else if((bootCount + 1) <= (uint8_t)BOOTCOUNT_MAX)
   else if(bootCount <= (BOOTCOUNT_MAX-1))
   {
      thisLoopNum = SETBOOTCOUNT(bootCount+1);
   }
   //After the 7th boot, can't indicate any more, so just increment the
   // loop count, until 0xff 7-boots,32-cycles
   else if(thisLoopNum == 0xff)
   {
      //Don't Increment...
   }
   //7+ boots, incrementing loopCount
   else
   {
      thisLoopNum++;
   }


   //Discard the CRC
   rt_xfer(0xff);
   rt_xfer(0xff);


   // Odd thing... the version in which this was changed, code-size
   //  *decreased* by 6 bytes... (should have increased due to additional
   //  call to findNextUsableChunk, below...)
   // So, tested this logic, by commenting-out the #if's here...
   // same code-size
   // OLD method was a bit hokey, but even if it was invalid logic
   // the old code-size shoulda been even smaller.
#if(defined(DO_FORMAT) && DO_FORMAT)
 #if(!defined(ALWAYS_FORMAT) || !ALWAYS_FORMAT)
   if(!cardFormatted)
 #endif //Otherwise (ALWAYS_FORMAT) this is *always* entered ('if' removed)
   {
  #if(defined(PRINT_FORMATTING) && PRINT_FORMATTING)
      print_P(PSTR("Formatting..."));
  #else
      printByte('F');
  #endif
      formatCard();

      //This is already handled in formatCard!
      // So now the code-size will shrink further... so confusing
      // same size, (optimizer musta recognized duplicate) but still...
      //Make sure we have valid start-values!
      //thisLoopNum = 1;
  #if(defined(PRINT_FORMATTING) && PRINT_FORMATTING)
      print_P(PSTR("done.\n\r"));
  #endif
   }
#endif



   //Regardless of whether formatting was necessary
   // thisLoopNum is now valid 
   //    (either from original usage-table, or 1 if formatted...)
   // For now, we always boot at the beginning of the card...
   // We need this here, because eventually the memo_chunkNum will need
   //  to be initialized with thisChunkNum_u8, here...
   thisChunkNum_u8 = findNextUsableChunk(0);
   
   memoReset();
   //memoPositionInUT = thisChunkNum_u8;
   //memoChunks = 0;
   //memoMinutes = 0;
   //memoLoopNum = thisLoopNum;


   //Don't let a new memo (soon after boot) wrap around to mark
   // as though it'd already looped once since boot...
   bootLoopNum = thisLoopNum;

#if( (defined(PRINT_USAGETABLE) && PRINT_USAGETABLE) \
   ||(defined(PRINT_BOOT_LOOPandCHUNK) && PRINT_BOOT_LOOPandCHUNK) )
   sprintf_P(stringBuffer, PSTR("ln:0x%"PRIx8",cn:0x%"PRIx8"\n\r"), 
         thisLoopNum, (uint8_t)thisChunkNum_u8);
   printStringBuff();
#endif

}


static __inline__ void formatCard(void)
{
   //Insert a delay... (writing isn't working... and I forgot N_CR, etc.
   // (in readTable)
   
   //This can't be uint8_t because 256 bytes need to be written
   // for-loop below would never exit
   uint16_t i;
   for(i=0; i<16; i++)
      rt_xfer(0xff);


   // Write an empty usage-table to the eeprom
   for(i=0 ; i<(USAGETABLE_SIZE); i++)
   {
      eeprom_update_utByte((uint8_t *)((uint16_t)i), UT_UNUSED_CHUNK); //0);
   }
   
   
   // Now write that to the SD Card...
   sd_writeUsageTable();

   //Make sure we have valid start-values!
   thisLoopNum = 1;
}




/*
static __inline__ 
   int32_t addSample(cirBuff_t *adcCirBuff)
      __attribute__((__always_inline__));
*/





//sd_writeUpdate States:
//Moved above for main to use sdWU_state
#define SDWU_BOOT             0xf000
//                ->SDWU_BEGIN_WRITING
#define SDWU_BEGIN_WRITING    0xf001
//                ->SDWU_SEND_DATA_START_TOKEN
//0 -> Send Data Token
#define SDWU_SEND_DATA_START_TOKEN  0x0000
//                -> ++ (default)
//default [1->BLOCKSIZE]: Send Data Bytes
//                -> +=2 (repeat until BLOCKSIZE+1)
//                 -> +=2 (BLOCKSIZE+1)
//BLOCKSIZE+1  - Send the CRC
//                -> ++ (BLOCKSIZE+2)
//BLOCKSIZE+2  - Get the Data Accepted Response from the card
//                -> SDWU_PREP_NEXT_BLOCK 
#define SDWU_PREP_NEXT_BLOCK  0xff00
//                -> SDWU_WRITE_BUSY 
#define SDWU_WRITE_BUSY       0xff01
//                -> SDWU_SEND_DATA_START_TOKEN
//                -> SDWU_STOP_WRITING
//                -> SDWU_UPDATE_UT
#define SDWU_STOP_WRITING     0xff70
//                -> SDWU_BEGIN_WRITING 
#define SDWU_UPDATE_UT        0xfff0
//                (stops writing, writes UT)
//                -> SDWU_BEGIN_WRITING 


//Static variables are not allowed in inline functions...
// so make it global !WEEE!
// Huh, in other cases, doing this *increased* code-size...
//   this time it *shrank* by four bytes...
// Why? Maybe because it's initialized by the normal global initializer?
// but data and bss haven't changed...
// I dunno...
uint16_t sdWU_state = SDWU_BOOT; // BEGIN_WRITING;




//a/o v61
//WEIRD... Did I just completely delete the Handspring Keyboard's code from
//audioThing?
// Sure, it's dead, but delete the code completely without throwing it in
// _old or something?!



//For the keyboard -> sample...
// This is basically a one-byte buffer between reception from the
// keyboard/keypad and writing to the SD with a sample
// (Setting bytePositionToSend=1 indicates that it should be written to SD)
unsigned char rxByte;
#if (defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)

//This indicates that there is a byte to write in the sample
// header. But also indicates that the usage-table needs
// to be reloaded to the SD at the end of this block.
// (a/o v50: Is that "But also" still true? Seems hokey... or does it have
//  to do with its value being 3, as explained below?)
// (This is all handled in sd_writeUpdate() )
// a/o v61: it *is* hokey... memoUpdate() doesn't do *anything* as far as
//          recording a memo to the SD card *unless* bytePositionToSend=1
//          This is now handled via two calls:
//              memoUpdate(key==MEMO_KEY);
//              kbSample_sendChar(key);
//Track the position _to_be_written_
// e.g. 0 = nothing to be written
//      1 = low nibble
//      2 = high nibble
uint8_t bytePositionToSend = 0;

//When this is true, bytePositionToSend (as set in the sample)
//  will be set to 3 (instead of 0) for a duration to indicate that
//  a note was taken... 
// This way, the entire SD card can be searched for notes by only reading
//  a single sample out of a huge chunk (who knows how big?)
uint16_t kbIndicateCount = 0;
#endif



//Not certain this is best, but it's how it was...
// when the keyboard's "memo" key was pressed, it would (essentially) call
// this function with memoPressed=TRUE
// when another key was pressed, it would essentially call this with FALSE
// Thus, an intermediate keypress would reset the memo counter.
// THIS WAS NOT called repeatedly, e.g. in a main-loop
// ONLY when *any* key was pressed.
// BECAUSE: pressing a key (e.g. typing) adds a memo at that moment.
void memoUpdate(uint8_t memoPressed)
{
   //SEE NOTE BELOW...
   uint8_t chunksToMark = 0;

   if(memoPressed)
   {
            //When the Memo Key isn't pressed for MEMO_TIMEOUT
            // it reverts to the current chunk
            // (so pressing it again will be currentChunk -1)
            static tcnter_t lastMemoTime = 0;
            //if(thisTime - lastMemoTime > MEMO_TIMEOUT)
            if(tcnter_isItTimeV2(&lastMemoTime, MEMO_TIMEOUT, FALSE))
               memoReset();


            // This is the time the key was pressed, 
            //  Not the amount of time to be marked on the SD this go-round
            //lastMemoTime = thisTime;


            memoMinutes += MEMO_MINUTES;

            uint8_t chunksTemp = memoChunksFromMinutes(memoMinutes);

            //chunksToMark Bug:
            //   if chunks weren't marked
            //     due at least to
            //        not enough written chunks since boot
            //        (were already marked)
            //        (...?)
            //   memoChunks *includes* chunks that weren't marked 
            //      (due to above)
            //   so chunksToMark disregards whether chunks were markable
            //   SO... is this a bug?
            //   It'll run findPreviousChunk this number of times
            //      and it should return none-found each time...
            //      and memoPositionInUT/memoLoopNum should go unchanged
            //   It should be fine.
            //   Actually, it's probably better than if it were
            //      the number of chunks that weren't marked
            //      as that'd increase with each memo press
            //      and the loop would keep checking more and more
            //      nonexistent chunks.
            //   There shouldn't be any risk of missing any;
            //      even if thisChunkNum_u8 increases in the midst of a
            //      memo-pressing-venture, memoPositionInUT won't
            //      (so no shift will occur)
            //      and the current chunk will be marked regardless
            chunksToMark = chunksTemp //memoChunksFromMinutes(memoMinutes)
                           - memoChunks;
      
            //Shouldn't this be +=??
            memoChunks = chunksTemp; //chunksToMark;
#if(defined(PRINT_MEMOSIZE) && PRINT_MEMOSIZE)
            sprintf_P(stringBuffer, 
                  PSTR("m%"PRIu8
#if(!defined(PRINT_MEMOMINS_ONLY) || !PRINT_MEMOMINS_ONLY)
                     "chu%"PRIu8"ctm%"PRIu8
#endif
                     "\n\r"),
                  memoMinutes
#if(!defined(PRINT_MEMOMINS_ONLY) || !PRINT_MEMOMINS_ONLY)
                  , memoChunks, chunksToMark
#endif
                  );
            printStringBuff();
#endif
   }
   else
   {
      memoReset();
      chunksToMark = 1;
   }


//More codesize increase...
//uint8_t mput=memoPositionInUT;
//uint8_t mln=memoLoopNum;

         uint8_t i;
         for(i=0; i<chunksToMark; i++)
         {

            //Don't forget that memoPositionInUT and memoLoopNum 
            // are both updated here
            // and will need to be re-written to the original locations

            //NOTE: memoPositionInUT and memoLoopNum used here:
            // earlier code-size experiments found that copying them
            // to a new (local) variable first, then running find
            // with those, then rewriting those variables to the original
            // actually reduces code-size...
            // (but wait, isn't that now handled *in* the function?)
//Here: Increases codesize 10B
//          uint8_t mput=memoPositionInUT;
//          uint8_t mln=memoLoopNum;
            if(findPreviouslyWrittenChunk(&memoPositionInUT, &memoLoopNum))
  //          if(findPreviouslyWrittenChunk(&mput, &mln))
            {
               writeMemoToUT = TRUE;
      
               //OldNote:
               //No need to update, it's already known to be different
               //a/o v47:
               // Is that true...? What if e.g. a note was taken at time t,
               // then at time t+3 the memo-button was pressed a number of
               // times, such that it should mark all the way back to t-1?
               // Then isn't the earlier marked chunk at t going to be
               // written twice...? 
               eeprom_write_utByte((uint8_t *)((uint16_t)memoPositionInUT), 
                                 memoLoopNum); 
      
      #if(defined(PRINT_MEMOVALS) && PRINT_MEMOVALS)
               //print_P(PSTR("W:"));

               sprintf_P(stringBuffer, 
                           PSTR("p:0x%"PRIx8"l:0x%"PRIx8
                                 "ln:0x%"PRIx8"cn:0x%"PRIx8"\n\r"),
                           //memoPositionInUT, memoLoopNum,
                           memoPositionInUT, memoLoopNum,
                           thisLoopNum, (uint8_t)thisChunkNum_u8);
               printStringBuff();
      #endif
            }
//          memoPositionInUT=mput;
//          memoLoopNum=mln;
         }
//memoPositionInUT=mput;
//memoLoopNum=mln;



            //At this point, only the eeprom UT has been updated
            // Since rxByte = MEMO_KEY_RETURN, it will ALSO be treated
            // as a normal key-press
            // which causes a mark at the current position
            // (which was *not* handled above)
            // So... since the eeprom has been updated, when the table is
            // written, it will also include the latest UT update from Memo

            //a/o v47:
            // Appears to be marking *two* chunks, with each keypress...
            // the current, and one prior. With the 512MB card, this made
            // sense, each chunk was only 52sec, and it was likely a
            // keypress might occur soon after a boundary...
            // But with the 8GB card, each chunk is 13minutes
            // So marking *two* chunks is absurd.
            // Might be best to assign a certain number of minutes, instead
            // AND then check whether those minutes cross-over into a
            // previous chunk (does it do that now?) e.g. keypress at
            // minute 14, 1min marking goes back to minute 13, so both
            // chunks would be marked... whereas keypress at minute 13,
            // one minute marking goes back to minute 12, same chunk...
            // only the first chunk would be marked.

            // BUT: even 13 minutes of marking is absurd... so something to
            // contemplate.
}



//This is still somewhat horribly named...
// better might be hssKB_handler_update()?
#if(defined(PUART_ECHO) && PUART_ECHO)
#error "puartEchoUpdate() should be revised and renamed... see nkp_update()... but it *may* work if these #errors are removed"
#include "_old/puartEchoUpdate.c"
#endif


#if(defined(PRINT_AVAILABLECHUNKS) && PRINT_AVAILABLECHUNKS)
void print_availableChunks(void)
{

      static uint8_t lastAvailableChunks;
      if(lastAvailableChunks != availableChunks)
      {
         lastAvailableChunks = availableChunks;

         sprintf_P(stringBuffer, PSTR("AC:%"PRIu8" "), availableChunks);
         printStringBuff();
      }

}
#else
 #define print_availableChunks()
#endif


#if(defined(__NLCD_H__))
void updateNLCD(void)
{
      if(nlcd_charactersChanged()
#if(defined(WRITING_SD) && WRITING_SD)
         && (sdWU_state == SDWU_WRITE_BUSY)
#endif
         )
      {
         setpinPORT(SD_CS_pin, SD_CS_PORT);
         nlcd_redrawCharacters();
         clrpinPORT(SD_CS_pin, SD_CS_PORT);
      }

}
#else
 #define updateNLCD()
#endif




//Trying a new tactic to make code more readable...
//Except it's no longer necessary in this case
//I'll leave it here for consideration in other such cases
/*
#if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
 #define O_clearWriteBusyCount() (writeBusyCount=0)
#else
 #define O_clearWriteBusyCount() 
#endif
*/


#if (defined(WRITING_SD) && WRITING_SD)
void writingSD(void)
{
      static tcnter_t lastTime = 0 ; //tcnter_get();

   #if(defined(BUTTON_IN_SAMPLE) && BUTTON_IN_SAMPLE)
      anaComp_update();
   #endif


   #if(!defined(LC_FC_WBC_BW_DISABLED) || !LC_FC_WBC_BW_DISABLED)
      {
      
         static uint32_t loopCount = 0;
   
         if(tcnter_isItTime(&lastTime, 1*TCNTER_SEC)) //250000))
         {
            //This is confusing... and probably the reason for v51-12
            //through v53:
            //if(fullCount)
            {
               //SEE NOTES re: printout affecting full-count... (v51ish)
               cli();
      #if(defined(PRINT_LCandFC) && PRINT_LCandFC)
               //char string[40];
               sprintf_P(stringBuffer, 
                           PSTR("lc:%"PRIu32", fc:%"PRIu32", "), 
                           loopCount, fullCount);
               printStringBuff();
      #endif
      #if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
               sprintf_P(stringBuffer, 
                           PSTR("wbc:%"PRIu32" bw:%"PRIu32"\n\r"),
                           writeBusyCount, blocksWritten);
               printStringBuff();
               writeBusyCount = 0;
      #endif
#warning "This could be removed if sendStringBlocking handled it..."
               stringBuffer[0] = '\0';
               fullCount = 0;
               sei();
            }
            loopCount=0;
         }
         loopCount++;         

//       cirBuff_get(&adcCirBuff);
   #endif
         sd_writeUpdate();
   #if(!defined(LC_FC_WBC_BW_DISABLED) || !LC_FC_WBC_BW_DISABLED)
      }
   #endif


}

#elif (defined(ADC_PASSTHROUGH) && ADC_PASSTHROUGH)
void adcPassthrough(void)
{
      int16_t adcVal = adcFR_get();

      if(adcVal > 0)
      {
//a/o v50:
//This is probably awkward, here, since AS I RECALL ADC_PASSTHROUGH
// basically disables everything else... right...?
//Does it even work anymore?
//Anyhow, adding this a/o v50:
#if(defined(AUDIO_OUT_ENABLED) && AUDIO_OUT_ENABLED)
      #ifdef __AVR_ATmega328P__
         OCR2B = adcVal >> 2;
      #else
         //1:1 would be TC1H=(uint8_t)(adcVal>>8); OCR1D=(uint8_t)adcVal;
         TC1H = (uint8_t)(adcVal>>6);
         OCR1D = (uint8_t)(adcVal<<2);
      #endif
#endif
      }
}

#else
void oldOldCirBufADCTesting(void)
{
#if 0
      if(thisTime >= nextSampleTime)
      {
         cirBuff_dataRet_t gotData;
         gotData = cirBuff_get(&adcCirBuff);
//int32_t gotData = addSample(&adcCirBuff);
         //Hopefully this should only happen a few times in the beginning
         if(gotData != CIRBUFF_RETURN_NODATA)
         {
            uint16_t gotSample = gotData;

            TC1H = (gotSample >> 8);
            OCR1D = (uint8_t)(gotSample);
      
            //It shouldn't happen often, but we should get a sample
            // immediately as it's available, I guess... 
            nextSampleTime = thisTime + SAMPLE_TCNT;

            //char string[20];
            sprintf_P(stringBuffer, PSTR("gotSample 0x%"PRIx16"\n\r"),
                                                gotSample);
            printStringBuff();   

            //lastSample = -1;
         }
         
      }

//    if(lastSample < 0)
//       lastSample = addSample(&adcCirBuff);
      
      if(cirBuff_availableSpace(&adcCirBuff))
      {
         addSample(&adcCirBuff);
      }
#endif
}
#endif



#if (defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
//NEITHER OF THESE SHOULD BE IN INTERRUPTS.
static int16_t kbBuffer = -1;

void kbSample_sendChar(char character)
{
   kbBuffer = character;
}

//a/o v61: kbSample_update() needs to be called in main
// this checks if there's data to be sent along with samples
// and sends them to sdWU() when it's ready...
// There's a *one byte* buffer, here... which should be plenty for most
// keyboards/typists. BUT e.g. maybe not enough if there's two inputs
// e.g. heartButton *and* NKP...
void kbSample_update(void)
{
   //This was stolen from nkp_update()
   //rxByte should only be updated when bytePositionToSend == 0
   // but what if we receive one when another's still loading?
   // realistically, shouldn't happen much. But not a bad idea to back it
   // up.
   // Probably would be wiser to handle this at the receiving end...
   // but that's NYI.
   // -1 indicates there's nothing waiting.
   //static int16_t rxBuffer = -1;

   //(Shouldn't this be kbBuffer>=0?)
   if((kbBuffer>0) && (bytePositionToSend == 0))
   {
      rxByte = kbBuffer;
      bytePositionToSend = 1;
      kbBuffer = -1;
   }
     

}
#endif

#if(defined(NKP_ENABLED) && NKP_ENABLED)
/* Measurements:
   key   buttonTimeVal  (mbv)
   C     147-151        19
   ---   340-344        43
   Left  275-279        35
   Right 255-259        32-33
   1     170-173        22
   2     383-388        48-49
   3     235-239        30
   4     190-196        24-25
   5     405-409        51-52
   6     84-88          11
   7     
   8     (row 789 appears to have a broken connection)
   9
   *     127-131        16-17
   0     41-45          6
   #     19-25          3-4


   I don't quite understand... the same circuit, the same values for FCPU
   and CLKDIV for the tcnter... yet the values measured are different than
   those found in anaButtons/0.50/testTiny861+NonBlocking/
*/

void nkp_update(void)
{

   int32_t buttonTimeVal = anaButtons_getDebounced();

   //Button was released...
   if(buttonTimeVal >= 0)
   {
      uint8_t mbv = (buttonTimeVal + 7) / 8;
      char key;

      //The minimum value measured is 15...
      //minButtonTime = (minButtonTime + 7) / 8;

      //These duplicate values could be removed by dividing by 16
      // apparently. 
      switch(mbv) //minButtonTime)
      {
               //case 2:
               case 3:
               case 4:
                  key='H'; //#
                  break;
               //case 4:
               //case 5:
               case 6:
                  key='0';
                  break;
               //case 13:
               case 16:
               case 17:
                  key='X'; //*
                  break;
               //case 22:
               case 28:
                  key='7';
                  break;
               //case 43:
               //case 44:
               case 55:
                  key='8';
                  break;
               //case 11:
               case 20:
                  key='9';
                  break;
               //case 20:
               case 24:
               case 25:
                  key='4';
                  break;
               //case 41:
               case 51:
               case 52:
                  key='5';
                  break;
               //case 9:
               case 11:
                  key='6';
                  break;
               //case 17:
               //case 18:
               case 22:
                  key='1';
                  break;
               //case 39:
               case 48:
               case 49:
                  key='2';
                  break;
               //case 24:
               case 30:
                  key='3';
                  break;
               //case 15:
               case 19:
                  key='C';
                  break;
               //case 35:
               case 40:
               case 41:
               case 42:
               case 43:
               case 44:
               case 45:
               case 46:
                  key='I'; // ----
                  break;
               //case 28:
               //case 29:
               case 35:
                  key='L'; // <
                  break;
               //case 26:
               case 32:
               case 33:
                  key='R'; // >
                  break;
               default:
                  key='?';
                  break;
      }

 #if (defined(PRINT_NKP_VALS) && PRINT_NKP_VALS)
      sprintf_P(stringBuffer,
            PSTR("NKP%"PRIi32"=%"PRIu8"="),
            buttonTimeVal, mbv);
      printStringBuff();

 #endif
      printByte(key);
      printByte(' ');

      memoUpdate(key=='I');

#if(defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
      kbSample_sendChar(key);
      //rxBuffer = key;
#endif
   }

}
#endif
















int main(void)
{
// adcFR_init();
   //This should be OK now that HEART_DMS=FALSE...
   // which is no longer relevent anyhow...
   heart_init();
   heart_setRate(0);  //Override WDT indication, etc.
// dmsWait(5*DMS_SEC);
   //haltError(0); //0x12);

// pauseIndicate(5);
// pauseIndicate(0x12);
// haltError(0);


   setoutPORT(Tx0pin, Tx0PORT);
   tcnter_init();

   //250000Hz / 4800 = 52.08
   //puar0_bitTcnt = 
   // Let's try 9600
//#define BIT_TCNT_LOCAL   26 //52
// puar_setBitTcnt(0, BIT_TCNT_LOCAL); //26); //52);
#if(!defined(PUAR_DISABLED) || !PUAR_DISABLED)
   puar_init(0);
#endif
// puat_setBitTcnt(0, BIT_TCNT_LOCAL); //26); //52);

#if(defined(__POLLED_UAT_H__))
   puat_init(0);
#endif

#if(defined(PUAT_STRINGERS) && PUAT_STRINGERS)
   printByte('!');
#endif

#if(defined(_DMSTIMER_HEADER_))
   dmsWait(DMS_SEC);
#else
   tcnter_wait(TCNTER_SEC);
#endif


/*
   while(1)
   {
      tcnter_update();
      puar_update(0);
      puat_update(0);

      int16_t rxByte = -1;
      if(puar_dataWaiting(0))
         rxByte = puar_getByte(0);

      //Echo...
      if(rxByte >= 0)
         printByte(rxByte);

      //Might interfere with puart...
      heart_update();
   }

   _delay_ms(2000);
*/
#if(PRINTER==PRINT_NLCD)
   nlcd_init();
#endif


   //Might have to be after spi_sd_init()...

   setpinPORT(SD_CS_pin, SD_CS_PORT);  


#if(defined(NLCD_BOOT_TEST) && NLCD_BOOT_TEST)
   //nlcd_gotoXY(5,3);
   //char string[] = "Hello World";
   //char *character = string;
   //dms6sec_t startTime = 0;
   //uint8_t character = '1';
   //while(1) //*character != '\0')
   //{
      //nlcd_drawChar(*character);
   // nlcd_appendCharacter(character++);

   // _delay_ms(1000);
   //}

   //drawChar does not work with redrawCharacters...
   // redrawCharacters *should* be called periodically in main, no?
   // It ain't happening.
   nlcd_appendCharacter('N');
   //These either don't work, or are overwritten...
   //nlcd_drawChar('E');
   //nlcd_drawChar('L');
   //nlcd_drawChar('L');
   //nlcd_drawChar('O');
   print_P(PSTR("LCD init"));
   nlcd_redrawCharacters();
#endif

#define PRINT_SAMPLERATE TRUE
#if(defined(PRINT_SAMPLERATE) && PRINT_SAMPLERATE)
   //print_P(sampleRateString);
   sprintf_P(stringBuffer, PSTR("%"PRIu16"S/s\n\r"),
                                           (uint16_t)SAMPLE_RATE);
   printStringBuff();
#endif


#define PRINT_SD_INIT   TRUE
#if(defined(PRINT_SD_INIT) && PRINT_SD_INIT)
   print_P(PSTR("\n\n\rsd_init()\n\r"));
#endif

   spi_sd_init();


   clrpinPORT(SD_CS_pin, SD_CS_PORT);

   spi_sd_readTable();

#if(defined(AUDIO_OUT_ENABLED) && AUDIO_OUT_ENABLED)
   pwmTimer_init();
#endif

   //heart_init();

// heart_setRate(0);  

   cirBuff_init(&adcCirBuff, ADC_CIRBUFF_SIZE, arrayForADCCirBuff);

   //Now handled by writeUpdate's init-state.
   //spi_sd_startWritingBlocks();

// spi_sd_startReadingBlocks();
#define INT10_MAX    511
   //uint16_t lastSample = INT10_MAX;

/*
   //Fill up the buffer before-hand... might help...
   while(cirBuff_availableSpace(&adcCirBuff))
   {
      addSample(&adcCirBuff);
   }  
*/
#if(defined(PRINT_GO) && PRINT_GO)
   print_P(PSTR("\n\n\rGo!\n\r"));
#elif (defined(PRINT_GO_RETURN) && PRINT_GO_RETURN)
   print_P(PSTR("\n\r"));
#endif


// tcnter_update();


#if(defined(_DMSTIMER_HEADER_))
#if(defined(DISABLE_DMS_INTERRUPT) && DISABLE_DMS_INTERRUPT)
   //This added in v5-17 (not in v5-16)
   // 5-16 "first go - sound output"
   // 5-17 is not labelled
   // And there doesn't seem to be any further information about why I did
   // this. As I recall, there *might* have been a popping sound/squeel
   // Not sure if this affected recording.
   // A/O v40: have yet to test this again... TODO
   //Disable the DMSTimer interrupt for playback...
   timer_compareMatchIntSetup(0, OUT_CHANNELA, FALSE);

   //A/O v50: I really don't remember what this was for, audio-out, maybe,
   //but why'd I not use timerCommon to initialize AudioOut? And then use
   //it here...? Ohhh, is this for the DMS-timer? Which has been long-since
   //disabled? Because the DMS interrupt was interfering with sound-output?

#error "oddthing"
#endif
#endif
   int32_t lastSample = -1;

   adcFR_init();

   setoutPORT(HEART_PINNUM, HEART_PINPORT);
   
   //?! a/o v43: Trying to remove PUART_ECHO necessity for
   //dang-near-everything, including update of nlcd...
   // boiled down to that sei() wasn't enabled... 
   // The only thing that *should* care about this is adc, no?
   // (dms has been removed)
   // Possibly the buffer was never being written to SD, so it was never
   // WRITE_BUSY...?
   sei();

   while(1)
   {
      tcnter_update();

      heart_update();
      
      //Use the heartbeat's pushbutton for the 'memo' key
      // (in addition to any that may be implemented elsewhere, e.g. on the
      //  Nokia's keypad).
      static uint8_t lastHeartButtonVal;

      //BUT we only want a 'memo' *once* for each *press*, 
      // not once for each *loop*
      //
      //NOTE: This does *not* handle bounce.
      // heart_getButton has been found to be relatively bounce-free
      // since the heartbeat LED uses the same port, it helps smooth thing
      // up a bit, and also prevents uber-high-speed sampling
      // (the pin is only polled *once per loop*, and currently we're
      // running about Whoa WTF... DUHHH!!

//#warning "heart_getButton interferes with heart, and is slow"
      uint8_t heartButton = heart_getButton();

      if(!lastHeartButtonVal && heartButton)
      {
         //TODO: This is a bit hokey, memoUpdate doesn't really do anything
         //except calculations unless kbSample_sendChar() is also called
         memoUpdate(TRUE);

#if(defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
         kbSample_sendChar('*');
#endif
      }       
      lastHeartButtonVal = heartButton;




      //Many of the following are *options*
      // their function-definitions may be nada depending on #defines...
      print_availableChunks();

#if(PRINTER==PRINT_NLCD)
      //Redraw the screen when necessary (and able)
      updateNLCD();
#endif

#if(defined(PERIODIC_PRINT_TEST) && PERIODIC_PRINT_TEST)
      static uint8_t character='A';
      static tcnter_t startTime=0;

      if(tcnter_isItTimeV2(&startTime, 1*TCNTER_SEC, FALSE))
         printByte(character++);
#endif
      
      //This basically handles everything-keyboard-related
      // Poorly named...
#if(defined(PUART_ECHO) && PUART_ECHO)
#error "PUART_ECHO/puartEchoUpdate() is ugly and should be replaced. nkp_update() is better-implemented, but not a direct-replacement"
      puartEchoUpdate();
#endif

#if(defined(NKP_ENABLED) && NKP_ENABLED)
      nkp_update();
#endif


#if (defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
      kbSample_update();
#endif
      //THESE ARE MUTUALLY EXCLUSIVE
      // Basically it should always be WRITING_SD, except for testing...
#if (defined(WRITING_SD) && WRITING_SD)
      writingSD();
#elif (defined(ADC_PASSTHROUGH) && ADC_PASSTHROUGH)
      adcPassthrough();
#else
      oldOldCirBufADCTesting();
#endif

   }
}



#if (defined(WRITETIMED) && WRITETIMED)
 #define w_xfer(a) spi_sd_transferByteWithTimer(a)
#else
 #define w_xfer(a) spi_sd_transferByte(a)
#endif




// If startingChunkNum is available, it will return it!
// So be sure to increment it beforehand if that's not desired
// There are a few cases:
//  1) Upon boot
//       findNextUsableChunk(0) may return 0
//  2) At the end of the card
//       thisChunkNum_u8 = 0xff, so increment it to 0
//       findNextUsableChunk(++thisChunkNum_u8)
//          may return 0, as well
//  3) At the end of a fragment e.g.
//       thisChunkNum_u8 = 5, increment it to 6 (which is known to be used)
//       findNextUsableChunk(++thisChunkNum_u8)
//          may come 'round to 20, who knows...
// DO NOT want to increment it internal to this function, because of case 1
// Further, it's *likely* there was no keypress in this chunk, so...
//  testing a just-written chunk is not a good idea
//  (e.g. just finished writing the last chunk in a fragment, chunk 0x70
//    so findNextUsableChunk(0x70) will return 0x70)
static __inline__ uint8_t findNextUsableChunk(uint8_t startingChunkNum)
{
   uint8_t chunkNum = startingChunkNum;

   //CURRENTLY: will halt until an empty block is found...
   // which may not happen! (the entire card has been used)
   while(0 != eeprom_read_utByte((uint8_t *)((uint16_t)chunkNum)))
   {
      //This could just be ++'d if it weren't for this old note:
      //(Something about using blockPosition+=chunkSize in the works
      // instead of using multiplication, below...)
      // FURTHER: chunkNum CAN'T be > USAGETABLE_SIZE here...
      //if(thisChunkNum_u8 >= (USAGETABLE_SIZE))
      // thisChunkNum_u8 = 0;
      //else
      // thisChunkNum_u8++;
      chunkNum++;
   }

   return chunkNum;
}


#if(PRINTER==PRINT_NLCD)
#if(!defined(DISABLE_PRINTANYWHERE) || !DISABLE_PRINTANYWHERE)
void printAnywhere(uint8_t charPos, const char *__fmt, uint8_t val)
{
   uint8_t lastPos;
   lastPos = nlcd_setCharNum(charPos);
   sprintf_P(stringBuffer, __fmt, val);
   printStringBuff();
   nlcd_setBufferPos(lastPos);
}
#else
void printAnywhere(uint8_t charPos, const char *__fmt, uint8_t val)
{
   //Doesn't *look* like this will be much smaller, but setCharNum is
   //inline... and likely only used here...
   //36 bytes! Weee!
   //uint8_t lastPos;
   //lastPos = nlcd_setCharNum(charPos);
   sprintf_P(stringBuffer, __fmt, val);
   printStringBuff();
   //nlcd_setBufferPos(lastPos);
}
#endif
#endif

//Advance thisChunkNum... So far it's only called in WriteUpdate's BusyState
// But that doesn't really make sense, a new state would make more sense
// Returns TRUE if the new chunk is consecutive with the last
//   (e.g. FALSE if the next consecutive chunk is in use OR
//               if the end of the card was reached, and wraparound was necessary)
static __inline__ uint8_t advanceThisChunkNum(void)
{
   //Used for testing whether we need to stop and jump
   uint16_t nextConsecutiveChunk_u16 = (uint16_t)thisChunkNum_u8 + 1;

   //This will wrap-around at the end of the card to 0
   uint8_t nextUsableChunk_u8 = findNextUsableChunk((uint8_t)(nextConsecutiveChunk_u16));

   thisChunkNum_u8 = nextUsableChunk_u8;


  #if(defined(PRINT_CHUNKADVANCE) && PRINT_CHUNKADVANCE)
   #if(defined(__NLCD_H__))
   //uint8_t lastPos;

   //lastPos = nlcd_setCharNum(0);
   printAnywhere(0, PSTR("%"PRIx8" "), thisChunkNum_u8);

   #else
   sprintf_P(stringBuffer, PSTR("cn0x%"PRIx8"  "), (uint8_t)thisChunkNum_u8);
   printStringBuff();
   //#if(defined(__NLCD_H__))
   //NLCD_SetCharNumFromEnd(0);
   //nlcd_setBufferPos(lastPos);
   #endif
  #endif


   //Check if the next chunk is available...
   // This must fail if we're at the end of the card!
   //Using u16 assures that when nextUsable wraps to zero
   // this will also fail for wraparound (if it was u8, 0 == 0 doesn't
   // fail on wraparound, here 256 != 0)
   if((nextConsecutiveChunk_u16 == (uint16_t)nextUsableChunk_u8))
      return TRUE;
   else
      return FALSE;
}


//This should only be accessed by SD_WriteUpdate
// one write, at the end of a block
// and one read at the beginning of the next
uint8_t newBlockIsConsecutive = FALSE;

//Called after a block has been completed
//  Updates as necessary:
//    thisChunkNum
//    blockNum_inThisChunk
//    thisLoopNum
//  Also writes newBlockIsConsecutive as appropriate
//     (False at end of fragment, end of card)
static __inline__ void setupNextBlock(void);





/* No Longer Used
//Returns the first block number in chunkNum
// NOTE: for chunkNum 0, we return blockNum=1 so as not to overwrite
//       the usage-table...
//       This can be tested-against if needed
//       (e.g. since a chunk is determined to be complete when 
//        a block-counter reaches numBlocksPerChunk
//        it would be necessary to: 
//           if(startingBlockNumFromChunk() == 1)
//                blockCounter = 1;  
static __inline__ uint32_t startingBlockNumFromChunk(uint8_t chunkNum)
{
   //Don't overwrite the usage-table!
   if(chunkNum == 0)
      return 1;
   else
      return (uint32_t)chunkNum * numBlocksPerChunk;
}
*/

//This calculates a block number from a Chunk Number and blockOffset
//  Generally:
//   chunkNum = thisChunkNum
//   blockOffset = &blockNum_inThisChunk (usually 0)
//
// If *blockOffset == 0 and chunkNum == 0
//   *blockOffset should be incremented, to prevent overwriting the
//    usage-table...
static __inline__ uint32_t blockFromChunkAndOffset(uint8_t chunkNum, uint32_t *blockOffset)
{
   //Don't overwrite the usage-table
   //chunkStartBlock is 0 if chunkNum = 0
   // If *blockOffset also == 0, then we must be in an init-state
   //  blockOffset should be incremented, accordingly
   if((chunkNum == 0) && (*blockOffset == 0))
   {
      *blockOffset = 1;
      return 1;         
   }


   //In all other cases, we assume that chunkNum and *blockOffset
   // are as-desired:
   // *blockOffset = 0 in other cases means we're starting a new chunk
   //              != 0 means we're resuming a stopped write in the middle of a chunk
   // Neither case needs to be handled specially

   uint32_t chunkStartBlock;

   chunkStartBlock = (uint32_t)chunkNum * numBlocksPerChunk;
                     //startingBlockNumFromChunk(chunkNum);
                     // just makes things complicated.

   return chunkStartBlock + *blockOffset;
}

// Finds the previously-written chunk, regardless of whether it was marked
//
// Will stop when no previously-written chunks are found
//   e.g. immediately after boot (assuming a previously-empty card)
//            we're on chunk 5
//            we attempt to call this 6 times
//            it will output 4 the first time, and return TRUE
//              ... 3, 2, 1, 0
//            on the 6th call and any following, it will return FALSE
//   e.g.2. We've recently reached the end of the card and wrapped around
//            (again, assuming this is the first boot...)
//            we're on chunk 2
//            we call this 5 times
//            it'll output 1, 0, 255, 254, 253
// ALSO It skips chunks which were skipped while writing...
//   e.g. we're on loop 2, at chunk 5, chunk 3 was marked in loop 1...
//            so loop 2's path was 0, 1, 2, -> 4, 5
//            so calling this function once outputs 4
//            twice outputs 2 (NOT 3)
//
// This returns
//   If the found Chunk is unmarked:
//    1 if a previously-written chunk was found
//    //2 if it was found in a previous loop (chunkNum wraped from 0->255)
//
//   FALSE 
//      if not found (e.g. we're writing the first chunk this boot)
//      if the found chunk has already been marked...
//
// e.g.
//    uint8_t chunkTemp = thisChunkNum_u8;
//    if(findPreviouslyWrittenChunk(&chunkTemp))
//       ... markAt(chunkTemp)
//
// NOTE: This function does directly indicate whether it had to
//       loop back (e.g. from 0 to 255) by returning 2
// CRAP. Need loopNum to halt testing when it's gone too far...
// NOTE: It works *directly* with chunkNum and loopNum, internally
//       Both values will be updated regardless of whether a chunk was
//         found... This way, if a limit was reached, consecutive calls
//         will not have to re-run the search-loop.
//         Those same values will be tested and fail again next time
extern __inline__ 
uint8_t findPreviouslyWrittenChunk(uint8_t* chunkNum, uint8_t* loopNum)
{
   //Working with Temp variables instead of the actual pointers
   // caused the code-size to drop down to what it was pre-functionifying
   // (8160 -> 8146)
   uint8_t chunkTemp = *chunkNum;
   uint8_t loopTemp = *loopNum;

   //Assume one will be found... if not, this'll be changed to indicate
   uint8_t retVal = 1;

   //This is the located-chunk's value in the usage-table
   uint8_t ut; 

   do
   {
      //Wrap around, and decrement chunkTemp
      // This doesn't handle when booting
      // E.G. 5 min in after boot memo-pressed for 10min, 
      //  I THINK THIS IS AN OLD NOTE, I"M PRETTY SURE IT DOES...
      if(chunkTemp == 0x00)
      {
         // Don't attempt to wrap-around to the end of the usage
         // table, if it just booted...
         // Also doesn't make sense to loop twice... NYH
         if(loopTemp > bootLoopNum)
         {
            chunkTemp = 0xff;
            loopTemp--;
            //retVal = 2;
         }
         else
         {
            //return FALSE;
           retVal = FALSE;
           break;
         }
      }
      else
         chunkTemp--;

   
      //Check whether this chunk has been written in this loop
      // (marked with this loop number, or 0)
      ut = eeprom_read_utByte((uint8_t *)((uint16_t)chunkTemp));

      //AKA ALSO return chunks that have already been marked in this loop
      if(ut == loopTemp)
      {
         //But don't write them, either.
         //return FALSE;
         retVal = FALSE; //writeMemoToUT = FALSE;
         break;
      }

   //Skip non-zero utVals
   } while( (ut != 0) ); //&& (ut != memoLoopNum) );


   *chunkNum = chunkTemp;
   *loopNum = loopTemp;
   //If it didn't return yet, then we musta found one that was unmarked
   return retVal; //TRUE;
}



//Doesn't seem to affect the problems encountered...
//See SDWU_WRITE_SEVERAL_SAMPLES's definition/notes
__inline__ cirBuff_dataRet_t cirBuff_getSafe(cirBuff_t *cb)
{
   cirBuff_dataRet_t retVal;
   cli();
   retVal = cirBuff_get(cb);
   sei();
   return retVal;
}




//sd_writeUpdate States:
//Moved above for main to use sdWU_state
/*
#define SDWU_BOOT             0xf000
//                ->SDWU_BEGIN_WRITING
#define SDWU_BEGIN_WRITING    0xf001
//                ->SDWU_SEND_DATA_START_TOKEN
//0 -> Send Data Token
#define SDWU_SEND_DATA_START_TOKEN  0x0000
//                -> ++ (default)
//default [1->BLOCKSIZE]: Send Data Bytes
//                -> +=2 (repeat until BLOCKSIZE+1)
//                 -> +=2 (BLOCKSIZE+1)
//BLOCKSIZE+1  - Send the CRC
//                -> ++ (BLOCKSIZE+2)
//BLOCKSIZE+2  - Get the Data Accepted Response from the card
//                -> SDWU_PREP_NEXT_BLOCK 
#define SDWU_PREP_NEXT_BLOCK  0xff00
//                -> SDWU_WRITE_BUSY 
#define SDWU_WRITE_BUSY       0xff01
//                -> SDWU_SEND_DATA_START_TOKEN
//                -> SDWU_STOP_WRITING
//                -> SDWU_UPDATE_UT
#define SDWU_STOP_WRITING     0xff70
//                -> SDWU_BEGIN_WRITING 
#define SDWU_UPDATE_UT        0xfff0
//                (stops writing, writes UT)
//                -> SDWU_BEGIN_WRITING 


//Static variables are not allowed in inline functions...
// so make it global !WEEE!
// Huh, in other cases, doing this *increased* code-size...
//   this time it *shrank* by four bytes...
// Why? Maybe because it's initialized by the normal global initializer?
// but data and bss haven't changed...
// I dunno...
uint16_t sdWU_state = SDWU_BOOT; // BEGIN_WRITING;
*/
// And globalling these two had no code-size effect...
uint8_t sdWU_dataState = 0;
cirBuff_dataRet_t sdWU_sample;

//extern
static __inline__ void sd_writeUpdate(void)
{
   //This is the byte number *in the packet* (including the header and CRC)
   // which HAS BEEN SENT
   // e.g. if packetByteNum = 0, nothing has been sent (in this packet)
   //         packetByteNum = 1, Data Token has been sent
   //                         3, Data Token + Two Data Bytes have been sent
   // so CRC starts at 1+512 (BLOCKSIZE+1)
   // AFTER the CRC is sent, and whatnot, then this'll be SDWU_WRITE_BUSY
   // until it's no longer busy and we return it to 0
   //static uint16_t packetByteNum = 0;

   switch(sdWU_state)
   {
      case SDWU_BOOT:
         thisChunkNum_u8 = findNextUsableChunk(0); 
         //This'll be incremented in SDWU_BEGIN_WRITING if thisChunkNum=0...        
         blockNum_inThisChunk = 0;

         sdWU_state = SDWU_BEGIN_WRITING;

         break;
#warning "SDWU_STOP_WRITING isn't very happy as a state-machine state..."
      //ONLY at the END of the SD Card/fragment
      // OR in the special-case where the usage-table has to be updated
      case SDWU_STOP_WRITING:
         //Stop Transmission Token:
         //printBuff("Stop Transmission: 0xfd\n\r");
         w_xfer(0xfd);

         //What about the busy state... ?
         // It's handled in SDWU_BEGIN_WRITING...

         sdWU_state = SDWU_BEGIN_WRITING;
         //Intentional fall-through

      //Moved here, to do calcs during busy (probably unnecessary, but
      // shouldn't hurt...
      case SDWU_BEGIN_WRITING:
         {
            //After StopTransmission is sent, a "busy" will occur 1 byte later
            // (according to ELM)
      
            // While waiting for the BUSY, we can determine the next position
         
            //At this point thisChunkNum AND blockNum_inThisChunk should be set
            // (though blockNum_inThisChunk may be 0 and need to be incremented)
            //  We could've gotten here in a few cases:
            //    After SDWU_BOOT
            //       thisChunkNum = First Available On Card
            //       blockNum_inThisChunk = 0 (may be incremented)
            //    After a normal SDWU_STOP_WRITING (end of fragment/card)
            //       thisChunkNum = Next Available
            //       blockNum_inThisChunk = 0 (may be incremented)
            //    After a usageTable Update (NOT at the end of a chunk)
            //       thisChunkNum  hasn't changed
            //       blockNum_inThisChunk has been incremented
            //    After a usageTable Update (At the end of a chunk)
            //       thisChunkNum = Next Available
            //       blockNum_inThisChunk = 0 (may be incremented)
            //
            // In ALL cases
            //    thisChunkNum is writable
            //    blockNum_inThisChunk is
            //       writable
            //       AND/OR ZERO (which may be incremented)
            // 
            // SDWU_BEGIN_WRITING NO LONGER verifies the chunk's availability
            //   that should be handled before-hand

           #if(defined(PRINT_CHUNKREENTRY) && PRINT_CHUNKREENTRY)
            sprintf_P(stringBuffer, PSTR("cr:0x%"PRIx8), (uint8_t)thisChunkNum_u8);
            printStringBuff();
           #endif

            //Calculate the block number
            uint32_t startBlock =
               blockFromChunkAndOffset(thisChunkNum_u8, &blockNum_inThisChunk);

#warning "These are WithTimer transactions!"

//This only occurs when the usage-table is updated (or at the end of card)!
// which only occurs once per chunk, at most.
// 
/*
#if(defined(__NLCD_H__))
            setpinPORT(SD_CS_pin, SD_CS_PORT);
            nlcd_redrawCharacters();
            clrpinPORT(SD_CS_pin, SD_CS_PORT);
#endif
*/
            //This should grab the BUSY... (from SDWU_STOP_WRITING)
            spi_sd_getRemainingResponse(NULL, 0, 0);

            spi_sd_startWritingBlocks(startBlock);
         }
         sdWU_state = SDWU_SEND_DATA_START_TOKEN; //0;
         break;
      //Send the CRC (unused, but two bytes must be sent)
      case (BLOCKSIZE+1):
         {
            //Two unverified CRC bytes
            w_xfer(0x00);
            w_xfer(0x00);
            sdWU_state++;
      
#if(defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)   
            //Decrement after each block...
            if(kbIndicateCount)
               kbIndicateCount--;
#endif
         }
         break;
      //Verify data was received (after CRC)
      case (BLOCKSIZE+2):
         {
            //SD should respond "Immediately" with 0bxxx00101
            // to indicate that the data was accepted
            uint8_t dataResponse = w_xfer(0xff);
            //Check that the data was accepted
            if((dataResponse & 0x1f) != 0x05)
            {
              #if(defined(PRINT_BAD_DATARESPONSE) && PRINT_BAD_DATARESPONSE)
               #if(defined(__NLCD_H__))
               static uint8_t bdrCount;
               bdrCount++;
               printAnywhere(7, PSTR("BDR%"PRIu8"    "), bdrCount);
               //uint8_t lastPos;
               //lastPos = nlcd_setCharNum(7);
               //sprintf_P(stringBuffer, PSTR("BDR%"PRIu8"   "), bdrCount);
               //printStringBuff();
               //nlcd_setBufferPos(lastPos);
               #else
               //char string[35];
               sprintf_P(stringBuffer,
                     PSTR("Bad Data Resp1: 0x%"PRIx8"\n\r"),
                     dataResponse);
               printStringBuff();
               #endif
              #endif
#if(defined(HALT_ON_BAD_DATARESPONSE) && HALT_ON_BAD_DATARESPONSE)
               haltError(0x0f);
#else
#warning "HALT_ON_BAD_DATARESPONSE IS NOT TRUE"
#endif
            }

         //TESTING ONLY
            //Print out the write-response
            // Seems good every time!
            // Guess the state-machine is working... surprised I haven't
            // gotten *any* errors above
            /*
            {
               cli();
               char string[35];
               sprintf_P(string, PSTR("wr: 0x%"PRIx8"\n\r"), dataResponse);
               printBuff(string);
               sei();
            } */

            //Writing doesn't begin until another 8 clocks are received
            // It should respond with 0x00 that it's busy, but we'll just
            // assume that's the case...
            w_xfer(0xff);
            //So technically we're just using sdWU_state 
            //as a state-indicator now...
            sdWU_state = SDWU_PREP_NEXT_BLOCK; //SDWU_WRITE_BUSY;
            //writeBusyCount = 0;
         }
         break;
      case SDWU_PREP_NEXT_BLOCK:
         //Advance blockNum_inThisChunk
         //        -> thisChunkNum
         //           -> thisLoopNum
         setupNextBlock();

         //SD Card could still be busy writing from the end of the block
         sdWU_state = SDWU_WRITE_BUSY;
         break;
      //At the end of EVERY 512B block...
      // The card will be busy for a little while while updating the FLASH
      // This is the last state before the next packet (normally)
      //This state can be tested in main for things like using the SPI bus
      // for other purposes, etc...
      case SDWU_WRITE_BUSY:
         //STILL BUSY... (the data line is held low)
         if(0xff != w_xfer(0xff))
         {
           #if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
            writeBusyCount++;
           #endif
            break;
         }
         else
         {
            //No longer busy, determine the next state
            if(usageTableUpdateState == 1)
               sdWU_state = SDWU_UPDATE_UT;
            else if(!newBlockIsConsecutive)
               sdWU_state = SDWU_STOP_WRITING;
            else //New block is consecutive, and not updating the UT
               sdWU_state = SDWU_SEND_DATA_START_TOKEN; //0;
         }

         break;
      case SDWU_UPDATE_UT:
         // Send the Stop Transmission Token:
         w_xfer(0xfd);
         // It'll be busy for a little while...
         spi_sd_getRemainingResponse(NULL, 0, 0);

         //THIS WILL BLOCK!!!
         // (what about the keyboard, etc?)
         sd_writeUsageTable();

         usageTableUpdateState = 0;

         //The block/chunk stuff has already been set-up
         sdWU_state = SDWU_BEGIN_WRITING;
         break;
      //Send the Data Token
      case SDWU_SEND_DATA_START_TOKEN: //0:
         w_xfer(0xfc);
         sdWU_state++;
         //Intentional Fallthrough
         break;   //IntentionalFallthrough Broken for timing testing
      //Data bytes:
      default:
         {
#if(defined(SDWU_WRITE_SEVERAL_SAMPLES) && SDWU_WRITE_SEVERAL_SAMPLES)
            //This is a multipart test, except it can't be implemented as
            //such because of the #if, above...
            //(well, it can, but it fscks with VIM's syntax hilighting)
            //The "if" and "break" below takes care of the second test
            // (see cTests/switchEarlyBreak.c)
            // so it's basically: 
            // while((sdWU_state < BLOCKSIZE+1) && cirBuff_get() != NODATA)
            while (sdWU_state < BLOCKSIZE+1) 
#endif
            {
               //This'll either break-early from the switch-statement
               // or break from the while-loop
               // Depending on which is enabled.
               if((sdWU_sample = cirBuff_getSafe(&adcCirBuff))
                               == CIRBUFF_RETURN_NODATA )
                  break; 
               
               //We'll only have gotten this far if there's data in the
               //circular buffer

               //Write *one* sample to the SD Card

               //Moved to global, since statics aren't supposed to be
               // in inline functions...
               ////static uint8_t sdWU_dataState = 0;
               ////static cirBuff_dataRet_t sample;
               //But why is *sample* static?!


               uint8_t sampleHigh = (sdWU_sample>>8);
              #if(defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
               if(bytePositionToSend == 0)
               {
                     //Indicate that something has been noted, recently
                     if(kbIndicateCount)
                        sampleHigh |= (0x03<<2);
               }
               //bytePositionToSend is set to 1 when kbData is available
               if(bytePositionToSend == 1)
               {

                     //SEE NOTES where bytePositionToSend set to 1
                     // regarding why this is located here
                     // basically, maybe the keypress occurred before
                     // a chunk-transition...
                     // Otherwise, chunk marked -> chunk Advanced ->
                     //   kbData stored (chunk with data not marked)

                     //Update the usage-table to indicate
                     // that something was logged...
                     // But we needn't do it more than once per chunk
                     if( (eeprom_read_utByte(
                                 (uint8_t *)((uint16_t)thisChunkNum_u8))
                              != thisLoopNum )
                        || writeMemoToUT )
                     {
                        writeMemoToUT = FALSE;
                        usageTableUpdateState = 1;
                        //don't use write_byte instead of update_byte
                        // since this could've entered due to 
                        // writeMemoToUT
                        eeprom_update_utByte(
                              (uint8_t *)((uint16_t)thisChunkNum_u8), 
                                                         thisLoopNum);
                     }
                     
                     bytePositionToSend++;
                     sampleHigh |= ((rxByte<<4) | (0x01<<2));
                  }
                  else if(bytePositionToSend == 2)
                  {
                     bytePositionToSend = 0;
                     sampleHigh |= ((rxByte&0xf0) | (0x02<<2));
                     //Reset the kbIndicateCount after each byte...
                     kbIndicateCount = KBINDICATE_BLOCKS;
               }
              #endif
               w_xfer(sampleHigh);
               uint8_t sampleLow = (uint8_t)(sdWU_sample);
               w_xfer(sampleLow);
               sdWU_dataState = 0;
               sdWU_state+=2;
            } //While Loop (if implemented)

         } //Default Case Container

         break; //Break from the Default Case
   } //Close the Switch  

}



#if(defined(AUDIO_OUT_ENABLED) && AUDIO_OUT_ENABLED)
void pwmTimer_init(void)
{
 #ifdef __AVR_ATmega328P__
   uint8_t error = 0;

   //error |= timer_setWGM(2, WGM_FAST_PWM);
   error |= timer_init(2, CLKDIV1, WGM_FAST_PWM);

   error |= timer_setOutputModes(2, OUT_B, COM_CLR_ON_COMPARE);

   //For testing...
   OCR2B = 0x80;

   setoutPORT(PD3, PORTD);

   //This is the whole reason I made haltError(), duh...
   if(error)
   {
      heart_blink(2);
      while(1)
      {
         tcnter_update();
         heart_update();
      }
   }

 #else
   //Stolen and modified from LCDdirectLVDSv54:

   //Value to count to...
   //Since the ADC is ten bits, limit the PWM to this, as well.
   TC1H = 0x03;
   OCR1C = 0xff;


   //Timer1 on the Tiny861 uses a strange CLKDIV scheme...
   // (but it's nicer!)
   // The divisor is (1<<(csbits-1))
   // so a divisor of 1 = (1<<0) = (1<<(1-1)), (csbits = 0x1)
   // 256 = (1<<8) = (1<<(9-1)), (csbits = 0x9)
   // 512 = (1<<9) = (1<<(10-1)), (csbits = 0xA)
   // ...
   // (0x0 stops the timer)
   /*
      uint16_t divisor;
      uint8_t csbits = 0;
      for(divisor=CLKDIV; divisor != 0; divisor>>=1)
         csbits++;
         writeMasked(csbits, 0x0f, TCCR1B);
   */

#define PWMTIMER_PRESCALER 1

#if ((PWMTIMER_PRESCALER != 64) && (PWMTIMER_PRESCALER != 32) && \
    (PWMTIMER_PRESCALER != 16) && \
    (PWMTIMER_PRESCALER != 8) && \
     (PWMTIMER_PRESCALER != 4)  && (PWMTIMER_PRESCALER != 2) && \
     (PWMTIMER_PRESCALER != 1))
#error "PWMTIMER_PRESCALER must be a power of 2, from 1 to 64"
#endif

   //Figured this out in cTools/dePower.c...
   //64 is overkill here, since the deadTimer prescaler only goes to 8...
#define divToCS(div) \
   ( (div == 64) ? 7 : (div == 32) ? 6 : (div == 16) ? 5 : (div == 8) ? 4 \
     : (div == 4) ? 3 : (div == 2) ? 2 : (div == 1) ? 1 : 0)


   //CSBITS (through PLL/8) (CS10 is bit 0)
   //CS12:10   CS12  CS11  CS10     PLL division
   //1         0     0     1        1
   //2         0     1     0        2
   //3         0     1     1        4
   //4         1     0     0        8
   #define CSBITS divToCS(PWMTIMER_PRESCALER) //<<CS10 should be redundant


   pll_enable();

   //Set the Timer1 clock prescaler...
   writeMasked(CSBITS,
               ((1<<CS13) | (1<<CS12) | (1<<CS11) | (1<<CS10)),
               TCCR1B);


   //FastPWM
   setbit(PWM1D, TCCR1C);

   setoutPORT(PB5, PORTB); //+OC1D


   writeMasked(((0<<WGM11) | (0<<WGM10)), //FastPWM (combined with above)
               ((1<<WGM11) | (1<<WGM10)), // (affects all PWM channels)
               TCCR1D);

   TCCR1C = ( (1<<COM1D1) | (0<<COM1D0)
            | (1<<PWM1D) );


   TC1H = 0x00;
   OCR1D = 0x00;
 #endif
}
#endif


//Called after a block has been completed
//  Updates as necessary:
//    thisChunkNum
//    blockNum_inThisChunk
//    thisLoopNum
//  Also writes newBlockIsConsecutive as appropriate
//     (False at end of fragment, end of card)
static __inline__ void setupNextBlock(void)
{
#if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
   blocksWritten++;
#endif
               
   blockNum_inThisChunk++;

   if(blockNum_inThisChunk == numBlocksPerChunk)
   {
      blockNum_inThisChunk = 0;

      //thisChunkNum_u8 is updated in advanceChunk()
      // so we need a temp variable here.
      uint8_t oldChunk = thisChunkNum_u8;

      newBlockIsConsecutive =
         advanceThisChunkNum();

      if((thisChunkNum_u8 <= oldChunk)
         && (thisLoopNum < 0xff))
            thisLoopNum++;
   }
   else  //Next block within a chunk...
      newBlockIsConsecutive = TRUE;
}


#include "printer.c"
#include "sd.c"

//This is now, just as hokily, included in the appropriate xxx_spi.h file
//#include "spi.c"


//#if(PRINTER==PRINT_NLCD)
//#include "nlcd.c"
//#endif
//#include "anaButtons.c"
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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/main.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
