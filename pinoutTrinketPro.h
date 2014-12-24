/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




#ifndef __PINOUT_TRINKETPRO_H__
#define __PINOUT_TRINKETPRO_H__

//
//VOLTAGE: 3.3V
//

// ###### CRYSTAL OSCILLATOR ###############################
// #                                                       #
// # IMPORTANT NOTE: IF THIS IS A NEW CHIP!                #
// # DO NOT RUN 'make fuse' until this bit is soldered-up! #
// #########################################################
//
// Pin 9 (XTAL1)  ------+-----||-->GND
//                      |    22pF    
//                   -------     
//           16MHz     ===
//           Crystal -------
//                      |    22pF              
// Pin 10 (XTAL2) ------+-----||-->GND



//
//###########################
//#  AUDIOTHING  PINOUT:    #
//###########################
//
// audioThing Pinout, first-go:
//
//                      ATmega328P (Trinket-Pro DIP-clone)
//                       ____________________
//                      |         |_|        |
//           /RESET ----|* 1 PC6      PC5 28 |--(SCL)[A6] N/C
//SD/LCD_MISO >---------|* 2 PD0      PC4 27 |--(SDA)[A5] N/C
//SD_MOSI/LCD_SDATA <---|* 3 PD1      PC3 26 |--(ADC3)[A3] N/C
// N/C (USB)[D2]--------|# 4 PD2      PC2 25 |--(ADC2)[A2] N/C
//audio out <-OC2B------|  5 PD3      PC1 24 |--(ADC1)[A1] N/C   
//SD/LCD_SCK <----------|  6 PD4      PC0 23 |-------------ADC0-< audio in 
//     +3V3 ------------|  7 VCC      GND 22 |---------> GND         
//      GND ------------|  8 GND     AREF 21 |-------------------||--> GND
//      XTAL1 <---------|  9 PB6     AVCC 20 |---------> +3V3
//      XTAL2 <---------| 10 PB7      PB5 19 |--------------< PRG_SCK/pRx0
//SD_CS <---------------| 11 PD5      PB4 18 |------------> PRG_MISO/Heart
//anaButton ><--(AIN0)--| 12 PD6      PB3 17 |-------------< PRG_MOSI/pTx0
// N/C (USB)[D7](AIN1)--|#13 PD7      PB2 16 |------------------> LCD_nRST
//LCD_nCS <-------------| 14 PB0      PB1 15 |-------------------> LCD_DnC
//                      |____________________|
// * = On FTDI Header
// # = On USB
// () = Potential Later use for pin
// [] = TrinketPro name for pin


// NOTES:
//  SPI:
//    Using the USART in SPI mode
//     (as opposed to the dedicated SPI port, since I need it for SPI
//      programming)
//    TxDn = MOSI
//    RxDn = MISO
//    XCKn = SCK
//   NOTE: This is *NOT* compatible with the FTDI programmer/debug header
//  USB:
//    The USB pins are unused in this project, so should *plausibly* still
//    work with the default TrinketPro bootloader.



// ****** PROGRAMMING HEADER / PUART / HEART ******
//
// I use the programming-header for debugging uart and heartbeat...
// These pins can be found on the device datasheet
// For newly-supported devices (e.g. atmega328p) they can also be found in
//  e.g. _commonCode[-local]/_make/atmega328p.mk
//
// This is a single-row 0.1in header, but the same pins are used on most
// dual-row .1in programming headers in both 6-pin and 10-pin variants
// (different pinouts, of course)
//
// 1  GND
// 2  V+ [ SEE NOTE ]
// 3  SCK   PB5   pRx0  (puar)   (Old: Handspring kbInput)
// 4  MOSI  PB3   pTx0  (puat)   outToPC
// 5  /RST
// 6  MISO  PB4   Heart
//
// NOTE ON V+:
// V+ on the programming-header is, in this case, used to supply power TO
// the programmer's output buffers.
// It is NOT to get power *from* the programmer.
// This because of the built-in battery. Also the requirements that the LCD
// and SD-Card run on 3.3V (whereas some programmers might supply 5V).
// Most programmers have a jumper for this purpose.


