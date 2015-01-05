/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#include <stdio.h>
#include <inttypes.h>
#include <math.h> //fabs
//THIS IS ENTIRELY EXPERIMENTAL:

//The Idea:
//Given a 4x4 pushbutton matrix (for now)
// with an unknown pinout...
//
// Is there a set of resistance-values that can be attached in *any*
// configuration such that each button's resistance [path] is unique?
//
// The dilemma:
//
// With an unknown pinout, it's quite likely the rows and columns will not
// be in order. In fact, rows and columns may be interspersed in the
// resistor-network.
//
// In which case, unlike a unit which has rows separated from columns,
// resistor RD is necessary to complete the chain so-as not to miss a
// pushbutton.
//
// The simple way to make any button decipherable would be to use
// resistances that are 2^n in value. BUT: doing-so introduces some
// considerations:
// In this example, 8 resistors are necessary (at least 7 of different
// values?)
// Then we have resistances from 1k to 128k
// With 1% resistors, the 128k could easily be off by 1k,
//  Thus, one potential path (pushbutton across the 128k resistor)
//   of 1k+2k+4k+8k+16k+32k+64k = 127k
//  Could look identical to another potential path (pushbutton bypassing
//   all but the 128k resistor)

// Another potential method would be to take a root of these values, e.g.
// cubeRoot(2^n) results in something like 
// (from poorly-labelled scribblings):
//  1k, 1.23k, 1.515k, 1.88k, 2.29k, 2.83k, 4.28k

// The *thought* is, these values would lead to easily-decipherable
// resistance-paths that aren't *huge* in difference
// (Thus, e.g. if T=RC is such that RG*C=1ms, and RA*C=127ms... 255ms is
//  quite slow for measurements, but, RG*C=1us and RA*C=127us might be too
//  hard to measure with anaButtons...)
// AND might be plausible with a bit of fudging with
//  standard-resistor-values... (parallel/serial resistors?)
//  without being as prone to tolerance-error...


//  PD6 ><----+---------+-->   >--*-------*-------*-------*   These are
//            |         |          \       \       \       \   pushbuttons
//            |         /         \ O     \ O     \ O     \ O  <-------
//    .1uF  -----  RG   \         ,\ O    ,\ O    ,\ O    ,\ O 
//          -----  ??k  /           \ \     \ \     \ \     \ \
//            |         \              *       *       *       *
//            |         |              |       |       |       |
//            V         +-->   >--*----|--*----|--*----|--*----|
//           GND        |          \   |   \   |   \   |   \   |
//                      /         \ O  |  \ O  |  \ O  |  \ O  |
//                 RF   \         ,\ O |  ,\ O |  ,\ O |  ,\ O |
//                 ??k  /           \ \|    \ \|    \ \|    \ \|
//                      \              *       *       *       *
//                      |              |       |       |       |
//                      +-->   >--*----|--*----|--*----|--*----|
//                      |          \   |   \   |   \   |   \   |
//                 RE   /         \ O  |  \ O  |  \ O  |  \ O  |
//                 ??k  \         ,\ O |  ,\ O |  ,\ O |  ,\ O |
//                      /           \ \|    \ \|    \ \|    \ \|
//                      \              *       *       *       *
//                      |              |       |       |       |
//                      +-->   >--*----|--*----|--*----|--*----|
//                      |          \   |   \   |   \   |   \   |
//  This resistor       |         \ O  |  \ O  |  \ O  |  \ O  |
//  is only        RD   \         ,\ O |  ,\ O |  ,\ O |  ,\ O |
//  necessary for  ??k  /           \ \|    \ \|    \ \|    \ \|
//  unknown pinouts     \              *       *       *       *
//                      /              |       |       |       |
//                      |              v       v       v       v
//                      |
//                      |              v       v       v       v
//                      |              |       |       |       |
//                      '--------------+-/\/\/-+-/\/\/-+-/\/\/-+
//                                        ??k     ??k     ??k  |
//                                        RC      RB      RA   / Rmin
//                                                             \??k=1k?
//          This resistor, to ground, is necessary since the   /
//          pin (PD6) will be used as an output to charge the  |
//          capacitor to 3V3, EVEN WHEN a button is pressed.   V
//                                                            GND


// In a setup with an unknown pinout, there would be *every* possibility
// for buttons bypassing resistor[s]... E.G. in one case, All Resistors,
// except Rmin, would be bypassed, In another case, only RG might be
// bypassed. In another case only RA, in another RF, RC, and RA might
// be... 

// When no buttons are pressed, however, the case will always be
// RG+RF+RE+RD+RC+RB+RA+Rmin

// So, for now, the argument 'activeResistors' is a 7-bit value 
// representing which
// resistors are NOT bypassed (Rmin can't be bypassed)

