/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//adcFreeRunning.c 0.10
//#include _ADCFREERUNNING_HEADER_
#include "adcFreeRunning.h"
#include <avr/interrupt.h>
#include _BITHANDLING_HEADER_
// Basic idea:
//   adcFR_init();
//   while(1) {
//     int16_t adcVal = adcFR_get(< adcNum if NUM_ADCS>1 >);
//     if(adcVal >= 0)
//        do something with adcVal
//   }


//This is for selecting a specific ADC (with NUM_ADCS=1)
#ifdef __AVR_ATtiny861__
 // It only correponds to those that fit in MUX4:0 in ADMUX (MUX5 NYI)
 //DO NOT CHANGE THIS without ALSO CHANGING DIDR0 in init()
 #define ADCMUX1_VAL 0x06  //ADC6
 #define ADCMUX1_MASK   0x1f

 #if ((ADCMUX1_VAL != 0x06) || (NUM_ADCS != 1))
  #warning "ADCMUX1_VAL and/or MUX2 necessary, Code must be implemented for 2 channels. Must also change DIDR for the selected ADCs"
 #endif

#elif (defined(__AVR_ATmega328P__))

 //DO NOT CHANGE THIS without ALSO CHANGING DIDR0 in init()
 #define ADCMUX1_VAL 0x00  //ADC0
 #define ADCMUX1_MASK   0x0f

#else
 #error "Currently only the ATTiny861 and ATmega328P are supported"
 #error "(it's entirely plausible I mistyped the ATTiny861 test, here)"
#endif




//ADC Prescaler (ADCSRA Bits 2-0):
//  ADPS2   ADPS1   ADPS0     DivisionFactor (FCPU/DF)
//    0       0       0         2
//    0       0       1         2      1<<1 = 2
//    0       1       0         4      1<<2 = 4
//    0       1       1         8      1<<3 = 8
//    1       0       0         16     1<<4 = 16
//    1       0       1         32     1<<5 = 32
//    1       1       0         64     1<<6 = 64
//    1       1       1         128    1<<7 = 128

//FCPU/16 -> 16MHz/16/(13cyc/sample) = 76.923kS/s
// (was defaulting to 8 times that, which is a bit much)
//#define ADC_ADPS_SPECIFIC   ((1<<ADPS2) | (0<<ADPS1) | (0<<ADPS0))
// or /32, which is less than CD quality... 
//#define ADC_ADPS_SPECIFIC   ((1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0))
#warning "Running ADC at 19kS/s... 8kHz max audio signal..."
#warning "IF F_CPU=16MHz!"
#define ADC_ADPS_SPECIFIC     (DESHIFT(ADC_CLKDIV)) //64)) //32))

//ATTiny861:
//ADC 4,5,6 are on PA5,6,7 (that shift killed me!)
//ADC 3,7,9 are on PA4, PB4, PB6
//  -----------------------------------------
//   adc0 / PA0 (optionally: DI)
//   adc1 / PA1 (optionally: DO)
//   adc2 / PA2 (optionally: SCK)
//   adc3 / PA4
//   adc4 / PA5
//   adc5 / PA6
//   adc6 / PA7         <---- Line In.
//   adc7 / PB4
//   adc8 / PB5 / OC1D
//   adc9 / PB6 
//   adc10 / PB7 / RESET