//_PGM_xxx_yyy_NAME_ is a new method, a/o atmega328p.mk
// Most devices don't yet have it implemented, so must put actual pin/port
// names here (The commented pin/port names are from another device!)
//Likewise: If you decide to use different pins than those on the
// programming-header, type them here (e.g. the FTDI header?):

//The polled-uart (PUART) is a 9600bps UART at 3.3V TTL levels
// To connect to an RS-232 port, use an approprate level-shifter
// To connect to a USB to TTL-Serial converter, make sure it outputs 3.3V
//  TTL signals.
//pRx0 and pTx0 are relative to the microcontroller
// (so connect pRx0 to the PC's Tx, and pTx0 to the PC's Rx)

//These pins can be relocated anywhere (they needn't be shared with the
//programming-header)
//Merely redefine them, here, as appropriate, and rewire.
#define Rx0pin    _PGM_SCK_PIN_NAME_   //e.g. PB2
#define Rx0PORT   _PGM_SCK_PORT_NAME_  //e.g. PORTB

#define Tx0pin    _PGM_MOSI_PIN_NAME_  //e.g. PA6
#define Tx0PORT   _PGM_MOSI_PORT_NAME_ //e.g. PORTA


//The heartbeat pin definition is in the makefile...




// ***** NOKIA LCD / SD-CARD ***********

// The Nokia LCD and the SD-Card share the SPI pins, on the USART:

//Changing these definitions will NOT affect which port SPI runs on 
// (the SPI peripheral port, vs the USART). 
//It is ONLY implemented on the USART, for now.
//These definitions are used only for initializing the pins (?)

//THESE PINS CANNOT BE CHANGED without code rewriting
#define SPI_MOSI_pin    PD1 //This definition may be unnecessary...
#define SPI_MOSI_PORT   PORTD
#define SPI_MISO_pin    PD0 //This definition may be unnecessary...
#define SPI_MISO_PORT   PORTD
#define SPI_SCK_pin     PD4 //Hardcoded in usart_spi.c
#define SPI_SCK_PORT    PORTD

// NOTE The Pull-Up on /CS
// The SD-Card should be disabled during Reset and BootLoading
// (The AVR's pull-ups are not enabled during Reset)
//
//      +3V3      +3V3
//        ^         ^   .=======================
//        |         |   | | N/C  ||
//        |         |   | |------||
// PD0 <--|------------ | | MISO ||
//        |         |   |========||      SD-Card
//        |      .----- | | GND  ||      SPI pinout
//        |      |  |   |========||      (SD-Card is the 'Slave' device)
// PD4 >--|------------ | | SCK  ||
//        |      |  |   |========||
//        /      |  '-- || 3.3V  ||
//   10k  \      |      |========||
//        /      +----- || GND   ||
//        |      |      |========||
// PD1 >--|------------ | | MOSI ||
//        |      |      |========||
// PD5 >--+------------ | | /CS  ||
//               |      |===========||
//               V       \  | N/C   ||
//              GND       \=====================
//     

//This pin can be relocated wherever you desire
// Just change these definitions and rewire as appropriate.
#define SD_CS_pin    PD5
#define SD_CS_PORT   PORTD

//THESE PINS CANNOT BE CHANGED without code rewriting
#define SD_MOSI_pin  SPI_MOSI_pin 
#define SD_MOSI_PORT SPI_MOSI_PORT
#define SD_MISO_pin  SPI_MISO_pin 
#define SD_MISO_PORT SPI_MISO_PORT
#define SD_SCK_pin   SPI_SCK_pin 
#define SD_SCK_PORT  SPI_SCK_PORT



// The Nokia LCD used has 8 pins and is well-documented online...
// Pin1 on the left, looking at the back of the LCD
// I will not be using the documented-names. Instead I'll use AKA's.
//
//Num| Name | AKA  | Description                            | uC Pin
//------------------------------------------------------------------------
// 1 | VDD  | +3V3 |                                        |
// 2 | SCK  | SCK  |                                        | SPI_SCK/PD4
// 3 | SDIN | MOSI | Serial Data Input                      | SPI_MOSI/PD1
// 4 | D/C  | DnC  | Data/Command Select: Data=H, Command=L | PB1
// 5 | SCE  | nCS  | Serial Chip Enable (Active Low)        | PB0 
// 6 | GND  | GND  |                                        |
// 7 | VOUT | ---- | VOUT >---|(---->GND (10uF)             |
// 8 | RES  | nRST | Reset (Active Low)                     | PB2

