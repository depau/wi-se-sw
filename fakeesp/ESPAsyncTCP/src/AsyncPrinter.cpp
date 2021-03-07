/*
  Asynchronous TCP library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include "AsyncPrinter.h"

AsyncPrinter::AsyncPrinter()
  : _client(nullptr)
  , _data_cb(nullptr)
  , _data_arg(nullptr)
  , _close_cb(nullptr)
  , _close_arg(nullptr)
  , _tx_buffer(nullptr)
  , _tx_buffer_size(TCP_MSS)
  , next(nullptr)
{}

AsyncPrinter::AsyncPrinter(AsyncClient *client, size_t txBufLen)
  : _client(client)
  , _data_cb(nullptr)
  , _data_arg(nullptr)
  , _close_cb(nullptr)
  , _close_arg(nullptr)
  , _tx_buffer(nullptr)
  , _tx_buffer_size(txBufLen)
  , next(nullptr)
{
  _attachCallbacks();
  _tx_buffer = new (std::nothrow) cbuf(_tx_buffer_size);
  if(_tx_buffer == nullptr) {
    panic(); //What should we do?
  }
}

AsyncPrinter::~AsyncPrinter(){
  _on_close();
}

void AsyncPrinter::onData(ApDataHandler cb, void *arg){
  _data_cb = cb;
  _data_arg = arg;
}

void AsyncPrinter::onClose(ApCloseHandler cb, void *arg){
  _close_cb = cb;
  _close_arg = arg;
}

int AsyncPrinter::connect(IPAddress ip, uint16_t port){
  if(_client != nullptr && connected())
    return 0;
  _client = new (std::nothrow) AsyncClient();
  if (_client == nullptr) {
    panic();
  }

  _client->onConnect([](void *obj, AsyncClient *c){ ((AsyncPrinter*)(obj))->_onConnect(c); }, this);
  if(_client->connect(ip, port)){
    while(_client && _client->state() < 4)
      delay(1);
    return connected();
  }
  return 0;
}

int AsyncPrinter::connect(const char *host, uint16_t port){
  if(_client != nullptr && connected())
    return 0;
  _client = new (std::nothrow) AsyncClient();
  if (_client == nullptr) {
    panic();
  }

  _client->onConnect([](void *obj, AsyncClient *c){ ((AsyncPrinter*)(obj))->_onConnect(c); }, this);
  if(_client->connect(host, port)){
    while(_client && _client->state() < 4)
      delay(1);
    return connected();
  }
  return 0;
}

void AsyncPrinter::_onConnect(AsyncClient *c){
  (void)c;
  if(_tx_buffer != nullptr){
    cbuf *b = _tx_buffer;
    _tx_buffer = nullptr;
    delete b;
  }
  _tx_buffer = new (std::nothrow) cbuf(_tx_buffer_size);
  if(_tx_buffer) {
    panic();
  }

  _attachCallbacks();
}

AsyncPrinter::operator bool(){ return connected(); }

AsyncPrinter & AsyncPrinter::operator=(const AsyncPrinter &other){
  if(_client != nullptr){
    _client->close(true);
    _client = nullptr;
  }
  _tx_buffer_size = other._tx_buffer_size;
  if(_tx_buffer != nullptr){
    cbuf *b = _tx_buffer;
    _tx_buffer = nullptr;
    delete b;
  }
  _tx_buffer = new (std::nothrow) cbuf(other._tx_buffer_size);
  if(_tx_buffer == nullptr) {
    panic();
  }

  _client = other._client;
  _attachCallbacks();
  return *this;
}

size_t AsyncPrinter::write(uint8_t data){
  return write(&data, 1);
}

size_t AsyncPrinter::write(const uint8_t *data, size_t len){
  if(_tx_buffer == nullptr || !connected())
    return 0;
  size_t toWrite = 0;
  size_t toSend = len;
  while(_tx_buffer->room() < toSend){
    toWrite = _tx_buffer->room();
    _tx_buffer->write((const char*)data, toWrite);
    while(connected() && !_client->canSend())
      delay(0);
    if(!connected())
      return 0; // or len - toSend;
    _sendBuffer();
    toSend -= toWrite;
  }
  _tx_buffer->write((const char*)(data+(len - toSend)), toSend);
  while(connected() && !_client->canSend()) delay(0);
  if(!connected()) return 0; // or len - toSend;
  _sendBuffer();
  return len;
}

bool AsyncPrinter::connected(){
  return (_client != nullptr && _client->connected());
}

void AsyncPrinter::close(){
  if(_client != nullptr)
    _client->close(true);
}

size_t AsyncPrinter::_sendBuffer(){
  size_t available = _tx_buffer->available();
  if(!connected() || !_client->canSend() || available == 0)
    return 0;
  size_t sendable = _client->space();
  if(sendable < available)
    available= sendable;
  char *out = new (std::nothrow) char[available];
  if (out == nullptr) {
    panic(); // Connection should be aborted instead
  }

  _tx_buffer->read(out, available);
  size_t sent = _client->write(out, available);
  delete out;
  return sent;
}

void AsyncPrinter::_onData(void *data, size_t len){
  if(_data_cb)
    _data_cb(_data_arg, this, (uint8_t*)data, len);
}

void AsyncPrinter::_on_close(){
  if(_client != nullptr){
    _client = nullptr;
  }
  if(_tx_buffer != nullptr){
    cbuf *b = _tx_buffer;
    _tx_buffer = nullptr;
    delete b;
  }
  if(_close_cb)
    _close_cb(_close_arg, this);
}

void AsyncPrinter::_attachCallbacks(){
  _client->onPoll([](void *obj, AsyncClient* c){ (void)c; ((AsyncPrinter*)(obj))->_sendBuffer(); }, this);
  _client->onAck([](void *obj, AsyncClient* c, size_t len, uint32_t time){  (void)c; (void)len; (void)time; ((AsyncPrinter*)(obj))->_sendBuffer(); }, this);
  _client->onDisconnect([](void *obj, AsyncClient* c){ ((AsyncPrinter*)(obj))->_on_close(); delete c; }, this);
  _client->onData([](void *obj, AsyncClient* c, void *data, size_t len){ (void)c; ((AsyncPrinter*)(obj))->_onData(data, len); }, this);
}