// ADC ISR....
// The ADC value available corresponds to the channel selected
// two interrupts ago... since this is toggling, that means we 
// are setting the same ADC number as we're receiving the data from
//The ADC has (upon interrupt):
//  just finished a conversion and already 
//  started the next based on MUX value set up in the previous interrupt
//                                              ¦
//                       ____    ____    ____   ¦____
//  ADC_INT:           /  0   XX  1   XX  2   XX¦ 3   XX
//                  ¯¯  ¯.¯¯¯    ¯.¯¯    ¯.¯¯   ¦¯.¯¯
//                        ·______  ·_____  ·____¦  ·_____
//  ADMUX Set:   init_x  /  0     X  1    X  0  ¦ X  1
//             ¯¯.¯¯¯¯.¯  ¯¯¯¯.¯¯   ¯¯.¯¯   ¯¯.¯¦   ¯¯.¯¯
//                ·    ·_____  ·_____  ·_____  ·¦____  ._____
//  Converting ADC: x /  x    X  0    X  1    X ¦0    X  1
//                 ¯¯. ¯¯¯¯¯¯.  ¯¯¯¯¯.  ¯¯¯¯¯.  ¦¯¯¯¯.  ¯¯¯¯¯.
// <int#>_<ch#>        ·_____  ·_____  ·_____  ·¦____  ·_____  ·_____
//  ADC Data:      x  /  x_x  X  x_x  X  0_0  X ¦1_1  X  2_0  X  3_1
//                 ¯¯  ¯¯¯¯¯¯   ¯¯¯¯¯   ¯¯¯¯¯   ¦¯¯¯¯   ¯¯¯¯¯   ¯¯¯¯¯
//                                              ¦
// Thus, the data available at a given ADC interrupt
//       is from the ADMUX setting *two* interrupts prior
//       the data was sampled slightly less than one interrupt prior
//                             all data valid --v
//                                              ¦
// <int#>_<MUX# set>     ____    ____    ____   ¦____    ____   ¦____
//  ADC_INT:           /  0_0 XX  1_1 XX  2_0 XX¦ 3_1 XX  4_0 XX¦ 5_1 
//                  ¯¯  ¯¯¯¯¯    ¯¯¯¯    ¯¯¯¯   ¦¯¯¯¯    ¯¯¯¯   ¦¯¯¯¯
//  (Sampling on edges)       a       b       c ¦     d       e ¦
//  <int#>_<ADC#>       _____ ¦ _____ ¦ _____ ¦ ¦____ ¦ _____ ¦ ¦____
//  Converting:   x_0 /  x_0  X  0_0  X  1_1  X  2_0  X  3_1  X  4_0
//                 ¯¯. ¯¯¯¯¯¯.  ¯¯¯¯¯.  ¯¯¯¯¯.  ¦¯¯¯¯.  ¯¯¯¯¯.  ¦¯¯¯¯
// <int#>_<ch#>        ·_____  ·_____  ·_____  ·¦____  ·_____  ·¦____
//  ADC Data:      x  / x_0   X x_0   X 0_0   X 1_1   X 2_0   X 3_1  
//                 ¯¯  ¯¯¯¯¯¯   ¯¯¯¯¯   *¯¯¯¯   *¯¯¯¯   ¯¯¯¯¯   ¦¯¯¯¯
//                                      |       |               ¦
//                                      v_______¦_______ _______|___
//  adcVal[0]                           |  a            |  c     
//                                       ¯¯¯¯¯¯¯¦¯¯¯¯¯¯¯ ¯¯¯¯¯¯¯|¯¯¯
//                                              v_______________v__
//  adcVal[1]                                   | b             | d
//                                               *¯¯¯¯¯¯¯¯¯¯¯¯¯¯ *¯
//                                               |               |
//                                               v_______________v____
//  adcValSynced                                 | a & b         | c & d
//                                                ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯ ¯¯
//                                     



void adcFR_init(void);


uint8_t adcValNew = FALSE;

#if(NUM_ADCS == 1)
uint16_t adcVal;

//Returns - if the value was already gotten...
int16_t adcFR_get(void)
{
   if(adcValNew)
   {
      cli();
         uint16_t aVal = adcVal;
         adcValNew = FALSE;
      sei();
      return aVal;
   }
   else
      return -adcVal;
}

#if(!defined(ADC_ISR_EXTERNAL) || !ADC_ISR_EXTERNAL)
ISR(ADC_vect) //, ISR_NOBLOCK)
{

   //ATtiny861: "ADCL must be read first, then ADCH"
   //ATmega328p: same
   //It's *plausible* that optimization might reorder this...
   // In fact, it's entirely plausible it'd reorder anything I type here,
   // besides assembly. Hopefully it's smarter than that
   //(Must be, right? Otherwise we'd *always* have to throw stuff in
   //assembly, when working with registers that'd work differently if
   //out-of-order...)
   //adcVal = ADCL;
   //adcVal |= (ADCH<<8);
   //Better-Yet, maybe the device header-file includes an ADC 'register'
   //that handles proper reading...?
   adcVal = ADC;
   //Yep. And viewing the assembly-listing, it's definitely in order:
   //lds   r24, 0x0078  //ADCL
   //lds   r25, 0x0079  //ADCH
   //sts   0x0126, r25
   //sts   0x0125, r24

   //The old method: Most likely would've worked properly
   // but I'm trying to figure out this "shitty sound" issue in audioThing
   // v50-52
   //adcVal = ADCL | (ADCH<<8);
   adcValNew = TRUE;
}  
#endif

