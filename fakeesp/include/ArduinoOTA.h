//
// Created by depau on 3/6/21.
//

#ifndef WI_SE_SW_ARDUINOOTA_H
#define WI_SE_SW_ARDUINOOTA_H

#include <functional>

typedef enum {
    OTA_AUTH_ERROR,
    OTA_BEGIN_ERROR,
    OTA_CONNECT_ERROR,
    OTA_RECEIVE_ERROR,
    OTA_END_ERROR
} ota_error_t;

class FakeArduinoOTA {
public:
    typedef std::function<void(void)> THandlerFunction;
    typedef std::function<void(ota_error_t)> THandlerFunction_Error;
    typedef std::function<void(unsigned int, unsigned int)> THandlerFunction_Progress;

    void begin() {};

    void handle() {};

    void setPassword(char *) {}

    void setPassword(const char *) {}

    void setRebootOnSuccess(bool) {}

    void setHostname(char *) {}

    void setHostname(const char *) {}

    void setPort(int) {}

    void onStart(THandlerFunction) {}

    void onEnd(THandlerFunction fn) {}

    void onError(THandlerFunction_Error fn) {}

    void onProgress(THandlerFunction_Progress fn) {}

};

extern FakeArduinoOTA ArduinoOTA;

#endif //WI_SE_SW_ARDUINOOTA_H
