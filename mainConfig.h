/* mehPL:
 *    This is Open Source, but NOT GPL. I call it mehPL.
 *    I'm not too fond of long licenses at the top of the file.
 *    Please see the bottom.
 *    Enjoy!
 */




#ifndef __MAINCONFIG_H__
#define __MAINCONFIG_H__


//This file contains most of the configuration-options:
// e.g. what to print, or not print, where to print it
// enable Audio Output, etc...



//#define NLCD_BOOT_TEST TRUE


//This should be usable in more ways than just START_STOP (whatever that
//means)
// It should essentially make the card look as though it's this length
// So loops should occur after this amount of time
// And the usage-table should cover *this* size, rather than the full card
// THIS IS NOT compatible with audioThing7p9, as there's no (current) way
// to indicate a size-override in the card's formatting header.
// audioThing7p9 should still be able to read the usage-table, and that's
// all I'm working with right now.
//#define START_STOP_TEST 300    //seconds



//Choose one of these below, in PRINTER
#define PRINT_NULL   0
#define PRINT_PUAT   1  //RS-232      //6.7k
#define PRINT_NLCD   2  //Nokia LCD   //8.0k (yikes!)

#define PRINTER   PRINT_PUAT



//WEIRD... earlier the same code PRINT_PUAT=6.7k, PRINT_NLCD=8.0k
// now NLCD fits and PUAT doesn't.

// NULL = 7818
// NLCD = 158 over

//a/o v48:
// NLCD = 7964
// PUAT = 8010
// PUAT_STRINGERS = 8070



//Disable RS-232 reception (what is it used for these days? Previously,
//the Stowaway Keyboard, which has died long ago)
#define PUAR_DISABLED   TRUE




//Enable real-time audio output via PWM...
// This is hokily-implemented and mainly only for debugging purposes.
// It does *not* allow for playback of stored-memos.
#define AUDIO_OUT_ENABLED TRUE


//AUDIO_PASSTHROUGH is entirely different... See below.



//Enable the Nokia Keypad via anaButtons
#define NKP_ENABLED  TRUE



//For determining which values correspond to which keys...
//#define PRINT_NKP_VALS         TRUE




//This is just to remove a significant portion of the NON-High-Capacity
// code for SD-Cards, to save code-space while testing...
// It's by no means reliable for distribution
// Further, it could be better handled, to gain even more codespace.
// THIS SHOULD NOT BE TRUE.
//#define SD_HC_ONLY TRUE
// But it's nearly 200B larger with it (with NON-SD-HC support?)




//This has been the case since v5-17... This should serve no purpose.
// Trying to piece-together a way to not have to use an interrupt
// so-as not to interfere with recording, etc.
//a/o v50: DMS has been replaced with tcnter
//#define DISABLE_DMS_INTERRUPT TRUE







//DO_FORMAT enables card-formatting
// If the card is not already formatted *FOR AUDIOTHING* then it will be
// if this is TRUE
// *NOTE* audioThing does NOT use a normal filesystem
// SO: MAKE SURE you DON'T WANT TO KEEP what's on your SD-Card!
// It *will* be lost.
// OTOH, if this is NOT true, then it won't work, because there's no way to
// format an SDCard in the "audioThing" format, without using this device.
#define DO_FORMAT       TRUE //FALSE


#if(!defined(DO_FORMAT) || !DO_FORMAT)
 #warning "FORMATTING HAS BEEN DISABLED!!!!"
#endif




//Some of these belong to SPI_SD...

//ALWAYS_FORMAT formats the card each time the device is powered-up
// So don't use this if you want to keep any memos from a previous go-round
// (or there's any risk of the power being lost)
//#define ALWAYS_FORMAT TRUE


//PRINT_COMMAND prints every command sent to the SD-Card via
//spi_sd_sendCommand()
//This is fine for early-testing--getting the SD Card to work *at all*
// but probably *won't* work when the device is supposed to be recording
// samples... Rather, it'll probably crash due to backed-up interrupts
//#define PRINT_COMMAND TRUE

//PRINT_R1 prints every R1 response from the SD-Card via
//spi_sd_getR1response()
//R1 responses arrive after the vast-majority of commands sent to the card
//Probably not a good idea to leave this, similar to above.
//#define PRINT_R1         TRUE


//PRINT_CSD prints the CSD (Card-Structure Description, or som'n?)
//Basically whether the card is standard or high-capacity, etc. as well as
//some detail about its block-structure.
#define PRINT_CSD     TRUE



