//
// Created by depau on 3/7/21.
//

#ifndef WI_SE_SW_PROGMEM_H
#define WI_SE_SW_PROGMEM_H

// Fake progmem
#define PROGMEM
#define PGM_P const char *
#define __FlashStringHelper char *
#define memcpy_P memcpy
#define memmove_P memmove
#define strlen_P strlen
#define strcpy_P strcpy
#define vsnprintf_P vsnprintf
#define pgm_read_byte(arg) (*(arg))

#endif //WI_SE_SW_PROGMEM_H
