/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */






#include "sd.h"


//Now that we're switching over to tcnter from dms...
#if(defined(_DMSTIMER_HEADER_))
   #define timeout_t dms4day_t
   #define TIMEOUT_SEC  DMS_SEC
   #define timeout_init()  dmsGetTime()
   #define timeout_check(p_start, time) \
               dms_isItTimeV2((p_start), (time), FALSE)
#else //assuming tcnter...
   #define timeout_t myTcnter_t
   #define TIMEOUT_SEC  TCNTER_SEC
   #define timeout_init()  ({ tcnter_update(); tcnter_get(); })
                                 //should return the value of tcnter_get()
   #define timeout_check(p_start, time) \
   ({ \
      tcnter_update(); \
      tcnter_isItTimeV2((p_start), (time), FALSE); \
   })
#endif









#warning "HCS_Enabled needs to be verified!"
#define HCS_ENABLED TRUE


//#warning "THIS NEEDS TO BE READ/SET"
// Most cards default to 512, newer ones can ONLY do 512
// Would be wise to verify it's set to 512, but I don't feel like copying
// over the code from audioThing_v6_2
#define BLOCKSIZE 512 //2048
// This is the BLOCKSIZE as it would be represented by (1<<BLOCKSIZE_SHIFT)
#define BLOCKSIZE_SHIFT (DESHIFT(BLOCKSIZE))

#if(BLOCKSIZE_SHIFT != 9)
#error "BLOCKSIZE should be 512.. This'll cause some math issues..."
#endif


//These are still in main (later to be makefiled?)
//#define PRINT_COMMAND TRUE
//#define PRINT_R1         TRUE
//#define PRINT_CSD     TRUE
//#define PRINT_REMAINING_RESPONSE  TRUE
//#define PRINT_SWB TRUE


//#define PRINT_BAD_DATARESPONSE TRUE
////#define PRINT_SDCONNECTED       TRUE
//#define PRINT_CANTSETBLOCKSIZE TRUE
//#define PRINT_BADCSDVER        TRUE
////#define PRINT_SIZE              TRUE


/* Moved to pinout.h
#define SD_CS_pin    PA3
#define SD_CS_PORT   PORTA
#define SD_MOSI_pin  PA1
#define SD_MOSI_PORT PORTA
#define SD_MISO_pin  PA0
#define SD_MISO_PORT PORTA
#define SD_SCK_pin   PA2
#define SD_SCK_PORT  PORTA
*/



uint32_t extractBitsFromU8Array(uint8_t highBit, uint8_t lowBit,
                                uint8_t array[], uint8_t arrayBits);





//This might have to be uint64_t for SDHC...
uint32_t sd_numBLOCKSIZEBlocks = 0;








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


//void spi_sd_startReadingBlocks(void);
void spi_sd_startWritingBlocks(uint32_t startBlock);

void spi_sd_init(void);



//Internal, called by spi_sd_init():
//2B!
static __inline__
void spi_sd_v1_finishInit(void)
   __attribute__((__always_inline__));
//And the remainder from 8094 to 8086...
static __inline__
void spi_sd_v2_finishInit(void)
   __attribute__((__always_inline__));

void spi_sd_58andA41(uint8_t hcsEnabled);


static __inline__
void spi_sd_initSizeSpecs(void)
   __attribute__((__always_inline__));

//This is CMD58, but only returns the CCS value...
// (In other words, the voltage-ranges are NOT TESTED)
//If doGetCCS is true, then it will return -1 until the power-up-status bit
//is no longer "busy" (the CCS bit is invalid until power-up-status is not
//busy)
// Thus, allowing-for calling this within a loop.
//This change, addition of the doGetCCS argument, is unnecessary. It
//doesn't do anything.
//RETURNS: -1 if the card is still busy powering-up
//         0 if the CCS bit is 0 (standard-capacity card)
//         1 if the CCS bit is 1 (high-capacity card)
#define DO_GET_CCS   TRUE
#define DONT_GET_CCS FALSE
int8_t spi_sd_sendCMD58(uint8_t doGetCCS);

//Note csdv1 is not the same as sd_v1
static __inline__
void spi_sd_csdv1_calcBlocksizeBlocks(void)
   __attribute__((__always_inline__));

static __inline__
void spi_sd_csdv2_calcBlocksizeBlocks(void)
   __attribute__((__always_inline__));


// This is my code-flow... attempting to combine various branch-pieces of
// Figures 7-1 and 7-2
//
// spi_sd_init()
//  spi_init()
//  port directions
//  delay 20ms
//  toggle the clock several times (transferByte(0xff))
//  |
//  send CMD0
//   repeat until valid R1 response (0x01)
//  |
//  send CMD8
//  |
//  CommandIsLegal?
//   |
//   n--v1finish()
//   |   58and41(FALSE)
//   |    sendCMD58(DONT_GET_CCS)
//   |   /sendEmptyCommand(55)
//   |   |aCMD41_setHCS(FALSE)
//   |   \sendCommand(ACMD41)
//   |   (repeat until initialized)
//   |
//   y--v2finish()
//       58and41(HCS_ENABLED)
//        sendCMD58(DONT_GET_CCS)
//       /sendEmptyCommand(55)
//       |aCMD41_setHCS(HCS_ENABLED)
//       \sendCommand(ACMD41)
//       (repeat until initialized)
//
//       sendCMD58(DO_GET_CCS)
//
//  initSizeSpecs()
//   high-capacity?
//    |
//    n--cv1Calc()
//    |
//    y--cv2Calc()

