/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




#include "projInfo.h"   //Don't include in main.h 'cause that's included in other .c's?
#include "main.h"

#include <util/delay.h> //For delay_us in pll_enable...

#include "sd_crc7.h"
#include "pinout.h" // Really this is just for puart...
#include _POLLED_UAR_HEADER_
#include _POLLED_UAT_HEADER_
#include _CIRBUFF_HEADER_
#include <string.h>
#include <inttypes.h>
#include "adcFreeRunning.h"
#include <avr/pgmspace.h>//progmem.h"
#include _HSSKB_HEADER_ //Stowaway Keyboard
#include <avr/eeprom.h>



//#warning "THIS NEEDS TO BE READ/SET"
// Most cards default to 512, newer ones can ONLY do 512
// Would be wise to verify it's set to 512, but I don't feel like copying
// over the code from audioThing_v6_2
#define BLOCKSIZE 512 //2048
// This is the BLOCKSIZE as it would be represented by (1<<BLOCKSIZE_SHIFT)
#define BLOCKSIZE_SHIFT (DESHIFT(BLOCKSIZE))

#if(BLOCKSIZE_SHIFT >= 10)
#error "BLOCKSIZE should be 512.. This'll cause some math issues..."
#endif



//In S/s. Currently 19230S/s
#define SAMPLE_RATE (F_CPU/ADC_CLKDIV/ADC_CALC_CYCLES)



//Thought the pull-up mighta caused a crashed-keyboard
// since reading 3.3V on the KB's Tx pin, but only receiving VCC=3.1V
// But, crashed again over-night with no-pull-up.
// Also, added a diode on the output:
// KB Tx >---|<|----> uC Rx (with pull-up)
// so with pull-up it should be near the same voltage as VCC
// So, allegedly, according to absolute bullshit, the theory is that I woke
// quite grouchilly on both occasions, and proceeded to write some stuff
// that wasn't so great to people who mightn't have deserved to take the
// burden for all of humanity's downfalls...
// And, plausibly, something is trying to teach me not to do that
// and plausibly that's why the keyboard keeps crashing.
// right? I mean, it makes total sense. (the wind picks up, as does the
// gull).
// So, just for a moment, let's imagine this might be true... where does
// this leave us...? Seriously, just remove me from existance, please
// both in this realm and the spiritual, I don't want to go to heaven nor
// hell, nor be existant in any way, conscious or un. Should *I* take the
// same burdens I might have, possibly maybe placed upon someone who *was*
// being directly offensive to me? Maybe she mightn't have intended it that
// way, but I took it that way. So, ... and I didn't even say it to her,
// I kept it to myself. Am I, too, supposed to accept the burden that my
// mere *thoughts* are somehow more offensive to the world than her
// *actually acting on* hers?
// 
// So then, maybe, somehow this wasn't some sort of smiting, but a relief
// since that ... rant... was not logged... Maybe I'm being told I've been
// released of the burden... that wind sure died down...
//
// Regardless, there *was* non-ranting stuff I'd written and thought
// and damned-near certain occurances that deserve to be logged...
// that in fact were the whole *point* of developing this damned thing
// in the first place. So isn't it damned ironic that some of these damned
// certain bits of evidence keep getting wiped out of existence due to
// ... what... a bug? That hasn't shown itself at any time *except* on
// these occasions...? That I'd left it running for days on end before
// without issue... and now...?

// Or, am I *really* supposed to accept the burden that my mere thoughts
// are ... supposed to be downright saintly...

//#define RX_NOPULLUP TRUE
#define DO_FORMAT       TRUE //FALSE

//Something ain't right...
// Commenting ALWAYS_FORMAT out, outputs 8098 a/o v22-4
// Uncommenting outputs 6064!
//#define ALWAYS_FORMAT TRUE
//#define PRINT_COMMAND TRUE
//#define PRINT_R1         TRUE
//#define PRINT_CSD     TRUE
//#define PRINT_REMAINING_RESPONSE  TRUE
//#define PRINT_USAGETABLE TRUE //FALSE
//#define PRINT_GO         FALSE
#define PRINT_BOOT_RETURN TRUE
//#define PRINT_BOOT_TLN   TRUE
#define LC_FC_WBC_BW_DISABLED TRUE
//#define PRINT_LCandFC TRUE
//#define PRINT_BWandWBC         FALSE
//#define PRINT_LOOPCOUNT_1SEC   TRUE
//#define PRINT_LOOPNUM       TRUE
//#define PRINT_MEMOVALS   TRUE
#define PRINT_MEMOSIZE TRUE
//#define PRINT_SWB TRUE
//#define PRINT_FORMATTING TRUE
//#define MUT_BUG_FINDER   TRUE

//#define PRINT_BAD_DATARESPONSE TRUE
////#define PRINT_SDCONNECTED       TRUE
//#define PRINT_CANTSETBLOCKSIZE TRUE
//#define PRINT_BADCSDVER        TRUE
////#define PRINT_SIZE              TRUE

#define PRINT_CHUNKADVANCE TRUE



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





//#define OSCCAL_TESTING TRUE
//#define PUART_ECHO_ONLY  TRUE
#define PUART_ECHO TRUE
#define KB_TO_SAMPLE TRUE
#define HSSKB_TRANSLATE TRUE

#if(!defined(HSSKB_TRANSLATE) || !HSSKB_TRANSLATE)
   #error "Memo, etc, don't work without HSSKB_TRANSLATE"
#endif



//#define BUTTON_IN_SAMPLE TRUE
//#define TESTING_ANACOMP  TRUE
#define WRITING_SD TRUE
//#define START_STOP_TEST 30     //seconds
//#define ADC_PASSTHROUGH TRUE
//Not sure what this does anymore:
//#define PRINTOUT         TRUE

//#define PRINTEVERYBYTE   TRUE
// Synchronization problem writing at full speed...?
// adding nop seemed to fix it... but it's slower...
#define WRITETIMED      FALSE
//#define PRINT_REMAINING_RESPONSE  TRUE


#if ((defined(PRINTOUT) && PRINTOUT) \
            || ((defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)))
#define BUFFSIZE  4
#else
#define BUFFSIZE  16//96
#endif
uint16_t buffer[BUFFSIZE];

cirBuff_t myCirBuff;


#define SD_CS_pin    PA3
#define SD_CS_PORT   PORTA
#define SD_MOSI_pin  PA1
#define SD_MOSI_PORT PORTA
#define SD_MISO_pin  PA0
#define SD_MISO_PORT PORTA
#define SD_SCK_pin   PA2
#define SD_SCK_PORT  PORTA



//Connecting the Button/DAC array to AIN0 (PA6) (negative input)
// AIN1 is PA5 (positive input) tied to a voltage-divider @ VCC/2
#define BUTTON_PIN   PA6
// Kinda hokey, just a number of loops...
#define CHARGE_TIME  0xf0
uint8_t newCompTime = FALSE;
tcnter_t compTime = 0;


#define NO_B            0
#define VOL_PLUS_B      1
#define VOL_MINUS_B     2
#define PLUS_B          3
#define MINUS_B         4
#define PLAY_PAUSE_B    5
#define STOP_B          6
#define FWD_B           7
#define REV_B           8

// This isn't necessary...
// I thought it might compile them as separate functions
//   when they're not used, but it's apparently smart enough not to compile
//   them at all...
// However, it's NOT smart enough not to give warnings
//  e.g. "something's static, but in an inline function..."
#if((defined(BUTTON_IN_SAMPLE) && BUTTON_IN_SAMPLE) \
      || (defined(TESTING_ANACOMP) && TESTING_ANACOMP))
extern __inline__ uint8_t anaComp_getButton(void)
{
   if(!newCompTime)
      return NO_B;

   tcnter_t t = compTime;
   newCompTime = FALSE;

   if( (t>230) )
      return NO_B;
   //+ ~216-221 TCNTs
   else if( (t>200) ) //&& (t<230) )
      return PLUS_B;
   //Vol+ ~180-182 TCNTs
   else if( (t>170) ) //&& (t<190) )
      return VOL_PLUS_B;
   //Vol- ~152-156 TCNTs
   else if( (t>140) ) // && (t<170) )
      return VOL_MINUS_B;
   //Stop ~ 128-133
   else if( (t>120) ) //&& (t<140) )
      return STOP_B;
   //- ~95-97
   else if( (t>90) ) // && (t<100) )
      return MINUS_B;
   //FWD ~68-71
   else if( (t>65) ) // && (t<75) )
      return FWD_B;


   //This and REV are confusing, as well
   // I'm *certain* I measured 330Ohms on Play/Pause
   // and 1kOhms on REV, but they're returning opposite values for TCNTs...
   //Play/Pause ~27-30
   else if( (t>26) ) //&& (t<35) )
      return PLAY_PAUSE_B;

   //FWD and REV are confusing...
   // The way it fits in *my* hand is apparently upside-down
   // So the names here match the proper orientation (spelling upright)
   //REV ~20-24
   else if( (t>15) ) //&& (t<26) )
      return REV_B;

   else
      return NO_B;
}
/*
//Unused...
tcnter_t anaComp_getCompTime(void)
{
   if (newCompTime)
   {
      newCompTime = FALSE;
      return compTime;
   }
   else
      return 0xffff;
}
*/
extern __inline__ void anaComp_update(void)
{
   static tcnter_t startTime = 0;
   static uint8_t state=0;

   switch(state)
   {
      //Charge the capacitor...
      case 0:
         setoutPORT(BUTTON_PIN, PORTA);
         setpinPORT(BUTTON_PIN, PORTA);
         state++;
         break;
      //Give it some time to make sure it's charged...
      case 1:
         {
            static uint8_t chargeTime = 0;

            if(chargeTime >= CHARGE_TIME)
            {
               state++;
               chargeTime = 0;
            }
            else
               chargeTime++;
         }
         break;
      //Start discharging through the buttons...
      case 2:
         startTime = tcnter_get();
         setinPORT(BUTTON_PIN, PORTA);
         clrpinPORT(BUTTON_PIN, PORTA);   //Remove the pull-up
         ACSRA = (0<<ACD)  // Don't disable the anaComp
               | (0<<ACBG) // Don't Use the internal voltage reference
                           //  for +input. See ADMUX: REFS2..0
               | (0<<ACO)  // This is read-only...
               | (1<<ACI)  // Clear the interrupt flag
               | (0<<ACIE) // Don't enable the interrupt
               | (0<<ACME) // Don't use the ADC Multiplexer to select
                           //  the inputs...
               | (1<<ACIS1)// With Below, select anaComp output Rising-edge
               | (1<<ACIS0);//  for the interrupt-flag (don't care)
         ACSRB = (0<<HSEL) // Don't use hysteresis
               | (0<<HLEV) //  Don't care about the hysteresis level
               | (1<<ACM2) // With Below, select AIN2 as the positive input
               | (0<<ACM1) //  AIN0 as negative  
               | (0<<ACM0);    
         state++;
         break;
      //Wait until the analog comparator switches state...
      case 3:
         // The anaComp output should be positive when the capacitor has
         // discharged below the positive input
         if(getbit(ACO, ACSRA))
         {
            newCompTime = TRUE;
            compTime = tcnter_get() - startTime;
            state = 0;
         }
         break;
      //Shouldn't get here...
      default:
         break;
   }
}

