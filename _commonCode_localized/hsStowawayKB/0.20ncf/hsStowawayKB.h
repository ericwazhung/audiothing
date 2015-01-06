/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




//hsStowawayKB.h  0.20ncf-5
//    Interfacing with the ol' Handspring Visor Deluxe's Stowaway Keyboard

// 0.20ncf-5 - Adding mehPL and putting this up online, WOOT!
// 0.20ncf-4 - a/o audioThing40, GCC48
//             PROGMEM must be const...
// 0.20ncf-3 - Return -> '\n' instead of '\r'
// 0.20ncf-2 - Adding Memo Key Returns '«'
// 0.20ncf-1 - Obviously, we need to put this in program-space...
//             Using sineTable0.99 as a starting-point
// 0.20ncf - Switching over to a lookup-table...
//           Using the array-filling stylings of LCDreIDer...
//           Was 1014B overflow, now only 70! YAY!
// 0.10ncf-1 - Revising test app to output min and max values
//             For determining how to handle unused keys...
// 0.10ncf - first version... no makefile, etc... (not commonFiled)

// The Handspring Visor Deluxe's Cradle Pinout:
//    (LCD facing us, i.e. the same as the keyboard's pinout)
//    
//      (hsTx)
//     2.7V,3mA   GND   kbTx
//         |       |     |
//         v       v     v
//         8 7 6 5 4 3 2 1
//         | | | | | | | |
// -..--------------------------..-
// |     keys...                  |
// .                              .
// |         <SpaceBar>           |
// -..--------------------------..-


// Just guessing, based on readable data, 
//   it appears to be running at 9600bps

// The Handspring Visor Deluxe can supply up to 3mA via the Tx pin
//  this is designed specifically for keyboards, etc. Cool.
//  It's rated at 2.7V, so in my 3.6V (3.3V?) system, I just throw a diode
//  (1N914) inbetween 3.3V and Pin 8, to drop .6V or so... seems to work
// (Possibly unnecessary)

// Key Mapping determined by hand...
//   It apears that each key-down is < 0x80, and key-up is the same | 0x80
//   Also, it appears that each key-up is sent twice
// This takes the raw value received by the keyboard
//   and returns the appropriate character (or 0 if not-mapped)
// Also handles Shift and Caps-Lock
//   NOTE: CapsLock is always activated when pressed
//         To DEactivate, hold Shift then press CapsLock
//         (since there's no indicator)
//         ALSO: Shift doesn't override caps-lock 
//         (if it's on, it's all caps, regardless of Shift)

#ifdef __AVR_ARCH__
 #include <avr/pgmspace.h>
#else
// #define prog_uint8_t const uint8_t
 #define PROGMEM 
 #define pgm_read_byte(pArrayItem) (*(pArrayItem))
#endif

const uint8_t keyMapUnshifted[] PROGMEM =
{
      //Top Row]=
      [0x00]=  '1',
      [0x01]=  '2',
      [0x02]=  '3',
      [0x04]=  '4',
      [0x05]=  '5',
      [0x06]=  '6',
      [0x07]=  '7',
      [0x34]=  '8',
      [0x35]=  '9',
      [0x36]=  '0',
      [0x30]=  '-',
      [0x31]=  '=',
      [0x32]= 0x08,  //Backspace

      //Second Row
      [0x19]=  0x09, //Tab
      [0x09]= 'q',
      [0x0a]= 'w',
      [0x0b]= 'e',
      [0x0c]= 'r',
      [0x0d]= 't',
      [0x0e]= 'y',
      [0x3c]= 'u',
      [0x3d]= 'i',
      [0x3e]= 'o',
      [0x3f]= 'p',
      [0x38]=  '[',
      [0x39]=  ']',
      [0x3a]=  '\\',

      //Third Row
      [0x11]= 'a',
      [0x12]= 's',
      [0x13]= 'd',
      [0x14]= 'f',
      [0x15]= 'g',
      [0x16]= 'h',
      [0x44]= 'j',
      [0x45]= 'k',
      [0x46]= 'l',
      [0x47]=  ';',
      [0x40]=  '\'',
      [0x41]= '\n',
         //0x0D,  //Carriage Return (No Newline?)

      //Fourth Row
      [0x03]= 'z',
      [0x10]= 'x',
      [0x2c]= 'c',
      [0x2d]= 'v',
      [0x2e]= 'b',
      [0x2f]= 'n',
      [0x4c]= 'm',
      [0x4d]= ',',
      [0x4e]= '.',
      [0x48]= '/',


      //Skipping the arrows for now...

      //Bottom Row]=
      //Skipping CTRL=0x1a, Fn=0x22, Alt=0x23, Cmd=0x8

      //Space-Bar
      [0x17]=  ' ',     //Space-Bar
      //"Space" Key
      [0x37]=  ' ',  //"Space" key, with Fn-"New"

      [0x0f]=  '`',
      //Skipping Done=0x4f
      //Skipping Del=0x50, since arrows are NYI

      //"Memo" Key
      // WOW. The compiler is smart enough to recognize from toChar that
      // rxByte  would never == MEMO_RETURN (prior to (uint8_t)ing it
      // and completely wiped out the related code
      // "large integer implicitly truncated to unsigned type"
      // so:
      // uint8_t rxChar = ...
      // if(rxChar == MEMO_RETURN) 
      // {}
      // oh, I guess it's not so impressive...
      // ~= if(<0-255> == 1000)
#define MEMO_RETURN  0xAB  //((uint8_t)'«')  //0xAB
      [0x4a]= MEMO_RETURN
};