// This flow-chart is old, and a bit over-simplified...
//    spi_sd_init()
//      |
//   cardVersion?
//    /v1     \v2
//   /         \                    //
// v1finish   v2finish
//   |          |
//  58and41(F)  58and41(HCS_ENABLED)
//   |          |
//   |         getCCS()
//   |          |
//    \         /
//    initSizeSpecs
//       |
//    high-capacity?
//     /n       \y
//    /          \                  //
// cv1Calc      cv2Calc
//   



// SD Card Physical Layer Specification Version 3.00:
// Contains two figures for SPI initialization (Figures 7-1 and 7-2).
// These two figures are basically the same, but with different details
// Here I try to combine them


// Redrawn from Figure 7-1:
//
// This whole block is basically nothing more than Power On (see F7-2)
// .....................................................................
// .             {In SD Bus mode} ("from any state except inactive")   .
// .                    |                                              .
// .                    V                                              .
// .                  CMD0                                             .
// . Power On           |                                              .
// .        \           V                                              .
// .         ----> {Idle State}    ----> (SD Bus operation modes)      .
// .                    |                  (irrelevent for SPI)        .
// .....................................................................
//                      |
//                      V
//                    CMD0 + /CS asserted    (switch to SPI mode)
//                      |
//                      V
//              {SPI Operation Mode}
//                      |
//  CMD0                |
//  (from any state     |
//   in SPI mode) ----->|
//                      |    This drawing disregards CMD8-invalid
//                      |    which indicates the Card Version (1.x vs 2.x)
//                      |    See figure 7-2.          vvvvvvvvvvvvvvvvvvv
// .....................|................................................
// .                    V                                               .
// .                  CMD8  ("It is mandatory for the host compliant to .
// . non-supported   /  |     Physical Spec Version 2.00 to send CMD8") .
// . voltage range `/   |                                               .
// ................/....|................................................
//                v     |                                               
//   {CARD UNUSABLE}    |
//   {BY THIS HOST}     |
//             ^        V
//              \____CMD58 (Read OCR) ("Not Mandatory... it is
//                   /  |               recommended ... for voltage range")
//        invalid   /   |
//        command `/    |
//                v     |                    / 1: Executing Internal 
//{NOT SD MEMORY CARD}  |   __              |     Intialization Process
//            ^         V  V  \             |  2: High Capacity Cards:
//             \_____ACMD41    |`Card Busy <     A: CMD8 was not issued
//                      |  \__/             |       Prior to ACMD41
//                      |                   |    B: ACMD41 was issued with
//                      |                    \      HCS=0
// .....................|.................................................
// .                    V                                                .
// .                 CMD58 (Get CCS) ("Host shall issue CMD58 [here] to get
// .                    |              card capacity information (CCS)") .
// .                    |                                                .
// .....................|.................................................
//                      |           CMD58, here, is *not* shown for Ver 1.x
// ^^^^^^^^^^^^^^^^^^^  |           cards in Figure 7-2.
// Card Identification  |
// Mode                 |
// ^^^^^^^^^^^^^^^^^^^  |
//______________________|_________________________________________________
//                      |
// vvvvvvvvvvvvvvvvvvv  |
// Data Transfer        |
// Mode                 |
// vvvvvvvvvvvvvvvvvvv  V


//NOTE: CMD1 should not be used... (see original  Figure 7-1)



// Redrawn from Figure 7-2: SPI Mode Initialization Flow
//
//                  Power On
//                      |
//                      V
//                    CMD0 + /CS asserted    (switch to SPI mode)
//                      |
//                      V
//                    CMD8
//                   /    \
// Illegal Command -/      \- Response received without "illegal command"
//                 /        \
//                V          V
// GOTO:      {Ver 1.x}  {Ver 2.x+}
//
// (see the appropriate flow-chart continuation, below)


//                  {Ver 1.x}
//                      |
//                      |
//                      |                                               
//   {CARD UNUSABLE}    |
//   {BY THIS HOST}     |
//             ^        V
//              \____CMD58 (Read OCR) ("Not Mandatory... it is
//                   /  |               recommended ... for voltage range")
//        invalid   /   |
//        command `/    |
//                v     |                    
//{NOT SD MEMORY CARD}  |   __                  
//            ^         V  V  \             / Executing Internal     
//             \_____ACMD41*   |`Card Busy <  Intialization Process   
//                      |  \__/             \ "Card returns
//                      |                      in_idle_state=1" WTF?!
//   "Card returns      |                      
//    in_idle_state=0" `|   *NOTE: ACMD41's argument, here = 0x0 
//  THIS CAN"T BE RIGHT |
//                      V
//          [ Ver 1.x Standard Capacity SD Card ]



