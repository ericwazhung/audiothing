/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#include "anaButtons.h" // Really this is just for puart...


#if(defined(ANAB_BLOCKING) && ANAB_BLOCKING)
#include <avr/interrupt.h>
#endif

#if(!defined(ANABUTTONS_DIGITALIO) || !ANABUTTONS_DIGITALIO)   
static __inline__
void setupAnaComp(void)
{
#ifdef __AVR_ATtiny861__
#define ACSR_REG  ACSRA
         ACSRA = (0<<ACD)  // Don't disable the anaComp
#warning "This is a result of an older version... BandGap was unused on this device... (Why not?)"
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
               | (0<<ACM0);// ??? This doesn't match the drawing...
#elif (defined(__AVR_ATmega8515__))
#define ACSR_REG ACSR
         ACSR  = (0<<ACD)  // Don't disable the anaComp
               | (1<<ACBG) // DO USE the internal voltage reference
                           //  for +input.
               | (0<<ACO)  // This is read-only...
               | (1<<ACI)  // Clear the interrupt flag
               | (0<<ACIE) // Don't enable the interrupt
               | (0<<ACIC) // Don't use the Timer1 Input Capture
               | (1<<ACIS1)// Select anaComp output Rising-edge
               | (1<<ACIS0);//  for the interrupt-flag (don't care)
#else
   #error "This MCU not yet supported..."
#endif
}
#endif



static __inline__
uint8_t isThresholdCrossed(void)
{

//Not sure if this works....
//#define AINS_SWAPPED TRUE
#if(!defined(ANABUTTONS_DIGITALIO) || !ANABUTTONS_DIGITALIO)   
 #if(defined(AINS_SWAPPED) && AINS_SWAPPED)
   //Not sure if this actually works... untested
   return !getbit(ACO, ACSR_REG);
 #else
   // The anaComp output should be positive when the capacitor has
   // discharged below the positive input
   return getbit(ACO, ACSR_REG);
 #endif
#else //DigitalIO
   return !getpinPORT(ANABUTTONS_PIN, ANABUTTONS_PORT);
#endif

}




//Debouncing functions by reading t=RC multiple times as long as the button
//is pressed. After the button is released, it returns the minimum measured
//time value.
// The idea being that a noisy/dirty contact in a button in series with a 
// resistor could never cause a measured resistance *less* than the 
// resistor-value, but it *could* cause a resistance *greater* than the 
// resistor-value.
// (ACTUALLY It's quite plausible that external interference will cause the
// threshold-crossing earlier than normal. This is actually happening quite
// a bit a/o sdramThing3.0-8... shielded-cables and short leads is probably
// a good idea.)
//
//At the user-side, this results in a button not being registered until it
//is released.
// Returns a negative value if no measurement is registered
// A positive value represents the number of tcnts between the end of the
// capacitor-charging-cycle and the crossover of the threshold voltage...

uint16_t anaB_samples = 0; //see note regarding "samples" below.
//These are only valid immediately after getDebounced returns a value!
uint16_t anaB_minSamples = 0;
uint32_t anaB_measurementCount = 0;
   
ANABUTTONS_INLINEABLE 
int32_t anaButtons_getDebounced(void)
{
   static int32_t minButtonTime = ANABUTTONS_NOBUTTON;
   int32_t thisUpdate = anaButtons_update();
   int32_t retVal = ANABUTTONS_NOBUTTON;


   //Got a value...
   if(thisUpdate >= 0)
   {

      if(minButtonTime == ANABUTTONS_NOBUTTON)
      {
         //Gotta do this at the beginning, because we don't know when it
         //may be read-back (after a sample is returned)... 
         anaB_measurementCount = 0;

         minButtonTime = thisUpdate;
         anaB_minSamples = anaB_samples;
      }
      else if(thisUpdate < minButtonTime)
      {
         anaB_minSamples = anaB_samples;  
         minButtonTime = thisUpdate;
      }

      anaB_measurementCount++;
   }
   //The button was released, return the value
   else if(thisUpdate == ANAB_BUTTON_RELEASED)
   {
      int32_t mbtTemp = minButtonTime;
      minButtonTime = ANABUTTONS_NOBUTTON;

      return mbtTemp;
   }
   //thisUpdate is negative, (and NOT ButtonReleased)
   // we don't really need to handle them specially.
   else 
   {
   }

   return retVal;

}

