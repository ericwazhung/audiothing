#/* mehPL:
# *    This is Open Source, but NOT GPL. I call it mehPL.
# *    I'm not too fond of long licenses at the top of the file.
# *    Please see the bottom.
# *    Enjoy!
# */
#
#
TARGET = audioThing

#MCU = attiny861
MCU = atmega328p

FCPU = 16000000UL


## Default FUSE overrides ##
ifeq ($(MCU), attiny861)
#For attiny861 ONLY:
# Use the PLL for the system clock, at 16MHz
FUSEL = 0xe1

#Enable EESAVE
# This project uses the EEPROM for multiple purposes, but one important one
# is its being used to store the character-bitmap used for the LCD
# (due to program-memory limitations)
# So, don't erase that.
# (In one version of AVR-DUDE, reprogramming 256Bytes of eeprom took 1min!)
#FUSEH = 0xd7

# Use the short projInfo to save code-space... 
#CFLAGS += -D'PROJINFO_SHORT=TRUE'
#Clear it out completely...
CFLAGS += -D'_PROJINFO_OVERRIDE_'

else
ifeq ($(MCU), atmega328p)
#For atmega328p ONLY:
# Use the crystal-oscillator (16MHz)
FUSEL = 0xff

# EESAVE is not yet enabled for the atmega328p
# and may never be, as it's *plenty* of program-space for the font...
# And the new version of AVR-Dude is *much faster*

# Don't need to use the short projInfo to save code-space... 
#CFLAGS += -D'PROJINFO_SHORT=TRUE'
endif
endif



## Project-specific source-code ##
# GENERALLY, any of your '.c' files would be listed in MY_SRC.
# You can just ignore this next note...
# This project has several .c files, but they are *not* listed here,
#  and, in fact, *the C files* are #included in main, or elsewhere.
# There are various reasons for this; *some* of it, admittedly, is laziness
# But other reasons include: As I Understand: inline functions cannot be 
# *exclusively* inlined unless they exist in the same file as where they're
# referenced... OTHERWISE, they will be compiled *both* inline *and* as a
# separate *not inline* function, thus eating up code-space (especially if
# they're large).
MY_SRC = main.c



# If, for some reason, you need to see a listing of all the #defines
#  available, then uncomment this, then 'make clean' and 'make' and view
#  _BUILD/<TARGET>.o
#CFLAGS += -E -dM 



#TODO (you can just ignore this)
#Attempts at storing the font at the second half of the eeprom:
# This almost worked, see avrCommon.mk's linking stuff...
#CFLAGS += -Wl,--section-start=.eepFont=0x810100




# I've never used this:
# Override the default optimization level:
#OPT = 3


#########################
## SAVING CODESPACE    ##
##    and              ##
## PRINTF OF FLOATS    ##
#########################
#
# THE DEFAULT is NOT TO ALLOW printf() of floats
# Similarly,
# THE DEFAULT is TO INCLUDE stdio functionality, even if it's never used
#   (this is *quite large* unless minimized, and can get even larger)
# Look into _commonCode/_make/avrCommon.mk
#   for a few simple options to add here...




CFLAGS += -Wpacked-bitfield-compat


#OSCCAL_SET is seldom properly-implemented and can be ignored
# Basically it just reminds you to calibrate the RC oscillator whenever
# burning the fuses in a new chip.
# But, realistically, I've found that the default calibration value is
# plenty accurate enough for most of my needs
OSCCAL_SET = FALSE










# These are the paths to the main (shared) code...
# They might be overridden in the block below (if using Local)
# (Basically, if you're not me, then this WILL be overridden below)
COMREL = ../../..
COMDIR = $(COMREL)/_commonCode



