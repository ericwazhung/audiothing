/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#include <stdio.h>
//#include "../0.97/cirBuff.h"
#include _CIRBUFF_HEADER_

//I dunno about threading nor OSX interrupts, so I can't test blocking here...

#define LENGTH 10

#if (defined(CIRBUFF_NO_CALLOC) && CIRBUFF_NO_CALLOC)
#warning "Testing without CALLOC"
 cirBuff_data_t   buffer[LENGTH];
#endif

int main(void)
{
   cirBuff_t BuffMan;
   int i, j;
   int byteRead;
   
#if (defined(CIRBUFF_NO_CALLOC) && CIRBUFF_NO_CALLOC)
   cirBuff_init(&BuffMan, LENGTH, buffer);
#else
   cirBuff_init(&BuffMan, LENGTH);
#endif

   printf(" Buffer is %d bytes\n", LENGTH);  
   printf("First, we add bytes, one by one, without blocking\n"
          " Then we read them all, plus two additional to test read\n"
          " X indicates bytes not written due to buffer being full\n"
          " Y indicates bytes not read due to buffer being empty\n");

   for(i=0; i< 12; i++)
   {
      for(j=0; j<i; j++)
      {
         if(cirBuff_add(&BuffMan, j+'A', DONTBLOCK))
            printf("X");
      }
      
      printf("%d/%d bytes written/unused, writePos:%d, readPos:%d\n", j, 
                           cirBuff_availableSpace(&BuffMan),
                           BuffMan.writePosition, 
                           BuffMan.readPosition);
   
      printf(" Rereading bytes: "); 
      for(j=0; j<i+2; j++)
      {
         byteRead = cirBuff_get(&BuffMan);
         
         if(byteRead != CIRBUFF_RETURN_NODATA)
            printf("'%c' ", byteRead);
         else
            printf("Y");
      }
      printf("\n");
   }
   
   printf(" Step1 Done\n");
   
   for(i=0; i< 12; i++)
   {
      for(j=0; j<i; j++)
      {
         cirBuff_add(&BuffMan, j+'A', DONTBLOCK);
         j++;
         cirBuff_add(&BuffMan, j+'A', DONTBLOCK);
         
         byteRead = cirBuff_get(&BuffMan);
         
         if(byteRead >= 0)
            printf("%c ", byteRead);
         else
            printf("0");
      }
      
      printf("%d written: ", j);
      
      for(j=0; j<i+2; j++)
      {
         byteRead = cirBuff_get(&BuffMan);
         
         if(byteRead >= 0)
            printf("%c ", byteRead);
         else
            printf("0");
      }
      printf("\n");
   }
   
   printf(" Step2 Done\n");
   printf("yup\n");
   printf("length=%d, readPos=%d, writePos=%d\n", (int)BuffMan.length, (int)BuffMan.readPosition, (int)BuffMan.writePosition);
   printf("yup2\n");
   printf("yup3\n");
   printf("hardCalcNextWritePos=6\n"); //6%11);
   printf("nextWritePos=%d\n", (BuffMan.writePosition + 1)%(BuffMan.length));
   
   //Learn som'n new every day... printf doesn't print until a \n is received?!
   printf(" Now we're testing blocking... the program should halt when\n"
          " The buffer is full... (Press CTRL-C to kill)\n");
   //Test blocking as best we can...
   for(i=0; i<15; i++)
   {
      cirBuff_add(&BuffMan, i+'A', DOBLOCK);
      printf("%c \n", i+'A');
   }
   
#if (!defined(CIRBUFF_NO_CALLOC) || !CIRBUFF_NO_CALLOC)
   cirBuff_destroy(&BuffMan);
#endif

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
 * /home/meh/_avrProjects/audioThing/57-heart2/_commonCode_localized/cirBuff/1.00/test/main.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
