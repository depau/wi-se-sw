//
// Created by depau on 3/11/21.
//

#ifndef WI_SE_SW_AUTOBAUD_H
#define WI_SE_SW_AUTOBAUD_H

#include <HardwareSerial.h>
#include <cstdint>

int wise_autobaud_detect(HardwareSerial *serial);

int wise_autobaud_closest_std_rate(int32_t rawBaud);

#endif //WI_SE_SW_AUTOBAUD_H