#endif //BUTTON_IN_SAMPLE || TESTING_ANACOMP



void haltError(uint8_t errNum);

//Left global so we can keep track of memory usage...
char stringBuffer[40];


void puat_sendStringBlocking(char string[]);

void puat_sendStringBlocking_P(PGM_P P_string)
{
   sprintf_P(stringBuffer, P_string);
   puat_sendStringBlocking(stringBuffer);
   stringBuffer[0]='\0';
}


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
   puat_sendStringBlocking(stringBuffer);
}




uint32_t extractBitsFromU8Array(uint8_t highBit, uint8_t lowBit,
                                uint8_t array[], uint8_t arrayBits);

uint32_t fullCount = 0;
//This might have to be uint64_t for SDHC...
uint32_t sd_numBLOCKSIZEBlocks = 0;


#if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
//This used to track whether it was time to cycle back to block-zero
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

//thisChunkNum_u16 is the one currently being written
// UNLESS between block-complete and Init
WriteBlocks:
   Init:
      FindNextAvailableChunk_IncludingThis:
         while(!chunkAvailable(thisChunkNum_u16))
            thisChunkNum_u16++
      //endChunk = findFragmentEnd(thisChunkNum_u16)
      //             endChunk = thisChunkNum_u16
      //             while(chunkAvailable(endChunk))
      //                endChunk++
      //             endChunk--
      START
   Start:
      SD_WriteMultiple(startBlockFromChunk(thisChunkNum_u16))
      WRITE
   Write:
      ...
   Block Complete:
      blockNum_inThisChunk++;
      if(blockNum_inThisChunk == numBlocksPerChunk)
         thisChunkNum_u16++
         blockNum_inThisChunk=0
         if(!chunkAvailable(thisChunkNum_u16)) //FindNextAvailableChunk_AFTERThis(
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
// thisChunkNum_u16...
uint8_t memoPositionInUT = 0;

//This is the number of chunks that have been requested to be marked 
//  (regardless of whether they were marked already, or if there are
//   enough written chunks to mark)
// in this go-round with the memo-key
// This is used for calculating the number of step-backs necessary 
//   when the key is pressed again (a/o memoMinutes)
uint8_t memoChunks = 0;

//The number of minutes to mark each time memo is pressed
#define MEMO_MINUTES 5

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
uint16_t thisChunkNum_u16 = 0;


//Even with two calls, it's smaller to inline this...
// what about three?
static __inline__ 
void memoReset(void)
{
   memoLoopNum = thisLoopNum;
   memoPositionInUT = thisChunkNum_u16;
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
   cli();   
   uint8_t adcLow = ADCL; //Read ADCL first, then ADCH isn't updated until
   uint8_t adcHigh = ADCH; // it's read.
   //uint8_t adcLow = ADCL;

   //Moved so playback only occurs alongside cirBuff writes
   // so hopefully we can get a sense for what's missing...
   //1:1 would be TC1H=(uint8_t)(adcVal>>8); OCR1D=(uint8_t)adcVal;
   //TC1H = adcHigh; //(uint8_t)(adcVal>>6);
   //OCR1D = adcLow; //(uint8_t)(adcVal<<2);

#if (defined(BUTTON_IN_SAMPLE) && BUTTON_IN_SAMPLE)
   //Tried |=ing the last bit, but it increased code-size!
   cirBuff_data_t adcVal = (uint16_t)adcLow | (((uint16_t)adcHigh)<<8)
                         | (((uint16_t)anaComp_getButton())<<10);
#else
   cirBuff_data_t adcVal = (uint16_t)adcLow | (((uint16_t)adcHigh)<<8);
#endif

   if(cirBuff_add(&myCirBuff, adcVal, DONTBLOCK))
   {
      fullCount++;
//    puat_sendStringBlocking("CirBuff Full!\n\r");
//    haltError(0x0e);
   }
   else
   {
      //1:1 would be TC1H=(uint8_t)(adcVal>>8); OCR1D=(uint8_t)adcVal;
      TC1H = adcHigh; //(uint8_t)(adcVal>>6);
      OCR1D = adcLow; //(uint8_t)(adcVal<<2);
   }

// tcnter_update();
// puar_update(0);
   sei();
}

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

/* Random SPI/SD Notes:

   Apparently it helps to get an additional byte (even though it's expected
   to be 0xff) after a command...

TODO: Maybe add power-up transistor...?
   Be sure to test with power-down/power-up...
      Once CMD0 is received correctly, it ignores CRC errors on following
      CMD0's... but on the first one, doesn't even reply with an error
      (returns 0xff)

   CRC7 on commands only seems to be relevent for first CMD0 and CMD8
    CMD55 and CMD41 don't seem to give a CRC error.

   CRC error on CMD8 returns R1 response with CRC error, but does not
    return excess response...

   This stuff is described in section 7.2.2 of the PLSv3.00
 */

//void spi_sd_readTable(void);

//void spi_sd_startReadingBlocks(void);
void spi_sd_startWritingBlocks(uint32_t startBlock);
void sd_writeUpdate(void);

void haltError(uint8_t errNum)
{
   //Disable the SD card...
   // (then insertion while powered should be OK)
   setpinPORT(SD_CS_pin, SD_CS_PORT);
   
   //Reenable the timer interrupt so the heart will update...
   timer_compareMatchIntSetup(0, OUT_CHANNELA, TRUE);
   
   //Thought about disabling the ADC interrupt, but it shouldn't be
   // necessary... so it fills up the cirBuff and once it's full
   // audio passthrough is stopped...

   heart_blink(errNum);
   while(1)
   {
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

void spi_sd_init(void);
void pwmTimer_init(void);
void pll_enable(void);
void spi_sd_sendEmptyCommand(uint8_t cmdNum);
void spi_sd_sendCommand(uint8_t *command, uint8_t length);
//CAREFUL with this one... see definition
uint8_t spi_sd_getR1response(uint8_t getRemaining);
uint8_t spi_sd_getRemainingResponse(uint8_t *buffer, uint8_t delayBytes, 
                                          uint8_t expectedBytes);
#define r1ResponseValid(response)   (!getbit(7, (response)))

//Not safe to use this after the timer's being used by other things...
// also it halts processing during the communication
uint8_t spi_sd_transferByteWithTimer(uint8_t txByte);
uint8_t spi_sd_transferByte(uint8_t txByte);

//To just send a string, without caring if it blocks...
void puat_sendStringBlocking(char string[])
{
   uint8_t i;

   for(i=0 ; string[i] != '\0' ; i++)
   {
      tcnter_update();
      puat_update(0);
      puat_sendByte(0, string[i]);

      extern uint8_t puat_txState[NUMPUATS];

      //17-3ish... why wasn't puat_dataWaiting() enough?!
      //  puat_txState, here, was originally txState, but couldn'ta been
      //  from another library... could it? Doubtful.
      while(puat_dataWaiting(0) || (puat_txState[0] != TXSTATE_IDLE) )
      {
         tcnter_update();
         puat_update(0);
      }
   }

}


#define USAGETABLE_SIZE (256)

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
          && (0 != eeprom_read_byte((uint8_t *)usageTablePosition)) ) 
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
          && (0 == eeprom_read_byte((uint8_t *)ut_nextUsed)) )
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
   for(i=0; i<(BLOCKSIZE-USAGETABLE_SIZE); i++)
   {
      rt_xfer(i);
   }

   // Write the usage-table stored in the eeprom to the SD card
   for(i=0 ; i<(USAGETABLE_SIZE); i++)
   {
      rt_xfer(eeprom_read_byte((uint8_t *)i));
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
      puat_sendStringBlocking(stringBuffer);
#endif
      haltError(0x0f);
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
      ut = eeprom_read_byte((uint8_t *)((uint16_t)chunkNum));\
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
   puat_sendStringBlocking_P(PSTR("Format: "));
#endif

   //Could use do-while with uint8_t iterator?
   uint16_t i;
   for(i=0; i<(BLOCKSIZE-USAGETABLE_SIZE); i++)
   {
      uint8_t byteIn = rt_xfer(0xff);
#if(defined(PRINT_USAGETABLE) && PRINT_USAGETABLE)
      //sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), byteIn);
      //puat_sendStringBlocking(stringBuffer);
      //WOOT! This saved 8 bytes!
      printHex(byteIn);
#endif

#if(BLOCKSIZE-USAGETABLE_SIZE <= 256)
      if( byteIn != i )
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
   puat_sendStringBlocking_P(PSTR("\n\rUsageTable: "));
#endif

   // Grab the data-table (either way, we have to read the rest of the 
   //  block)
   // 
   for(i=0 ; i<(USAGETABLE_SIZE); i++)
   {
      uint8_t byteIn = rt_xfer(0xff);

#if(defined(PRINT_USAGETABLE) && PRINT_USAGETABLE)
      //sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), byteIn);
      //puat_sendStringBlocking(stringBuffer);
      //WOOT! This second one saved 180-106 Bytes
      // (since sprintf_P needn't be compiled?)
      // No... 'cause that's used by main, etc.... so... wtf.
      printHex(byteIn);
#endif
/*
      if(byteIn)
         usageTable[i/8] |= (1<<(i%8));
*/
      eeprom_update_byte((uint8_t *)i, byteIn);


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
      puat_sendStringBlocking_P(PSTR("Formatting..."));
  #else
      puat_sendByte(0, 'F');
  #endif
      formatCard();

      //This is already handled in formatCard!
      // So now the code-size will shrink further... so confusing
      // same size, (optimizer musta recognized duplicate) but still...
      //Make sure we have valid start-values!
      //thisLoopNum = 1;
  #if(defined(PRINT_FORMATTING) && PRINT_FORMATTING)
      puat_sendStringBlocking_P(PSTR("done.\n\r"));
  #endif
   }
#endif



   //Regardless of whether formatting was necessary
   // thisLoopNum is now valid 
   //    (either from original usage-table, or 1 if formatted...)
   // For now, we always boot at the beginning of the card...
   // We need this here, because eventually the memo_chunkNum will need
   //  to be initialized with thisChunkNum_u16, here...
   thisChunkNum_u16 = findNextUsableChunk(0);
   
   memoReset();
   //memoPositionInUT = thisChunkNum_u16;
   //memoChunks = 0;
   //memoMinutes = 0;
   //memoLoopNum = thisLoopNum;


   //Don't let a new memo (soon after boot) wrap around to mark
   // as though it'd already looped once since boot...
   bootLoopNum = thisLoopNum;

#if( (defined(PRINT_USAGETABLE) && PRINT_USAGETABLE) \
   ||(defined(PRINT_BOOT_TLN) && PRINT_BOOT_TLN) )
   sprintf_P(stringBuffer, PSTR("ln:0x%"PRIx8",cn:0x%"PRIx8"\n\r"), 
         thisLoopNum, (uint8_t)thisChunkNum_u16);
   puat_sendStringBlocking(stringBuffer);
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
      eeprom_update_byte((uint8_t *)((uint16_t)i), 0);
   }
   
   
   // Now write that to the SD Card...
   sd_writeUsageTable();

   //Make sure we have valid start-values!
   thisLoopNum = 1;
}




uint32_t packetCount = 0;

