/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//usart_spi.h 0.10ncf-6

//0.10ncf-6 stealing usart_spi.c from audioThing v64
//          Made notes re: SPI Mode and edges and MSBism...
//
//0.10ncf-5 adding _USART_SPI_HEADER_ instead, and modifying makefiles
//0.10ncf-4 BaudRateCalculator:
//          A bit of a nicity, but not a bad idea...
//          Also "Basic Usage"
//          Also improvements to calculations, and overrideable Fast/Slow
//          bauds.
//0.10ncf-3 "slow" = transferByteWithTimer() isn't slow enough to see in a
//				simple "echo" method used in testMega328p+WithTimer
//          So... I guess I've gotta do some math and 'scoping
//          Regardless, the data seems to be coming through properly.
//          Yeah, we're talking ~960bps, much too fast to see without a
//          'scope. But it's *definitely* switching bit-rates
//          Also answering other questions: It does *not* seem to be
//          buffering.
//0.10ncf-2 Looking into "withTimer" transactions
//          These are a remnant of the USI-SPI interface, and should
//          probably be removed at some point. But, they're a good way of
//          testing lower "baud" rates.
//          adding: spi_transferByteWithBaud(txByte, baudRegVal)
//0.10ncf-1 Seems to work great with single-byte transmissions, but
//          transmission of a string seems to become corrupt
//          (Where's it at, PUAT? PUAR? SPI?)
//          HEART. 'HEART_REMOVED' added to the test code.
//0.10ncf - This is the first version of usart_spi, it's stolen from
//				usi_spi-0.10ncf, and modified heavily.
//          Also added testMega328P
//FROM usi_spi.h:
//0.10ncf - This is the first version moved to _commonCode
//          Stolen from audioThing49/50
//          This likely won't be used any time soon, as audioThing is being
//          moved to the Mega328p which doesn't have USI
//          BUT, this is a good starting-point for implementing the
//          USART-SPI intended-to-be-used on the 328p


#ifndef __USART_SPI_H__
#define __USART_SPI_H__
#include <stdint.h>
//This is EARLY-TESTING and needs to be changed: TODO
//TODO: SEE THE BOTTOM OF THIS FILE
//#include "usart_spi.c"


//Basic Usage:
//
// //NOTE: This baud is likely to be *not perfectly-matched*
// //Mainly we need just a *general* value... e.g. some devices need
// // *roughly* 400kbps, so 1Mbps would be a bit much.
//
// #define BAUD_ROUGHLY 10000000 //bits per second
//
// main()
// {
//    spi_init(SPI_BRR_FROM_BAUD(BAUD_ROUGHLY));
//
//    while(1)
//    {
//        //Send 'A' and grab an incoming byte
//        //NOTE: This BLOCKS until the transaction completes
//        uint8_t byteIn =
//          spi_transferByte('A');
//
//        //Do something with byteIn?
//    }


//SPI doesn't really have a "baud" rate, per-se...
// In fact, since it's synchronous, in most cases it should be entirely
// functional if it's straight-up bit-banged at completely random rates
// as long as it's no faster than the devices are capable of...
// But this peripheral specifies a baud-rate... Ideally, eventually, it
// will be *as fast as possible*, but until that time, we'll choose
// something reliable.
// And, woot, don't even need to do math:
// Table 20-7 lists:
// Heh, actually, 1MHz isn't *so* fast... let's use that:
// Fosc=16MHz: UBRR0 = 0, U2X0=0 -> 1MHz
// Also, apparently, I'm using FOSC=8MHz... duh.
// SEE BELOW for SPI_BRR_FROM_BAUD has some notes.
// This may be *well over* 1MHz, even with 8MHz F_CPU.
#ifndef USART_SPI_FAST_BAUD_REG_VAL
 #define USART_SPI_FAST_BAUD_REG_VAL 0
#endif

//0xfff is the largest value that can be stored in the baud-rate register
// Hoping it's slow-enough that the transaction-time can be seen with the
// test-code via simply sending data and waiting for it to echo.
// (not in a mathy-sorta-mood at the moment)
#ifndef USART_SPI_SLOW_BAUD_REG_VAL
 #define USART_SPI_SLOW_BAUD_REG_VAL 0xfff
#endif


// This can be used in, e.g. spi_init(SPI_BRR_FROM_BAUD(1000000))
// It won't result in an exact baud-rate, but something close.
// E.G. some devices require *roughly* 400kbps, so
// spi_init(SPI_BRR_FROM_BAUD(400000)) should give something around there
// If you need more precision, calculate it yourself!
// TODO: Double-speed mode is not implemented (does it apply to SPI?)
// ERM: It *claims* that the baud-rate is the same as in Async mode, but
// apparently that math doesn't line up (Table 20-1)
#define SPI_BRR_FROM_BAUD(baudRoughly) \
	( ( (F_CPU/2/(baudRoughly)) == 0 ) ? 0 : \
	  ( ( ((F_CPU/2/(baudRoughly)) - 1) > 0x0fff ) ? 0x0fff : \
	      ((F_CPU/2/(baudRoughly)) - 1) ) )



void spi_init(uint16_t baudRegVal);

//OLD NOTES re: usi_spi and audioThing <= v49:
// Still somewhat relevent, these functions DO BLOCK.
//Not safe to use this after the timer's being used by other things...
// also it halts processing during the communication
//The above note regarding the original spi_sd_ functions...
// still may be relevent to spi in general...
uint8_t spi_transferByteWithBaud(uint8_t txByte, uint32_t baudRegVal);
uint8_t spi_transferByteWithTimer(uint8_t txByte);
uint8_t spi_transferByte(uint8_t txByte);



//This is EARLY-TESTING and needs to be changed: TODO
#include "usart_spi.c"
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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/usart_spi/0.10ncf/usart_spi.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