#elif (NUM_ADCS == 2)   //multi-ADCs isn't yet implemented...
#error "Multi-Channel ADC-FreeRunning has not been implemented for quite some time"
#if 0

#warning "NUM_ADCS1 was last implemented with uint8, left-adjusted, 2 ADCs is NYI"
uint16_t adcVal[NUM_ADCS];

int16_t adcFR_get(uint8_t adcNum)
{
   if(adcValNew)
   {
      cli();
         uint16_t aVal = adcVal[adcNum];
         adcValNew = FALSE;
      sei();
      return aVal;
   }
   else
      return -adcVal[adcNum];
}


#if (!defined(REMOVE_SYNCED) || !REMOVE_SYNCED)
uint8_t adcValSynced[NUM_ADCS];
#endif


#if(!defined(DISABLE_ADC_OVERLAP_CHECK) || !DISABLE_ADC_OVERLAP_CHECK)
volatile uint8_t in_adc_vect = 0xff;
volatile uint8_t stuck_in_adc = FALSE;
#endif


//THIS IS THE 2-ADC ISR... it hasn't been reimplemented recently
ISR(ADC_vect)
{
#if(!defined(DISABLE_ADC_OVERLAP_CHECK) || !DISABLE_ADC_OVERLAP_CHECK)
   //fc seems like a good test-val for normal... 
   // realisitically, it shouldn't be allowed to have more than two 
   // running simultaneously, or who knows what'll happen to dmsUpdate
   // etc..
   // then again, other interrupt sources may slow it down...
   if(!stuck_in_adc && (in_adc_vect > 0xfd))
      in_adc_vect--;
   else
   {
      stuck_in_adc = TRUE;
      //Disable the ADC interrupt
      ADCSRA &= ~((1<<ADIE));

      return;
   }
#elif(!defined(ADC_LOWSPEED) || !ADC_LOWSPEED)
  #warning "Old Warning, when lots of stuff happened in the ADC Interrupt:"
  #warning "ADC overlap checking is NECESSARY at high-speed"
#endif
   


   // This'll be ++'d to 0 before setting ADMUX the first time...
   static uint8_t selectedADC = 1;


   // The ADC value available corresponds to the channel selected
   // two interrupts ago... since this is toggling, that means we 
   // are setting the same ADC number as we're receiving the data from
   

   //All data is available when selectedADC == 0 -> 1
   //WAS: if(selectedADC) NOW: since selectedADC is changed INSIDE
   // the test is reversed
   if(!selectedADC)
   {
      selectedADC = 1;
      adcVal[1] = ADCH; //Could get rid of this line
                        // adcVal[1] is only used in main to print
      // ADMUX = (1<<ADLAR) + (1<<MUX2) + selectedADC;
      ADMUX = (1<<ADLAR) + (1<<MUX2) + 1;

      uint8_t a0, a1;

      a0 = adcVal[0];
      a1 = adcVal[1]; //And set this to ADCH instead

#if (!defined(REMOVE_SYNCED) || !REMOVE_SYNCED)
      adcValSynced[0] = a0; //adcVal[0];
      adcValSynced[1] = a1; //adcVal[1];  //just written...
#endif

      adcValNew = TRUE;

      sei();
   }
   else //NOW: if(selectedADC) //WAS:if(!selectedADC)
   {
      selectedADC = 0;
      adcVal[0] = ADCH;
      ADMUX = (1<<ADLAR) + (1<<MUX2); //+0

      sei();
      
      //dms_update();
   }

#if(!defined(DISABLE_ADC_OVERLAP_CHECK) || !DISABLE_ADC_OVERLAP_CHECK)
   cli();
   in_adc_vect++; // = FALSE;
   sei();
#endif
}
#endif
#else //NUM_ADCS > 2
 #error "Doesn't yet support NUM_ADCS > 2"