//Similar to PRINT_R1, this prints every byte of the SD-Card's responses to
//commands... Not a good idea to leave this.
//#define PRINT_REMAINING_RESPONSE  TRUE





//PRINT_USAGETABLE prints out the 'audioThing' "usageTable"
// the usageTable keeps track of saved-memos
#define PRINT_USAGETABLE TRUE //FALSE





//*** These two are mutually-exclusive:
//This just prints "Go!" after boot... 
// This occurs after the SD card is initialized and sampling begins
#define PRINT_GO        TRUE
//Rather than "Go!" this just prints a return after boot...
//#define PRINT_GO_RETURN TRUE





//PRINT_BOOT_LOOPandCHUNK prints the loop-number and chunk-number
//started-at on boot. (e.g. if there're memos stored immediately at the
//beginning of the card from a previous power-up, the first chunk-number
//used, upon boot this time, will be *after* those memos)
#define PRINT_BOOT_LOOPandCHUNK  TRUE






//These three are only valid if WRITING_SD is TRUE

//LC_FC_WBC_BW_DISABLED disables *both* of the following options
//#define LC_FC_WBC_BW_DISABLED TRUE


//PRINT_LCandFC prints the loop-count and full-count once per second
//These descriptions need to be verified:
// The loop-count is the number of calls to writingSD() that have occurred
//  since the last printout
// The full-count is the number of times the internal sample-buffer was
//  full (samples were lost)
// Not certain, but, it's something like, if fullCount is non-zero, then we
// need to decrease the load, such that the loopCount increases to handle
// the lost samples...
#define PRINT_LCandFC TRUE


//PRINT_BWandWBC prints:
// the number of 512Byte blocks-written
// and the write-busy-count (the amount of time spent waiting for the
//  SD-Card to finish writing a block)
#define PRINT_BWandWBC         TRUE







//PRINT_ECHOLOOPCOUNT_1SEC prints the number of times puartEchoUpdate() was
//called each second
//SEE PUART_ECHO, this should be basically useless, now.
//TODO: This needs to be revisited... puartEchoUpdate() is kinda hokey.
//#define PRINT_ECHOLOOPCOUNT_1SEC  TRUE


//PRINT_LOOPNUM prints the current loop number once per second
// (The loopNum is the number of times the SD-Card has filled and started
// again from the beginning)
//TODO: This is *within* puartEchoUpdate() which is stupid.
//       Requiring PUAR_ECHO to be TRUE, so its use is basically null.
//#define PRINT_LOOPNUM       TRUE


//PRINT_MEMOVALS
// prints several statistics each time a "memo" is noted:
//   memoPositionInUT
//   memoLoopNum
//   thisLoopNum (how're those going to differ? Maybe a long memo that
//                wraps around to a previous loop?)
//   thisChunkNum
#define PRINT_MEMOVALS   TRUE


//PRINT_MEMOSIZE
// prints the length of a memo (as increased by consecutive presses of the
// 'memo' button).
// By default, this includes:
//  the duration in minutes
//  the duration in chunks
//  how many *actual* chunks need to be marked
//   (e.g. the 'memo' button was pressed five minutes ago, and is now being
//    pressed again, requesting to go back 20 minutes, but some of that
//    time was already marked in the previous memo)
// TODO verify the above
#define PRINT_MEMOSIZE TRUE

//PRINT_MEMOMINS_ONLY
// overrides PRINT_MEMOSIZE's default, and *only* prints the duration of
// the memo, in minutes. (PRINT_MEMOSIZE must ALSO be TRUE)
   //86->132 (this + above enabled)
//   #define PRINT_MEMOMINS_ONLY TRUE



//PRINT_SWB prints out 'sWB' when spi_sd_startWritingBlocks() is called
// As I recall, this is called each time a write begins at a new location
// e.g. if the card reaches the end, and loops
// or if a chunk is encountered that has already been marked as a memo, so
// writing has to stop, then start at the end of that memo...
// plausibly, also, block-writing may stop whenever the memo-key is pressed
// in order to update the usage-table...
#define PRINT_SWB TRUE


//PRINT_FORMATTING prints "Formatting..." and "done." when the card gets
//formatted. If this is FALSE, it merely prints "F"
#define PRINT_FORMATTING TRUE