//                  {Ver 2.x+}
//                      |
//                      |
//                <CMD8 Response>
//non-supported   /     |
//voltage range `/      |-Compatible voltage range
// OR           |       | AND check-pattern correct
//Check-pattern |       |
//error         |       |
//              |       |   THIS SECTION IS IDENTICAL to the {Ver 1.x} 
//              |       |   flow-chart EXCEPT ACMD41's argument: HCS=1
//              |       |   (And potential reasons for ACMD41 'busy' )
// .............|.......|.................................................
// .            v       |                                                .
// . {CARD UNUSABLE}    |                                                .
// . {BY THIS HOST}     |                                                .
// .           ^        V                                                .
// .            \____CMD58 (Read OCR) ("Not Mandatory... it is recommended
// .                 /  |               ... for voltage range")          .
// .      invalid   /   |                                                .
// .      command `/    |                                                .
// .              v     |                                                .
// . {NOT SD MEM CARD}  |   __                SEE Figure 7-1.            .
// .          ^         V  V  \             / Executing Internal         .
// .           \_____ACMD41*   |`Card Busy <  Intialization Process      .
// .                    |  \__/             \ "Card returns              .
// .                    |                      in_idle_state=1" WTF?!    .
// . "Card returns      |                                                .
// .  in_idle_state=0" `|   *NOTE: ACMD41's argument, here, with HCS = 1 .
// .THIS CAN"T BE RIGHT |       (THIS host supports High-Capacity cards) .
// .                    |                                                .
// .....................|.................................................
//                      |
//                    CMD58 (Get CCS)
//                    /   \
//            CCS=0 -/     \- CCS=1
//                  /       \
//                 v         v
// [         Ver 2.00+ ]  [ Ver 2.00+              ]
// [ Standard Capacity ]  [ High/Extended Capacity ]
//


void spi_sd_sendEmptyCommand(uint8_t cmdNum);
void spi_sd_sendCommand(uint8_t *command, uint8_t length);
//CAREFUL with this one... see definition
uint8_t spi_sd_getR1response(uint8_t getRemaining);
uint8_t spi_sd_getRemainingResponse(uint8_t *buffer, uint8_t delayBytes, 
                                          uint8_t expectedBytes);

//This tests whether there *is* an R1 response, not whether the response is
// "Valid command" (right?)
//R1 responses always have 0 in bit 7
#define r1ResponseVerify(response)   (!getbit(7, (response)))

#define r1CommandIsLegal(response) \
               (!getbit(R1_INVALID_COMMAND, (response)))

//Apparently these never handled sd's chip-select
// these definitions are identical to the original functions
#define spi_sd_transferByteWithTimer(txByte) \
   spi_transferByteWithTimer(txByte)

#define spi_sd_transferByte(txByte) \
   spi_transferByte(txByte)






//a/o v31: This was originally for play-back testing... long long ago
// this and readU16
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
               printStringBuff();
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
   printStringBuff();
   byteNum++;
#endif

            uint8_t byte1 = spi_sd_transferByte(0xff);
#if(defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)
   sprintf_P(stringBuffer, PSTR("byte %"PRIu16": "), byteNum);
   printStringBuff();
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
// Table 4-36: RESPONSE: R7
// Much of CMD8 is echoed in R7 response, the only difference I see is the
// Transmission bit. (Oh and CRC7 would probably be different)
// End Bit--------------------------------------------------------------v
// CRC7---------------------------------------------------------vvvvvvv
// Check Pattern (echo)--------------------------------vvvvvvvv
// Voltage Acccepted---2.7-3.6V-------------------vvvv
// Reserved-----------------vvvvvvvv vvvvvvvv vvvv
// Command Index-----vvvvvv
// Transmission----v
// Start----------v
//lec12: CMD8 --> 01 001000 00000000 00000000 00000001 10101010 0000111 1
// THESE BIT VALUES ARE FOR CMD8, NOT the response
// ffsample/avr/mmc.c says it's 0x87 (which is what I get)
// 0x48 = Transmission bit: 0x40 | CMD8: 0x08
// 0xAA is arbitrary (?) Check Pattern
// 0x01 = voltage 2.7-3.6
uint8_t spi_sd_CMD8[] = {0x48, 0, 0, 1, 0xAA, CRC_TO_BE_CALCD}; //5}; //0x0f};
//NOTE: Illegal Commands apparently don't check CRC...

//lec12: CMD58 -> 01 111010 00000000 00000000 00000000 00000000 0111010 1
//uint8_t spi_sd_CMD58[] = {0x7A, 0, 0, 0, 0, 0x75};
uint8_t spi_sd_CMD58[] = {(0x40 | 58), 0, 0, 0, 0, CRC_TO_BE_CALCD};
//Since the CRC is followed by a 1-bit, a 0-bit indicates we need to calc
//#define CRC_TO_BE_CALCD  0

//Command CMD55 used for indicating the next command is an ACMD
//uint8_t spi_sd_CMD55[] = {0x40|55, 0, 0, 0, 0, CRC_TO_BE_CALCD};
//Command ACMD41, but since it's sent after CMD55, and otherwise looks like
// a regular command, I'd rather call it aCMD41

