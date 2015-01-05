/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




#include "usart_spi.h"



//OLD NOTES from usi_spi:
//Not safe to use this after the timer's being used by other things...
// also it halts processing during the communication
//The above note regarding the original spi_sd_ functions...
// still may be relevent to spi in general...

//usart_SPI NOTE:
// transferByteWithTimer() is used to slow the bit-rate
// e.g. during initialization of the SD-Card, and possibly always with the
//      Nokia LCD
// transferByte() goes as quickly as possible

//TODO: In order to simulate this functionality with the usart_spi, we have
//      to actually *change* the baud-rate before transmission
//      With/WithoutTimer transactions are interwoven in audioThing, so we
//      need a way to handle switching the baud rate when necessary...
//      FOR THIS EARLY TEST (0.10ncf), we'll just remove WithTimer, and
//      work our way up...
uint8_t spi_transferByteWithTimer(uint8_t txByte);
uint8_t spi_transferByte(uint8_t txByte);



void spi_init(uint16_t baudRegVal)
{
   //TODO: If speed-changes/mode-changes are necessary, we'll need to
   //consider this:
   //(THIS WILL BE NECESSARY to match "with timer" transfers...)
   //ATmega328p datasheet:
   //Before doing a re-initialization with changed baud rate, data mode, or
   //frame format, be sure that there is no ongoing transmissions during 
   //the period the registers are changed.
   //The TXCn Flag can be used to check that the Transmitter has completed
   //all transfers, and the RXCn Flag can be used to check that there are 
   //no unread data in the receive buffer. 
   //Note that the TXCn Flag must be cleared before each transmission 
   //(before UDRn is written) if it is used for this purpose.

   //ATmega328p datasheet:
   //To ensure immediate initialization of the XCKn output the baud-rate
   //register (UBRRn) must be zero at the time the transmitter is enabled.
   //Contrary to the normal mode USART operation the UBRRn must then be
   //written to the desired value after the transmitter is enabled, but 
   //before the first transmission is started.
   UBRR0 = 0;

   //This taken directly from the datasheet, modified only for the
   //specific USART# and adding comments:

   /* Setting the XCKn port pin as output, enables master mode. */
   //WHOA! They FINALLY started making DDR names related to the
   //pin-functions?! I don't have to keep looking at the pinout?!
   // This could be quite handy *EVERYWHERE*.   
   //XCK0_DDR |= (1<<XCK0);
   //No such luck... Which is fine, because it'd not be
   //backwards-compatible with older avr-libc's and/or avr-gcc's
#ifdef __AVR_ATmega328P__
   setoutPORT(PD4, PORTD);
#else
 #error "usart_spi currently only works with the atmega328p"
 #error "If your device has a USART that can be used in SPI-mode,"
 #error "then it may simply be a matter of initializing the direction of"
 #error "the appropriate XCK pin/port..."
#endif

   /* Set MSPI mode of operation and SPI data mode 0. */
#warning "NEED TO VERIFY SPI MODE"
   UCSR0C = (1<<UMSEL01)      // \ MASTER
          | (1<<UMSEL00)      // / SPI
          | (0<<UDORD0)       // MSB sent first
          | (0<<UCPHA0)       // Clock Phase     \  SPI Mode 0: Sample _|¯
          | (0<<UCPOL0);      // Clock Polarity  /  Setup ¯|_
   /* Enable receiver and transmitter. */
   UCSR0B = (1<<RXEN0)|(1<<TXEN0);
   /* Set baud rate. */
   // IMPORTANT: The Baud Rate must be set after the transmitter is enabled
   UBRR0 = baudRegVal; 

}


//transferByte() both *sends* AND *receives* a byte... a function of SPI;
//all transfers are bidirectional.
//So the return-value is the received-byte

//TODO: These mightaswell be #defines... don't waste CPU cycles and
//program-space(?) jumping just to jump-again!
// (re: program-space... actually, adding an argument to *each* call, might
//  eventually overtake the program-space benefits... ToPonder)
uint8_t spi_transferByte(uint8_t txByte)
{
   return spi_transferByteWithBaud(txByte, USART_SPI_FAST_BAUD_REG_VAL);
}



//For initial testing purposes:
// USART_SPI_SLOW_BAUD_REG_VAL = 0xfff
//The baud rate is determined by:
//
// BAUD = Fosc/2/(UBRRn+1) = (8MHz/2=4MHz)/(0x1000) = 976bps
// Too fast to see between typing a character and watching the echo (in
// testMega328p+WithTimer)
// Time to 'scope...
uint8_t spi_transferByteWithTimer(uint8_t txByte)
{
   return spi_transferByteWithBaud(txByte, USART_SPI_SLOW_BAUD_REG_VAL);
}


uint8_t spi_transferByteWithBaud(uint8_t txByte, uint32_t baudRegVal)
{
   //TODO:
   // The USART is buffered, are we going to receive the byte *just*
   // received, or the one before it?

   if(UBRR0 != baudRegVal)
      spi_init(baudRegVal);

   //TODO:
   //The data-sheet shows the first-step as being:
   /* Wait for empty transmit buffer */
   while ( !( UCSR0A & (1<<UDRE0)) );
   //This should also probably be placed in spi_init()
   // Or at least before it, here... TODO.

   //A transfer is initiated by writing to UDR0:
   UDR0 = txByte;

   //Datasheet:
   /* Wait for data to be received */
   while ( !(UCSR0A & (1<<RXC0)) );

   
   #if(defined(PRINTEVERYBYTE) && PRINTEVERYBYTE)
   sprintf_P(stringBuffer, PSTR("tx:0x%"PRIx8" -> rx:0x%"PRIx8"\n\r"),
                                       txByte, UDR0);
   puat_sendStringBlocking(stringBuffer);
#endif

   //Get the received-byte and return it.
   return UDR0;
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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/usart_spi/0.10ncf/usart_spi.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
