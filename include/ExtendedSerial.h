//
// Created by depau on 3/13/21.
//

#ifndef WI_SE_SW_EXTENDEDSERIAL_H
#define WI_SE_SW_EXTENDEDSERIAL_H


#include <HardwareSerial.h>

class ExtendedSerial : public HardwareSerial {

public:
    ExtendedSerial(int uart_nr) : HardwareSerial(uart_nr) {};

    // This is uart_detect_baudrate from esp8266-arduino, modified to return the measured value.
    // The closest standard rate can be obtained by calling ::autobaudGetClosestStdRate
    int autobaudMeasure();

    static int autobaudGetClosestStdRate(int32_t rawBaud);

    void sendBreak();
};


extern ExtendedSerial ExtSerial0;
extern ExtendedSerial ExtSerial1;


#endif //WI_SE_SW_EXTENDEDSERIAL_H