// Resistor:   GFEDCBA
// e.g. 127=0b01111111 results in no button-pressed
//      0  =0b00000000 results in i.e. the upper-right button in the 
//                     drawing, which bypasses all but Rmin
//NUM_Rs excludes Rmin...
#define NUM_Rs 7
float calcResistance(float rMin, float rVals[], uint8_t activeResistors)
{
   int i;

   float pathResistance = rMin;

   for(i=0; i<NUM_Rs; i++)
   {
      if((activeResistors&(1<<i)))
      {
         pathResistance += rVals[i];
      }
   }

   //printf("activeResistors: 0x%02"PRIx8" pathResistance=%.3f\n", activeResistors,
   //      pathResistance);

   return pathResistance;
}


//This is a list of standard 5% resistor-values 
// stolen from http://ecee.colorado.edu/~mcclurel/resistorsandcaps.pdf
// I've never noticed some of these values... (1.3? Brown Orange Whatever?)
// Oh, maybe... Brown-Orange-White comes to mind.
//1.0 is being assumed for Rmin... so not listed here, but it is standard
float stdRes[]={1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
   3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};

#define NUM_STD (sizeof(stdRes)/sizeof(stdRes[0]))

//This'll return the next set of 7 each time it's called...
uint32_t get7_std(float rVals[])
{
   static uint32_t i=0;

   //Increment i until the next set of 7 is found...
   // e.g. 0b000...01111111 will be the first,
   //      0b000...10111111 will likely be the next
   for(i++; i<(1<<NUM_STD); i++)
   {
      int count = 0;
      int j;
      for(j=0; j<32; j++)
      {
         if(i&(1<<j))
         {
            if(count < NUM_Rs)
               rVals[count] = stdRes[j];

            count++;
         }
      }

      if(count == 7)
      {
         break;
      }
   }

   if(i==(1<<NUM_STD))
      return -1;

   return i;
}


//Returns the zero-count...
// (call a second-time with doPrint if zero-count is 0)
int getSpecs(float rMin, float rVals[], int doPrint)
{
   int i;
   float pathVals[(1<<NUM_Rs)];
   
   float differenceMin = 1000000;
   uint8_t minA=0, minB=0;

   float differenceMax = -1;
   int zeroCount = 0;
   for(i=0; i<(1<<NUM_Rs); i++)
   {
      //Calculate the resistance of every possible path
      pathVals[i] = calcResistance(rMin, rVals, i);

      int j;

      for(j=1; j<i; j++)
      {
         //Determine the minimum difference between every path...
         float pathDifference = fabs(pathVals[i] - pathVals[j]);
         
         if(pathDifference < 0.001)
            zeroCount++;

         if(pathDifference < differenceMin)
         {
            minA = i;
            minB = j;
            differenceMin = pathDifference;
         }

         if(pathDifference > differenceMax)
            differenceMax = pathDifference;
      }
   }

   if(doPrint)
   {
      printf("rVals=\n");
      for(i=0; i<NUM_Rs; i++)
      {
         printf(" %.3f\n", rVals[i]);
      }
     
      printf("Maximum path difference: %.3f\n", differenceMax);
   
      printf("Minimum path difference: %.3f\n", differenceMin);
      printf("  0x%02"PRIx8" = %.3f\n", minA, 
                                    calcResistance(rMin, rVals, minA));
      printf("  0x%02"PRIx8" = %.3f\n", minB, 
                                    calcResistance(rMin, rVals, minB));

      if(zeroCount)
      {
         printf("***zeroCount = %d***\n", zeroCount);
      }
   }
   return zeroCount;
}



int main(int argc, char *argv[])
{
   float rMin = 1;
   float rVals[NUM_Rs] = {2,4,8,16,32,64,128};

   int i;


   float root = 1;

   if(argc > 1)
   {
      int invalidArg = 0;

      if(argc == 2)
      {
         if(1!=sscanf(argv[1], "%f", &root))
            invalidArg=1;
      }
      else if(argc == NUM_Rs+1)
      {
         for(i=0; i<NUM_Rs; i++)
         {
            if(1!=sscanf(argv[i+1], "%f", &(rVals[i])))
               invalidArg=1;
         }
      }
      else
         invalidArg=1;

      if(invalidArg)
      {
         printf("Invalid argument\n");
         return 1;
      }
   }

   if(argc != NUM_Rs+1)
   {
      printf("\n\n%.3f root\n", root);
      printf(" pow((2^n),%.3f))\n", (1/root));
   }
   else
      printf(" Custom RVals\n");


   for(i=0; i<NUM_Rs; i++)
   {
      rVals[i] = pow(rVals[i], (1/root));
   }

   getSpecs(rMin, rVals, 1);



   uint32_t get7;

   do
   {
      get7 = get7_std(rVals);



      if(getSpecs(rMin, rVals, 0) == 0)
         getSpecs(rMin, rVals, 1);

//      printf("0x%08"PRIx32"\n", get7);

   }while(get7 != -1);

   return 0;

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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/anaButtons/0.50/resistorCalc/resistorCalc.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