################# SHOULD NOT CHANGE THIS BLOCK... FROM HERE ############## 
#                                                                        #
# This stuff has to be done early-on (e.g. before other makefiles are    #
#   included..                                                           #
#                                                                        #
#                                                                        #
# If this is defined, we can use 'make copyCommon'                       #
#   to copy all used commonCode to this subdirectory                     #
#   We can also use 'make LOCAL=TRUE ...' to build from that code,       #
#     rather than that in _commonCode                                    #
LOCAL_COM_DIR = _commonCode_localized
#                                                                        #
#                                                                        #
# If use_LocalCommonCode.mk exists and contains "LOCAL=1"                #
# then code will be compiled from the LOCAL_COM_DIR                      #
# This could be slightly more sophisticated, but I want it to be         #
#  recognizeable in the main directory...                                #
# ONLY ONE of these two files (or neither) will exist, unless fiddled with 
SHARED_MK = __use_Shared_CommonCode.mk
LOCAL_MK = __use_Local_CommonCode.mk
#                                                                        #
-include $(SHARED_MK)
-include $(LOCAL_MK)
#                                                                        #
#                                                                        #
#                                                                        #
#Location of the _common directory, relative to here...                  #
# this should NOT be an absolute path...                                 #
# COMREL is used for compiling common-code into _BUILD...                #
# These are overriden if we're using the local copy                      #
# OVERRIDE the main one...                                               #
ifeq ($(LOCAL), 1)
COMREL = ./
COMDIR = $(LOCAL_COM_DIR)
endif
#                                                                        #
################# TO HERE ################################################









###################################
### commonCode ('commonThings') ###
###################################
#
# Here's where the commonThings are included... (and configured)
# There's a specific method for doing-so (which could probably be improved)
# Basically we need the name, location, version, and to include the
# associated makefile snippet.
# e.g.:
#  VER_THING1=1.00
#  THING1_LIB=$(COMDIR)/thing1/$(VER_THING1)/thing1
#  include $(THING1_LIB).mk
#
# For commonThings in the *early stages* they may not yet have a
# makefile-snippet, so need to be handled differently. These are usually
# labelled with a version number ending in 'ncf'
# e.g.:
#  VER_THING2=0.20ncf
#  THING2_LIBDIR=$(COMDIR)/thing2/$(VER_THING2)
#  CFLAGS += -D'_THING2_HEADER_="$(THING2_LIBDIR)/thing2.h"'
#  COM_HEADERS += $(THING2_LIBDIR)
#
# Finally, commonThings which *only* have a header-file are handled
# similarly to "thing2", above.


#TODO: The 'ncf' method should be revised, such that the entire directory
#      is copied when doing 'make localize'




#### commonCode Options ####
#
# Most commonThings have various configuration options to, e.g.:
#  * Reduce code-size
#  * Set associated pins/ports/registers
#  * Configure timing-based values such as baud-rates, etc.
# Some even have options to choose to rely on another commonThing rather
#  than yet-another.
#
# There are two methods to pass these options to the commonThing:
# e.g.:
#  THING1_OPTION = <value>
# or:
#  CFLAGS += -D'THING2_OPTION=<value>'
#
# Unfortunately, they are not interchangeable, you'll have to look into the
# associated makefile-snippets to determine if the first option is possible
# otherwise look into the commonThing's c/h files.
#
# THESE OPTIONS MUST BE LISTED *BEFORE* the commonThing is included




### Many commonThings rely on many other commonThings ###
#
# e.g. 'heartbeat' relies on either 'dmsTimer' or 'tcnter' (selectable),
#                  both of which rely on 'timerCommon'
#                  (heartbeat also relies on 'hfModulation' and maybe more)
#
# INCLUDING 'heartbeat' HERE *automatically includes* its dependencies
#
# This topic can be delved-into further, but, briefly:
#  * It is possible to explicitly include them, anyhow.
#  * It is possible to override the dependencies' versions
#  * It is possible to configure options for a dependency that isn't
#    explicitly included here.
#
# BUT: to do these requires listing those options *before* the includer is
# included...
#
# e.g.
#   # Override the version of timerCommon included by heartbeat:
#   VER_TIMER_COMMON = 1.21
#   ...
#   include $(HEARTBEAT_LIB).mk
#
#
# ALSO A CONSIDERATION:
#  Various commonThings may rely on the same other commonThing
#  The relied-on commonThing will be included by *the first* of the various
#  commonThings listed... which can lead to version-issues.
# e.g.
#  heartbeat may include tcnter 0.50
#  polled_uat may include tcnter 0.40
# 
# If heartbeat is included first, tcnter will be 0.50
# But if polled_uat is included first, tcnter will be 0.40
# FOR THE MOST PART newer versions are backwards-compatible
# So either include heartbeat first, or explicitly list VER_TCNTER=0.50
# before either heartbeat or polled_uat are included.




