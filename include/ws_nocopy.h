//
// Created by depau on 1/28/21.
//

#ifndef WI_SE_SW_WS_NOCOPY_H
#define WI_SE_SW_WS_NOCOPY_H

#include "ESPAsyncWebServer.h"

// Unexported functions from AsyncWebSocket.cpp
size_t webSocketSendFrameWindow(AsyncClient *client);
size_t webSocketSendFrame(AsyncClient *client, bool final, uint8_t opcode, bool mask, uint8_t *data, size_t len);

class AsyncWebSocketBasicMessageNoCopy: public AsyncWebSocketMessage {
private:
    size_t _len;
    size_t _sent;
    size_t _ack;
    size_t _acked;
    uint8_t * _data;
public:
    AsyncWebSocketBasicMessageNoCopy(const char * data, size_t len, uint8_t opcode=WS_TEXT, bool mask=false);
    explicit AsyncWebSocketBasicMessageNoCopy(uint8_t opcode=WS_TEXT, bool mask=false);
    ~AsyncWebSocketBasicMessageNoCopy() override;
    bool betweenFrames() const override { return _acked == _ack; }
    void ack(size_t len, uint32_t time) override ;
    size_t send(AsyncClient *client) override ;
};

#endif //WI_SE_SW_WS_NOCOPY_H