//To be called after an initiate-read or whatever I called it...
// Returns a u16 of the data read
// unless the card was returning 0xff's between blocks...
// or it received the data token or CRC
// then it returns -1
// Hopefully, then, this can be called in a while-loop without
//  blocking for too long.
static __inline__ int32_t spi_sd_readU16(void)
{
   static uint16_t packetByte = 0;

   uint16_t thisReceived;

   switch(packetByte)
   {
      //because packetByte is really packetByteCount...
      case BLOCKSIZE+1:
         //Discard the first CRC byte
         spi_sd_transferByte(0xff);
//       packetByte++;
//       return -1;
//       break;
//    case BLOCKSIZE+2:
         //Discard the second CRC byte
         spi_sd_transferByte(0xff);
         packetByte = 0;
         return -1;
         break;
      case 0:
         {
            uint8_t thisReceived;
            packetCount++;
            uint8_t i;
         //Waiting for Data Token... 0xfe
         // Keep requesting until the data token is received
         // (could be busy)
         for(i=0; i<5; i++)
         {
            if((thisReceived=spi_sd_transferByte(0xff)) == 0xfe)
            {
               packetByte++;
               break;
            }
            else if(thisReceived < 0xfe)
            {
#if(defined(PRINT_BAD_DATARESPONSE) && PRINT_BAD_DATARESPONSE)
               //char string[20];
               sprintf_P(stringBuffer, PSTR("DataErr:0x%"PRIx8"\n\r"),
                                                   thisReceived);
               puat_sendStringBlocking(stringBuffer);
#endif
               haltError(0x7);
            }
         }
         }
         return -1;
         break;
      default:
         {
            //Get two bytes...
#if(defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)
   static uint16_t byteNum= 0;
   //char string[20];
//   sprintf(string, "packetByte: %"PRIu16"\n\r", packetByte);
//   puat_sendStringBlocking(string);
   sprintf_P(stringBuffer, PSTR("byte %"PRIu16": "), byteNum);
   puat_sendStringBlocking(stringBuffer);
   byteNum++;
#endif

            uint8_t byte1 = spi_sd_transferByte(0xff);
#if(defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)
   sprintf_P(stringBuffer, PSTR("byte %"PRIu16": "), byteNum);
   puat_sendStringBlocking(stringBuffer);
   byteNum++;
#endif

            uint8_t byte2 = spi_sd_transferByte(0xff);
// WTF, this kept returning 0x1b as the high byte!
// thisReceived = (((uint16_t)(spi_sd_transferByteWithTimer(0xff))) << 8);
// thisReceived |= spi_sd_transferByteWithTimer(0xff);
// thisReceived = spi_sd_transferByte(0xff);

            thisReceived = (byte1<<8) | byte2;
            packetByte+=2;

            return thisReceived;
         }
         break;
   }
}

static __inline__ 
   int32_t addSample(cirBuff_t *myCirBuff)
      __attribute__((__always_inline__));


//For the keyboard -> sample...
unsigned char rxByte;
#if (defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
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
//Lessee... currently running at 19230S/s, that's 38460B/s / 512 = 75Blk/s
//Lessay we indicate for 1min after each note, that's 4507 blocks (2,407KB)
// So we'd reduce our search from reading every byte to reading
// 2 bytes (one sample) in 2million... seems reasonable...
#define KBINDICATE_BLOCKS  4507
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

   //Disable the pull-up (the keyboard mightn't like 3.6V on its output)
   // (Is that why it croaked?)
   // EXCEPT: If the KB's not connected, then garbage is received
   //  Options: 
   //      Add pull-down (will constantly receive Frame Errors!)
   //      Add voltage-divider
   //      Insert series diode (at KB)
   //             KB >---|<|----> UC (pulled-up, don't forget!)
   //       Hopefully vDrop is enough to satisfy...
   // WEIRD: This's been running for DAYS without a problem.
   //        Plug/Unplug didn't fix until later... several tries
#if(defined(RX_NOPULLUP) && RX_NOPULLUP)
   clrpinPORT(Rx0pin, Rx0PORT);
   setinPORT(Rx0pin, Rx0PORT);
#else
   setinpuPORT(Rx0pin, Rx0PORT);
#endif

   setoutPORT(Tx0pin, Tx0PORT);
   tcnter_init();

   //250000Hz / 4800 = 52.08
   //puar0_bitTcnt = 
   // Let's try 9600
//#define BIT_TCNT_LOCAL   26 //52
// puar_setBitTcnt(0, BIT_TCNT_LOCAL); //26); //52);
   puar_init(0);
// puat_setBitTcnt(0, BIT_TCNT_LOCAL); //26); //52);
   puat_init(0);

   dmsWait(DMS_SEC);

#if(defined(OSCCAL_TESTING) && OSCCAL_TESTING)
   uint32_t lastTime = 0;
   setoutPORT(HEART_PINNUM, HEART_PINPORT);
   while(1)
   {

      uint32_t thisTime = dmsGetTime();
      if(thisTime > lastTime)
      {
         lastTime = thisTime;
         togglepinPORT(HEART_PINNUM, HEART_PINPORT);
      }
   }
#endif
   //puat_sendStringBlocking_P(PSTR("\n\n\rBoot!\n\r"));


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
         puat_sendByte(0, rxByte);

      //Might interfere with puart...
      heart_update();
   }

   _delay_ms(2000);
*/
   spi_sd_init();

   spi_sd_readTable();

   pwmTimer_init();

   //heart_init();

// heart_setRate(0);  

   cirBuff_init(&myCirBuff, BUFFSIZE, buffer);

   //Now handled by writeUpdate's init-state.
   //spi_sd_startWritingBlocks();

// spi_sd_startReadingBlocks();
#define INT10_MAX    511
   //uint16_t lastSample = INT10_MAX;

/*
   //Fill up the buffer before-hand... might help...
   while(cirBuff_availableSpace(&myCirBuff))
   {
      addSample(&myCirBuff);
   }  
*/
#if(defined(PRINT_GO) && PRINT_GO)
   puat_sendStringBlocking_P(PSTR("\n\n\rGo!\n\r"));
#elif (defined(PRINT_BOOT_RETURN) && PRINT_BOOT_RETURN)
   puat_sendStringBlocking_P(PSTR("\n\r"));
#endif

// tcnter_update();

   //TCNTS are at 250K/s, 44.1kS/s -> 5.669
