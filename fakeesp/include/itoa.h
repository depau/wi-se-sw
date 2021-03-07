//
// Created by depau on 3/6/21.
//

#ifndef WI_SE_SW_ITOA_H
#define WI_SE_SW_ITOA_H

char* itoa (int val, char *s, int radix);

char* ltoa (long val, char *s, int radix);

char* utoa (unsigned int val, char *s, int radix);

char* ultoa (unsigned long val, char *s, int radix);

char* dtostrf (double val, signed char width, unsigned char prec, char *s);

//void reverse(char* begin, char* end);

#endif //WI_SE_SW_ITOA_H
