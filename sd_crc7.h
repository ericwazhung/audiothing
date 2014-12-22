/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#include <stdio.h>
#include <inttypes.h>
//#include "../_commonCode/bithandling/0.94/bithandling.h"
#include _BITHANDLING_HEADER_

#define POLY 0x89
uint8_t sd_generateCRC7(uint8_t dataIn[], uint8_t dataLength);
/*
int main(void)
{

   //G(x) = x^7 + x^3 + 1
   // ->   10001001 = POLY/Divisor...
#define POLY 0x89
   //Data In:         Padding/Remainder/n-bit CRC
   // /------------\  /-----\
   // 010101010101010 0000000...
   //  10001001               xor above with "polynomial" (skip leading 0)
   // ------------------
   // 000100011101010 0000000...  result
   //    10001001
   // ------------------
   //    000001111010 0000000
   //         1000100 1
   // ------------------
   //         0111110 1000000
   //          100010 01
   // ------------------
   //          011100 1100000
   //           10001 001
   // ------------------
   //           01101 1110000
   //            1000 1001
   // ------------------
   //            0101 0111000
   //             100 01001
   // ------------------
   //             001 0011100
   //               1 0001001
   // ------------------
   //               0 0010101

   uint8_t cmd0[] = {0x40, 0, 0, 0, 0};

   printf("CMD0: 0x40, 0, 0, 0, 0, 0 > CRC=%" PRIx8 "\n",
            sd_generateCRC7(cmd0, 5));

   uint8_t cmd17[] = {0x51, 0, 0, 0, 0};

   printf("CMD17: 0x51, 0, 0, 0, 0, 0 > CRC=%" PRIx8 "\n",
             sd_generateCRC7(cmd17, 5));


   uint8_t cmd17Response[] = {0x11, 0, 0, 0x09, 0};

   printf("cmd17Response: 0x11, 0, 0, 0x09, 0 > CRC=%" PRIx8 "\n",
            sd_generateCRC7(cmd17Response, 5));
   return 0;
}
*/
//dataIn is stored such that index0 contains the MSB...
uint8_t sd_generateCRC7(uint8_t dataIn[], uint8_t dataLength)
{
   uint8_t dataPadded[dataLength+1];

   //Copy dataIn to dataPadded
   uint8_t i;
   for(i=0; i<dataLength; i++)
      dataPadded[i] = dataIn[i];
   //Pad it...
   dataPadded[i] = 0;


   //This byte is to be xored with the polynomial...
   uint8_t byte; //=dataPadded[0];

   uint8_t j;

   byte = dataPadded[0];


   //Handle each input byte...
   for(j=0; j<dataLength; j++)
   {
      //Handle each bit within the byte...
      for(i=0; i<8; i++)
      {
         //skip 0-bits at the beginning
         if((byte & 0x80))
         {
            byte ^= POLY;
            //Since 1 is guaranteed in the MSb of both the poly and the 
            // byte, we know the resulting byte will now have a 0 in the
            //  MSb
         }
         //Now shift...
         byte <<= 1;
         byte |= getbit((7-i), dataPadded[j+1]);
      }
   }

   return byte;
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
 * /home/meh/_avrProjects/audioThing/55-git/sd_crc7.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