#endif

void adcFR_init(void)
{
   //ATtiny861: MUX5..0:
   // ADC4 = 000100
   // ADC5 = 000101
   ADMUX = 
#ifdef __AVR_ATtiny861__      
      (0<<REFS1) | (0<<REFS0)       //VCC Used as VRef
#elif defined(__AVR_ATmega328P__)
      (0<<REFS1) | (1<<REFS0)       //AVCC with external capacitor at AREF
#endif
      | (0<<ADLAR)               //Not using left-adjustment here...
      //| (1<<ADLAR)             //ADC result LEFT-adjusted
                                 // so 8-bit result read from ADCH
      | (ADCMUX1_MASK & ADCMUX1_VAL);     // Select ADC#

   //Before enabling the ADC, just configure the prescaler, etc.
   //Watch out for Read-Modify-Write on ADIF
   ADCSRA = (0<<ADEN)                  // Wait to Enable the ADC
            | (0<<ADSC)                // Wait to Start Conversion
            | (0<<ADATE)               // Not using Auto-Trigger (yet)
            | (1<<ADIF)                // Clear the interrupt flag
            | (0<<ADIE) // Wait to Enable the conversion-complete interrupt
         #if(defined(ADC_LOWSPEED) && ADC_LOWSPEED)
            | (0x07 & 0x06);  // or 9613...
         #elif (defined(ADC_ADPS_SPECIFIC))
            | (0x07 & ADC_ADPS_SPECIFIC);
         #else //if(!defined(ADC_LOWSPEED) || !ADC_LOWSPEED)
            | (0x07 & 0x05);  // Enable the ADC Prescaler at FCPU/32=250kHz
                              // or 19,230.8Samples/sec (13ADCclk/sample)
         #endif
   ADCSRB = 
#ifdef __AVR_ATtiny861__
            (0<<BIN)                   //Use unipolar (unsigned) inputs
            | (0<<GSEL)                //Use 1x gain
                                       //Bit 5 is reserved...
            | (0<<REFS2)               //Don't Care for VCC = VRef (ADMUX)
            | (0<<MUX5)                // (see ADCSRA, ADC0 selected)
            |
#endif
             (0x07 & 0x00);            // Free-Running mode

// _delay_us(100);

   //(1<<ADATE) -> 49000
   //(0<<ADATE) -> 74000 WTF?!

   //NOW enable the ADC...
   ADCSRA |= (1<<ADEN)     //ADC Enable
            | (1<<ADSC)    //ADC Start Conversion
            | (1<<ADATE)   //Auto-Trigger Enable
            | (1<<ADIF)    //Clear the interrupt-flag
            | (1<<ADIE);   //Enable the Interrupt


   //DIDR0 = (1<<ADC6:0D);    //Disable the digital input on ADC0-6
                              // These are NOT consecutive bits
   //DIDR1 = (1<<ADC10:7D);   // Ditto for ADC7-10

#ifdef __AVR_ATtiny861__
   DIDR0 = (1<<ADC6D);     //Disable the digital input on ADC6
#elif (defined(__AVR_ATmega328P__))
   DIDR0 = (1<<ADC0D);     //Disable the digital input on ADC0
#endif
   //ADC 4,5,6 are on PA5,6,7 (that shift killed me!)
   //Take the pins and disable digital circuitry
/* This is from the old commonFiled ADC code... it was commented-out here
   But don't we need to do this still???

   adc_takeInput(4);
   adc_takeInput(5);
   adc_takeInput(6);

   //ADC 3,7,9 are on PA4, PB4, PB6
   // These are ADC's connected to Voltage Follower Output (ADCVF)
   adc_takeInput(3);
   adc_takeInput(7);
   adc_takeInput(9);
*/
}