//        +3V3
//          ^
//          |       The LCD's Chip-Select should be disabled
//          /       During Reset and BootLoading
//          \ 10k   (The AVR's internal Pull-Up is not active during Reset)
//          /           
//          |
// PB0 >----+------> LCD Pin5/nCS


//For my own purposes, these are mapped to another header:
// 1 GND
// 2 +3V3
// 3 SCK  PD4
// 4 CS   PB0
// 5 Din  PD1
// 6 D/C  PB1
// 7 RST  PB2
// 8 anaButtons matrix


//These probably won't be used, code-wise... 
// since SPI should set them as appropriate
//THESE PINS CANNOT BE CHANGED without code rewrite
#define NLCD_SDATA_pin  SPI_MOSI_pin
#define NLCD_SDATA_PORT SPI_MOSI_PORT
#define NLCD_SCK_pin    SPI_SCK_pin
#define NLCD_SCK_PORT      SPI_SCK_PORT
//These pins can be relocated anywhere. Just modify these definitions and
//wire as appropriate.
#define NLCD_DnC_pin    PB1
#define NLCD_DnC_PORT      PORTB
#define NLCD_nCS_pin    PB0
#define NLCD_nCS_PORT      PORTB
#define NLCD_nRST_pin      PB2
#define NLCD_nRST_PORT  PORTB



//BUTTON_PIN on PA6/AIN0, when used...
//This pin can be relocated anywhere. Just modify this definition and wire
//as appropriate.
#define BUTTON_PIN      PD6









// *** Line-Level audio input/Microphone ***
//
// The microphone used has a built-in line-level amplifier
//  Based on the Rohm BA10358F (same as the LM358?).
//  Specifically, it's an old (1997-ish) microphone from a Macintosh
//  computer and has a strange elongated headphone-plug with an extra pin
//  for power.
//  A little searching... it's called the "PlainTalk Microphone" and can
//  still be purchased online.
//  e.g.:
//  http://www.amazon.com/Apple-M9060Z-A-Plaintalk-Microphone/dp/B0002DG09K
// True leet hackers would make their own with what they have on-hand, so
// assuming you don't have one of these relics on hand... 
// There's tons of information about making pre-amps for the
// electret-microphones found in nearly everything these days.
// I'm not particularly good at analog circuitry, so I'll leave that up to
// you.
// One potential source (circuitry unverified):
//  http://www.instructables.com/id/Arduino-Audio-Input/?ALLSTEPS
// 
//
// Once you have a line-level audio signal the circuit goes as shown.
//
// NOTE that there is NO Low-Pass FILTERING intended in this design.
// Audible aliasing occurs at high frequencies. The sample-frequency is
// 19.2kS/s, which results in a nyquist (why is it that some dude gets his
// name on such a friggin' obvious concept? We don't refer to <insert name
// here> every time we take a square-root. I ain't capitalizing that sh**)
// frequency of a little over 9KHz, which is still well-within
// audible-range. And, in fact, double-that is still audible for many, so
// it's quite likely aliasing will occur from every-day sound sources.
// Frankly, with my aging ears, it's kinda nice to be able to hear
// things at those frequencies again. Or maybe I'm just too lazy to
// calculate T=RC, and have run out of PCB space, anyhow.
// If it really bothers you, add your own filter!
//
//                                      VCC3V3
//                                        ^
//                                        |
//                                        /
//                                        \ 100K
//                                        /
//                                 4.7uF  |
//  Line-Level audio signal    >----|(----+------> PC0/ADC0
//  (e.g. PlainTalk Mic output)           |
//                                        /
//                                        \ 100K
//                                        /
//                                        |
//                                        V
//                                       GND

