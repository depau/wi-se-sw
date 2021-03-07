/**
 * @file Hash.cpp
 * @date 20.05.2015
 * @author Markus Sattler
 *
 * Copyright (c) 2015 Markus Sattler. All rights reserved.
 * This file is part of the esp8266 core for Arduino environment.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <openssl/sha.h>
#include <cstdio>

#include "WString.h"
#include "Hash.h"

/**
 * create a sha1 hash from data
 * @param data uint8_t *
 * @param size uint32_t
 * @param hash uint8_t[20]
 */
void sha1(const uint8_t *data, uint32_t size, uint8_t hash[20]) {
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, data, size);
    SHA1_Final(hash, &ctx);
}

void sha1(const char *data, uint32_t size, uint8_t hash[20]) {
    sha1((const uint8_t *) data, size, hash);
}

void sha1(const String &data, uint8_t hash[20]) {
    sha1(data.c_str(), data.length(), hash);
}

String sha1(const uint8_t *data, uint32_t size) {
    uint8_t hash[20];
    String hashStr((const char *) nullptr);
    hashStr.reserve(20 * 2 + 1);

    sha1(&data[0], size, &hash[0]);

    for (unsigned char i : hash) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", i);
        hashStr += hex;
    }

    return hashStr;
}

String sha1(const char *data, uint32_t size) {
    return sha1((const uint8_t *) data, size);
}

String sha1(const String &data) {
    return sha1(data.c_str(), data.length());
}