#if (defined(PRINTOUT) && PRINTOUT)
#define SAMPLE_TCNT  250000
#else
#define SAMPLE_TCNT  1//5//6//64//15   //6
#endif
   myTcnter_t nextSampleTime = tcnter_get() + SAMPLE_TCNT;

   //Disable the DMSTimer interrupt for playback...
   timer_compareMatchIntSetup(0, OUT_CHANNELA, FALSE);

   int32_t lastSample = -1;

   adcFR_init();

   setoutPORT(HEART_PINNUM, HEART_PINPORT);

   while(1)
   {
   // uint8_t heartState;
   // heartState = heart_update();
      tcnter_update();
      tcnter_t thisTime = tcnter_get();
      static tcnter_t nextTime = 0;
#if(defined(PUART_ECHO) && PUART_ECHO)
      puar_update(0);
   // puat_update(0);
      //static unsigned char rxByte;
      //Track the position _to_be_written_
      // e.g. 0 = nothing to be written
      //      1 = low nibble
      //      2 = high nibble
      //static uint8_t rxBytePosition = 0;
      static uint8_t byteNum = 0;
      static tcnter_t nextEchoTime = 0;
      static uint32_t echoLoopCount = 0;
   #define KBBUFFER_SIZE   20
      static char kbBuffer[KBBUFFER_SIZE];

      if(puar_dataWaiting(0) && (byteNum < (KBBUFFER_SIZE-1))
   #if(defined(KB_TO_SAMPLE) &&(KB_TO_SAMPLE))
            && (bytePositionToSend == 0)
   #endif
        )
      {
         rxByte = puar_getByte(0);

         //Toggle whenever data's received
         togglepinPORT(HEART_PINNUM, HEART_PINPORT);

   #if(defined(HSSKB_TRANSLATE) && HSSKB_TRANSLATE)
         rxByte = hsSKB_toChar(rxByte);
         
        if(rxByte != 0)
        {

         //SEE NOTE BELOW...
         uint8_t chunksToMark = 0;

         if(rxByte == MEMO_RETURN)
         {
            //When the Memo Key isn't pressed for MEMO_TIMEOUT
            // it reverts to the current chunk
            // (so pressing it again will be currentChunk -1)
            static tcnter_t lastMemoTime = 0;
      #define MEMO_TIMEOUT (2500000)   //10sec
            if(thisTime - lastMemoTime > MEMO_TIMEOUT)
               memoReset();


            // This is the time the key was pressed, 
            //  Not the amount of time to be marked on the SD this go-round
            lastMemoTime = thisTime;


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
            //      even if thisChunkNum_u16 increases in the midst of a
            //      memo-pressing-venture, memoPositionInUT won't
            //      (so no shift will occur)
            //      and the current chunk will be marked regardless
            chunksToMark = chunksTemp //memoChunksFromMinutes(memoMinutes)
                           - memoChunks;
      
            //Shouldn't this be +=??
            memoChunks = chunksTemp; //chunksToMark;
#if(defined(PRINT_MEMOSIZE) && PRINT_MEMOSIZE)
            sprintf_P(stringBuffer, 
                  PSTR("min:%"PRIu8"chu:%"PRIu8"ctm:%"PRIu8),
                  memoMinutes, memoChunks, chunksToMark);
            puat_sendStringBlocking(stringBuffer);
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
      
               //No need to update, it's already known to be different
               eeprom_write_byte((uint8_t *)((uint16_t)memoPositionInUT), 
                                 memoLoopNum); 
      
      #if(defined(PRINT_MEMOVALS) && PRINT_MEMOVALS)
               //puat_sendStringBlocking_P(PSTR("W:"));

               sprintf_P(stringBuffer, 
                           PSTR("p:0x%"PRIx8"l:0x%"PRIx8
                                 "ln:0x%"PRIx8"cn:0x%"PRIx8"\n\r"),
                           //memoPositionInUT, memoLoopNum,
                           memoPositionInUT, memoLoopNum,
                           thisLoopNum, (uint8_t)thisChunkNum_u16);
               puat_sendStringBlocking(stringBuffer);
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

         //Intentionally not elsing... treat a memo-key like a normal key
         // as well
         // that bytePositionToSend bit may be important
   #endif   //HSSKB_TRANSLATE
   

   #if(defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
            //This indicates that there is a byte to write in the sample
            // header. But also indicates that the usage-table needs
            // to be reloaded to the SD at the end of this block.
            // (This is all handled in sd_writeUpdate() )
            // ( Not sure if that piece can be moved back to main...)
            // WHY isn't the eeprom_update here, at least...?
            bytePositionToSend = 1;


            //Why on earth did I put the eeprom write-update in 
            // sd_writeUpdate....?
            // maybe so its chunk number would align with the data?
            // in case a key-press is handled near the end of a chunk
            // main() would receive the rxData, but the next 
            //   few writeUpdates could be used for locating a new chunk
            //   SD_WriteMultiple stop/start, end of card, etc...
            //   If eeprom was written immediately, it would be located
            //   at the chunk which was active *then*, whereas
            //   the data itself wouldn't be written with sample-data
            //   until the next chunk...
#warning "Do we have to worry about this with Memo, and its usage for marking the prior chunk, etc.?"
            // Might...
   #else
      #error "KB_TO_SAMPLE != TRUE, no logging will happen, either!"
   #endif

            //puat_sendByte(0, rxByte);
            kbBuffer[byteNum] = rxByte;
            byteNum++;
            kbBuffer[byteNum] = '\0';
         }  //rxByte != 0
      }  //DataWaiting

      if((thisTime >= nextEchoTime))
      {
         //This just causes regular blinking
         togglepinPORT(HEART_PINNUM, HEART_PINPORT);

         cli();
         //This'll repeat whatever's in the string buffer from before
         // (e.g. Go! on boot, but intended to repeat the kb input)
         puat_sendStringBlocking(kbBuffer);
         kbBuffer[0] = '\0';
   #if(defined(PRINT_LOOPCOUNT_1SEC) && PRINT_LOOPCOUNT_1SEC)
         sprintf_P(stringBuffer, PSTR("\n\rlc:%"PRIu32"\n\r"),
                                                echoLoopCount);
         puat_sendStringBlocking(stringBuffer);
   #endif
   #if(defined(PRINT_LOOPNUM) && PRINT_LOOPNUM)
         sprintf_P(stringBuffer, PSTR("\n\rtln:0x%"PRIx8"\n\r"),
                                                thisLoopNum);
         puat_sendStringBlocking(stringBuffer);
   #endif
         nextEchoTime = thisTime + 250000;
         
         byteNum = 0;
         stringBuffer[byteNum] = '\0';
         sei();
         echoLoopCount = 0;
      }

      echoLoopCount++;
#endif
#if(defined(PUART_ECHO_ONLY) && PUART_ECHO_ONLY)
      {}
#elif(defined(TESTING_ANACOMP) && TESTING_ANACOMP)
//    static tcnter_t nextTime = 0;

      anaComp_update();

      if((thisTime > nextTime) && newCompTime)
      {
         //char string[30];
         //sprintf_P(string, PSTR("anaCompTCNTS:%"PRIu32"\n\r"), 
         //                anaComp_getCompTime());
         sprintf_P(stringBuffer, PSTR("Button: %"PRIu8"\n\r"), 
               anaComp_getButton());
         puat_sendStringBlocking(stringBuffer);
         nextTime = thisTime + 250000;
      }

#elif (defined(WRITING_SD) && WRITING_SD)

//    static tcnter_t nextTime = 0;
//    static uint32_t loopCount = 0;

   #if(defined(BUTTON_IN_SAMPLE) && BUTTON_IN_SAMPLE)
      anaComp_update();
   #endif


   #if(!defined(LC_FC_WBC_BW_DISABLED) || !LC_FC_WBC_BW_DISABLED)
      #if(defined(PUART_ECHO) && PUART_ECHO)
   
      //Let's see if skipping this stuff when receiving makes it 
      // fast enough for 9600bps input
      //WTF: Enabling this causes loopCount to drop to 34500
      // FullCount regularly 4600!
      // (for a *single byte* test?!)
      // And that's *without* data being transmitted.
//    extern uint8_t rxState[];

//    if(rxState[0] == 0)
      #endif
      {
      
         static uint32_t loopCount = 0;
   
         if(thisTime > nextTime)
         {
            nextTime = thisTime + 250000;
            if(fullCount)
            {
               cli();
      #if(defined(PRINT_LCandFC) && PRINT_LCandFC)
               //char string[40];
               sprintf_P(stringBuffer, 
                           PSTR("lc:%"PRIu32", fc:%"PRIu32", "), 
                           loopCount, fullCount);
               puat_sendStringBlocking(stringBuffer);
      #endif
      #if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
               sprintf_P(stringBuffer, 
                           PSTR("wbc:%"PRIu32" bw:%"PRIu32"\n\r"),
                           writeBusyCount, blocksWritten);
               puat_sendStringBlocking(stringBuffer);
      #endif
#warning "This could be removed if sendStringBlocking handled it..."
               stringBuffer[0] = '\0';
               fullCount = 0;
               sei();
            }
      #if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
            writeBusyCount=0;
      #endif
            loopCount=0;
         }
         loopCount++;         

//       cirBuff_get(&myCirBuff);
   #endif
         sd_writeUpdate();
   #if(!defined(LC_FC_WBC_BW_DISABLED) || !LC_FC_WBC_BW_DISABLED)
      }
   #endif

#elif (defined(ADC_PASSTHROUGH) && ADC_PASSTHROUGH)
      int16_t adcVal = adcFR_get();

      if(adcVal > 0)
      {
         //1:1 would be TC1H=(uint8_t)(adcVal>>8); OCR1D=(uint8_t)adcVal;
         TC1H = (uint8_t)(adcVal>>6);
         OCR1D = (uint8_t)(adcVal<<2);
      }
#else
      if(thisTime >= nextSampleTime)
      {
         cirBuff_dataRet_t gotData;
         gotData = cirBuff_get(&myCirBuff);
//int32_t gotData = addSample(&myCirBuff);
         //Hopefully this should only happen a few times in the beginning
         if(gotData != CIRBUFF_RETURN_NODATA)
         {
            uint16_t gotSample = gotData;

            TC1H = (gotSample >> 8);
            OCR1D = (uint8_t)(gotSample);
      
            //It shouldn't happen often, but we should get a sample
            // immediately as it's available, I guess... 
            nextSampleTime = thisTime + SAMPLE_TCNT;

   #if (defined(PRINTOUT) && PRINTOUT)
            //char string[20];
            sprintf_P(stringBuffer, PSTR("gotSample 0x%"PRIx16"\n\r"),
                                                gotSample);
            puat_sendStringBlocking(stringBuffer); 
   #endif

            //lastSample = -1;
         }
         
      }

//    if(lastSample < 0)
//       lastSample = addSample(&myCirBuff);
      
      if(cirBuff_availableSpace(&myCirBuff))
      {
         addSample(&myCirBuff);
      }
#endif
      //heart_update();
   }
}

int32_t addSample(cirBuff_t *p_myCirBuff)
{
         int32_t dataIn = spi_sd_readU16();
         if(dataIn >= 0)
         {
            uint16_t data = dataIn;
            //Make it useable... 
/*          // it's actually int16, but we're loading to u8
            //Looks, so far, like the max value is ~ 0x400

            // treat it like a int16:
            int16_t * p_i16data;
            p_i16data = (int16_t *)(&data);
            int32_t temp = (*p_i16data);

            //Now scale it to fit in an int8
            // Again, it looks like the max value is around 0x400
            // so don't scale by INT16_MAX, but by sample-max...
            //temp = (temp*INT10_MAX)/(0x1000);
            temp >>= 5;
            //temp = (temp*INT8_MAX)/(0x800);   //(INT16_MAX);

            //Now shift the center to 127 in a u8
            //temp += INT8_MAX;
            temp += INT10_MAX;
*/
            //NOW: It's unsigned raw int16... so just shift it into uint10
            data >>= 6;

            cirBuff_add(p_myCirBuff, data, DONTBLOCK); 
                                          //(uint16_t)temp, DONTBLOCK);

#if (defined(PRINTOUT) && PRINTOUT)
            //char string[30];
            sprintf_P(stringBuffer, 
                  PSTR("addSample 0x%"PRIx16"->0x%"PRIx16"\n\r"),
                  data, (uint16_t)data);
            puat_sendStringBlocking(stringBuffer);
#endif
            return data; // (uint16_t)temp;
         }
         else
            return -1;
}


static __inline__
void spi_StrobeClockAndShift(void)
   __attribute__((__always_inline__));

//This would make more sense as a #define for USICR's value
//  but I want to keep all the notes here...
void spi_StrobeClockAndShift(void)
{
   USICR =
      //Set the USI port to three-wire mode
      (0<<USIWM1) | (1<<USIWM0)
      //Set the SPI port to use Timer0 compare-match for clocking
      //NOGO doesn't output on SCK pin
      //    | (0<<USICS1) | (1<<USICS0);
      //These three bits aren't exactly independent...
      //Set the SPI port to clock in data on the positive edge
      // This can be changed independently
      | (0<<USICS0)
      //Set the data register to clock on external clock pulses
      // (will be generated via software...)
      | (1<<USICS1)
      //Use the software-clock (USITC will toggle the pin, which will
      // in turn increment the counter, and latch data)
      | (1<<USICLK)

      //Strobe the clock (toggle the output pin)
      | (1<<USITC);

   asm("nop;");
}


//From the Physical Layer Specification v3.01 page 66:
// quoted part is the CRC
//CMD0(Arg=0)   --> 01 000000 00000000000000000000000000000000 "1001010" 1 
//   lec12:        01 000000 00000000 00000000 00000000 00000000 1001010 1
//CMD17(Arg=0)  --> 01 010001 00000000000000000000000000000000 "0101010" 1 
//CMD17 Response -> 00 010001 00000000000000000000100100000000 "0110011" 1 
#define CRC_TO_BE_CALCD 0
//Apparently the CRC is being ignored on CMD0?
//uint8_t spi_sd_CMD0[] = {0x40, 0, 0, 0, 0, CRC_TO_BE_CALCD}; //5 //0x95};
//From lec12_sd_card.pdf:
// NOTE: lec12 crc's don't match my CRC7 generator
//   which is odd, since it works with the supplied sequences from PLSv3
// Necessary for SDHC, expands ACMD41
//           Indicates we're supplying 2.7-3.6V ---vvvv
//lec12: CMD8 --> 01 001000 00000000 00000000 00000001 10101010 0000111 1
// ffsample/avr/mmc.c says it's 0x87 (which is what I get)
uint8_t spi_sd_CMD8[] = {0x48, 0, 0, 1, 0xAA, CRC_TO_BE_CALCD}; //5}; //0x0f};
//NOTE: Illegal Commands apparently don't check CRC...

//lec12: CMD58 -> 01 111010 00000000 00000000 00000000 00000000 0111010 1
//uint8_t spi_sd_CMD58[] = {0x7A, 0, 0, 0, 0, 0x75};
//Since the CRC is followed by a 1-bit, a 0-bit indicates we need to calc
//#define CRC_TO_BE_CALCD  0

//Command CMD55 used for indicating the next command is an ACMD
//uint8_t spi_sd_CMD55[] = {0x40|55, 0, 0, 0, 0, CRC_TO_BE_CALCD};
//Command ACMD41, but since it's sent after CMD55, and otherwise looks like
// a regular command, I'd rather call it aCMD41
#define HCS_ENABLED  0
uint8_t spi_sd_aCMD41[] = 
                     {0x40|41, 0 | HCS_ENABLED, 0, 0, 0, CRC_TO_BE_CALCD};


uint8_t spi_sd_CMD16[] = {0x40|16, 0, 0,
                           (uint8_t)((((uint16_t)BLOCKSIZE)>>8)),
                           (uint8_t)(BLOCKSIZE),
                           CRC_TO_BE_CALCD};


//Bit numbers of the R1 response...
#define R1_IDLE            0
#define R1_INVALID_COMMAND 2

//This should be called BEFORE heart_init or init_dmsTimer
//CURRENTLY Output->FALL Sample->RISE
// Appears to be correct...