//PRINT_BAD_DATARESPONSE enables printout in THREE locations, if there's a
// "Bad Data" response from the SD-Card:
//   spi_sd_readU16()
//   sd_writeUsageTable()
//   sd_writeUpdate()
// NOTE: "BDR" messages on the OLD audioThing only seemed to occur when the
// SD-Card was bumped in its horrendous connector.
// A/O v50, with the mega328P and a new SD-Card, BDR seems to be happening
// REGULARLY. Surely this is a bug that needs to be fixing. But the point
// is, this message shouldn't come through very often (plausibly never)
// if things are working right...
#define PRINT_BAD_DATARESPONSE   TRUE

//THIS SHOULD PROBABLY BE TRUE...
//This should probably always be true, at least when I figure out how to
// handle it properly (might have to reset... since the SD card mighta lost
// power, and thus register-settings, etc...)
//TODO: This is implemented on only two of the three cases above.
//#define HALT_ON_BAD_DATARESPONSE  TRUE





//printAnywhere allows for the Nokia LCD to begin printing at any location
//on the screen, (normally it scrolls)
// This is for important messages like BDR, which may come through
// repeatedly... we want to see that it's updating, but don't need a log
//TODO: printAnywhere is Nokia-LCD-Only, yet there's code for RS-232?
//      It's hokey. And DISABLE_PRINTANYWHERE is not functional if the NLCD
//      is enabled, so I dunno.
//#define DISABLE_PRINTANYWHERE TRUE



//PRINT_SDCONNECTED
// prints "SD Connected!" on entry to spi_sd_initSizeSpecs()
// ...at which point we've already determined whether the card is Standard
// or High Capacity, etc. so it's definitely communicating.
#define PRINT_SDCONNECTED        TRUE

//PRINT_CANTSETBLOCKSIZE
// indicates when there's an error setting the block-size... this should
// never happen, as we always use 512B blocks, which is supposed to be
// supported by all devices (and is the *only* size supported by
// high-capacity cards)
#define PRINT_CANTSETBLOCKSIZE   TRUE


//PRINT_BADCSDVER
// indicates whether this SD-Card uses a CSD different than the ones
// supported by audioThing... Currently shouldn't be an issue... but who
// knows.
#define PRINT_BADCSDVER       TRUE


//PRINT_SIZE
// prints the size of the SD-Card in blocks and kB
#define PRINT_SIZE               TRUE


//PRINT_CHUNKADVANCE
// indicates when we've advanced to the next chunk
// In audioThing, SDCards are divided into 256 chunks, each of which can
// contain a memo...
//20B
#define PRINT_CHUNKADVANCE TRUE

//PRINT_CHUNKREENTRY
// indicates when a chunk has to be entered (including reentry) e.g.:
//   upon boot
//   upon encountering a used chunk ('memo') and having to stop and start
//     again after it
//   upon encountering the end of the card, and looping to the beginning
//   upon an update to the usage-table (when the 'memo' button is pressed)
#define PRINT_CHUNKREENTRY  TRUE


//PRINT_AVAILABLECHUNKS
// prints the number of unused-chunks whenever it changes
// (e.g. usually when 'memo' is pressed)
#define PRINT_AVAILABLECHUNKS  TRUE







//THIS IS REALLY HORRIBLY NAMED:
// less-so now, but still.
//puartEchoUpdate(), enabled here, handles keyboard input, especially from
//the HSSKB
// It does *NOT* Handle the Nokia Keypad.
//TODO: puartEchoUpdate() is quite similar to nkp_update
//
//I think part of the point of keeping these separate was to have multiple
//inputs available at the same time... e.g. for short notes, the device
//could be entirely self-contained and stand-alone with the built-in keypad
//but if I needed to take long notes, I could plug in the keyboard
//
//Realistically, puartEchoUpdate() should be removed and nkp_update should
//become a bit more generalized and maybe take-on some of puartEchoUpdate's
//functionality...
//puartEchoUpdate() has been moved to _old/ and should probably not be used
//#define PUART_ECHO TRUE


//This isn't very-well-named, either...
// NOT ONLY does it allow loading of keyboard-data to the audio-samples, 
// but it
// ALSO INDICATES that the usage-table needs to be updated on the SDCard
// WITHOUT THIS, there is NO MEMO-TAKING. No usage-table.
// (a/o v50: I really don't remember how this works)
// (a/o v61: kbSample_sendByte() and memoUpdate() take care of this...
//  see notes elsewhere)
//THIS MUST BE TRUE.
#define KB_TO_SAMPLE TRUE



