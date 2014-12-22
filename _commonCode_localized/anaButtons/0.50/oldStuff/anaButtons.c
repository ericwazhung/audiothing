/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#include "anaButtons.h" // Really this is just for puart...


//These notes made long-after coding... so may not match exactly...
//......................                          .......................
//  Microcontroller    .                          .
//                     .                          .  Button Array...
//                 ^   .                          .  
//                 |   .                          . 
//                 \   .                          .  
// Charge ---.     /   .                          .  E.G.:
//           |     \   .                          .        
//           o     /   . AIN0                     .                 
//          |¯-_   |   . (PA6)                    .        __|__     
//    + ----|   >--+----------------------+-----------+---O     O----+
//          |_-¯   |   .                  |       .   |              |
//                 |   .                ¯¯¯¯¯     .   |      .       \
//           _-¯|  |   .                _____     .   |      .       /
//        _-¯ + |--'   . AIN2    ^        |       .   |      .       \
//       <      |      . (PA5)   |        |       .   |              /
//        ¯-_ - |-----------.    \        V       .   |    __|__     |
//           ¯-_|      .    |    /                .   `---O     O----+
//                     .    |    \                .                  |
//                     .    |    /                .                  \
//                     .    |    |                .                  /
//                     .    +----+ 1.65V Ref      .                  \
//                     .    |    |                .                  /
//                     .    |    \                .                  |
//                     .  ¯¯¯¯¯  /                .                  V
//                     .  _____  \                .
//                     .    |    /
//                     .    |    |
//                     .    V    V


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




//Left in main...
//#define BUTTON_IN_SAMPLE TRUE
//#define TESTING_ANACOMP  TRUE


//Connecting the Button/DAC array to AIN0 (PA6) (negative input)
// AIN1 is PA5 (positive input) tied to a voltage-divider @ VCC/2
//#define BUTTON_PIN PA6



// Kinda hokey, just a number of loops...
//#define CHARGE_TIME   0xf0

uint8_t buttonPressed = FALSE;

uint8_t newCompTime = FALSE;
tcnter_t compTime = 0;

/*
#define NO_B            0
#define VOL_PLUS_B      1
#define VOL_MINUS_B     2
#define PLUS_B          3
#define MINUS_B         4
#define PLAY_PAUSE_B    5
#define STOP_B          6
#define FWD_B           7
#define REV_B           8
*/

// This isn't necessary...
// I thought it might compile them as separate functions
//   when they're not used, but it's apparently smart enough not to compile
//   them at all...
// However, it's NOT smart enough not to give warnings
//  e.g. "something's static, but in an inline function..."
#if((defined(BUTTON_IN_SAMPLE) && BUTTON_IN_SAMPLE) \
      || (defined(TESTING_ANACOMP) && TESTING_ANACOMP) \
      || (defined(NKP_ANACOMP_TESTING) && NKP_ANACOMP_TESTING))
static __inline__ uint8_t anaComp_getButton(void)
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


// Generally this probably shouldn't be used (come on, int32?!)
// but it's good for determining values...
//Returns -1 if nothing's new
static __inline__ int32_t  anaComp_getCompTime(void)
{
   if(!newCompTime)
      return -1;


   newCompTime = FALSE;

   return compTime;
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
            buttonPressed = TRUE;
            state = 0;
         }

         if( tcnter_get() - startTime > BUTTON_TIMEOUT )
         {
            buttonPressed = FALSE;
         }

         break;
      //Shouldn't get here...
      default:
         break;
   }
}

#endif //BUTTON_IN_SAMPLE || TESTING_ANACOMP


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
 * /home/meh/_avrProjects/audioThing/55-git/_commonCode_localized/anaButtons/0.50/oldStuff/anaButtons.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
