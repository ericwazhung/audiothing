/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */


//#warning "TODO: Don't bother updating the usage-Table if it's already been done in this chunk"
#warning "TODO: Key Input for how far back to mark used..."
// e.g. "Memo" key, with each key-press marks usage-table
//                  back further... (5min each press? logarithmic?)
//
//Marked Earlier This Loop
//                 v             v---- In This Chunk (loop three)
//     . . . . 0 0 3 0 2 0 0 1 1 n . . . . 
//     . . ....^-^-^-^---^-^-----^    <---Path up to now
//
//  Say we want to mark this, and 5 prior chunks...
//   (including one that was previously marked)
//
// Don't need to treat previously-marked any differently
// It's easy to see how we got here, even in reverse...
//
//                                        n 1 1 0 0 2 0 3 0 0 . . . .
//                                        ^-x-x-^-^-x-^-^-^
//                                        | \_/ | | . | | |
//                    This Chunk (Mark) --/  .  | | . | | |
//                                           .  | | . | | |
//          (Not Used This Loop, Skip) ....../  | | . | | |
//                                              | | . | | |
//                       1-prior (Mark) --------/ | . | | |
//                       2-prior (Mark) ----------/ . | | |
//                                                  . | | |
//                              (Skip) ............./ | | |
//                                                    | | |
//                       3-prior (Mark) --------------/ | |
//                                                      | |
//  4-prior Marked earlier in this loop \               | |
//                     OK to mark again  >--------------/ |
//       Definitely Include it in count /                 |
//                                                        |
//          5-prior (Last To Be Marked) ------------------/
//
//                               v----Now...
//     . . . . 0 3 3 3 2 3 3 1 1 3 . . . .
//
// Easy...
// Now comes the difficulty:
// How do we keep track of how far back we want to go...?
//  Say:
//   We're in n (near the n->n+1 transition)
//   Press the Memo key twice
//   It Advances to n+1
//   Continue pressing the Memo key three more times
//  OK, simple... use a different ut_counter for the memo-key
//   and step back each time the key's pressed, in real-time
//   until ... is there a time-out?
//
//  ... so... 
//    Always mark *this* chunk during a keypress
//    Only update it if it hasn't already been marked
//    Separate memo-tracker
//      Incremented 5min each time Memo is pressed
//      Resets itself to "now" after 10sec unpressed
//
// Not bad... but what when we're marking at the edge of a loop?
//
// In loop 4
//                0 1 2 3 4 . . . . 0xfc 0xfd 0xfe 0xff<--Loop After Here
//                | | | | |          |    |    |    |
// UsageTable:    0 n 3 0 2 . . . .  0    0    1    0 
//   
// ToMark:        4 4 ................... 3 ....... 3
//
// (don't mark 'em 4, left of the loop-point, or it won't make sense...)



// IMPLEMENTATION CODE SNIPPETS... a/o v23




//See description...
// Basically, this is the usageTable position that is to be marked
// when the "memo" key is pressed...
// Each time it's pressed, this will jump to the previous position in the
// usage table and that position will be marked
// After ten seconds of not-being-pressed it will reset to follow
// thisChunkNum...
uint8_t memoPositionInUT = 0;

//Loop Number to be written when Memo is pressed
uint8_t memoLoopNum = 0;

// Don't allow Memo's loopNum to decrease below bootLoopNum
uint8_t bootLoopNum = 0;

//Rather than testing whether the memoPositionInUT in eeprom contains
// memoLoopNum, to determine whether to update the SD, use this instead
// (because memoPositionInUT may not contain a valid position
//  if memo was pressed more times than there is data...)
uint8_t writeMemoToUT = FALSE;



// INIT:


   //Regardless of whether formatting was necessary
   // thisLoopNum is now valid 
   //    (either from original usage-table, or 1 if formatted...)
   // For now, we always boot at the beginning of the card...
   // We need this here, because eventually the memo_chunkNum will need
   //  to be initialized with thisChunkNum, here...
   thisChunkNum = findNextUsableChunk(0);
   
   memoPositionInUT = thisChunkNum;

   memoLoopNum = thisLoopNum;
   //Don't let a new memo (soon after boot) wrap around to mark
   // as though it'd already looped once since boot...
   bootLoopNum = thisLoopNum;