# Override adcFreeRunning's default Interrupt Service Routine with one from
# main.c
CFLAGS += -D'ADC_ISR_EXTERNAL=TRUE'


# Probably best not to enable the Watch-Dog Timer while testing/debugging!
WDT_DISABLE = TRUE
# (Make this option visible when reading the FLASH memory)
PROJ_OPT_HDR += WDT_DIS=$(WDT_DISABLE)






VER_HSSKB=0.20ncf
HSSKB_LIBDIR=$(COMDIR)/hsStowawayKB/$(VER_HSSKB)
CFLAGS += -D'_HSSKB_HEADER_="$(HSSKB_LIBDIR)/hsStowawayKB.h"'
COM_HEADERS += $(HSSKB_LIBDIR)


VER_NLCD=0.20ncf
NLCD_LIBDIR=$(COMDIR)/nlcd/$(VER_NLCD)
CFLAGS += -D'_NLCD_HEADER_="$(NLCD_LIBDIR)/nlcd.h"'
COM_HEADERS += $(NLCD_LIBDIR)


VER_ADC_FREE_RUNNING=0.10ncf
ADC_FREE_RUNNING_LIBDIR=$(COMDIR)/adcFreeRunning/$(VER_ADC_FREE_RUNNING)
CFLAGS += \
-D'_ADC_FREE_RUNNING_HEADER_="$(ADC_FREE_RUNNING_LIBDIR)/adcFreeRunning.h"'
COM_HEADERS += $(ADC_FREE_RUNNING_LIBDIR)




#These are device-specific, but mightaswell include them both, here
# Then they'll both be in _commonCode-local
# (Because they're ncf, they won't take up program-space unless used)

# This is used by the atmega328p:
VER_USART_SPI=0.10ncf
USART_SPI_LIBDIR=$(COMDIR)/usart_spi/$(VER_USART_SPI)
CFLAGS += -D'_USART_SPI_HEADER_="$(USART_SPI_LIBDIR)/usart_spi.h"'
COM_HEADERS += $(USART_SPI_LIBDIR)

# This is used by the attiny861:
VER_USI_SPI=0.10ncf
USI_SPI_LIBDIR=$(COMDIR)/usi_spi/$(VER_USI_SPI)
CFLAGS += -D'_USI_SPI_HEADER_="$(USI_SPI_LIBDIR)/usi_spi.h"'
COM_HEADERS += $(USI_SPI_LIBDIR)

# This is also used by the attiny861:
VER_TINYPLL=0.10ncf
TINYPLL_LIBDIR=$(COMDIR)/tinyPLL/$(VER_TINYPLL)
CFLAGS += -D'_TINYPLL_HEADER_="$(TINYPLL_LIBDIR)/tinyPLL.c"'
COM_HEADERS += $(TINYPLL_LIBDIR)






# Override cirBuff's uint8_t data default
CFLAGS+=-D'cirBuff_data_t=uint16_t'
CFLAGS+=-D'cirBuff_dataRet_t=uint32_t'
CFLAGS+=-D'CIRBUFF_RETURN_NODATA=(UINT16_MAX+1)'
# Override cirBuff's uint8_t length/position
CFLAGS+=-D'cirBuff_position_t=uint8_t'
CFLAGS+=-D'CIRBUFF_NO_CALLOC=TRUE'
#Adding these made no change in size... odd...
# Because it's inline...?
CFLAGS += -D'CIRBUFF_EMPTY_UNUSED=TRUE'
CFLAGS += -D'CIRBUFF_AVAILABLESPACE_UNUSED=TRUE'

#8000-7964
CIRBUFF_INLINE = TRUE