// Can't locate SPI Mode (0-3) nor timing diagrams, nor edge-listings...
// Best I can find is P118, Figure 4-38, Physical Layer Spect v3 Final:
// This is for SD interface, and the best I can find regarding SPI 
//  is essentially "same as SD"
//             0       1       2         STOP
//  SDCLK: ___/¯¯¯\___/¯¯¯\___/¯¯¯\....___________
//         ___ _______ _______ ________
// DAT[3:0]___X_______X_______/
//
//  Then we have Change On Rising, Sample on Falling
//  This does NOT match the ELM document, but it doesn't seem to work.
//
//  It also doesn't seem to match Figures 6-8 through 6-11 (WTF?)
//
//  AND Figure 4-40 makes everything that much more complicated...
//  as it seems to suggest that the Master changes on rising edges,
//     sampled on falling edges,
//   yet the slave (SD card) seems to change after falling-edges,
//     sampled on rising.

// OY: ELM says it's Mode 0... (or "3 seems to work")
// it didn't work
// SanDisk (and figures above) show (for SD timing, not SPI mode):
//            
//             |-|<--- (>=50ns)
//             v v
//    ____/¯¯¯\___/¯¯¯\____
//            .   .
//            . ____
//   In     XXXX____XXXXXXX
//            .   .
//            .  _____
//  Out     XXXXX_____XXXXX
//            .  ^.
//            |  |
//            |--|<---0-50ns
//
//  Change on Fall, Sample on Rise


//Globalizing this to see about memory issues...
uint8_t csdVal[21];


void spi_sd_init(void)
{
   //Use PA2..0 for the USI (default is PB2..0)
   USIPP = (1<<USIPOS);
   setinpuPORT(SD_MISO_pin, SD_MISO_PORT);      //uC DI / MISO

   setoutPORT(SD_MOSI_pin, SD_MOSI_PORT);       //uC DO / MOSI
   setpinPORT(SD_MOSI_pin, SD_MOSI_PORT);
   
   setoutPORT(SD_SCK_pin, SD_SCK_PORT);         //uC SCK out
   clrpinPORT(SD_SCK_pin, SD_SCK_PORT);      //Set initial value (0)
                                             // According to the only
                                             // documentation I can find

   setoutPORT(SD_CS_pin, SD_CS_PORT);  //SD /CS
   setpinPORT(SD_CS_pin, SD_CS_PORT);  //Deselect it for now...


   //We're not supposed to access the card until at least 1ms after powerup
   _delay_ms(20);

   //To initialize the SD card in SPI mode, we first have to use 100-400kHz
   // afterwards, the bit-rate doesn't matter
   // We'll use 300kHz, since the document doesn't specify whether this is
   // toggling-rate or frequency
   // so it should be good 'nough (either 300, or 150)
   // and should be flexible for OSCCAL stuff, as well
   //This is now irrelevent... setting the timer to CLKDIV64 means that one
   //(1) TCNT is used beteween each clock transition...
   //with PLL system clock, this is 16MHz=FCPU -> 250k TCNTS/sec -> 125kHz CLK

   //...Now handled in the dmsTimer


   //During initialization: 
   //    the DO pin must be held high (done above)
   //    The clock must pulse 74+ times
   uint8_t i;
   for(i=0; i<200; i++)
   {
      //Since the SD card isn't selected, the pull-up will load
      // the data-register with 0xff upon reception...
      // so there shouldn't even be any glitches.
      spi_sd_transferByteWithTimer(0xff);
   }

   //Don't think there's any reason to keep this here...
   // don't need a delay between toggling the clock and CSing it, is there?

   //Enable the SD card
   clrpinPORT(SD_CS_pin, SD_CS_PORT);
   
   //Wait for a byte-length to make sure it gets it...?
   
   
   //Send CMD0
   uint8_t r1Response = 0xff;

   dms4day_t startTime = dmsGetTime();
   while(r1Response != 0x01)
   {
      spi_sd_sendEmptyCommand(0);

      //There shouldn't be a remaining response...
      r1Response = spi_sd_getR1response(TRUE);
      
      if(dmsGetTime() - startTime > 10*DMS_SEC)
         haltError(0x77);
   }

   //It should respond with 0x01, to indicate that it's in the idle state

#warning "TBR:"
   clrpinPORT(SD_CS_pin, SD_CS_PORT);

   //Send CMD8 to (begin to) determine if this is a newer (SDHC) card
   spi_sd_sendCommand(spi_sd_CMD8, sizeof(spi_sd_CMD8));
   r1Response=spi_sd_getR1response(FALSE);

   //If CMD8 is received and processed by a card supporting it,
   // it repies with an R7 response, which is an R1 *immediately* followed
   // by four additional bytes (immediacy is assumed by Figure 7-12)
   //IMMEDIACY ASSUMED:  ------------v  v----Should be 4???
   spi_sd_getRemainingResponse(NULL, 0, 0);

   // (This coulda been handled by getR1Response(TRUE)
   //  but at some point CMD8's response will *actually* have to be
   //  handled)

/*
   //FOLLOWING Figure 7-2 of Physical Layer Spec v3.00
   //For now we assume that all cards I've got available aren't SDHC
   // Thus, if we get an error 0x07, we know there's more coding ahead
   if(r1ResponseValid(r1Response) && 
                                 !getbit(R1_INVALID_COMMAND, r1Response))
   {
      //clrpinPORT(HEART_PINNUM, HEART_PINPORT);
      //while(1) asm("nop;");
      haltError(0x07);
   }

   pauseIndicate(r1Response);
*/ 
   //'course it's possible it didn't respond...

   //See SD Spec Physical Layer 3 Final... Figure 7-2
/* Not mandatory... only necessary for voltage range...
   //Send CMD58 to read the Operating Conditions Register (OCR)
   spi_sd_sendCommand(spi_sd_CMD58, sizeof(spi_sd_CMD58));

   //CMD58 responds with an R3 response, which is just an R1
   //  followed by four additional bytes...
   r1Response = spi_sd_getR1response();

   if(r1ResponseValid(r1Response) && 
                                 !getbit(R1_INVALID_COMMAND, r1Response))
   {
      //Get the OCR bytes...
      uint32_t ocr=0;
      ocr |= ((spi_sd_transferByteWithTimer(0xff)) << 24);
      ocr |= ((spi_sd_transferByteWithTimer(0xff)) << 16);
      ocr |= ((spi_sd_transferByteWithTimer(0xff)) << 8);
      ocr |= (spi_sd_transferByteWithTimer(0xff));

   }
*/

   /* This might only be necessary for detecting a card type...
      and setting operating conditions different than default
      NO: it's used on newer cards, CMD1 was used prior...
   */

   r1Response = 0xff;
// uint8_t attempts=0;



   startTime = dmsGetTime();

   //Try ACMD41 until it's no longer idle...
   while((!r1ResponseValid(r1Response) || getbit(R1_IDLE, r1Response)))
//       && (attempts < 100))
   {  
//    attempts++;

      // Send ACMD41 (argument 0x0)
      //   Initiate Initialization Process
      //    ACMDn implies CMD55 (no args) followed by CMDn
      // So send CMD55
      spi_sd_sendEmptyCommand(55); //spi_sd_CMD55, sizeof(spi_sd_CMD55));
      //Shouldn't be a remaining response...
      r1Response=spi_sd_getR1response(TRUE);
      // Should probably test its validity and legality

#warning "changeHere Should have no effect"
      //r1Response = 0xff;


      spi_sd_sendCommand(spi_sd_aCMD41, sizeof(spi_sd_aCMD41));
      //Shouldn't be a remaining response...
      r1Response=spi_sd_getR1response(TRUE);


      if(dmsGetTime() - startTime > 10*DMS_SEC)
         haltError(0x88);
   }

//#warning "This value is too low... should probably use time instead"
// if(attempts >= 100)
//    haltError(0x88);

   //AT THIS POINT
   // we should be OK!
#if(defined(PRINT_SDCONNECTED) && PRINT_SDCONNECTED)
   puat_sendStringBlocking_P(PSTR("SD Connected!\n\r"));
#endif
   //Set the block-length so we know what to expect...
   // doesnt really matter currently
   r1Response = 0xff;

   spi_sd_sendCommand(spi_sd_CMD16, sizeof(spi_sd_CMD16));
   //Shouldn't be a remaining response...
   r1Response = spi_sd_getR1response(TRUE);

   if(r1Response != 0x00)
   {
#if(defined(PRINT_CANTSETBLOCKSIZE) && PRINT_CANTSETBLOCKSIZE)
     puat_sendStringBlocking_P(PSTR("Can't Set BLK_SZ:512\n\r"));
#endif
     haltError(1);
   }

   //This is where CMD58 should probably be, to determine whether the CSD
   // format is SDHC or standard SD 
   // (but that's indicated in the CSD as well, no?)

   //We should get the CSD (Card Specific Data)
   // for *at least* the card-size... so writes can be wrapped-around
   // CSD: Section 5.3.2 in SP Spec Physical Layer 3.00 (PDF p144)
   // HAH! Or see Figure 7.1, says to issue command 58 (Card Capacity)
   // Actually, it appears that this only tells whether it's SDHC
   //SEND_CSD
   spi_sd_sendEmptyCommand(9); 
   r1Response = spi_sd_getR1response(FALSE);
   //Response Data could arrive N_CX bytes later... (0-8)
   // It looks like (and has been described as) a single-block data-read
   //   e.g. it starts with 0xfe, and ends with two bytes CRC
   // The CSD is 128bits (16 bytes)
   // getRemainingResponse loads 0xfe, CRC, and one additional byte...
   //  indicating the end... (1+16+2+1 = 20)
   //  and we're expecting 19 (1+16+2)

   //Globalizing this to see about memory issues...
   //uint8_t csdVal[21];
   spi_sd_getRemainingResponse(csdVal, 8, 19);

   //Make sure it's CSD structure v1.0 Standard Capacity (bits127,126=0,0)
   // v2.0 isn't handled yet... High/Extended Capacity (bits127,126=0,1)
   // other values are reserved
   uint8_t csd_version;
   csd_version = extractBitsFromU8Array(127, 126, &(csdVal[1]), 128);
   
   if(csd_version != 0)
   {
#if(defined(PRINT_BADCSDVER) && PRINT_BADCSDVER)   
      puat_sendStringBlocking_P(PSTR("CSDv???\n\r"));
#endif
      haltError(1);
   }

   uint16_t c_size;
   c_size = extractBitsFromU8Array(73, 62, &(csdVal[1]), 128);

   uint8_t read_bl_len; 
   read_bl_len = extractBitsFromU8Array(83, 80, &(csdVal[1]), 128);

   uint8_t c_size_mult; 
   c_size_mult = extractBitsFromU8Array(49, 47, &(csdVal[1]), 128);
   

#if(defined(PRINT_CSD) && PRINT_CSD)
   //We know this is true because of haltError above...
   puat_sendStringBlocking_P(PSTR("CSDv1.0\n\r"));

   sprintf_P(stringBuffer, PSTR("C_SIZE=%"PRIu16"\n\r"), c_size);
   puat_sendStringBlocking(stringBuffer);
   
   sprintf_P(stringBuffer, PSTR("READ_BL_LEN=%"PRIu8"\n\r"), read_bl_len);
   puat_sendStringBlocking(stringBuffer);
   
   sprintf_P(stringBuffer, PSTR("C_SIZE_MULT=%"PRIu8"\n\r"), c_size_mult);
   puat_sendStringBlocking(stringBuffer);
#endif




   //Number of blocks on the card...
   // Memory Capacity = (C_SIZE+1)*(2^(C_SIZE_MULT+2)*(2^READ_BL_LEN)
   // We only need the number of blocks... so no need to *2^READ_BL_LEN
   // Though we should probably use that elsewhere to make sure the
   // Block Size is 512...
   // in fact...
/*
   if((read_bl_len) != BLOCKSIZE_SHIFT)
   {
      //Should be OK, since CMD16 didn't error...
      puat_sendStringBlocking_P(PSTR("READ_BL_LEN!=9, OK?\n\r"));
      // haltError(2);
      //puat_sendStringBlocking_P(PSTR(" OK? CMD16 OK\n\r"));
   }
*/

   //SD Physical Layer Spec v3.00:
   // memory capacity = BLOCKNR * BLOCK_LEN
   //    BLOCKNR = (C_SIZE+1) * MULT
   //    MULT = 2^(C_SIZE_MULT+2)         (C_SIZE_MULT<8)
   //    BLOCK_LEN = 2^(READ_BL_LEN)      (READ_BL_LEN<12)
   //
   // "To indicate a 2GB card, BLOCK_LEN shall be 1024 bytes
   //  Therefore, the maximal capacity that can be coded is
   //  4096*512*1024 = 2 G bytes"
   // "The maximum data area size of Standard Capacity SD Card is
   //  4,153,344 sectors (2028MB)"

   // capacity = (C_SIZE+1) * 2^(C_SIZE_MULT+2) * BLOCK_LEN
   //          = (C_SIZE+1) * 2^(C_SIZE_MULT+2) * 2^READ_BL_LEN

   //This is the number of READ_BL_LEN blocks, NOT 512B blocks...
   // until later... READ THAT AGAIN.
   sd_numBLOCKSIZEBlocks = 
                     ((uint32_t)c_size+1)*((uint32_t)1<<(c_size_mult+2));
   //Now we convert it to 512B blocks...
   if(read_bl_len > BLOCKSIZE_SHIFT)
      sd_numBLOCKSIZEBlocks <<= (read_bl_len - BLOCKSIZE_SHIFT);
   else if(read_bl_len < BLOCKSIZE_SHIFT)
      sd_numBLOCKSIZEBlocks >>= (BLOCKSIZE_SHIFT - read_bl_len);

#if(defined(PRINT_SIZE) && PRINT_SIZE)
   sprintf_P(stringBuffer, PSTR("512B Blks:%"PRIu32),
                                                   sd_numBLOCKSIZEBlocks);
   puat_sendStringBlocking(stringBuffer);
   sprintf_P(stringBuffer, PSTR("=%"PRIu32"kB\n\r"),
         ((sd_numBLOCKSIZEBlocks>>(10-BLOCKSIZE_SHIFT))));
//       ((sd_numBLOCKSIZEBlocks>>10)<<BLOCKSIZE_SHIFT));

   puat_sendStringBlocking(stringBuffer);
#endif
   //This'll screw up math, above... test should be earlier, but it doesn't
   // affect functionality as long as it halts...
   if(read_bl_len < BLOCKSIZE_SHIFT)
      haltError(1);

#if(defined(START_STOP_TEST) && START_STOP_TEST>0)
   sd_numBLOCKSIZEBlocks = (38461*2 * START_STOP_TEST / 512);
   //861;   //Roughly 5 seconds...
#endif


/* This shouldn't be in SD_Init... it's not specific to SD cards
   but to the usage-table...
#if(USAGETABLE_SIZE != 256)
#error "This math needs revision if USAGETABLE_SIZE!=256"
#endif
   // This is sd_numBlocks / (numChunks==256)
   numBlocksPerChunk = (sd_numBLOCKSIZEBlocks>>8);
*/
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
//       thisChunkNum_u16 = 0xff, so increment it to 0
//       findNextUsableChunk(++thisChunkNum_u16)
//          may return 0, as well
//  3) At the end of a fragment e.g.
//       thisChunkNum_u16 = 5, increment it to 6 (which is known to be used)
//       findNextUsableChunk(++thisChunkNum_u16)
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
   while(0 != eeprom_read_byte((uint8_t *)((uint16_t)chunkNum)))
   {
      //This could just be ++'d if it weren't for this old note:
      //(Something about using blockPosition+=chunkSize in the works
      // instead of using multiplication, below...)
      // FURTHER: chunkNum CAN'T be > USAGETABLE_SIZE here...
      //if(thisChunkNum_u16 >= (USAGETABLE_SIZE))
      // thisChunkNum_u16 = 0;
      //else
      // thisChunkNum_u16++;
      chunkNum++;
   }

   return chunkNum;
}

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
//    uint8_t chunkTemp = thisChunkNum_u16;
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
      ut = eeprom_read_byte((uint8_t *)((uint16_t)chunkTemp));

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