//More from the old main()... quite messy, most was already commented-out.
#if 0
   while(1)
   {
#if(!defined(DISABLE_ADC_OVERLAP_CHECK) || !DISABLE_ADC_OVERLAP_CHECK)
      if(stuck_in_adc)
      {
         //can't use the DMS timer, since it's updated in the adcInt!
         //static dms4day_t pauseTime = 0; // = dmsGetTime();
         static uint32_t state = 0;

         if(!state)
         {
            char string[30];
            sprintf_P(string, PSTR("\n\rToo Many ADC Ints!\n\r"));
            TransmitString(string);
         }

         state++;

         //50000 ~= .5sec
         if(state > 50000)
         {
            //This is redundant...
            // we can't get here until all adcInts are processed
            // so this will be 0xff on the first test 
            if(in_adc_vect == 0xff)
            {   
               //Enable the ADC interrupt
               ADCSRA |= (1<<ADIE);
               stuck_in_adc = FALSE;
               state = 0;
            }
         }
      }  
#endif

      static uint32_t cycleCount = 0;
      cycleCount++;

      static dms4day_t cycStartTime = 0;
      dms4day_t now = dmsGetTime();

      //Drops from 174100cyc/5s to 1786cyc/5s
      // multDiv ~= 2.8ms
      // with shittons of interrupts and no heart...
      // pre << 8: inlined multDiv -> 2490cyc/5s
      // after -> 2414cyc/5s
//    int32_t md = multDiv(((int32_t)adcValSynced[0])<<24, 
//                         ((int32_t)adcValSynced[1]<<16),
//                         SINE_MAX); //now);//SINE_MAX);
   
      // 9214 cyc/5sec (with now) 9118 with SINE_MAX (WTF?)
      // What about scaling coeff by 2^n then using >> instead of /?!

      // -- >> instead gives 15538cyc/5sec
      // still not fast enough to be in an interrupt at 9615ints/sec.

      //Sheesh, optimization ruined it... or was it just that the values
      // were so large, 0 was all that was left in the lower 32b? 
      /*md = (((int32_t)((((int32_t)adcValSynced[0])<<24)+0x00f8f7f6) 
                   * ((int32_t)(((int32_t)adcValSynced[1])<<16)+0xf800f7f6)
            ) >> 15);
      */           /// SINE_MAX; //now);//SINE_MAX);
//    md = (int32_t)((((int32_t)((int32_t)(adcValSynced[0]))<<8) | adcValSynced[1])
//       * (((int32_t)((int32_t)(adcValSynced[1]))<<8) | adcValSynced[1]))
//    >> 8;
/*
      static uint16_t sampleNum = 0;

      goertz_processSample(&(goertz[0]), adcValSynced[0]);
      //goertz_processSample(&(goertz[1]), adcValSynced[1]);

      sampleNum++;

      if(sampleNum == N)
      {

         sampleNum = 0;
         goertz_reset(&(goertz[0]));
         goertz_reset(&(goertz[1]));
         //Q1 = 0;
         //Q2 = 0;
      }
*/

      if((dms4day_t)(now - cycStartTime) >= (5*DMS_SEC))
      {
         static uint8_t shiftAmount = 0;
         static int16_t shiftTest = INT16_MIN;

         //This is kinda hokey...
   //    while(in_adc_vect){}
         cli();
#if (!defined(REMOVE_SYNCED) || !REMOVE_SYNCED)
   
         //Is cli/sei necessary with syncing? probably...
            uint8_t a1 = adcValSynced[0];
            uint8_t a2 = adcValSynced[1];
            uint8_t p1 = pwmValSynced[0];
            uint8_t p2 = pwmValSynced[1];
#else
            uint8_t a1 = adcVal[0];
            uint8_t a2 = adcVal[1];
            uint8_t p1 = OCR1A;
            uint8_t p2 = OCR1B;
#endif
         sei();
         cycStartTime = now;
         char string[30];
         sprintf_P(string, PSTR("\n\rc:%lu "), cycleCount);
         TransmitString(string);
         sprintf_P(string, PSTR("a:%d,%d "), a1, a2);
         TransmitString(string);
         sprintf_P(string, PSTR("p:%d,%d  "), p1, p2);
         TransmitString(string);
         sprintf_P(string, PSTR("r:%" P_SMULT3 ",%" P_SMULT3 " "),
            goertz_getRealPart(&(goertzReady[0])),
            goertz_getRealPart(&(goertzReady[1])));
         TransmitString(string);
         sprintf_P(string, PSTR("i:%" P_SMULT3 ",%" P_SMULT3),
            goertz_getImagPart(&(goertzReady[0])),
            goertz_getImagPart(&(goertzReady[1])));
         TransmitString(string);
      }
      
      heartUpdate();
      
/*
      //This would allow phase-shift if not called often enough
      //if(now - startTime >= dmsPeriod)
      //if(now >= startTime + dmsPeriod)
      if((dms6sec_t)(now-startTime) >= dmsPeriod)
      {
      }
*/
   }


   /*
   while(1)
   {
      if(adc_sumUpdate())
      {
         static uint16_t updateCount = 0;
//       static dms6sec_t lastTime = 0;
//       dms6sec_t thisTime = dmsGetTime();

         updateCount++;


//       #define ADC_UPDATE_TIME    (3*DMS_SEC)
//       if(thisTime - lastTime > ADC_UPDATE_TIME)
         if((updateCount >= 2) && !(USI_UART_dataInTransmitBuffer()))
         {
//          lastTime = thisTime;
            char adcString[50];

            //Vout (from voltage followers)
            uint32_t adc0 = adcSum[3] >> SUMPRECISION;
            uint32_t adc1 = adcSum[7] >> SUMPRECISION;
            uint32_t adc2 = adcSum[9] >> SUMPRECISION;
            sprintf(adcString, "o %d: %ld,%ld,%ld\n\r",updateCount, \
                  adc0,adc1,adc2);
            TransmitString(adcString);

            //Vtest (voltage at test pins)
            adc0 = adcSum[4] >> SUMPRECISION;
            adc1 = adcSum[5] >> SUMPRECISION;
            adc2 = adcSum[6] >> SUMPRECISION;
            sprintf(adcString, "t %d: %ld,%ld,%ld\n\r",updateCount, \
                        adc0,adc1,adc2);
            TransmitString(adcString);

            updateCount = 0;
*/
   /*
            //Calculate resistance between ADC0 and ADC1
            //ADC reads a value 8*larger than PWM...
            // (it has three extra bits of precision after summing)
            //#define PWM_ADC_SCALE 8
            #define PWM_ADC_SCALE ((int32_t)(1<<(SUM_EXTRA_BITS + 2)))
            int32_t vOut = (pwm[0] - pwm[1])*PWM_ADC_SCALE;
            int32_t vResistorUT = (adc0 - adc1);
            int32_t vDiff = vOut - vResistorUT;
            // (200ohms in series with Rut)
            //ResistorUT = Vrut*200 / (vOut - vResistorUT)
            sprintf(adcString, "pe %ld,%ld\n\r", \
                     (int32_t)pwm[0]*PWM_ADC_SCALE, \
                     (int32_t)pwm[1]*PWM_ADC_SCALE);
            TransmitString(adcString);

            if(vDiff != 0)
            {
               // 1/Irut = Rut/Vrut = 200/(Vout-Vrut) = Rfixed/(Vout-Vrut)
               //                                     = 1/Ifixed...
               // ...OK 
               int32_t ResistorUT = adc0*200/vDiff - adc1*200/vDiff; 
                                    //vResistorUT * 200 / vDiff;
               sprintf(adcString, "R[0-1] = %ld\n\r", ResistorUT);
            }
            else
               sprintf(adcString, "R[0-1]: vDiff = 0!\n\r");

            TransmitString(adcString);
*/

}
#endif //0