//Old Note:
   //The button has *just* been pressed, disregard the first value...
   // (When no button is pressed, the capacitor discharges quite slowly, on
   // the order of seconds. Meanwhile the tcnt is running... 
   // So when the button is initially pressed, it cuts the open-circuit
   // discharge time short, which could appear as a valid value, but
   // way-larger than the actual value.


// a/o 0.45b (and prior)
// states:
//    0 Enable Capacitor-Charging
//    1 Wait for charged
//    2 Begin discharge
//    3 Wait for the threshold-crossing

// a/o 0.50:
#define ANAB_STATE_START_CHARGE     0
//    0 Enable Capacitor-Charging
//      ->1
#define ANAB_STATE_CHARGE           1
//    1 Wait for charged
//      Charged?
//         Y ->2
//         N ->1
#define ANAB_STATE_RELEASE_CHARGE   2
//    2 Begin discharge
//          Button pressed (from before?)
//              Y    ->4
//              N    ->3
#define ANAB_STATE_CHECK_FOR_PRESS  3
//    3 Wait for threshold-crossing
//          Threshold Before Unpressed-Time-Out?
//              Y    A button is pressed, begin sampling: (0..2->4)
//              N    No button pressed, repeat: (0..2->3)
//      -> 0
#define ANAB_STATE_CLOCK_DISCHARGE  4
// BLOCKING
//    4 Loop
//        tcnter_update
//      until threshold-crossing or button-Time-Out
//         Threshold before Button-Time-Out?
//              Y    return loopTcnts
//                   -> 0
//              N    Button Released (buttonPressed = FALSE)
//                   Return BUTTON_RELEASED
//                   -> 0
//       -> 0
// Non-blocking
//   3b 