VER_CIRBUFF=1.00
CIRBUFF_LIB=$(COMDIR)/cirBuff/$(VER_CIRBUFF)/cirBuff
include $(CIRBUFF_LIB).mk



# Polled UAR/T stuff:
# TCNTER options should be defined via heartbeat...
#CFLAGS += -D'TCNTER_SOURCE_VAR=(TCNT0L)'
#Should only be necessary for tcnts which are in a variable...
#CFLAGS += -D'TCNTER_SOURCE_EXTERNED=TRUE'
#Should this be 256 or 255?
# heartbeat is now using tcnter, so this should be handled. 
#CFLAGS += -D'TCNTER_SOURCE_OVERFLOW_VAL=(_DMS_OCR_VAL_)'
#CFLAGS += -D'_SPECIALHEADER_FOR_TCNTER_=_DMSTIMER_HEADER_'
# 1MHz (TCNTER) / 38400bps = 26.04
# HERE: 26 = 9600
#####
# A/O v50: This "magic number" appears to be the result of:
# 16MHz (the Tiny861's F_CPU)
# / 64 (CLKDIV64... where's that at?)
# = 249600 TCNTS/sec
# /9600bps = 26
# Now, why was I looking into this...? Oh yeah...
#  Seemingly-Unrelatedly:
#   spi_transferByteWithTimer() used the TCNT-increment-rate to determine
#   the spi bit-rate...
#   So, WithTimer() SPI transactions were running at ~250kbps (see below)
#Yes, we're still working with PUAR/T:
CFLAGS += -D'BIT_TCNT=26'
# Don't use pua/r to update/init tcnter, it will be done in main.c
CFLAGS += -D'TCNTER_STUFF_IN_MAIN=TRUE'
# Similarly, don't use heartbeat update/init for tcnter...
CFLAGS += -D'HEART_TCNTER_UPDATES_AND_INIT=FALSE'
#TCNTER_INLINE = TRUE
#TRUE

#Since we had to determine that magic-number above, might as well use it
# here, too...
# These are only relevent with the usart_spi in the atmega328p
# ALSO: They could probably be optimized a bit... plausibly speeding up the
# system
# Likewise, the ultimate math used may round things improperly... We'll see
# These override the extreme (default) values in usart_spi
CFLAGS += -D'USART_SPI_SLOW_BAUD_REG_VAL=(SPI_BRR_FROM_BAUD(F_CPU/64))'
# The fast baud-rate was determined purely by the speed of the USI...
# But, for the sake of initial-testing, let's try to match that.
# Actually, for some reason, it's got a NOP... why?
# StrobeClockAndShift() is inlined and two instructions, so at F_CPU=16MHz
# that'd've been 16/2 = 8Mbps
# Actually, can't beat that, even with the USART_SPI
# So the only benefit to the dedicated SPI controller is if it was
# interrupt-based, or somehow otherwise allowed processing during
# transmission... ToPonder. (Interrupts occuring only 16 instructions later
# don't make a whole lot of sense... takes roughly that to enter/return!)
CFLAGS += -D'USART_SPI_FAST_BAUD_REG_VAL=(SPI_BRR_FROM_BAUD(8000000))'




# These might save some space, but probably need to be *before*
# heartbeat...
#VER_TIMERCOMMON = 1.21

ifeq ($(MCU), attiny861)
## LOL: What was timerCommon being used for, then?
# I think *just* setWGM and selectDivisor
# Because, the tiny861's timers are a bit strange...?
CFLAGS += -D'TIMER_INIT_UNUSED=TRUE'
CFLAGS+=-D'TIMER_SETOUTPUTMODES_UNUSED=TRUE'
#CFLAGS+=-D'TIMER_SELECTDIISOR_UNUSED=TRUE'
#CFLAGS+=-D'TIMER_SETWGM_UNUSED=TRUE'
CFLAGS+=-D'TIMER_COMPAREMATCHINTSETUP_UNUSED=TRUE'
CFLAGS+=-D'TIMER_OVERFLOWINTENABLE_UNUSED=TRUE'
endif