#define BEGIN_WRITING (0xf001)
//Static variables are not allowed in inline functions...
// so make it global !WEEE!
// Huh, in other cases, doing this *increased* code-size...
//   this time it *shrank* by four bytes...
// Why? Maybe because it's initialized by the normal global initializer?
// but data and bss haven't changed...
// I dunno...
uint16_t sdWU_packetByteNum = BEGIN_WRITING;
// And globalling these two had no code-size effect...
uint8_t sdWU_dataState = 0;
cirBuff_dataRet_t sdWU_sample;


extern __inline__ void sd_writeUpdate(void)
{
   //This is the byte number *in the packet* (including the header and CRC)
   // which HAS BEEN SENT
   // e.g. if packetByteNum = 0, nothing has been sent (in this packet)
   //         packetByteNum = 1, Data Token has been sent
   //                         3, Data Token + Two Data Bytes have been sent
   // so CRC starts at 1+512 (BLOCKSIZE+1)
   // AFTER the CRC is sent, and whatnot, then this'll be WRITE_BUSY_STATE
   // until it's no longer busy and we return it to 0
   //static uint16_t packetByteNum = 0;
#define WRITE_BUSY_STATE   (0xffff)
//We've reached the end of the SD card...
#define STOP_WRITING (0xf000)
//#define BEGIN_WRITING (0xf001)

// static uint16_t packetByteNum = BEGIN_WRITING;

   switch(sdWU_packetByteNum)
   {
#warning "STOP_WRITING isn't very happy as a state-machine state..."
      //ONLY at the END of the SD Card/fragment
      // OR in the special-case where the usage-table has to be updated
      case STOP_WRITING:
         //Stop Transmission Token:
         //puat_sendStringBlocking("Stop Transmission: 0xfd\n\r");
         w_xfer(0xfd);


         if(usageTableUpdateState == 2)
         {
            //Gotta grab that BUSY...
            spi_sd_getRemainingResponse(NULL, 0, 0);

            //THIS WILL BLOCK! YAY!
            // What about that good-ol keyboard, etc...?
            sd_writeUsageTable();

            usageTableUpdateState = 3;

            //Return to handle determining which is the next block to write
            sdWU_packetByteNum = WRITE_BUSY_STATE;
//#error "need to resend startWriting!"
            break;
         }
         else
            sdWU_packetByteNum = BEGIN_WRITING;
         //Intentional fall-through

      //Moved here, to do calcs during busy (probably unnecessary, but
      // shouldn't hurt...
      case BEGIN_WRITING:
         {
         //After StopTransmission is sent, a "busy" will occur 1 byte later
         // (according to ELM)
      
         // While waiting for the BUSY, we can determine the next position
         
         //At this point chunkNum should be the next expected chunkNum
         // (regardless of whether that chunk is already used
         //   ...that's what we determine here.)
         //   It should be wrapped to 0 if the card-end was reached prior
         // On init, it will be 0, and will likely be available
         // After Block Complete at the end of a fragment, 
         //     it will be the next UNAVAILABLE chunk...
         //     or 0, if the end of the card was reached.
         // it should work for both cases...


/* LIES ALL LIES!!! 
   This note was due to a stringBuffer bug which *appeared* to result in
   two startWritingBlocks() calls after every keypress
   Fact is, it wasn't due to the explanation below.
   That explanation was assuming I'd used the 
   check-chunks-after-every-block method
   but REALLY we only check for chunks after a chunk completes.
   IOW: No Chunk-Bypassing Bug.

         //This doesn't work, because after the usageTable is updated
         // due to a key-press, etc. the next block in the same chunk
         // will indicate as used (by this same loop!)
         //while(0 != eeprom_read_byte((uint8_t *)thisChunkNum_u16))

         //This is ugly, but it should work, see cTools/testAssignTest.c
#warning "thisLoopNum == 0xff is poorly handled."
         // NOTE: This does NOT fix the bug in the thisLoopNum==0xff case
         // Not much can be done for it.
         uint8_t utVal;
         while( (0 != (utVal=eeprom_read_byte((uint8_t *)thisChunkNum_u16)))
               && (utVal != thisLoopNum) )
*/
/* 
#warning "One more time... when we reach BEGIN_WRITING after a utUpdate, wo
n't this cause it to search for the next chunk?!"

NO. BEGIN_WRITING IS ONLY *ONLY* called at:
      * boot
      * changeover from the last available chunk in a fragment
         to the next fragment
      * After the card-end is reached

   updateTable is doesn't call BEGIN_WRITING
   It handles sd_startWritingBlocks on its own.
   
*/

            thisChunkNum_u16 = findNextUsableChunk(thisChunkNum_u16);
#if(defined(PRINT_CHUNKADVANCE) && PRINT_CHUNKADVANCE)
            sprintf_P(stringBuffer, PSTR("cn:0x%"PRIx8), (uint8_t)thisChunkNum_u16);
            puat_sendStringBlocking(stringBuffer);
#endif
            //Now thisChunkNum_u16 IS available...
         
            //Calculate the block number
            uint32_t chunkStartBlock;
            chunkStartBlock = startingBlockNumFromChunk(thisChunkNum_u16);

            //Don't overwrite the usage-table!
            //See note for startingBlockNumFromChunk()...
            if(chunkStartBlock == 1)
               blockNum_inThisChunk = 1;
            //Reset the counter for the next go-round...
            else
               blockNum_inThisChunk = 0;


#warning "These are WithTimer transactions!"

   
            //This should grab the BUSY... (from STOP_WRITING)
            spi_sd_getRemainingResponse(NULL, 0, 0);

            spi_sd_startWritingBlocks(chunkStartBlock);
         }
         sdWU_packetByteNum = 0;
         break;
      //Send the CRC (unused, but two bytes must be sent)
      case (BLOCKSIZE+1):
         {
            //Two unverified CRC bytes
            w_xfer(0x00);
            w_xfer(0x00);
            sdWU_packetByteNum++;
         
            //Decrement after each block...
            if(kbIndicateCount)
               kbIndicateCount--;
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
               //char string[35];
               sprintf_P(stringBuffer,
                     PSTR("Bad Data Resp1: 0x%"PRIx8"\n\r"),
                     dataResponse);
               puat_sendStringBlocking(stringBuffer);
#endif
               haltError(0x0f);
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
               puat_sendStringBlocking(string);
               sei();
            } */

            //Writing doesn't begin until another 8 clocks are received
            // It should respond with 0x00 that it's busy, but we'll just
            // assume that's the case...
            w_xfer(0xff);
            //So technically we're just using sdWU_packetByteNum 
            //as a state-indicator now...
            sdWU_packetByteNum = WRITE_BUSY_STATE;
            //writeBusyCount = 0;
         }
         break;
      //At the end of EVERY 512B block...
      // The card will be busy for a little while while updating the FLASH
      // This is the last state before the next packet (normally)
      case WRITE_BUSY_STATE:
         {
            uint8_t sdResponse = w_xfer(0xff);
            
            //STILL BUSY...
            if(sdResponse != 0xff)
            {
#if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
               writeBusyCount++;
#endif
               break;
            }
            //NO LONGER BUSY, USAGE-TABLE NEEDS TO BE UPDATED
            else if(usageTableUpdateState == 1)
            {
               usageTableUpdateState = 2;

               sdWU_packetByteNum = STOP_WRITING;
               //It should return here to handle the else case, below
               // (STOP_WRITING handles this when usageTableUpdate=TRUE)
            
               break;
            }



            //NO LONGER BUSY if we've gotten this far...
            // This is the NORMAL case... 

            //else //sdResponse == 0xff (no longer busy)
            //{
               //There are two cases where we can arrive here...
               // Normal (at the end of every block)
               // After usageTableUpdate rewrites the usageTable...
               //      via STOP_WRITING
               //   The usageTable writes occur after *every* block
               //   NOT aligned with a chunk
               //   Running it cancels the WriteMultipleBlocks command
               //   so it needs to be retransmitted!
               //   but where?
               //   and what if this go-round is a stop-anyhow...?


#if(defined(PRINT_BWandWBC) && PRINT_BWandWBC)
               blocksWritten++;
#endif
               
               blockNum_inThisChunk++;

               if(blockNum_inThisChunk == numBlocksPerChunk)
               {
                  blockNum_inThisChunk = 0;

                  //This will wrap-around at the end of the card to 0
                  // handled later, it's uint16_t
                  thisChunkNum_u16++;

                  //Check if the next chunk is available...
                  // This must fail if we're at the end of the card!
                  // Also, don't read the eeprom, if that's the case...

                  //At the END OF THE CARD...
                  // This is essentially thisChunkNum_u16%256
                  // since tCN is uint16 (because it can increment past
                  // 255 for findNextAvailableChunk? Is that still the
                  // case?)
                  if(((uint8_t)thisChunkNum_u16) == 0)
                  {
                     //Regardless of whether chunk 0 is in use
                     // we still have to stop writing, and restart later
                     // BEGIN_WRITING will take care of finding the next
                     // actually available chunk (if 0 is in use)


                     //See explanation in else below...
                     sdWU_packetByteNum = STOP_WRITING;

                     //At the end of the card, increment loopNum...
                     if(thisLoopNum < 0xff)
                        thisLoopNum++;
                  }
                  //Next chunk IS AVAILABLE (and sequential)
                  else if(0 == eeprom_read_byte((uint8_t *)thisChunkNum_u16))
                  {
                     //Next chunk IS AVAILABLE... nothing to do here
                     // just start writing the next packet...
                     sdWU_packetByteNum = 0;
#if(defined(PRINT_CHUNKADVANCE) && PRINT_CHUNKADVANCE)
                     sprintf_P(stringBuffer, PSTR("cn:0x%"PRIx8), 
                           (uint8_t)thisChunkNum_u16);
                     puat_sendStringBlocking(stringBuffer);
#endif

                  }
                  //Next chunk is NOT AVAILABLE
                  else
                  {
                     //Next chunk is NOT AVAILABLE (in use)
                     // Stop writing, and it'll start again at the next
                     // available chunk
                     // (thisChunkNum_u16 will be updated accordingly
                     //  and SHOULD be left pointing to the next
                     //  unavailable chunk)
                     sdWU_packetByteNum = STOP_WRITING;
                  }

               }
               //At a block-transition inside a chunk
               // Nothing to do here...
               else
                  sdWU_packetByteNum = 0;
         // } // End of No Longer Busy case

            //If the usage-table was written to SD, then we need to
            // reroute the next state based on whether we were in the
            // middle of a chunk, or at the end of one...
            if(usageTableUpdateState == 3)
            {
               usageTableUpdateState = 0;

               //Starting a new chunk...
               // (either the end of the card, or next chunk not available)
               if(sdWU_packetByteNum == STOP_WRITING)
               {
                  //Since writing has already been stopped, 
                  // don't resend the stopWriting command...
                  // skip that state.
                  //The chunkNum, etc. will be calculated in the next state
                  sdWU_packetByteNum = BEGIN_WRITING;
               }
               //Continuing mid-chunk...
               // (packetByteNum == 0)
               // (either next sequential chunk is available, or
               //  at a block-transition within a chunk)
               else
               {
                  //Otherwise we're not at the end of a fragment
                  // if it weren't for the usage-table update we'd just
                  // continue with the old MultipleBlockWrite command
                  // But instead, we have to send the MultiplBlockWrite
                  // command, again
                  // at one after whatever block we were at before...
                  uint32_t reStartBlock;
                  //Get the chunk-offset
                  //reStartBlock = (uint32_t)thisChunkNum_u16 
                  //                * numBlocksPerChunk;
                  // Heh, there woulda been a bug allowing usage-table
                  //  overwriting, with the old method!
                  //  Nah, 'cause thisChunkNum_u16==0 wouldn't be the case
                  //    if we're here (mid-chunk)
                  reStartBlock = startingBlockNumFromChunk(thisChunkNum_u16);

                  //And add the blockNum offset
                  //This has been incremented/zeroed appropriately...
                  reStartBlock += blockNum_inThisChunk;
                  
                  spi_sd_startWritingBlocks(reStartBlock);
               }
            } //End of usage-table-update -> state-redirecting
         } //This entire case is in brackets... here's the end.
         //No More
         //Intentional Fall-through if sdResponse is non-busy (0xff)
         // and we haven't reached the end of the SD Card...
         break;
      //Send the Data Token
      case 0:
         w_xfer(0xfc);
         sdWU_packetByteNum++;
         //Intentional Fallthrough
         break;   //IntentionalFallthrough Broken for timing testing
      //Data bytes:
      default:
         {
            //Moved to global, since statics aren't supposed to be
            // in inline functions...
            //static uint8_t sdWU_dataState = 0;
            //static cirBuff_dataRet_t sample;
            switch(sdWU_dataState)
            {
               case 0:
                  //Only do this if there's data in the cirBuff...
                  //cirBuff_dataRet_t sample;
                  sdWU_sample = cirBuff_get(&myCirBuff);
                  if(sdWU_sample != CIRBUFF_RETURN_NODATA)
                     sdWU_dataState++;
                  break;
               case 1:
                  {
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
                     if( (eeprom_read_byte((uint8_t *)thisChunkNum_u16)
                              != thisLoopNum )
                        || writeMemoToUT )
                     {
                        writeMemoToUT = FALSE;
                        usageTableUpdateState = 1;
                        //don't use write_byte instead of update_byte
                        // since this could've entered due to 
                        // writeMemoToUT
                        eeprom_update_byte((uint8_t *)thisChunkNum_u16, 
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
               // sdWU_dataState++;
               // break;
               // }
               //case 2:
               // {
                  uint8_t sampleLow = (uint8_t)(sdWU_sample);
                  w_xfer(sampleLow);
                  sdWU_dataState = 0;
                  sdWU_packetByteNum+=2;
                  break;
                  }
               default:
                  break;   //Shouldn't get here...
            }
/*
            {
               uint8_t sampleHigh = (sample>>8);
               uint8_t sampleLow = (uint8_t)(sample);
               w_xfer(sampleHigh);
               w_xfer(sampleLow);
               sdWU_packetByteNum+=2;
            }
*/
         }
   }  

}


void spi_sd_startWritingBlocks(uint32_t startBlock)
{
   //Start at the second block (don't overwrite the usage Table)
   // (TODO: SDHC cards use BLOCK numbers in the argument
   //  Standard-Capacity cards use BYTE numbers in the argument!)
   uint8_t spi_sd_CMD25[] = {0x40|25, 0, 0, (uint8_t)(BLOCKSIZE>>8), 
                                 (uint8_t)(BLOCKSIZE), CRC_TO_BE_CALCD};


#if(defined(PRINT_SWB) && PRINT_SWB)
   puat_sendStringBlocking_P(PSTR("sWB\n\r"));
#endif
   
   // ASSUMING STANDARD CAPACITY!
/* uint32_t *byteAddressArgument;
   byteAddressArgument = (uint32_t *)(&(spi_sd_CMD25[1]));

   *byteAddressArgument = (startBlock<<BLOCKSIZE_SHIFT);
*/
   //NOGO above... Appears to write low-byte first

   startBlock <<= BLOCKSIZE_SHIFT;

   spi_sd_CMD25[1] = (uint8_t)(startBlock>>24);
   spi_sd_CMD25[2] = (uint8_t)(startBlock>>16);
   spi_sd_CMD25[3] = (uint8_t)(startBlock>>8);
   spi_sd_CMD25[4] = (uint8_t)(startBlock);

   //puat_sendStringBlocking_P(PSTR("sWB\n\r"));

   uint8_t r1Response;

   //spi_sd_sendEmptyCommand(25);
   spi_sd_sendCommand(spi_sd_CMD25, sizeof(spi_sd_CMD25));

   //SEE BELOW:
   r1Response = spi_sd_getR1response(TRUE);
   
   //There's suppose to be a byte or two before sending the first packet
   // this should work... (now handled in TRUE above)
   //spi_sd_getRemainingResponse(NULL, 0, 0);
}

/*
void spi_sd_startReadingBlocks(void)
{
   //Start reading at the beginning of the SD card...
   //uint8_t spi_sd_CMD18[] = {0x40|18, 0, 0, 0, 0, CRC_TO_BE_CALCD};
   uint8_t r1Response;

   spi_sd_sendEmptyCommand(18); 
#warning "CHANGE HERE:"
   // wasn't TRUE before...
   r1Response = spi_sd_getR1response(TRUE);

}
*/
uint8_t spi_sd_transferByteWithTimer(uint8_t txByte)
{
   USIDR = txByte;

      //This might actually not be safe, because it would also clear bits
      // that are flagged as 1...
      // Also might rewrite the counter value to its old value if it had
      //  been updated during the read->write of USISR
      //  (only a problem when not software-clocked...)
      //clrbit(USIOIF, USISR);
#define spi_clearCounterOverflowFlag_AndCounter()  USISR = (1<<USIOIF)
      spi_clearCounterOverflowFlag_AndCounter();
      
      //Shift the bits until the data completes
      while(!getbit(USIOIF, USISR))
      {
         //Do this first, so we know the first bit will be a full TCNT
         uint8_t tcntStart = TCNT0L;

         while(TCNT0L == tcntStart) asm("nop;");

         spi_StrobeClockAndShift();
      }
      
      //The flag doesn't reset itself...
      spi_clearCounterOverflowFlag_AndCounter();

      uint8_t dataIn = USIDR;

#if (defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)
      sprintf_P(stringBuffer, PSTR("0x%"PRIx8">0x%\n\r"PRIx8));
      puat_sendStringBlocking(stringBuffer);
#endif

      return dataIn;
}

uint8_t spi_sd_transferByte(uint8_t txByte)
{
   USIDR = txByte;
   spi_clearCounterOverflowFlag_AndCounter();

// while(!getbit(USIOIF, USISR))
// {
//    spi_StrobeClockAndShift();
// }

   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();
   spi_StrobeClockAndShift();

   spi_clearCounterOverflowFlag_AndCounter();

#if(defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)
   sprintf_P(stringBuffer, PSTR("tx:0x%"PRIx8" -> rx:0x%"PRIx8"\n\r"),
                                       txByte, USIDR);
   puat_sendStringBlocking(stringBuffer);
#endif

   return USIDR;
}

//Global so we see it in the memory usage...
uint8_t emptyCommand[] = {0,0,0,0,0,0};

//Most commands are empty (just a command number, 
//  followed by 4 0's and CRC)
void spi_sd_sendEmptyCommand(uint8_t cmdNum)
{
   emptyCommand[0] = 0x40 | cmdNum;
   //Clear whatever CRC may have been written in a previous sendCommand...
   emptyCommand[5] = CRC_TO_BE_CALCD;

   spi_sd_sendCommand(emptyCommand, 6);
}

void spi_sd_sendCommand(uint8_t *command, uint8_t length)
{
   uint8_t i;

   if(command[length-1] == CRC_TO_BE_CALCD)
   {
      command[length-1] = sd_generateCRC7(command, length-1) | 1;
   }

#if(defined(PRINT_COMMAND) && PRINT_COMMAND)
   puat_sendStringBlocking_P(PSTR("CMD: "));
#endif
   for(i=0; i<length; i++)
   {
      spi_sd_transferByteWithTimer(command[i]);
#if(defined(PRINT_COMMAND) && PRINT_COMMAND)
      sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), command[i]);
      puat_sendStringBlocking(stringBuffer);
#endif
   }