// **** Audio Out ****
//
// Audio Out is used only for debugging,
//  Currently, audioThing does not support playback on the device itself.
//
//  WARNING: there is no (intended) low-pass filter for the PWM signal.
//  At 16MHz F_CPU, the PWM frequency is 62500Hz, well above hearing-range.
//  Though, it's *quite likely* this is bad for your ears... (like staring
//  into an infra-red laser). I don't know, Don't trust me.
//  Also, it's probably prone to audible distortion...
//  Also, leaving this circuit attached draws a decent amount of current,
//  so consider disconnecting the whole thing when not in use.
//  ALSO, the speaker is, in fact, a solenoid, which, in fact, could send
//  voltage spikes back into the uC... I haven't had trouble with it, but
//  yahneverknow.
//
// THIS PIN CANNOT BE CHANGED without rewriting code...
//
//                 47uF      47ohm
// OC2B/PD3 >------|(--------/\/\/\----.          Tiny in-ear speaker
//                                     |              /| (What're these,
//                                     /           __/ |  usually, 32ohm?
//                             500ohm  \<---------|  | |  So we're at about
//                      Potentiometer  /          |  | |  15mA...?)
//                                     |       .--|__| |
//                                     V       |     \ |
//                                    GND      V      \|
//                                            GND


// **** Heartbeat ****
//
// The "Heartbeat" can serve several purposes, especially for 
// debugging/developing:
//  (And may be *disabled*... see makefile: 'HEART_REMOVED')
//  LED:
//  * Fades in and out smoothly if there's enough free CPU cycles
//    Or choppily if the CPU is heavily-loaded
//  * Can be used for error-indication (blinks a certain number of times)
//  Button (Momentary):
//    Can be used for any purpose, but currently acts as a 'memo' button.
//
//                             LED   330-1Kohm
// PRG_MISO/PB4 ><------+------|<|----/\/\/\-----> VCC3V3
//                      |      
//                      O |
//                        |- Pushbutton
//                      O |
//                      |
//                      V
//                     GND
//

// ***** anaButtons / Nokia Keypad *****
//
//  THIS WILL LIKELY NOT WORK without a bit of hacking and experimentation,
//  which is not well-documented.
//
//  AS-IS: Just wire-up PD6 through a 0.1uF capacitor to ground.
//
//            0.1uF
//  PD6 ><-----||-----> GND
//
//
//
//  For the brave:
//
//  A matrix-keypad can be wired-up to a single digital I/O pin via the
//  'anaButtons' interface.
//
//  (See more info in _commonCode_localized/anaButtons/)
//
//  In my case,
//  The Nokia phone's original PCB has been stripped of all its components,
//  and its keypad has been wired-up similar to below.
//  Though it has 16 buttons, it's not a 4x4 matrix. As I recall, it was a
//  5x4 matrix, where some buttons were missing. Unfortunately, I can't 
//  recall exactly how it's wired on the PCB, though it does use these
//  resistance/capacitance values. A regular 4x4 matrix could be 
//  wired as below.
//
//  NOTE: The anaButtons "buttonTime" measurements will *likely* vary from
//  device-to-device, especially if, e.g., you use a different F_CPU, or
//  low-precision resistors. So, implementation of anaButtons requires 
//  running the test-program, located in
//   _commonCode_localized/anaButtons/0.50/testMega328P+NonBlocking/
//  then changing the associated if() statements in this directory's main.c
//  UNFORTUNATELY: This has yet to be well-documented.
//  SO REALISTICALLY: The implementation of anaButtons in audioThing is a
//  bit of work.
//
//  INSTEAD: Just connect a capacitor between PD6 and ground.
//
//                                    
//  PD6 ><----+---------+-------*-------*-------*-------*   These are
//            |         |        \       \       \       \   pushbuttons
//            |         /       \ O     \ O     \ O     \ O  <-------
//    .1uF  -----       \       ,\ O    ,\ O    ,\ O    ,\ O 
//          -----   5k  /         \ \     \ \     \ \     \ \
//            |         \            *       *       *       *
//            |         |            |       |       |       |
//            V         +-------*----|--*----|--*----|--*----|
//           GND        |        \   |   \   |   \   |   \   |
//                      /       \ O  |  \ O  |  \ O  |  \ O  |
//                      \       ,\ O |  ,\ O |  ,\ O |  ,\ O |
//                  5k  /         \ \|    \ \|    \ \|    \ \|
//                      \            *       *       *       *
//                      |            |       |       |       |
//                      +-------*----|--*----|--*----|--*----|
//                      |        \   |   \   |   \   |   \   |
//                      /       \ O  |  \ O  |  \ O  |  \ O  |
//                  5k  \       ,\ O |  ,\ O |  ,\ O |  ,\ O |
//                      /         \ \|    \ \|    \ \|    \ \|
//                      \            *       *       *       *
//                      |            |       |       |       |
//                      +-------*----|--*----|--*----|--*----|
//                               \   |   \   |   \   |   \   |
//                              \ O  |  \ O  |  \ O  |  \ O  |
//                              ,\ O |  ,\ O |  ,\ O |  ,\ O |
//                                \ \|    \ \|    \ \|    \ \|
//                                   *       *       *       *
//                                   |       |       |       |
//                                   |       |       |       |
//                                   +-/\/\/-+-/\/\/-+-/\/\/-+
//                                       1k      1k      1k  |
//                                                           /
//                                                           \ 1k
//        This resistor, to ground, is necessary since the   /
//        pin (PD6) will be used as an output to charge the  |
//        capacitor to 3V3, EVEN WHEN a button is pressed.   V
//                                                          GND

