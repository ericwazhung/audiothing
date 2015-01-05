#/* mehPL:
# *    This is Open Source, but NOT GPL. I call it mehPL.
# *    I'm not too fond of long licenses at the top of the file.
# *    Please see the bottom.
# *    Enjoy!
# */
#
#
# Sample makefile 
# original by Eric B. Weddington, et al. for WinAVR
# modified for _common "libraries" and other goodies by Eric Hungerford
#
# Maybe someday to be released to the Public Domain
# Please read the make user manual!
#
#
# On command line:
# make = Make software.
# make clean = Clean out built project files.
#
# To rebuild project do "make clean" then "make".
#


# APPARENTLY ORDER-WRITTEN MATTERS
# (old message from another chip...)
# HOWEVER: writing FUSEX does not seem to work... 
# (old message from another chip...)
# Keeps returning 0x01 which isn't even possible...
# Since the default value is 0xff
# However, all bits except bit0 are unused
# and the default value is fine.
# Likewise (a/o 328p):
# Writing 0xff results in write-failure, and a mismatch when verifying
# "was 0xff and is now 7"
# Which seems to suggest that even though the data-sheet shows unused bits
# as 1, they're actually 0...?
# Also, writing 0x07 doesn't cause a failure.

#These values are 328p defaults, EXCEPT using the RC oscillator at 8MHz
#  (with no division by 8, which is default)
#...UNLESS overridden in the project-makefile


ifndef FUSEX
# BODLEVEL2-0, 111 is default, the rest are unused
# See notes above...
FUSEX = 0x07
#0xff
endif

# I don't recall, exactly, what this is... As I recall, it has something 
# to do with OSCCAL not being calibrated
# It has been long-since determined that the default OSCCAL values are
# precise enough for most of my needs, so this probably isn't really
# necessary. Either way, this is what it is...
FUSE_NEWCHIP_WARN = TRUE

ifndef FUSEH
FUSEH = 0xd9
endif


ifndef FUSEL
FUSEL = 0xe2
endif
# Fuses:
# 1 = not-programmed (disabled)

# Fuse extended byte:
#   The mintduino device reads-back as 0x05
#   The 328p Default is 0xff
# 0xff = 1 1 1 1   1 1 1 1 <-- BODLEVEL0
#        \_unused__/ ^ ^------ BODLEVEL1
#                    +-------- BODLEVEL2
#
# Fuse high byte:
#   The mintduino reads-back as 0xda
#      =               1 0     Select Reset Vector 0x3C00
#                              Boot Size: 1024 words
#   The 328p Default is 0xd9   (Boot from 0x0000, all memory available)
# 0xd9 = 1 1 0 1   1 0 0 1 <-- BOOTRST   
#        ^ ^ ^ ^   ^ ^ ^------ BOOTSZ0
#        | | | |   | +-------- BOOTSZ1
#        | | | |   + --------- EESAVE
#        | | | +-------------- WDTON
#        | | +---------------- SPIEN
#        | +------------------ DWEN
#        +-------------------- RSTDISBL

# Fuse low byte:
#   The mintduino reads-back as 0xff
#                  1 1 1   = Low Power Crystal Oscillator 8-16MHz
#            1 1         1 = Crystal Oscillator, Slowly-Rising power
#
#   The 328p Default is 0x62   
#        0 1 1 0   0 0 1 0 = DEFAULT (8Mhz internal RC oscillator / 8)

#   Preferred when not using an oscillator (RC oscillator = 8MHz)
# 0xe2 = 1 1 1 0   0 0 1 0
#        ^ ^ \ /   \--+--/
#        | |  |       +------- CKSEL 3..0 (external clock NOT selected)
#        | |  +--------------- SUT 1..0 (max start-up time)
#        | +------------------ CKOUT (Don't output clock)
#        +-------------------- CKDIV8 (Don't divide)



# My programming-header and its muxed defaults:
# (Your pin-numbers may vary)
#
#             Pin Name         Default Use
# 1 GND
# 2 V+
# 3 SCK       PB5              Rx0 (polled_uar)
# 4 MOSI      PB3              Tx0 (polled_uat)
# 5 /RST
# 6 MISO      PB4              HEARTBEAT


#These can be overridden in your project's makefile...
# (And, if your project doesn't have puar/t or heartbeat, these don't
#  matter)

# E.G. LCDdirectLVDS (ATtiny861) cannot use the defaults
#  as the programming-MOSI == OC1A, which is in use, so cannot be used for
#   polled_uat's Tx0
#   Similar for the other pins.
#  So the project makefile should name these as appropriate
#  (A/O LCDdirectLVDS70, it has yet to be implemented in this way)


# HOWEVER: This is a reasonable idea for *early testing* regardless of the
# ultimate pinout
# So maybe rather than overriding the real names, define defaults here that
# will only be used if the real names aren't given...
# This is a new concept a/o audioThing v50

# These are makefile variables, NOT preprocessor variables
# The file _make/avrCommon.mk will convert them to e.g.:
# _PGM_MISO_PIN_NAME_ preprocessor definitions via #defines...

# Heartbeat
PGM_MISO_PIN_NAME = PB4
PGM_MISO_PORT_NAME = PORTB

# Tx0
PGM_MOSI_PIN_NAME = PB3
PGM_MOSI_PORT_NAME = PORTB

# Rx0
PGM_SCK_PIN_NAME = PB5
PGM_SCK_PORT_NAME = PORTB


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
# * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/_make/atmega328p.mk
# *
# *    (Wow, that's a lot longer than I'd hoped).
# *
# *    Enjoy!
# */
