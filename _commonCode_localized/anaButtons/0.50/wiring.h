/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */



//These notes made long-after coding... so may not match exactly...


// USING THE ANALOG-COMPARATOR
//......................                          .......................
//  Microcontroller    .                          .                     .
//                     .                          .  Button Array...    .
//                 ^   .                          .                     .
//                 |   .                          .                     .
//                 \   .                          .                     .
// Charge ---.     /   .                          .  E.G.:              .
//           |     \   .                          .                     .
//           o     /   . AIN0+  [AIN1-]           .                     .
//          |¯-_   |   . (PA6)  [PB3]             .        __|__        .
//    + ----|   >--+----------------------+-----------+---O     O----+  .
//          |_-¯   |   .                  |       .   |              |  .
//                 |   .                ¯¯¯¯¯     .   |      .       \  .
//           _-¯|  |   .                _____     .   |      .       /  .
//        _-¯ - |--'   . AIN2-   ^        |       .   |      .       \  .
//       <      |      . (PA5)   |        |       .   |              /  .
//        ¯-_ + |-----------.    \        V       .   |    __|__     |  .
//           ¯-_|      .    |    /                .   `---O     O----+  .
//                     .    |    \                .                  |  .
//   Is it the drawing .    |    /                .                  \  .
//   that's flipped?   .    |    |                .                  /  .
//   Polarity....      .    +----+ 1.65V Ref      .                  \  .
//   (Tiny861 was      .    |    |                .                  /  .
//    drawn opposite)  .    |    \   [Bandgap     .                  |  .
//                     .  ¯¯¯¯¯  /    =1.23V]     .                  V  .
//                     .  _____  \                .                     .
//                     .    |    /                .......................
//                     .    |    |
//                     .    V    V



// USING A DIGITAL GPIO
//......................                          .......................
//  Microcontroller    .                          .                     .
//                     .                          .  Button Array...    .
//                     .                          .                     .
//                     .                          .                     .
//                     .                          .                     .
// Charge ---.         .                          .  E.G.:              .
//           |         .                          .                     .
//           o         .                          .                     .
//          |¯-_       . (E.G. PA6)               .        __|__        .
//    + ----|   >--+----------------------+-----------+---O     O----+  .
//          |_-¯   |   .                  |       .   |              |  .
//                 |   .                ¯¯¯¯¯     .   |      .       \  .
//           _-¯|  |   .                _____     .   |      .       /  .
// Read ----<   |--'   .                  |       .   |      .       \  .
//           ¯-_|      .                  |       .   |              /  .
//                     .                  V       .   |    __|__     |  .
//                     .                 GND      .   `---O     O----+  .
//                     .                          .                  |  .
//                     .                          .                  \  .
//                     .                          .                  /  .
//                     .                          .                  \  .
//                     .                          .                  /  .
//                     .                          .                  |  .
//                     .                          .                  V  .
//                     .                          .                     .
//                     .                          .......................





//  A matrix-keypad can be wired-up to a single digital I/O pin via the
//  'anaButtons' interface.
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





// There's something missing here... how does a resistance correspond to a
// button? Probably a good idea to revisit audioThing35


// Nokia Keys (KiloOhms):
//
//
//               ----- (16)      > (12)
//       C (7)              < (13)
//      
//       1 (8)     2 (18)      3 (11)
//
//       4 (9)     5 (19)      6 (4)
//
//       7 (10)    8 (20)      9 (5)
//   
//       * (6)     0 (2)       # (1)



// a/o sdramThing3.0-0.9 (0.10):
// Would be nice to have multiple simultaneously-pressed buttons as an
// option.
// As I recall, the Nokia keypad was wired in such a way that the
// lowest-resistance key-path overrode all other keys.
// Thus, only one button could be recognized at a time.
// FURTHER, there was something to do with measuring the lowest value
// possible... e.g. by pressing a 20kOhm button, there's no way the
// resistance (even with bounce, etc) could be *lower* than 20kOhm... So
// there was something to do with determining the *lowest* value, as a
// means of debouncing... Is that here, or was that in audioThing's main
// code?
//
// Is that same system usable in a multi-key-press setup?
// e.g.
// <-----+--.
//       |  |            A  B  C  |  Resistance
//     | O  \           ----------|------------
// A  -|    /  4k        0  0  0  |  7k
//     | O  \            0  0  1  |  6k
//       |  |            0  1  0  |  5k
//       +--+            0  1  1  |  4k
//       |  |            1  0  0  |  3k
//     | O  \            1  0  1  |  2k
// B  -|    /  2k        1  1  0  |  1k
//     | O  \            1  1  1  |  0 ohms
//       |  |
//       +--+
//       |  |
//     | O  \
// C  -|    /  1k
//     | O  \
//       |  |
//       +--'
//       | <<<<<< Should a resistor be here?
//     -----
//      ---
//       -
//
//   In this system multiple keys-pressed can be detected...
//   
//   The question remains, can this system be used with anaButtons?
//   TODO: Because AIN0 *charges* the capacitor: 0ohms should not be an
//   option... (the pin associated with AIN0 would drive high a pin
//   connected straight to ground)
//   