// ***** POWER *****
//
// It's common-practice, and for good reason, to decouple the
// power-supply... add 0.1uF capacitors as close as possible between the
// power-pins of each device. Also, not a bad idea to have a larger
// capacitor at the entry of the power-source to the PCB (22uF? Larger?)
//
// This device runs on a Li-Ion battery from an old cell-phone
// The battery has three pins
//
// + ~3.7V
// T "Thermal" (May be serial-data. Regardless: Necessary for Charging)
// - GND
//
// These three pins are exposed at a connector outside the device in order
// to charge the battery. The three pins are connected to the old (and
// otherwise mostly dead) cell-phone that the battery came from.
//
// + and - are connected to the devices via a voltage-regulator.
// The TrinketPro includes this voltage-regulator already, but I dun have a
// TrinketPro, so had to use parts on-hand. One nice side-effect is that
// the voltage-regulator has an "Enable" input, which may later be used to
// push-button power-up, and software-power-down (especially when the
// battery is low).
// Also, this same design can be used to individually switch power to
// different devices for power-saving (e.g. the LCD) and/or power-cycling
// (e.g. I have an SD card that doesn't initialize without a power-cycle).
//
//                                  3.0V Regulator
//                                   _______________
//                                  |               |
//  To       .-------+----+-----+---| Vin      Vout |----+---+--> +3V3 out
// "Charger" |    +  |    |     |   |               |    |   |
//           |     -----  |     |   |               |    /   |
//           | .--T ---   |     O   | Enable  'GND' | 1k \  --- 1uF
//   + 1 >---' |   -----  |    /    |_______________|    /  ---
//   T 2 <-----'    ---   |   /         |        |       |   |
//   - 3 >---.       |    |     O-------+        '-------+---+
//           |       |    |             |                |   |
//           |       |   --- 0.1uF      /(optional)      /   |
//           |       |   ---            \ 100k           \  --- 1uF
//           |       |    |             /         100ohm /  ---
//           |       |    |             |                |   |
//           '-------+----+-------------+----------------+---+---> GND
//
//                  ^^^         ^^^
//                Li-Ion       Power
//                Battery      Switch
//
//
// AVCC/AREF:
//   AVCC and AREF should both be connected to .1uF capacitors to GND
//   AVCC should, additionally, be connected to VCC3V3
//   Ideally, it would be isolated from the digital supply, but the
//   system's entirely functional if it's tied directly to the same supply.
//


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
 * /home/meh/_avrProjects/audioThing/57-heart2/pinoutTrinketPro.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