const uint8_t shiftMapNumeric[] PROGMEM =
{
      ['0'-'0']= ')',
      ['1'-'0']= '!',
      ['2'-'0']= '@',
      ['3'-'0']= '#',
      ['4'-'0']= '$',
      ['5'-'0']= '%',
      ['6'-'0']= '^',
      ['7'-'0']= '&',
      ['8'-'0']= '*',
      ['9'-'0']= '('
};


char hsSKB_toChar(uint8_t rxByte)
{
   //To be used in "shift" variable...
   #define RIGHT_SHIFT  (0x02)
   #define LEFT_SHIFT   (0x01)
   #define CAPS_LOCK    (0x04)
   static uint8_t shift = 0;

   uint8_t keyChar = 0;

   //Used for NON-alpha keys...
   uint16_t byteWithShift;
   byteWithShift = (uint16_t)rxByte 
                   | ((shift & (RIGHT_SHIFT | LEFT_SHIFT) ) ? 0x100 : 0x0);
      
                  //((uint16_t)(shift&0x01)<<8);

   //This might be better as a look-up table...
   //First Switch Statement is for non-alpha keys...
   // alphabet handled later... (shift's easier to handle)
   switch(rxByte)
   {
      //CapsLock
      //Have to handle Caps Lock weirdly since there's no indicator
      // So: Caps Lock = CapsOn, SHIFT-CapsLock = CapsOff
      case 0x18:     //Caps Lock
         if(shift & (RIGHT_SHIFT | LEFT_SHIFT))
            shift &= ~(CAPS_LOCK);
         else
            shift |= CAPS_LOCK;
         break;
      //Left Shift
      // Key-Down
      case 0x58:           //Left-Shift
         shift |= LEFT_SHIFT;
         break;
      // Key Up
      case 0xd8:
         shift &= ~(LEFT_SHIFT);
         break;
      //Right Shift
      // Key Down
      case 0x59:           //Right-Shift
         shift |= RIGHT_SHIFT;
         break;
      // Key Up
      case 0xd9:
         shift &= ~(RIGHT_SHIFT);
         break;
      default:
         //Lookup the unshifted version...
         if(rxByte < sizeof(keyMapUnshifted))
            keyChar = pgm_read_byte(&(keyMapUnshifted[rxByte]));
         
         if(shift)
         {
            //If it's an alpha-char shift it manually...
            if((keyChar>='a') && (keyChar<='z'))
               return keyChar - 0x20; //A=0x41, a=0x61
            else if(shift & (RIGHT_SHIFT | LEFT_SHIFT))
            {
               //If it's 0-9
               // These can be shifted from another lookup table...
               if((keyChar>='0') && (keyChar<='9'))
                  keyChar = pgm_read_byte(&(shiftMapNumeric[keyChar-'0']));
               else
               {
                  switch(keyChar)
                  {
                     case '`':   return '~';
                     case '-':   return '_';
                     case '=':   return '+';
                     case '[':   return '{';
                     case ']':   return '}';
                     case '\\':  return '|';
                     case ';':   return ':';
                     case '\'':  return '"';
                     case ',':   return '<';
                     case '.':   return '>';
                     case '/':   return '?';
                     default:    return keyChar;
                  }
               }
/* Am Thinkink These few cases of direct-remapping aren't going to 
   save any code-space...
               //Unshifted: [ \ ] -> Shifted: { | }
               // Map directly...
               else if((keyChar>='[') && (keyChar<=']')
                  keyChar+=0x20;

               //
               else if((keyChar
*/
            }
         }
   }
   //Kinda a hokey mixture between breaks and returns...
   // so...
   return keyChar;
}


//Unimplemented Keys:
//Skipping CTRL=0x1a, Fn=0x22, Alt=0x23, Cmd=0x08
//Skipping Done=0x4f
//Skipping Del=0x50, since arrows are NYI
// ArrowUp=0x49, ArrowDown=0x52
// ArrowLeft=0x51, ArrowRight=0x53
//Date=0x33, Phone=0x3b, ToDo=0x42, Memo=0x4a

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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/hsStowawayKB/0.20ncf/hsStowawayKB.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