#This should be up-to-date by including heartbeat.
#VER_HFMODULATION = 1.00
#HFMODULATION_LIB = $(COMDIR)/hfModulation/$(VER_HFMODULATION)/hfModulation
#include #(HFMODULATION_LIB).mk


VER_HEARTBEAT = 1.50
HEARTBEAT_LIB = $(COMDIR)/heartbeat/$(VER_HEARTBEAT)/heartbeat
# Don't include HEART source-code, for size tests...
# ALSO: the heartbeat can take a bit of time in heartUpdate(), which can
# interfere with things that need to happen quickly. (e.g. ADC values being
# written to the SD-Card).
# TODO: This needs to be explored and optimized...
HEART_REMOVED = TRUE


### DMS (the deci-millisecond (.0001 seconds) timer has been removed
# DMS requires a periodic interrupt, which interfered with the ADC, causing
# a high-frequency popping-sound.
# Instead, use tcnter
#CFLAGS += -D'_DMS_CLKDIV_=CLKDIV64'
DMS_EXTERNALUPDATE = FALSE
#HEART_DMS = TRUE
HEART_TCNTER = TRUE



#override heartBeat's preferred 4s choice...
CFLAGS += -D'_WDTO_USER_=WDTO_1S'
CFLAGS += -D'HEART_INPUTPOLLING_UNUSED=TRUE'
CFLAGS += -D'HEART_GETRATE_UNUSED=TRUE'
CFLAGS += -D'DMS_WAITFN_UNUSED=TRUE'
CFLAGS += -D'HEARTPIN_HARDCODED=TRUE'


# Heartbeat is generally on MISO:
#  since AVRs have high drive output, the LED shouldn't affect programming
# This can be overridden if necessary

ifeq ($(MCU), attiny861)
CFLAGS += -D'HEART_PINNUM=PB1'
CFLAGS += -D'HEART_PINPORT=PORTB'
# Generally there's no good reason to use LED_TIED_LOW
# And, in fact, quite a few *bad* reasons to.
# Wire it like this, and don't change LED_TIED_HIGH:
# PB1 >---+---|<|---/\/\/\-----> V+
#         |
#         V
#  optional momentary-pushbutton to ground
CFLAGS += -D'HEART_LEDCONNECTION=LED_TIED_HIGH'
endif
# The mega328p has the above pre-defined in its associated .mk file...
# Can also *see* where it's at in pinoutTrinketPro.mk



include $(HEARTBEAT_LIB).mk



# Anabuttons:
# Anabuttons includes TCNTER if it isn't already
# But since heartbeat uses a newer version, this needs to be after that.
CFLAGS += -D'ANABUTTONS_DIGITALIO=TRUE'

#See "BUTTON_PIN" in the appropriate pinout.h file
ifeq ($(MCU), attiny861)
CFLAGS += -D'ANABUTTONS_PIN=PA6'
CFLAGS += -D'ANABUTTONS_PORT=PORTA'
else
ifeq ($(MCU), atmega328p)
CFLAGS += -D'ANABUTTONS_PIN=PD6'
CFLAGS += -D'ANABUTTONS_PORT=PORTD'
endif
endif

VER_ANABUTTONS = 0.50
ANABUTTONS_LIB = $(COMDIR)/anaButtons/$(VER_ANABUTTONS)/anaButtons
include $(ANABUTTONS_LIB).mk


#Because TCNTER in heartbeat is newer than that in POLLED_UAR/T,
# heartbeat should be listed first

PUAT_INLINE = TRUE
PUAR_INLINE = TRUE

# For PUAR: 
CFLAGS += -D'SKIP_WAIT_TO_SAMPLE_START=TRUE'

CFLAGS += -D'NUMPUATS=1'
CFLAGS += -D'NUMPUARS=1'



VER_POLLED_UAR = 0.50
POLLED_UAR_LIB = $(COMDIR)/polled_uar/$(VER_POLLED_UAR)/polled_uar
include $(POLLED_UAR_LIB).mk