//HCS_ENABLED is no more...
// for v1 cards, the argument should be 0 regardless
// for v2 cards, hcs_enabled needs to be enabled (assuming we've got HCS
// support fully-implemented HERE, which isn't quite the case, yet).

//#define HCS_ENABLED   0
uint8_t spi_sd_aCMD41[] = 
                     {0x40|41, 0, 0, 0, 0, CRC_TO_BE_CALCD};
//                   {0x40|41, 0 | HCS_ENABLED, 0, 0, 0, CRC_TO_BE_CALCD};

//HCS is bit 30 of the argument, which lies in index 1, right?
#define spi_sd_aCMD41_setHCS(doEnable) \
               (spi_sd_aCMD41[1] = ((doEnable) ? 0x40 : 0))

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
#ifdef __AVR_ATmega328P__
   //This baud-rate will be overridden as necessary
   spi_init(USART_SPI_SLOW_BAUD_REG_VAL);
   //spi_init() for the usart-usi handles port-directions
   // but add the pull-up (Does this function?)
   // usart_spi's test-code returned '0' when MISO was floating...   
   setinpuPORT(SD_MISO_pin, SD_MISO_PORT);      //uC DI / MISO
#else
#warning "Should probably move this to usi_spi..."
   //Use PA2..0 for the USI (default is PB2..0)
   USIPP = (1<<USIPOS);
   setinpuPORT(SD_MISO_pin, SD_MISO_PORT);      //uC DI / MISO

   setoutPORT(SD_MOSI_pin, SD_MOSI_PORT);       //uC DO / MOSI
   setpinPORT(SD_MOSI_pin, SD_MOSI_PORT);
   
   setoutPORT(SD_SCK_pin, SD_SCK_PORT);         //uC SCK out
   clrpinPORT(SD_SCK_pin, SD_SCK_PORT);      //Set initial value (0)
                                             // According to the only
                                             // documentation I can find
#endif
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

   //dms4day_t startTime = dmsGetTime();
   timeout_t startTime = timeout_init();

   while(r1Response != 0x01)
   {
      spi_sd_sendEmptyCommand(0);

      //There shouldn't be a remaining response...
      r1Response = spi_sd_getR1response(TRUE);
      
      //if(dmsGetTime() - startTime > 10*DMS_SEC)
      if(timeout_check(&startTime, 10*TIMEOUT_SEC))
         haltError(0x77);
   }

   //We get here when it's responded with 0x01,
   // to indicate that it's in the idle state

#warning "TBR:"
   clrpinPORT(SD_CS_pin, SD_CS_PORT);

   //Send CMD8 to (begin to) determine if this is a newer (SDHC) card
   spi_sd_sendCommand(spi_sd_CMD8, sizeof(spi_sd_CMD8));
   //r1Response=spi_sd_getR1response(FALSE);
   uint8_t cmd8_responseBuffer[7];
   
   cmd8_responseBuffer[0]=spi_sd_getR1response(FALSE);

   //If CMD8 is received and processed by a card supporting it,
   // it replies with an R7 response, which is an R1 *immediately* followed
   // by four additional bytes (immediacy is assumed by Figure 7-12)
   //IMMEDIACY ASSUMED:  ------------v  v----Should be 4???
   //spi_sd_getRemainingResponse(NULL, 0, 0);
   spi_sd_getRemainingResponse(&(cmd8_responseBuffer[1]), 0, 4);

   // (This coulda been handled by getR1Response(TRUE)
   //  but at some point CMD8's response will *actually* have to be
   //  handled...?)

   //FOLLOWING Figure 7-2 of Physical Layer Spec v3.00
   // (PDF page 183)
   //For now we assume that all cards I've got available aren't SDHC
   // Thus, if we get an error 0x07, we know there's more coding ahead
   // This note prior to SDHC card... and it appears to be somewhat
   // wrong... The SPI Mode Initialization Flow chart indicates that cards
   // v2.00+ will return a response, regardless if they're SDHC. So I guess
   // I lucked out with my 2GB card being v1.x, since the flow-chart is
   // quite a bit more complex for a 2.00+ card. The time has come.
   // One more time:
   // v1.x: 
   //       will return an R1 response with R1_INVALID_COMMAND
   //       THAT is the indicator that it's v1.x
   // v2.00+ (and SDHC):
   //       will return an R7 response withOUT R1_INVALID_COMMAND
   //       in the R1 portion of the R7 response
   //       THAT is the indicator that it's v2.00+
   // A LOT OF ASSUMPTIONS HERE:  TODO
   //       NOT CHECKING other R1 error bits!
   //       NOT CHECKING R7 response is as expected
   if(r1ResponseVerify(cmd8_responseBuffer[0]))
   {
      //if(!getbit(R1_INVALID_COMMAND, (cmd8_responseBuffer[0])))
      if(r1CommandIsLegal(cmd8_responseBuffer[0]))
         spi_sd_v2_finishInit();
      else
         spi_sd_v1_finishInit();
   }
   else  //This shouldn't happen unless there's an electrical problem...?
      haltError(0x02);


   //At this point, we've completed Figure 7-2 "SPI Mode Initialization"
   // but still need to determine card size, etc.
   spi_sd_initSizeSpecs();
}