#if(defined(PRINT_COMMAND) && PRINT_COMMAND)
   puat_sendStringBlocking_P(PSTR("\n\r"));
#endif
}

//If it doesn't come through within 16 tries, it will return 0xff!
// So be careful when testing those bits!
// if getRemaining is true, it'll call getRemainingResponse with NULL...
uint8_t spi_sd_getR1response(uint8_t getRemaining)
{
   uint8_t r1Response, i;

   //Wait for the response (it could be up to 8 transfers later)
   // This is according to N_CR = (0-8)
   //uint8_t r1Response = 0;
   for(i=0; i<16; i++)
   {  
      if((r1Response=spi_sd_transferByteWithTimer(0xff)) != 0xff)
         break;   //Some response recieved... while will test it.
   }


#if(defined(PRINT_R1) && PRINT_R1)
   sprintf_P(stringBuffer, PSTR("R1:0x%"PRIx8"\n\r"),
                                       r1Response);
   puat_sendStringBlocking(stringBuffer);
#endif


   //This should only be true when we're not expecting any...
   // e.g. CMD8, since we haven't yet implemented a host for devices
   // which support it... this'll just clear the response out, in 
   // case we can get further... (?) kinda hokey...
   if(getRemaining)
      spi_sd_getRemainingResponse(NULL, 0, 0);

   return r1Response;
}

