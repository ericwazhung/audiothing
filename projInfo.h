//Auto-generated by avrCommon.mk

#ifndef __PROJINFO_H__
#define __PROJINFO_H__
#include <inttypes.h>

#if (defined(_PROJINFO_OVERRIDE_) && _PROJINFO_OVERRIDE_)
 const uint8_t __attribute__ ((progmem)) \
   header[] = "";
#elif (defined(PROJINFO_SHORT) && PROJINFO_SHORT)
 const uint8_t __attribute__ ((progmem)) \
   header[] = "audioThing57 2014-12-24 05:28:00";
#else //projInfo Not Shortened nor overridden
 const uint8_t __attribute__ ((progmem)) \
   header0[] = " /home/meh/_avrProjects/audioThing/57-heart2 ";
 const uint8_t __attribute__ ((progmem)) \
   header1[] = " Wed Dec 24 05:28:00 PST 2014 ";
 const uint8_t __attribute__ ((progmem)) \
   headerOpt[] = " WDT_DIS=TRUE ";
#endif

//For internal use...
//Currently only usable in main.c
#define PROJ_VER 57
#define COMPILE_YEAR 2014

#endif