// MAIN LOOP:
         rxByte = hsSKB_toChar(rxByte);


         if(rxByte == MEMO_RETURN)
         {
            static tcnter_t lastMemoTime = 0;
      #define MEMO_TIMEOUT (2500000)   //10sec
            if(thisTime - lastMemoTime > MEMO_TIMEOUT)
            {
               memoLoopNum = thisLoopNum;
               memoPositionInUT = thisChunkNum;
            }

            lastMemoTime = thisTime;

            //Find the previous chunk
            // (Don't include the current chunk, it's handled elsewhere)
            uint8_t ut;
            //uint8_t writeMemoToUT = TRUE;
            int16_t memoPosTemp = memoPositionInUT;
            uint8_t memoLoopTemp = memoLoopNum;


            //Guess it was lucky I missed this bug until now...
            // The math isn't working, and I'd forgotten to add this line
            // shouldn't be related, but is preventing unnecessary writes
            // at dangerous locations...
            // Or maybe the math is right...
            // maybe everything from 0x2a-0x3b (so far) is 0xff
      #if(!defined(MUT_BUG_FINDER) || !MUT_BUG_FINDER)
            //This'll be falsified as appropriate in the loop below...
            writeMemoToUT = TRUE;
      #endif

            do
            {
               /*
               //Decrement the position
               memoPosTemp--;

               //If it decrements past zero
               //  wrap around to 255
               //  decrement the loopNum
               if(memoPosTemp < 0)
               {
                  memoPosTemp = 255;

                  if(memoLoopTemp > bootLoopNum)
                  {
                     memoLoopTemp--;
                  }
                  else
                  {
                     
                  }
*/
               //Wrap around, and decrement memoLoopNum
               // This doesn't handle when booting
               // E.G. 5 min in after boot memo-pressed for 10min, 
               //  
               if(memoPositionInUT == 0x00)
               {

                  // Don't attempt to wrap-around to the end of the usage
                  // table, if it just booted...
                  // Also doesn't make sense to loop twice... NYH
                  if(memoLoopNum > bootLoopNum)
                  {
                     memoPositionInUT = 0xff;
                     memoLoopNum--;
                  }
                  else
                  {
                     writeMemoToUT = FALSE;
                     break;
                  }
               }
               else
                  memoPositionInUT--;


               ut = eeprom_read_byte(
                     (uint8_t *)((uint16_t)memoPositionInUT));

               //DON'T skip counting utVals which match this loop number...
               if(ut == memoLoopNum)
               {
                  //But don't write them, either.
                  writeMemoToUT = FALSE;
                  break;
               }
            //Skip non-zero utVals
            } while( (ut != 0) ); //&& (ut != memoLoopNum) );


            if(writeMemoToUT)
            {
               //No need to update, it's already known to be different
               eeprom_write_byte((uint8_t *)((uint16_t)memoPositionInUT),
                                                         memoLoopNum);
   #if(defined(PRINT_MEMOVALS) && PRINT_MEMOVALS)
               puat_sendStringBlocking_P(PSTR("W:"));
   #endif
            }

   #if(defined(PRINT_MEMOVALS) && PRINT_MEMOVALS)
               sprintf_P(stringBuffer, 
                           PSTR("mp:0x%"PRIx8" ml:0x%"PRIx8
                                 " ln:0x%"PRIx8" cn:0x%"PRIx8"\n\r"),
                           memoPositionInUT, memoLoopNum,
                           thisLoopNum, thisChunkNum);
               puat_sendStringBlocking(stringBuffer);
   #endif
            

            //At this point, only the eeprom UT has been updated
            // Since rxByte = MEMO_KEY_RETURN, it will ALSO be treated
            // as a normal key-press
            // which causes a mark at the current position
            // (which was *not* handled above)
            // So... since the eeprom has been updated, when the table is
            // written, it will also include the latest UT update from Memo
         }

         //Intentionally not elsing... treat it normally as well
         // that bytePositionToSend bit may be important

         if(rxByte != 0)
         { .... }
      
      
//WRITE UPDATE:
      
      
      //Data bytes:
      default:
         {
            //Moved to global, since statics aren't supposed to be
            // in inline functions...
            //static uint8_t sdWU_dataState = 0;
            //static cirBuff_dataRet_t sample;
            switch(sdWU_dataState)
            {
               case 0:
                  //Only do this if there's data in the cirBuff...
                  //cirBuff_dataRet_t sample;
                  sdWU_sample = cirBuff_get(&myCirBuff);
                  if(sdWU_sample != CIRBUFF_RETURN_NODATA)
                     sdWU_dataState++;
                  break;
               case 1:
                  {
                  uint8_t sampleHigh = (sdWU_sample>>8);
#if(defined(KB_TO_SAMPLE) && KB_TO_SAMPLE)
                  if(bytePositionToSend == 0)
                  {
                     //Indicate that something has been noted, recently
                     if(kbIndicateCount)
                        sampleHigh |= (0x03<<2);
                  }
                  //bytePositionToSend is set to 1 when kbData is available
                  if(bytePositionToSend == 1)
                  {
                     //Update the usage-table to indicate
                     // that something was logged...
                     // But we needn't do it more than once per chunk
                     if( (eeprom_read_byte((uint8_t *)thisChunkNum)
                              != thisLoopNum )
                        || writeMemoToUT )
                     {
                        writeMemoToUT = FALSE;
                        usageTableUpdateState = 1;
                        //don't use write_byte instead of update_byte
                        // since this could've entered due to 
                        // writeMemoToUT
                        eeprom_update_byte((uint8_t *)thisChunkNum, 
                                                         thisLoopNum);
                     }
                     
                     bytePositionToSend++;
                     sampleHigh |= ((rxByte<<4) | (0x01<<2));
                  }
                  else if(bytePositionToSend == 2)
                  {
                     bytePositionToSend = 0;
                     sampleHigh |= ((rxByte&0xf0) | (0x02<<2));
                     //Reset the kbIndicateCount after each byte...
                     kbIndicateCount = KBINDICATE_BLOCKS;
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
 * /home/meh/_avrProjects/audioThing/55-git/_Notes/MemoKey.c
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