//Only useful when the card is v2, right...?
// Indicates whether it's a high-capacity card.
uint8_t sd_hc  = FALSE;

void spi_sd_v1_finishInit(void)
{
   spi_sd_58andA41(FALSE);

   //At this point, we've completed Figure 7-2 "SPI Mode Initialization"
   // but still need to determine card size, etc.
   //spi_sd_initSizeSpecs();
   //Happens back in init...
}

void spi_sd_v2_finishInit(void)
{
   spi_sd_58andA41(HCS_ENABLED);

   int8_t ccsVal;

   while( 0 > (ccsVal = spi_sd_sendCMD58(DO_GET_CCS)) ) {}   

   sd_hc = ccsVal; //spi_sd_sendCMD58(DO_GET_CCS);
   
#define PRINT_SD_HC  TRUE
#if(defined(PRINT_SD_HC) && PRINT_SD_HC)
   sprintf_P(stringBuffer, PSTR("sd_hc=%"PRIu8"\n\r"), sd_hc);
   printStringBuff();
#endif   



   //At this point, we've completed Figure 7-2 "SPI Mode Initialization"
   // but still need to determine card size, etc.
   //spi_sd_initSizeSpecs();
   //Happens back in init...
}

//This is CMD58, "Read OCR" which can be called multiple times on init
// and might in fact return different values each time...
// (depending on the init-state)
//RETURNS: True if the card's CCS bit is 1 (high/extended capacity card)
//         WILL RETURN -1 if the card's power-up status is busy
//                        so this can be called in a loop
//  NOTE: "power-up status = Busy" may have less to do with loopability
//  than originally thought. It has been reimplemented here, but when it
//  was called in a loop in v63 at the *first* call to CMD58, it got stuck
//  in the loop. SO: power-up status may have more to do with the position
//  in the initialization-sequence, in which case looping may get stuck
//  permanently... Regardless, as/of v62.5 that particular loop was
//  removed, and the second call to sendCMD58 (which only occurs for v2+
//  cards) is still looped, but doesn't seem to be loop*ing*
//  TODO: this might benefit from looking into this bit's purpose further.
//        NOTE2: It doesn't seem to be very-well documented, nor does it
//        seem to be tested in the SPI Physical Spec's SPI flow-charts...
int8_t spi_sd_sendCMD58(uint8_t doGetCCS)
{
   uint8_t cmd58_response[7];

   //Send CMD58 to read the Operating Conditions Register (OCR)
   spi_sd_sendCommand(spi_sd_CMD58, sizeof(spi_sd_CMD58));

   //CMD58 responds with an R3 response, which is just an R1
   //  followed by four additional bytes...
   // Don't get the remaining-response here... save it for later.
   cmd58_response[0] = spi_sd_getR1response(FALSE);


   spi_sd_getRemainingResponse(&(cmd58_response[1]), 0, 4);

   //notes a/o v62.5 (From v63):
   //The remaining response is the Operating Conditions Register (OCR)
   //(32bits)
   // which defines CCS and the voltage-range. It also indicates the
   // power-up status, and should probably be handled slightly differently,
   // in order to verify this bit first.

   //NOTE: Section 4.9.4 is NOT relevent for SPI
   //      see section 7.3.1.3/7.3.2.4:
   //
//7.3.1.3
//   CMD58: READ_OCR

// bit:  39 .... 32 31 ....................... 0
//      |    R1    |       32bit OCR            |
// Byte: \    0   / \  1  /\  2  /\  3  /\  4  /   (cmd58_response[Byte])

//
//
//p92 (104) Section 5.1:
//   OCR bit position   OCR Fields Definition
// Byte 4
//   0-3      0-3       reserved                              \        //
//   4        4         reserved                              .
//   5        5         reserved                              .
//   6        6         reserved                              .
//   7        7         Reserved for Low Voltage Range        .
// Byte 3                                                     .
//   8        0         reserved                              .
//   9        1         reserved                              .
//   10       2         reserved                              .
//   11       3         reserved                              .
//   12       4         reserved                              . VDD
//   13       5         reserved                              . Voltage
//   14       6         reserved                              . Window
//   15       7         2.7-2.8                               .
// Byte 2                                                     . HIGH=
//   16       0         2.8-2.9                               . Supported
//   17       1         2.9-3.0                               .
//   18       2         3.0-3.1                               .
//   19       3         3.1-3.2                               .
//   20       4         3.2-3.3                               .
//   21       5         3.3-3.4                               .
//   22       6         3.4-3.5                               .
//   23       7         3.5-3.6                               /
// Byte 1
//   24       0         Switching to 1.8V Accepted (S18A)
//   25-29    1-5       reserved
//   30       6         Card Capacity Status (CCS) VALID ONLY when b31=1
//   31       7         Card power up status bit (busy) (LOW = powering up)
//
// CCS (Bit30): 0 indicates a standard-capacity card. 1 = High Capacity/XC



#define PRINT_OCR TRUE
#if(defined(PRINT_OCR) && PRINT_OCR)
   static char oldResponse[7];
   uint8_t responseDiffers = FALSE;

   uint8_t i;
   for(i=0; i<7; i++)
   {
      if( oldResponse[i] != cmd58_response[i] )
         responseDiffers = TRUE;

      oldResponse[i] = cmd58_response[i];
   }

   if(responseDiffers)
   {
      //Both cards (2GB and 8GB) are returning 0x00 ff 80 00
      //This is the entire list of operating-voltages
      // BUT NOT showing that the power-up status is correct
      // and therefore NOT showing the CCS bit.
      sprintf_P(stringBuffer, PSTR("OCR: 0x%"PRIx8" %"PRIx8" %"PRIx8" %"
                                       PRIx8"\n\r"),
                           cmd58_response[1], cmd58_response[2],
                           cmd58_response[3], cmd58_response[4]);
      printStringBuff();
   }
   else
   {
      static uint8_t count = '0';
      printByte(count);
      printByte(0x08); //backspace
      count++;
      if(count > '9')
         count = '0';
   }
#endif



   if(r1ResponseVerify(cmd58_response[0]))
   {
      if(r1CommandIsLegal(cmd58_response[0]))
      {
         //CCS is in OCR30 (bit 6 of the r3 response, in byte 1)
         //CCS is only valid when the power-up-status bit is 1...
         // Odd note: previous versions to v62.5/v63 returned bit 7
         // (power-up status) instead of bit 6 (CCS) *as the CCS* by
         //  mistake.
         // So it should've (and did) only work with SD-HC cards

         //v63-1 and v62.5-som'n implemented returning -1
         //Return -1 if the power-up-status is 'busy'
         if(!getbit(7, cmd58_response[1]))
            return -1;
         //Return the CCS (only valid if bit 7 is 1)
         else  
            return getbit(6, cmd58_response[1]);
      }
      else //Communication error?
         haltError(0x03);
   }
   else //Communication error?
      haltError(0x03);

   //Shouldn't get here, but the optimizer might not see it that way...
   //return 0 has been changed to return -1 a/o v64. It shouldn't matter...
   return -1;
}