//HSSKB_TRANSLATE goes along with PUART_ECHO
// There was a time that the raw scan-codes from the HandSpring keyboard
// could be used in some way. But there's no reason, realistically, not to
// use this to translate the scan-codes into actual letters.
// And this should probably be removed, since that keyboard died.
//#define HSSKB_TRANSLATE TRUE





//BUTTON_IN_SAMPLE works with anaComp, and should probably be removed
// as it's been long-since replaced by anaButtons
//#define BUTTON_IN_SAMPLE TRUE



//THESE ARE ROUGHLY MUTUALLY EXCLUSIVE:

//WRITING_SD should basically always be true, except for testing
// It actually writes samples to the SD card
#define WRITING_SD TRUE

//ADC_PASSTHROUGH merely takes the ADC value and feeds it straight to
//audio-out, if AUDIO_OUT_ENABLED is also true
//It DOES NOT WORK with WRITING_SD also TRUE
//#define ADC_PASSTHROUGH TRUE

//If *neither* ADC_PASSTHROUGH nor WRITING_SD is true, then we've got
//oldOldCirBufADCTesting() which is Old Old, and I don't really recall
//what it does.







//PRINTEVERYBYTE allegedly prints out every byte received from the SD-Card
// but only really does-so when read via spi_sd_readU16()
//#define PRINTEVERYBYTE   TRUE



//WRITETIMED replaces many SD transactions with "timed" transactions, which
//are slowed for reliability-testing
//Old Note:
// Synchronization problem writing at full speed...?
// adding nop seemed to fix it... but it's slower...
#define WRITETIMED      FALSE







//The number of minutes to mark each time the 'memo' button is pressed
//(I can't recall, but the first press of 'memo' might only go back *one*
// minute... TBD)
//Also, this is the *ideal* case, and is actually determined based on the
//size of the memory-card and the size of the usage-table... e.g. an 8GB
//card, as I recall, can't mark a chunk shorter than 13minutes... So,
//pressing the "memo" key once or twice will mark a 13 minute memo...
//Pressing it three times will mark a (15>13) : 13*2 = 26minute memo
//Yes, the limited usage-table needs improvement.
// On a 512MB SD-Card, we could get away with 51sec of "resolution"
#define MEMO_MINUTES 5



//USAGETABLE_SIZE is the size of the usage-table, which indicates which
//chunks have memos to be saved. As-Is, the usage-table is limited to 256
//chunks, and therefore 256 memos.
//For now, this shouldn't be changed.
#define USAGETABLE_SIZE (256)


//KBINDICATE_BLOCKS
// Whenever a keypress is detected, the device marks samples for quite some
// time thereafter indicating that somewhere in the recent past there was
// keyboard-input.
// This makes searching for text quite a bit faster:
//  e.g., an entire 8GB SD-Card full of saved-memos would otherwise have
//  to have every sample (4Billion) read. Instead, it can be searched by
//  checking one sample from every 2MB of samples, so only has to look
//  through a maximum of 8 thousand samples to find text.
//Lessee... currently running at 19230S/s, that's 38460B/s / 512 = 75Blk/s
//Lessay we indicate for 1min after each note, that's 4507 blocks (2,407KB)
// So we'd reduce our search from reading every byte to reading
// 2 bytes (one sample) in 2million... seems reasonable...
//THIS VALUE SHOULD NOT BE CHANGED, as audioThing[Desktop] has it
//hard-coded
#define KBINDICATE_BLOCKS  4507


//MEMO_TIMEOUT is the number of seconds to wait between presses of the
// 'memo' button before assuming that we're taking a new 'memo'
// e.g. if you press the memo-button five times, no further-apart than
// 10sec each, then the duration of the memo is increased by MEMO_MINUTES
// each time... 5memo-presses * 5minutes = a 25minute memo.
// If you then wait *more than* 10sec, it begins a new memo, starting
// again at 5 minutes
// (It might actually start at *one* minute, then increase by 5
// thereafter?)
#define MEMO_TIMEOUT (10*TCNTER_SEC) //(2500000)   //10sec





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
 * /home/meh/_avrProjects/audioThing/57-heart2/mainConfig.h
 *
 *    (Wow, that's a lot longer than I'd hoped).
 *
 *    Enjoy!
 */