CFLAGS += -D'PUAT_SENDSTRINGBLOCKING_ENABLED=TRUE'

VER_POLLED_UAT = 0.70
POLLED_UAT_LIB = $(COMDIR)/polled_uat/$(VER_POLLED_UAT)/polled_uat
include $(POLLED_UAT_LIB).mk





# COM_HEADERS... 
#   These are commonThings that don't have their own makefile snippets
#   Generally, just header files full of #defines
#   But also commonThings that are in their early-stages
#     (usually versioned e.g. 0.10ncf 'ncf' = "Not CommonFiled")
# COM_HEADERS are necessary, mostly, for 'make localize'...
# This particular method also allows for 
#   e.g. '#include _BITHANDLING_HEADER_'

VER_BITHANDLING = 0.95
BITHANDLING_HDR = $(COMDIR)/bithandling/$(VER_BITHANDLING)/
COM_HEADERS += $(BITHANDLING_HDR)
CFLAGS += -D'_BITHANDLING_HEADER_="$(BITHANDLING_HDR)/bithandling.h"'


VER_ERRORHANDLING = 0.99
ERRORHANDLING_HDR = $(COMDIR)/errorHandling/$(VER_ERRORHANDLING)/
COM_HEADERS += $(ERRORHANDLING_HDR)
#No "_ERRORHANDLING_HEADER_" ?
# Where is that used, timerCommon? Shouldn't that be changed? TODO


include $(COMDIR)/_make/reallyCommon2.mk


#/* mehPL:
# *    I would love to believe in a world where licensing shouldn't be
# *    necessary; where people would respect others' work and wishes, 
# *    and give credit where it's due. 
# *    A world where those who find people's work useful would at least 
# *    send positive vibes--if not an email.
# *    A world where we wouldn't have to think about the potential
# *    legal-loopholes that others may take advantage of.
# *
# *    Until that world exists:
# *
# *    This software and associated hardware design is free to use,
# *    modify, and even redistribute, etc. with only a few exceptions
# *    I've thought-up as-yet (this list may be appended-to, hopefully it
# *    doesn't have to be):
# * 
# *    1) Please do not change/remove this licensing info.
# *    2) Please do not change/remove others' credit/licensing/copyright 
# *         info, where noted. 
# *    3) If you find yourself profiting from my work, please send me a
# *         beer, a trinket, or cash is always handy as well.
# *         (Please be considerate. E.G. if you've reposted my work on a
# *          revenue-making (ad-based) website, please think of the
# *          years and years of hard work that went into this!)
# *    4) If you *intend* to profit from my work, you must get my
# *         permission, first. 
# *    5) No permission is given for my work to be used in Military, NSA,
# *         or other creepy-ass purposes. No exceptions. And if there's 
# *         any question in your mind as to whether your project qualifies
# *         under this category, you must get my explicit permission.
# *
# *    The open-sourced project this originated from is ~98% the work of
# *    the original author, except where otherwise noted.
# *    That includes the "commonCode" and makefiles.
# *    Thanks, of course, should be given to those who worked on the tools
# *    I've used: avr-dude, avr-gcc, gnu-make, vim, usb-tiny, and 
# *    I'm certain many others. 
# *    And, as well, to the countless coders who've taken time to post
# *    solutions to issues I couldn't solve, all over the internets.
# *
# *
# *    I'd love to hear of how this is being used, suggestions for
# *    improvements, etc!
# *         
# *    The creator of the original code and original hardware can be
# *    contacted at:
# *
# *        EricWazHung At Gmail Dotcom
# *
# *    This code's origin (and latest versions) can be found at:
# *
# *        https://code.google.com/u/ericwazhung/
# *
# *    The site associated with the original open-sourced project is at:
# *
# *        https://sites.google.com/site/geekattempts/
# *
# *    If any of that ever changes, I will be sure to note it here, 
# *    and add a link at the pages above.
# *
# * This license added to the original file located at:
# * /home/meh/_avrProjects/audioThing/55-git/makefile
# *
# *    (Wow, that's a lot longer than I'd hoped).
# *
# *    Enjoy!
# */