//This exists in both v1 and v2 initialization-schemes...
// The only difference is that if v2 is the case, then we should tell the
// SD card if this, the host, supports High-Capacity cards (HCS)
// ... so hcsEnabled should only be TRUE if we've already determined it's a
// v2 card AND the code supports HCS (which is TBI)
// I guess, ultimately, hcsEnabled could also be called "isV2"
void spi_sd_58andA41(uint8_t hcsEnabled)
{
// pauseIndicate(r1Response);
   //'course it's possible it didn't respond...

   //See SD Spec Physical Layer 3 Final... Figure 7-2
/* CMD58 is Not mandatory... only necessary for voltage range...
   see spi_sd_sendCMD58getCCS()->getCCS()
*/
   //Was this re: cmd58????
   /* This might only be necessary for detecting a card type...
      and setting operating conditions different than default
      NO: it's used on newer cards, CMD1 was used prior...
   */
//This should be taken-care-of a/o v62.5/63, and certainly by v64...
//#warning "CMD58 was ignored for v1 cards... now it's implemented... they need to be retested"
   
   //HERE we're ONLY reading CMD58 to get the voltage-range
   // BUT the voltage-range is NOT YET TESTED
   // AND the voltage-range is available regardless of whether the
   // power-up-status bit is 1 or 0, so there's no need to wait for a
   // non-negative return-value.
   spi_sd_sendCMD58(DONT_GET_CCS);

   //Not sure why to initialize this to 0xff, but it was that way before,
   //as well (before moving to 58andA41())
   uint8_t r1Response = 0xff;
// uint8_t attempts=0;



   //dms4day_t startTime = dmsGetTime();
   timeout_t startTime = timeout_init();

   //Try ACMD41 until it's no longer idle...
   // ACMDs are preceded with CMD55 (not shown in flow-chart, de-facto)
   while((!r1ResponseVerify(r1Response) || getbit(R1_IDLE, r1Response)))
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

      spi_sd_aCMD41_setHCS(hcsEnabled);

      spi_sd_sendCommand(spi_sd_aCMD41, sizeof(spi_sd_aCMD41));
      //Shouldn't be a remaining response...
      r1Response=spi_sd_getR1response(TRUE);


      //if(dmsGetTime() - startTime > 10*DMS_SEC)
      if(timeout_check(&startTime, 10*TIMEOUT_SEC))
         haltError(0x88);
   }

}