//Other things that might be useful...

/*
#if (!defined(ADC_LOWSPEED) || !ADC_LOWSPEED)
 #define ADC_STEPS_PER_DMS_COUNT 192
#else
 #define ADC_STEPS_PER_DMS_COUNT 96 //4808Hz -> 48/100 -> 96/200
#endif

#define DMS_COUNT_PER_ADC_STEPS 200 //100
*/

//------------------------


//This could probably be selectedADC = !selectedADC but I'm not certain
//selectedADC++;
//selectedADC &= 0x01;

// selectedADC++;
// selectedADC &= 0x01;
/*
   lds   r24, 0x0060
   subi  r24, 0xFF   ; 255
   andi  r24, 0x01   ; 1
   sts   0x0060, r24
*/

// selectedADC = !selectedADC;
/* ldi   r24, 0x00   ; 0
   lds   r25, 0x0060
   and   r25, r25
   brne  .+2         ; 0xc42 <__vector_11+0x52>
   ldi   r24, 0x01   ; 1
   sts   0x0060, r24
*/
// selectedADC ^= 0x01;
/* lds   r24, 0x0060
   ldi   r25, 0x01   ; 1
   eor   r24, r25
   sts   0x0060, r24
*/

// selectedADC = !(selectedADC&0x01);
/*
   lds   r24, 0x0060
   com   r24
   andi  r24, 0x01   ; 1
   sts   0x0060, r24
*/

