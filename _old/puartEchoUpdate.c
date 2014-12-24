/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




//a/o v50:
// puartEchoUpdate() has been noted in mainConfig.h
//  AND moved here, as the keyboard has long-since died.
//  AND this is quite ugly.



//[Most of] this removed from writingSD(), right near the top:
/*


    #if(!defined(LC_FC_WBC_BW_DISABLED) || !LC_FC_WBC_BW_DISABLED)
      #if(defined(PUART_ECHO) && PUART_ECHO)

      //Let's see if skipping this stuff when receiving makes it 
      // fast enough for 9600bps input
      //WTF: Enabling this causes loopCount to drop to 34500
      // FullCount regularly 4600!
      // (for a *single byte* test?!)
      // And that's *without* data being transmitted.
//    extern uint8_t rxState[];

//    if(rxState[0] == 0)
      #endif
      {
*/


//This is still somewhat horribly named...
// better might be hssKB_handler_update()?
#error "puartEchoUpdate() should be revised and renamed... see nkp_update()"












void puartEchoUpdate(void)
{     
      
      tcnter_t thisTime = tcnter_get();
      static tcnter_t nextEchoTime = 0;
      static char kbBuffer[KBBUFFER_SIZE];
      static uint8_t byteNum = 0;
      static uint32_t echoLoopCount = 0;
      puar_update(0);
   // puat_update(0);
      //static unsigned char rxByte;
      //Track the position _to_be_written_
      // e.g. 0 = nothing to be written
      //      1 = low nibble
      //      2 = high nibble
      //static uint8_t rxBytePosition = 0;

      if(puar_dataWaiting(0) && (byteNum < (KBBUFFER_SIZE-1))
   #if(defined(KB_TO_SAMPLE) &&(KB_TO_SAMPLE))
            && (bytePositionToSend == 0)
   #endif
        )
      {
         rxByte = puar_getByte(0);

         //Toggle whenever data's received
         togglepinPORT(HEART_PINNUM, HEART_PINPORT);

   #if(defined(HSSKB_TRANSLATE) && HSSKB_TRANSLATE)
         rxByte = hsSKB_toChar(rxByte);
         
        if(rxByte != 0)
        {
   
           memoUpdate(rxByte == MEMO_RETURN);
   
         //Intentionally not elsing... treat a memo-key like a normal ke
         // as well
         // that bytePositionToSend bit may be important
   #else //!HSSKB_TRANSLATE
        if(rxByte != 0)
        {
   #endif
      
   #if(defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
            //This indicates that there is a byte to write in the sample
            // header. But also indicates that the usage-table needs
            // to be reloaded to the SD at the end of this block.
            // (This is all handled in sd_writeUpdate() )
            // ( Not sure if that piece can be moved back to main...)
            // WHY isn't the eeprom_update here, at least...?
            bytePositionToSend = 1;


            //Why on earth did I put the eeprom write-update in 
            // sd_writeUpdate....?
            // maybe so its chunk number would align with the data?
            // in case a key-press is handled near the end of a chunk
            // main() would receive the rxData, but the next 
            //   few writeUpdates could be used for locating a new chunk
            //   SD_WriteMultiple stop/start, end of card, etc...
            //   If eeprom was written immediately, it would be located
            //   at the chunk which was active *then*, whereas
            //   the data itself wouldn't be written with sample-data
            //   until the next chunk...
#warning "Do we have to worry about this with Memo, and its usage for marking the prior chunk, etc.?"
            // Might...
            //Or was it just because it has to stop writing of the
            //SDcard's music samples, then update the usage-table on the
            //SDcard, then start music samples again (likely in the middle
            //of the same block)
            // so by handling it in there...? eh, maybe I'm lost again.
   #else
      #warning "KB_TO_SAMPLE != TRUE, no logging will happen, either!"
   #endif

            //printByte(rxByte);
            kbBuffer[byteNum] = rxByte;
            byteNum++;
            kbBuffer[byteNum] = '\0';
         }  //rxByte != 0
      }  //DataWaiting
      if((thisTime >= nextEchoTime))
      {
         //This just causes regular blinking
//       togglepinPORT(HEART_PINNUM, HEART_PINPORT);

         cli();
         //This'll repeat whatever's in the string buffer from before
         // (e.g. Go! on boot, but intended to repeat the kb input)
         printBuff(kbBuffer);
         kbBuffer[0] = '\0';

   #if(defined(PRINT_ECHOLOOPCOUNT_1SEC) && PRINT_ECHOLOOPCOUNT_1SEC)
         sprintf_P(stringBuffer, PSTR("\n\rlc:%"PRIu32"\n\r"),
                                                echoLoopCount);
         printStringBuff();
   #endif
   #if(defined(PRINT_LOOPNUM) && PRINT_LOOPNUM)
         sprintf_P(stringBuffer, PSTR("\n\rtln:0x%"PRIx8"\n\r"),
                                                thisLoopNum);
         printStringBuff();
   #endif

         nextEchoTime = thisTime + 250000;

         byteNum = 0;
         stringBuffer[byteNum] = '\0';
         sei();
         echoLoopCount = 0;
      }

      echoLoopCount++;
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
 * /home/meh/_avrProjects/audioThing/57-heart2/_old/puartEchoUpdate.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