//This'll just grab a bunch of bytes until 0xff has been received a couple
// times...
// it'll place them in buffer, and return the number of bytes.
// If buffer is NULL it won't write to it, and will instead just return
// the number of bytes...
// delayBytes is the number of 0xff's to send initially before giving up
// (e.g. CMD9=SEND_CSD replies R1 + Data, between R1 and Data could be
//  up to 8 0xff's returned...)
uint8_t spi_sd_getRemainingResponse(uint8_t *buffer, uint8_t delayBytes,
                                          uint8_t expectedBytes)
{
   uint8_t count = 0;
   uint8_t lastReceived = 0;

   uint8_t thisReceived = 0xff;
   uint8_t messageTransmitted = FALSE;

   //Because of do-while instead of while, we need to grab the first
   // byte after delayBytes (if no other data was received earlier)
   // This WILL grab the first byte if delayBytes == 0
   for(count=0; count<=delayBytes; count++)
   {
      thisReceived = spi_sd_transferByteWithTimer(0xff);
          
      if(thisReceived != 0xff)
         break;
   }

   count = 0;
   //TODO: Look into this further...
   //      If delayBytes are ALL 0xff, and delayBytes+1 is 0xff
   //      then 0xff will be added to the
   //      buffer, or printed...
   //if(delayBytes && (thisReceived == 0xff)
   //while(((thisReceived=spi_sd_transferByteWithTimer(0xff)) != 0xff)
   //    || (lastReceived != 0xff))
   do 
   {
      //lastReceived = thisReceived;
      if(buffer != NULL)
         buffer[count] = thisReceived;
      //else
      
#if(defined(PRINT_REMAINING_RESPONSE) && PRINT_REMAINING_RESPONSE)
      {
         if((!messageTransmitted) && (thisReceived != 0xff))
         {
            messageTransmitted = TRUE;
            puat_sendStringBlocking_P(PSTR("Additional Response: "));
         }
         if(messageTransmitted)
         {
            //char string[20];
            sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), thisReceived);
            puat_sendStringBlocking(stringBuffer);
         }
      }
#endif

      count++;
      lastReceived = thisReceived;

   //Keep doing this until two consecutive bytes are 0xff
   } while( (count < expectedBytes) |
         (
         ! (((thisReceived=spi_sd_transferByteWithTimer(0xff)) == 0xff)
                 && (lastReceived == 0xff)) )
         );

   //while(((thisReceived=spi_sd_transferByteWithTimer(0xff)) != 0xff)
   //          || (lastReceived != 0xff));


   count-=1;

#if(defined(PRINT_REMAINING_RESPONSE) && PRINT_REMAINING_RESPONSE)
   if(messageTransmitted)
   {
      //char string[35];
      sprintf_P(stringBuffer,PSTR("(%"PRIu8"Bytes -trailing 0xff)\n\r"), count);
      puat_sendStringBlocking(stringBuffer);
   }
#endif

   return count; //Don't count the two 0xff's indicating the end...
         
}

void pll_enable(void)
{
   //Stolen from LCDdirectLVDSv54:
   //Stolen from threePinIDer109t:

   //Set Timer1 to use the "asynchronous clock source" (PLL at 64MHz)
   // With phase-correct PWM (256 steps up, then back down) and CLKDIV1
   // this is 64MHz/512=125kHz
   // The benefit of such high PWM frequency is the low RC values necessary
   //  for filtering to DC.
   // "To change Timer/Counter1 to the async mode follow this procedure"
   // 1: Enable the PLL
   setbit(PLLE, PLLCSR);
   // 2: Wait 100us for the PLL to stabilize
   // (can't use dmsWait since the timer updating the dmsCount 
   //  hasn't yet been started!)
   _delay_us(100);
   //   dmsWait(1);
   // 3: Poll PLOCK until it is set...
   while(!getbit(PLOCK, PLLCSR))
   {
      asm("nop");
   }
   // 4: Set the PCKE bit to enable async mode
   setbit(PCKE, PLLCSR);

}

void pwmTimer_init(void)
{
   //Stolen and modified from LCDdirectLVDSv54:

   //Value to count to...
   //We want it to count 7 bits, 0-6 and reset at 7
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
}





//CSD is being a punk...
/*
typedef struct
{
   uint8_t  csd_structure  :  2;
   uint8_t  reserved1      :  6;
   uint32_t unused1        :  24;
   uint16_t ccc            :  12;
   uint8_t  read_bl_len    :  4;
   uint8_t  unused2        :  4;
   uint8_t  reserved2      :  2;
   uint16_t c_size         :  12; //<--This is aligned such that it's in
   uint16_t unused3        :  12;   // three bytes! two bits on each of the
   uint8_t  c_size_mult    :  3;    // surrounding bytes, and a full byte
   uint8_t  erase_blk_en   :  1;    // in between YAY!!!
   uint8_t  sector_size    :  7;    // Bits73:62
   uint8_t  unused4        :  8;
   uint8_t  reserved3      :  2;
   uint8_t  unused5        :  3;
   uint8_t  write_bl_len   :  4;
   uint8_t  unused6        :  1;
   uint8_t  reserved4      :  5;
   uint8_t  unused7        :  6;
   uint8_t  reserved5      :  2;
   uint8_t  crc            :  7;
   uint8_t  always1        :  1;
}  csd_v1_t __attribute__((__packed__));
*/

// Assuming uint8_t array, such that the MSB is located at array[0]..
// Also assuming that first byte is completely filled
// Also assuming we're not grabbing more than 32 bits...
// Also assuming we're not grabbing from more than 4 bytes (?)
//
// e.g. extractBitsFromU8Array(73, 62, csd_response, 128);
//#define extractBitsFromU8Array(highBit, lowBit, array, arrayBits)
uint32_t extractBitsFromU8Array(uint8_t highBit, uint8_t lowBit,
                                uint8_t array[], uint8_t arrayBits)
{
   //e.g. c_size: Bits73:62 in a 128-bit array
   //      arrayBytes = (128+7)/8 = 16
   uint8_t arrayBytes = (arrayBits+7)/8;

   //      highByteIndex = 16 - 1 - 73/8 = 15 - 9 = 6
   uint8_t highByteIndex = arrayBytes - 1 - highBit/8;

   //      lowByteIndex = 16 - 1 - 62/8 = 15 - 7 = 8
   uint8_t lowByteIndex = arrayBytes - 1 - lowBit/8;

   uint8_t i;

   uint32_t dataTemp = 0;

   //      numBytesToGet = 8 - 6 + 1 = 3
   uint8_t numBytesToGet = lowByteIndex - highByteIndex + 1;


   //      leftShift = 7 - 73 % 8 = 7 - 1 = 6
   uint8_t leftShift = 7 - highBit % 8;   // highBit & 0x07

   //    0  -> 2
   for(i=0; i<numBytesToGet; i++)
   {
      //      byteIndex = 6+0=6, 6+1=7, 6+2=8
      uint8_t byteIndex = highByteIndex+i;

      //      array[6], array[7], array[8]
      uint8_t thisByte = array[byteIndex];

//      printf("array[%"PRIu8"]=0x%"PRIx8"\n", byteIndex, thisByte);

      //If we're on the first (high) byte, remove the leading bits
      if(i==0)
      {
         //array[6]<<=6
         thisByte <<= leftShift;
         //array[6]>>=6
         thisByte >>= leftShift;
      }

      //      (array[6])<<(3-0-1)*8 = array[6]<<2*8 = array[6]<<16
      //            [7]   (3-1-1)*8 =      [7]  1*8 = array[7]<<8
      //            [8]   (3-2-1)*8 =      [8]  0*8 = array[8]<<0    
      dataTemp |= ((uint32_t)(thisByte)<<((numBytesToGet-i-1)*8));
   }

   // Now we should have all the bytes containing the data to be extracted
   // WITH the leading bits remove, but it still needs to be shifted right
   //      rightShift = 62 % 8 = 6 
   uint8_t rightShift = lowBit % 8; // == lowBit & 0x07

   //Duh, this dun woik, since dataTemp is more bytes than numBytesToGet
   //dataTemp <<= leftShift;

   dataTemp >>= rightShift;

   return dataTemp;
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
 * /home/meh/_avrProjects/audioThing/57-heart2/_old/main_tCNfix.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
