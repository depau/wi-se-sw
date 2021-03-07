//
// Created by depau on 3/7/21.
//

#include "md5.h"

#include <openssl/md5.h>


void MD5Init(md5_context_t *c) {
    MD5_Init(c);
}

void MD5Update(md5_context_t *c, const uint8_t *data, const uint16_t len) {
    MD5_Update(c, data, len);
}

void MD5Final(uint8_t md[16], md5_context_t *c) {
    MD5_Final(md, c);
}