//------------------------
#if 0
int main(void)
{

   //Found experimentally: assuming the free-running ADC is always 13
   // cycles per interrupt...
   // The default value was read to be 0x9f
   // This is of course device-specific
   OSCCAL = 0x9a;

   //*** Initializations ***

   //PinChange interrupts are controlled by the masks below...
   // They default to MOST pins enabled!
   // Clear them here and only enable them as appropriate (in usi_uart)
   PCMSK0=0;
   PCMSK1=0;

   //The ADC interrupt is used for dmsUpdate...
   // it (should) occur at 8MHz/32/13 = 19,230.76923076923Hz
   // There's probably some way to make this more accurate...
   init_dmsExternalUpdate(ADC_STEPS_PER_DMS_COUNT,DMS_COUNT_PER_ADC_STEPS);

   //What's now the ADC init function was here...
   // Followed by this, which was already commented-out...

/*
   adc_init();

   //Seriously?! 2^9 samples?! This can't be right.
   // Well...? Was taking 1024 samples on three channels 3072 samples
   // ADC clock is running at 8MHz/128 = 62.5kHz
   // All samples together took a little over 1sec
   // That's ~20 ADC cycles per sample
   // According to the datasheet, this is about right... normal=13 first=25
   //#define SUMPRECISION  9
   // But why is SUMNUM 2<< instead of 1<<?
   // (To gain an extra LSB when >>'d back)
   #define SUMPRECISION 0
   //With SUMNUM = 1, ~15 samples are performed before the
   //  transmit buffer is cleared... so let's set this to do 16 samples per
   //  sum, (and it should do two sums)
   //  hopefully this'll slow it down so the computer doesn't crash
   #define SUM_EXTRA_BITS 5
   //was 2<<SUMPRECISION...
   #define SUMNUM ((uint16_t)(1 << (SUMPRECISION + SUM_EXTRA_BITS)))

   //The ADC summer takes SUMNUM samples, and adds them together
   // Thus, by dividing the sum by SUMNUM you get the average of all the
   //  samples... (for noise reduction)
   // Or we an use the actual sum as a higher-precision sample
   //  (kinda questionable, but yahknow...)
   //  In this case, the maximum value is 
   //      (1024-1)*SUMNUM (NOT 1024*SUMNUM-1)
   //      = 130944 (1,1111,1111,1000,0000) (for SUMNUM=1<<7)
   #define SUM_MAX (((uint32_t)1023 * (uint32_t)SUMNUM)>>SUMPRECISION)
   //  SINCE: SUMPRECISION = 0...
   //   (If not, then we lose some of the extra precision
   //    And the max is changed by SUM_MAX>>SUMPRECISION
   //    It's not currently used, and I'm not sure why I would)
   adc_sumSetup(SUMNUM,(1<<4) | (1<<5) | (1<<6) | (1<<3) | (1<<7) | (1<<9));
   
   //Make sure we have valid adc values before contiuing
   // the first iteration should be alright, but who knows... might as well dispose of it.
   while(!adc_sumUpdate())
   {
      dms_update();
      heartUpdate();
   }
*/

}
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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/adcFreeRunning/0.10ncf/adcFreeRunning.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