void spi_sd_initSizeSpecs(void)
{
//#warning "This value is too low... should probably use time instead"
// if(attempts >= 100)
//    haltError(0x88);

   //AT THIS POINT
   // we should be OK!
#if(defined(PRINT_SDCONNECTED) && PRINT_SDCONNECTED)
   print_P(PSTR("SD Connected!\n\r"));
#endif
   //Set the block-length so we know what to expect...
   // doesnt really matter currently
   //Again, not sure why r1Response is initialized to 0xff
   // but it was the case before initSizeSpecs()...
   uint8_t r1Response = 0xff;

   //BLOCKSIZE is used in spi_sd_CMD16, as a constant... (512)
   // for High-Capacity cards, this value is mostly ignored
   // They can ONLY do 512byte memory-data transfers
   // So, really, just use 512 in all cases, it'd be easiest.
   spi_sd_sendCommand(spi_sd_CMD16, sizeof(spi_sd_CMD16));
   //Shouldn't be a remaining response...
   r1Response = spi_sd_getR1response(TRUE);

   if(r1Response != 0x00)
   {
#if(defined(PRINT_CANTSETBLOCKSIZE) && PRINT_CANTSETBLOCKSIZE)
     print_P(PSTR("Can't Set BLK_SZ:512\n\r"));
#endif
     haltError(1);
   }

   //OLD NOTE: And I think its location is wrong anyhow.
   //This is where CMD58 should probably be, to determine whether the CSD
   // format is SDHC or standard SD 
   // (but that's indicated in the CSD as well, no?)

   //We should get the CSD (Card Specific Data)
   // for *at least* the card-size... so writes can be wrapped-around
   // CSD: Section 5.3.2 in SP Spec Physical Layer 3.00 (PDF p144)
   // HAH! Or see Figure 7.1, says to issue command 58 (Card Capacity)
   // Actually, it appears that this only tells whether it's SDHC
   // CAREFUL: earlier sections of the spec are regarding SD mode, not SPI
   // (The above note may be INCORRECT, TODO: verify!)
   // Table 5-16 defines the CSD Register Fields.
   // There is no equivalent in the SPI section, so section 5 must be it,
   // in this case.
   //CMD9 is SEND_CSD, it has no arguments and returns R1
   //    FOLLOWED BY a data-block which is handled differently than a
   //    response...
   spi_sd_sendEmptyCommand(9); 
   r1Response = spi_sd_getR1response(FALSE);
   //Response Data could arrive N_CX bytes later... (0-8)
   // It looks like (and has been described as) a single-block data-read
   //   e.g. it starts with 0xfe, and ends with two bytes CRC
   // The CSD is 128bits (16 bytes)
   // getRemainingResponse loads 0xfe, CRC, and one additional byte...
   //  indicating the end... (1+16+2+1 = 20)
   //  and we're expecting 19 (1+16+2)
   //TODO: There may be some need to account for a delay here...????
   //      See 7.5.2.3
   // actually, getRemainingResponse might take it into account, since it
   // only starts paying attention after 0xff is not received (?)
   //Globalizing this to see about memory issues...
   // and it makes functionizing sizeCalcs easier, yay!
   //uint8_t csdVal[21];
   spi_sd_getRemainingResponse(csdVal, 8, 19);

   //Make sure it's CSD structure v1.0 Standard Capacity (bits127,126=0,0)
   // v2.0 isn't handled yet... High/Extended Capacity (bits127,126=0,1)
   // other values are reserved
   uint8_t csd_version;
   csd_version = extractBitsFromU8Array(127, 126, &(csdVal[1]), 128);


   //CSDv2.0: High-Capacity card
   if(csd_version == 1)
   {
      //This should also match sd_hc, which has already been set, right?
#if(defined(PRINT_CSD) && PRINT_CSD)   
      print_P(PSTR("CSDv2\n\r"));
#endif
      spi_sd_csdv2_calcBlocksizeBlocks();

   }
   //CSDv1: Standard capacity card (SDversion may be either 1.x or 2.0+)
   else if(csd_version == 0)
   {
#if(defined(PRINT_CSD) && PRINT_CSD)   
      print_P(PSTR("CSDv1\n\r"));
#endif
      spi_sd_csdv1_calcBlocksizeBlocks();
   }
   else
   {
#if(defined(PRINT_BADCSDVER) && PRINT_BADCSDVER)   
      print_P(PSTR("CSDv???\n\r"));
#endif
      haltError(1);
   }



#if(defined(START_STOP_TEST) && START_STOP_TEST>0)
//This overrides the calculated size for testing purposes...
   sd_numBLOCKSIZEBlocks = (38461*2 * START_STOP_TEST / 512);
   //861;   //Roughly 5 seconds...
#endif




#if(defined(PRINT_SIZE) && PRINT_SIZE)
   sprintf_P(stringBuffer, PSTR("512B Blks:%"PRIu32),
                                                   sd_numBLOCKSIZEBlocks);
   printStringBuff();
   sprintf_P(stringBuffer, PSTR("=%"PRIu32"kB\n\r"),
         ((sd_numBLOCKSIZEBlocks>>(10-BLOCKSIZE_SHIFT))));
//       ((sd_numBLOCKSIZEBlocks>>10)<<BLOCKSIZE_SHIFT));

   printStringBuff();
#endif

}