ANABUTTONS_INLINEABLE
int32_t anaButtons_update(void)
{
   static uint8_t buttonPressed = FALSE;
   static tcnter_t startTime = 0;
   static uint8_t state=0;


   //a/o 0.50: NYreI:
   //a/o 0.45b:
   //
   //
   //
   //
   //Discard readings which didn't have at least a few updates before
   //completion...
   //e.g. when the update function is called with a long pause between
   //e.g. due to another _update() function's taking quite some time for a
   //state-transition.  
   // There's a couple ways to accomplish this, and both may be wise...
   // 1) Track the number of tcnts since the last update/sample-call
   //    during the discharge-timer
   //    -- (not so great if the tcnter source overflows multiple times)
   // 2) Track the number of calls to update() during the discharge state
   //    discard the value if there's not at least one intermediate sample
   //    -- (there could easily be one at the start and a long delay after)
   //
   // (3?) For overflow detection: Reset a start-timer during each update?
   //      Expect a certain number of TCNTs (minimum processing time for a
   //      single main-loop) and discard if smaller...?
   //      -- (very case-specific... is it helpful?)
   //         (also, kinda taken-care-of by tcnter's overflow handling?)
   static uint16_t samples=0;

#if(defined(ANAB_CLI) && ANAB_CLI)
   uint8_t oldI=0;   
#endif

//May be only done once, unless BLOCKING and in the specific state...
do
{
   switch(state)
   {
      //Charge the capacitor...
      case ANAB_STATE_START_CHARGE:

         setoutPORT(ANABUTTONS_PIN, ANABUTTONS_PORT);
         setpinPORT(ANABUTTONS_PIN, ANABUTTONS_PORT);

         state = ANAB_STATE_CHARGE;
         break;
      //Give it some time to make sure it's charged...
      case ANAB_STATE_CHARGE:
         {
            static uint8_t chargeTime = 0;

            if(chargeTime >= CHARGE_TIME)
            {
               state = ANAB_STATE_RELEASE_CHARGE;
               chargeTime = 0;
            }
            else
               chargeTime++;
         }
         break;
      //Start discharging through the buttons (if pressed)...
      case ANAB_STATE_RELEASE_CHARGE:

         samples = 0;
         startTime = tcnter_get();


         if(buttonPressed)
         {
            state = ANAB_STATE_CLOCK_DISCHARGE;
#if(defined(ANAB_CLI) && ANAB_CLI)
            CLI_SAFE(oldI);
#endif
         }
         else
            state = ANAB_STATE_CHECK_FOR_PRESS;
         
         setinPORT(ANABUTTONS_PIN, ANABUTTONS_PORT);
         clrpinPORT(ANABUTTONS_PIN, ANABUTTONS_PORT); //Remove the pull-up
      
#if(!defined(ANABUTTONS_DIGITALIO) || !ANABUTTONS_DIGITALIO)   
         setupAnaComp();
#endif


         samples = 0;
         break;
      //The last measurement timed-out, so no button was pressed
      // Check whether a button is pressed this time...
      case ANAB_STATE_CHECK_FOR_PRESS:
         
         if(isThresholdCrossed())
         {
            buttonPressed = TRUE;
            state = ANAB_STATE_START_CHARGE;
         }
         else if( tcnter_isItTime(&startTime, ANAB_UNPRESSED_TIMEOUT) )
         {
            //Don't allow a buttonPress to be detected just because the
            //threshold crossed...
            // Rather, just don't let it cross if no button is pressed
            // This could be hokey if calls to update are delayed ...?
            buttonPressed = FALSE; //Unnecessary, since it's already...
            state = ANAB_STATE_START_CHARGE;
         }
         break;
      //A button-press has been detected previously, measure the discharge
      // time
      case ANAB_STATE_CLOCK_DISCHARGE:

#if(defined(ANAB_BLOCKING) && ANAB_BLOCKING)
         tcnter_update();
#endif
         if(isThresholdCrossed())
         {
            uint32_t timeVal = tcnter_get() - startTime;
            // This seems *really* unlikely, but -return values are used
            // to indicate no value...
            // (and since this is so unlikely, indicating no-value seems
            // just as worthwhile... so could just leave it... besides,
            // if it's that large, then it should've timed-out.)
            // More importantly, doing the above math *in* a uint32_t
            // makes sure if the tcnter has wrapped, the value will
            // still be positive... and correct... right...?
            if(timeVal > INT32_MAX)
               timeVal = INT32_MAX;

            //A double-buffer...
            // anaB_samples is only valid when this returns >=0
            anaB_samples = samples;
            state = ANAB_STATE_START_CHARGE;
   

#if(defined(ANAB_CLI) && ANAB_CLI)
            SEI_RESTORE(oldI);
#endif
            return timeVal;
         }
         else if(tcnter_isItTime(&startTime, ANAB_BUTTON_TIMEOUT))
         {
            buttonPressed = FALSE;
            state = ANAB_STATE_START_CHARGE;
            return ANAB_BUTTON_RELEASED;
         }
         else
         {
            samples++;
            //If this were non-blocking, could return ANAB_SAMPLING
            // which should now be handled in the #else below...
         }

#if(!defined(ANAB_BLOCKING) || !ANAB_BLOCKING)
         return ANAB_SAMPLING;
#endif
         break;
      //Shouldn't get here...
      default:
         break;
   }
}
#if(defined(ANAB_BLOCKING) && ANAB_BLOCKING)
//Having this here, instead of within the state, causes the switchover from
//the previous-state to act as though it fell-through (without a break)
// The reason: To assure no long delays are inserted between the initial
// release of charging and the discharge-time-measurement.
//(This isn't *as* much a problem when merely testing *if* a button is
//pressed, but since the previous state could jump to one of *two* later
//states, we can't just remove the break and allow fall-through...
//This change a/o 0.50-2
while(state == ANAB_STATE_CLOCK_DISCHARGE);
#else
 #if (defined(ANAB_CLI) && ANAB_CLI)
  #error "ANAB_CLI only works with ANAB_BLOCKING"
 #endif
 #warning "NON_BLOCKING a/o v0.50... This is untested... Working on it."
while(0);
#endif


   return ANAB_NOTHING_TO_REPORT;
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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/anaButtons/0.50/anaButtons.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
