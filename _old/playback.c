/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */






#if 0 //This is all from the ol' playback days...
int32_t addSample(cirBuff_t *p_myCirBuff)
{
         int32_t dataIn = spi_sd_readU16();
         if(dataIn >= 0)
         {
            uint16_t data = dataIn;
            //Make it useable... 
/*          // it's actually int16, but we're loading to u8
            //Looks, so far, like the max value is ~ 0x400

            // treat it like a int16:
            int16_t * p_i16data;
            p_i16data = (int16_t *)(&data);
            int32_t temp = (*p_i16data);

            //Now scale it to fit in an int8
            // Again, it looks like the max value is around 0x400
            // so don't scale by INT16_MAX, but by sample-max...
            //temp = (temp*INT10_MAX)/(0x1000);
            temp >>= 5;
            //temp = (temp*INT8_MAX)/(0x800);   //(INT16_MAX);

            //Now shift the center to 127 in a u8
            //temp += INT8_MAX;
            temp += INT10_MAX;
*/
            //NOW: It's unsigned raw int16... so just shift it into uint10
            data >>= 6;

            cirBuff_add(p_myCirBuff, data, DONTBLOCK); 
                                          //(uint16_t)temp, DONTBLOCK);

#if (defined(PRINTOUT) && PRINTOUT)
            //char string[30];
            sprintf_P(stringBuffer, 
                  PSTR("addSample 0x%"PRIx16"->0x%"PRIx16"\n\r"),
                  data, (uint16_t)data);
            printStringBuff();
#endif
            return data; // (uint16_t)temp;
         }
         else
            return -1;
}
#endif //0

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
 * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_old/playback.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
