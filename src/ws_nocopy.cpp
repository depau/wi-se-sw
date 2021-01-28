//
// Created by depau on 1/28/21.
//

#include "ws_nocopy.h"

// This is basically a copy of AsyncWebSocketBasicMessage that does not allocate the buffer internally.
// It allows for a single buffer which UART can directly write into.
// It can only be used if there is one client only, since otherwise it will be freed.

AsyncWebSocketBasicMessageNoCopy::AsyncWebSocketBasicMessageNoCopy(const char *data, size_t len, uint8_t opcode,
                                                                   bool mask)
        : _len(len - 1), _sent(0), _ack(0), _acked(0), _data((uint8_t *) data) {
    _opcode = opcode & 0x07;
    _mask = mask;
    //_data = (uint8_t*)malloc(_len+1);
    if (_data == nullptr) {
        _len = 0;
        _status = WS_MSG_ERROR;
    } else {
        _status = WS_MSG_SENDING;
        //memcpy(_data, data, _len);
        _data[_len] = 0;
    }
}

AsyncWebSocketBasicMessageNoCopy::AsyncWebSocketBasicMessageNoCopy(uint8_t opcode, bool mask)
        : _len(0), _sent(0), _ack(0), _acked(0), _data(nullptr) {
    _opcode = opcode & 0x07;
    _mask = mask;

}

AsyncWebSocketBasicMessageNoCopy::~AsyncWebSocketBasicMessageNoCopy() {
    if (_data != nullptr)
        free(_data);
}

void AsyncWebSocketBasicMessageNoCopy::ack(size_t len, uint32_t time) {
    _acked += len;
    if (_sent == _len && _acked == _ack) {
        _status = WS_MSG_SENT;
    }
}

size_t AsyncWebSocketBasicMessageNoCopy::send(AsyncClient *client) {
    if (_status != WS_MSG_SENDING)
        return 0;
    if (_acked < _ack) {
        return 0;
    }
    if (_sent == _len) {
        if (_acked == _ack)
            _status = WS_MSG_SENT;
        return 0;
    }
    if (_sent > _len) {
        _status = WS_MSG_ERROR;
        return 0;
    }

    size_t toSend = _len - _sent;
    size_t window = webSocketSendFrameWindow(client);

    if (window < toSend) {
        toSend = window;
    }

    _sent += toSend;
    _ack += toSend + ((toSend < 126) ? 2 : 4) + (_mask * 4);

    bool final = (_sent == _len);
    auto *dPtr = (uint8_t *) (_data + (_sent - toSend));
    uint8_t opCode = (toSend && _sent == toSend) ? _opcode : (uint8_t) WS_CONTINUATION;

    size_t sent = webSocketSendFrame(client, final, opCode, _mask, dPtr, toSend);
    _status = WS_MSG_SENDING;
    if (toSend && sent != toSend) {
        _sent -= (toSend - sent);
        _ack -= (toSend - sent);
    }
    return sent;
}