void spi_sd_csdv1_calcBlocksizeBlocks(void)
{
#if(!defined(SD_HC_ONLY) || !SD_HC_ONLY)
   //Actually, it's larger(?) 32b:8040 16b:8044
   uint32_t c_size;  //This was uint16_t and only moved to uint32_t as an
                     // intermediate step toward SD_HC...
                     // Not sure if it'd be smaller if this was 16bit
   uint8_t read_bl_len; 
   uint8_t c_size_mult; 
   
   c_size = extractBitsFromU8Array(73, 62, &(csdVal[1]), 128);

   read_bl_len = extractBitsFromU8Array(83, 80, &(csdVal[1]), 128);

   c_size_mult = extractBitsFromU8Array(49, 47, &(csdVal[1]), 128);

#if(defined(PRINT_CSD) && PRINT_CSD)
   sprintf_P(stringBuffer, PSTR("C_SIZE=%"PRIu32"\n\r"), c_size);
   printStringBuff();
   
   sprintf_P(stringBuffer, PSTR("READ_BL_LEN=%"PRIu8"\n\r"), read_bl_len);
   printStringBuff();
   
   sprintf_P(stringBuffer, PSTR("C_SIZE_MULT=%"PRIu8"\n\r"), c_size_mult);
   printStringBuff();
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
      print_P(PSTR("READ_BL_LEN!=9, OK?\n\r"));
      // haltError(2);
      //print_P(PSTR(" OK? CMD16 OK\n\r"));
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

   //This'll screw up math, above... test should be earlier, but it doesn't
   // affect functionality as long as it halts...
   if(read_bl_len < BLOCKSIZE_SHIFT)
      haltError(1);
#else
#warning "STANDARD CAPACITY SD CARDS HAVE BEEN DISABLED"
#endif //!SD_HC_ONLY
}




void spi_sd_csdv2_calcBlocksizeBlocks(void)
{
   uint32_t c_size;
   
   c_size = extractBitsFromU8Array(69, 48, &(csdVal[1]), 128);

   //This should always be 512 (represented by value 9 => 2^9)
   //read_bl_len = 9; //extractBitsFromU8Array(83, 80, &(csdVal[1]), 128);


#if(defined(PRINT_CSD) && PRINT_CSD)
   sprintf_P(stringBuffer, PSTR("C_SIZE=%"PRIu32"\n\r"), c_size);
   printStringBuff();
#endif




   //Number of blocks on the card...
   // memory capacity = (C_SIZE+1) * 512 K Byte...
   // We only need the number of 512byte (BLOCKSIZE) blocks...

   sd_numBLOCKSIZEBlocks = ((c_size+1)*1024);  //...Right?




}



void spi_sd_startWritingBlocks(uint32_t startBlock)
{
   //Start at the second block (don't overwrite the usage Table)
//a/o v47: Should now be handled, see "if(!sd_hc)" below.
   // (TODO: SDHC cards use BLOCK numbers in the argument
   //  Standard-Capacity cards use BYTE numbers in the argument!)
   uint8_t spi_sd_CMD25[] = {0x40|25, 0, 0, (uint8_t)(BLOCKSIZE>>8), 
                                 (uint8_t)(BLOCKSIZE), CRC_TO_BE_CALCD};


#if(defined(PRINT_SWB) && PRINT_SWB)
   print_P(PSTR("sWB\n\r"));
#endif
   
   // ASSUMING STANDARD CAPACITY!
/* uint32_t *byteAddressArgument;
   byteAddressArgument = (uint32_t *)(&(spi_sd_CMD25[1]));

   *byteAddressArgument = (startBlock<<BLOCKSIZE_SHIFT);
*/
   //NOGO above... Appears to write low-byte first

   if(!sd_hc)
      startBlock <<= BLOCKSIZE_SHIFT;

   spi_sd_CMD25[1] = (uint8_t)(startBlock>>24);
   spi_sd_CMD25[2] = (uint8_t)(startBlock>>16);
   spi_sd_CMD25[3] = (uint8_t)(startBlock>>8);
   spi_sd_CMD25[4] = (uint8_t)(startBlock);

   //print_P(PSTR("sWB\n\r"));

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
   print_P(PSTR("CMD: "));
#endif
   for(i=0; i<length; i++)
   {
      spi_sd_transferByteWithTimer(command[i]);
#if(defined(PRINT_COMMAND) && PRINT_COMMAND)
      sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), command[i]);
      printStringBuff();
#endif
   }
#if(defined(PRINT_COMMAND) && PRINT_COMMAND)
   print_P(PSTR("\n\r"));
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


#define PRINT_R1 TRUE
#if(defined(PRINT_R1) && PRINT_R1)
#define PRINT_R1_NON_IDLE TRUE
#if(defined(PRINT_R1_NON_IDLE) && PRINT_R1_NON_IDLE)
   if(r1Response != 0x01)
#endif
   {
      sprintf_P(stringBuffer, PSTR("R1:0x%"PRIx8"\n\r"),
                                       r1Response);
      printStringBuff();
   }
#endif


   //a/o v63: the "CMD8" example is no longer relevent.
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
// expectedBytes:
//    TODO: Maybe should stop loading buffer at this point
//          but keep counting until 2 consecutive 0xff's?
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
            print_P(PSTR("Additional Response: "));
         }
         if(messageTransmitted)
         {
            //char string[20];
            sprintf_P(stringBuffer, PSTR("0x%"PRIx8" "), thisReceived);
            printStringBuff();
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
      printStringBuff();
   }
#endif

   return count; //Don't count the two 0xff's indicating the end...
         
}







//CSD is being a punk... CSDv1
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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/sd.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
