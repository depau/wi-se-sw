//
// Created by depau on 3/6/21.
//

#include <stdio.h>

char *itoa(int val, char *s, int radix) {
    if (radix == 16) {
        sprintf(s, "%x", val);
    } else {
        sprintf(s, "%d", val);
    }
    return s;
}

char *ltoa(long val, char *s, int radix) {
    if (radix == 16) {
        sprintf(s, "%lx", val);
    } else {
        sprintf(s, "%ld", val);
    }
    return s;
}

char *utoa(unsigned int val, char *s, int radix) {
    if (radix == 16) {
        sprintf(s, "%x", val);
    } else {
        sprintf(s, "%d", val);
    }
    return s;
}

char *ultoa(unsigned long val, char *s, int radix) {
    if (radix == 16) {
        sprintf(s, "%lx", val);
    } else {
        sprintf(s, "%ld", val);
    }
    return s;
}

char *dtostrf(double val, signed char width, unsigned char prec, char *s) {
    sprintf(s, "%f", val);
    return s;
}


#include "itoa.h"