//Connecting the Button/DAC array to AIN0 (PA6) (negative input)
// AIN1 is PA5 (positive input) tied to a voltage-divider @ VCC/2
//#define ANABUTTONS_PIN   PA6



// Kinda hokey, just a number of loops...
//#define CHARGE_TIME   0xf0


/* SONY RM-MC29F headphone-cord remote:
        ___           ___
       /   \         /   \
      |  -  |       |  +  |
       \___/         \___/
           ___________
          /           \
         /   |> / ||   \
        |               |
        | |<<       >>| |
        |       _       |
         \     |_|     /
          \___________/

     |¯¯---__       __---¯¯|
     |  -    |     |    +  |
      ¯¯---__| VOL |__---¯¯


      
                                       mbt         220ohm   
                        (sdramThing3.0-_anaDigiTesting-2)   3.5-5
#define NO_B            0  (open)      N/A                  N/A
#define VOL_PLUS_B      1  9.9k        123-128     119      122
#define VOL_MINUS_B     2  8.4k        105-109     104      105
#define PLUS_B          3  11.9k       147-153     142      143
#define MINUS_B         4  5.17k       67-69       63       65
#define PLAY_PAUSE_B    5  330 ohms    46-52       22       23
#define STOP_B          6  7.1k        89-93       85       88
#define FWD_B           7  3.65k       50-52       45       47
#define REV_B           8  1k          22-23       16       15

The mbt (minimumButtonTime) is dang-near linear with respect to resistance
EXCEPT 330ohms which appears like 3.65k
'scoping was similarly indecipherable, the only difference was that for 330
the maximum voltage (when charging) was lower... which makes sense; the 
load (10+mA) seems to have lowered the output voltage... 
so it takes less time to discharge to the VinL threshold voltage.
But wait... that ain't right... This seems to be taking *more* time (than
1k).

There is an internal capacitor, as I recall... could that be affecting it?

The internal capacitor is in parallel with the 330ohm resistor (yup).
Interesting: 22k in series causes all buttons to return mbt=5
Must be overflow combined with som'n...?

220ohm does better...
Odd, just the jumper between the resistor and the button array (resistor
shorted) causes the original values to decrease by 5
Adding the resistor 
So, 330ohms is still reading *higher* than 1kohms...
but I've lost steam, despite its goofiness which could be interesting to
pursue. For now, 220ohms+jumper = discernable values


The buttons are wired such that the lowest-valued resistance *overrides*
the others... e.g. by pressing Reverse and Vol+ the measured resistance
will be that of Reverse, not some parallel/serial combination.

        __|__      
<--+---O     O----+
   |              |
   |      .       \
   |      .       /
   |      .       \
   |              /
   |    __|__     |
   `---O     O----+
                  |
                  \
                  /
                  \
                  /
                  |
                  V

There may be a bit of confusion here, as physically the remote fits in my
hand better upside-down, so, e.g. the FWD/REV buttons might be swapped in
the code (possibly their definitions).
The *measured resistance* is much more recent, a/o 0.20-1, and the
orientation is as drawn... e.g. the >>| button (on the right in the 
drawing) is 3.65k.
Whether that matches the FWD_B definition (7) is uncertain (untested,
recently).  Further, the values in the commented-out code are probably 
no-longer valid, as they are implementation-dependent... (CPU Frequency,
capacitor-value, bandgap vs. voltage-divider vs. analogComparator or
digital I/O)
*/

/* This is an implementation for a specific device...
   (The Sony CD-Remote)
static __inline__ uint8_t anaButtons_getButton(void)
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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/anaButtons/0.50/wiring.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
