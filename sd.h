/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#ifndef __SD_H__
#define __SD_H__

#include <util/delay.h> //For delay_us in init...

#include "sd_crc7.h"
#include "pinout.h" // Really this is just for puart...

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





//This might have to be uint64_t for SDHC...
extern uint32_t sd_numBLOCKSIZEBlocks;








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





//CAREFUL with this one... see definition
//If it doesn't come through within 16 tries, it will return 0xff!
// So be careful when testing those bits!
// if getRemaining is true, it'll call getRemainingResponse with NULL...
uint8_t spi_sd_getR1response(uint8_t getRemaining);
//This'll just grab a bunch of bytes until 0xff has been received a couple
// times...
// it'll place them in buffer, and return the number of bytes.
// If buffer is NULL it won't write to it, and will instead just return
// the number of bytes...
// delayBytes is the number of 0xff's to send initially before giving up
// (e.g. CMD9=SEND_CSD replies R1 + Data, between R1 and Data could be
//  up to 8 0xff's returned...)
uint8_t spi_sd_getRemainingResponse(uint8_t *buffer, uint8_t delayBytes,
                                             uint8_t expectedBytes);
#define r1ResponseValid(response)   (!getbit(7, (response)))


//Apparently these never handled sd's chip-select
// these definitions are identical to the original functions
#define spi_sd_transferByteWithTimer(txByte) \
   spi_transferByteWithTimer(txByte)

#define spi_sd_transferByte(txByte) \
   spi_transferByte(txByte)






//a/o v31: This was originally for play-back testing... long long ago
// this and readU16
//extern uint32_t packetCount;


//To be called after an initiate-read or whatever I called it...
// Returns a u16 of the data read
// unless the card was returning 0xff's between blocks...
// or it received the data token or CRC
// then it returns -1
// Hopefully, then, this can be called in a while-loop without
//  blocking for too long.
static __inline__ int32_t spi_sd_readU16(void);


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
//uint8_t spi_sd_CMD8[] = {0x48, 0, 0, 1, 0xAA, CRC_TO_BE_CALCD}; //5}; //0x0f};
//NOTE: Illegal Commands apparently don't check CRC...

//lec12: CMD58 -> 01 111010 00000000 00000000 00000000 00000000 0111010 1
//uint8_t spi_sd_CMD58[] = {0x7A, 0, 0, 0, 0, 0x75};
//Since the CRC is followed by a 1-bit, a 0-bit indicates we need to calc
//#define CRC_TO_BE_CALCD  0

//Command CMD55 used for indicating the next command is an ACMD
//uint8_t spi_sd_CMD55[] = {0x40|55, 0, 0, 0, 0, CRC_TO_BE_CALCD};
//Command ACMD41, but since it's sent after CMD55, and otherwise looks like
// a regular command, I'd rather call it aCMD41
//#define HCS_ENABLED   0
//uint8_t spi_sd_aCMD41[] = 
//                   {0x40|41, 0 | HCS_ENABLED, 0, 0, 0, CRC_TO_BE_CALCD};


//uint8_t spi_sd_CMD16[] = {0x40|16, 0, 0,
//                         (uint8_t)((((uint16_t)BLOCKSIZE)>>8)),
//                         (uint8_t)(BLOCKSIZE),
//                           CRC_TO_BE_CALCD};


//Bit numbers of the R1 response...
#define R1_IDLE            0
#define R1_INVALID_COMMAND 2

//This should be called BEFORE init_heartBeat or init_dmsTimer
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
//uint8_t csdVal[21];










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
//uint8_t emptyCommand[] = {0,0,0,0,0,0};

//Most commands are empty (just a command number, 
//  followed by 4 0's and CRC)
void spi_sd_sendEmptyCommand(uint8_t cmdNum);

void spi_sd_sendCommand(uint8_t *command, uint8_t length);











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
                                uint8_t array[], uint8_t arrayBits);



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
 * /home/meh/_avrProjects/audioThing/55-git/sd.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
