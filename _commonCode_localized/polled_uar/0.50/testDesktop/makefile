#/* mehPL:
# *    This is Open Source, but NOT GPL. I call it mehPL.
# *    I'm not too fond of long licenses at the top of the file.
# *    Please see the bottom.
# *    Enjoy!
# */
#
#
#Stolen from hfmTest3
# TARGET gets put in the _BUILD directory... not sure how I feel about that
TARGET = polled_uar_Test

MY_SRC = main.c



COMREL = ../../../..
COMDIR = $(COMREL)/_commonCode


# These are global variables in--and values specific to--main.c
# To override the AVR stuff...
#extern uint8_t puar_readInput(uint8_t puarNum)
#CFLAGS += -D'getbit(...)=(lineVal)'
CFLAGS += -D'TCNTER_SOURCE_VAR=(timerCounter)'
CFLAGS += -D'TCNTER_SOURCE_EXTERNED=TRUE'
CFLAGS += -D'TCNTER_SOURCE_OVERFLOW_VAL=(9)'
CFLAGS += -D'BIT_TCNT=6'
CFLAGS += -D'TCNTER_STUFF_IN_MAIN=TRUE'

# Override the default sample-divisor of 8
# (Since BIT_TCNT is so small, here (=6), 6/8=0, so we won't see the
# samples occurring mid-bit. Instead divide by 2, so samples will occur
# exactly half-way through a bit.
# (for most purposes, especially on AVR, BIT_TCNT/8 has been found to be
#  reasonable, so it samples somewhere *after* the edge, but not too close
#  to the other edge, due to delays in the main-loop, which might delay 
#  the puar_update() calls. That effect isn't seen here.)
CFLAGS += -D'SAMPLE_DIVISOR=2'

# Change this based on the test you wish to perform...
# Setting this true may help when using BIT_TCNTs that are close to 
# the rate that the update-loop runs... 
#CFLAGS += -D'SKIP_WAIT_TO_SAMPLE_START=TRUE'

CFLAGS += -D'NUMPUARS=2'

# This *should* ONLY enable debug-printing,
# But it seems to be doing more than that...
CFLAGS += -D'PU_PC_DEBUG=TRUE'

# This is nice because it doesn't require a version number...
# But, when compiling, it places the intermediate files (.d, .o)
#  outside the build directory (one level higher, due to "../")
# Smarter, maybe, would be to extract the version number from the current
#  working-directory, and use my normal method
#  of $(COMDIR)/polled_uar/$(VER_POLLED_UAR)/polled_uar
#POLLED_UAR_LIB = ../polled_uar

#NEW:
#This should be set to a specific value for your project. E.G. uncomment:
#VER_POLLED_UAR = 0.40

# It is only set differently here so we don't have to change this every
# time a new version is created... THIS WILL NOT WORK in a project.
# (Comment it out)
VER_POLLED_UAR = $(notdir $(patsubst %/,%,$(dir $(CURDIR))))

POLLED_UAR_LIB = $(COMDIR)/polled_uar/$(VER_POLLED_UAR)/polled_uar

include $(POLLED_UAR_LIB).mk



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
# * /home/meh/_avrProjects/audioThing/65-reverifyingUnderTestUser/_commonCode_localized/polled_uar/0.50/testDesktop/makefile
# *
# *    (Wow, that's a lot longer than I'd hoped).
# *
# *    Enjoy!
# */
