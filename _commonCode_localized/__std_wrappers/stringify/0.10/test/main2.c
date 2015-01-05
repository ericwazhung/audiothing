/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


#include <stdio.h>
#include "../stringify.h"

/* from info cpp 3.4:
	There is no way to combine an argument with surrounding text and
	stringify it all together.  Instead, you can write a series of adjacent
	string constants and stringified arguments.  The preprocessor will
	replace the stringified arguments with string constants.  The C
	compiler will then combine all the adjacent string constants into one
	long string.

	That dun woik for #include...
*/

/*
#define test(a, b) \
{ \
#ifdef _TEST_ \
 4 \
#else \
 5 \
#endif \
}

int main(void)
{
	int a=1;
	int b=3;

	printf("a=1, b=3, test(a,b)=%d\n",test(a,b));
}
*/



#define VALUE  100
#define VALUE2 VALUE

#define VALUE3 (VALUE/2)

int main( int argc , char const *argv[] )
{
   printf("#define VALUE 100\n");
   printf("printf(\"%%s\", STRINGIFY(VALUE)) ->");
	    printf( "%s\n" , STRINGIFY(VALUE));

   printf("#define VALUE2 VALUE\n");
   printf("printf(\"%%s\", STRINGIFY(VALUE2)) ->");
	    printf( "%s\n" , STRINGIFY(VALUE2));

   printf("#define VALUE3 (VALUE/2)\n");
   printf("printf(\"%%s\", STRINGIFY(VALUE3)) ->");
	    printf( "%s\n" , STRINGIFY(VALUE3));

   printf("%s\n", STRINGIFY(STRINGIFY(VALUE3)));

   printf("%s\n", STRINGIFY(STRINGIFY_INDIRECTOR(VALUE3)));

   printf("%s\n", STRINGIFY_INDIRECTOR(STRINGIFY(VALUE3)));
#define STRINGIFY_INDIRECTOR( x) #x
#define STRINGIFY(x) STRINGIFY_INDIRECTOR(x)

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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/__std_wrappers/stringify/0.10/test/main2.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
