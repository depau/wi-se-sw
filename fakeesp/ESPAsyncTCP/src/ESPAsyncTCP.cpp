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
/*
Changes for July 2019

The operator "new ..." was changed to "new (std::nothrow) ...", which will
return NULL when the heap is out of memory. Without the change "soft WDT"
was the result, starting with Arduino ESP8266 Core 2.5.0. (Note, RE:"soft
WDT" - the error  reporting may improve with core 2.6.) With proir core
versions the library appears to work fine.
ref: https://github.com/esp8266/Arduino/issues/6269#issue-464978944

To support newer lwIP versions and buffer models. All references to 1460
were replaced with TCP_MSS. If TCP_MSS is not defined (exp. 1.4v lwIP)
1460 is assumed.

The ESPAsyncTCP library should build for Arduino ESP8266 Core releases:
2.3.0, 2.4.1, 2.4.2, 2.5.1, 2.5.2. It may still build with core versions
2.4.0 and 2.5.0. I did not do any regression testing with these, since
they had too many issues and were quickly superseded.

lwIP tcp_err() callback often resulted in crashes. The problem was a
tcp_err() would come in, while processing a send or receive in the
forground. The tcp_err() callback would be passed down to a client's
registered disconnect CB. A common problem with SyncClient and other
modules as well as some client code was: the freeing of ESPAsyncTCP
AsyncClient objects via disconnect CB handlers while the library was
waiting for an operstion to  finished. Attempts to access bad pointers
followed. For SyncClient this commonly occured during a call to delay().
On return to SyncClient _client was invalid. Also the problem described by
issue #94 also surfaced

Use of tcp_abort() required some very special handling and was very
challenging to make work without changing client API. ERR_ABRT can only be
used once on a return to lwIP for a given connection and since the
AsyncClient structure was sometimes deleted before returning to lwIP, the
state tracking became tricky. While ugly, a global variable for this
seemed to work; however, I  abanded it when I saw a possible
reentrancy/concurrency issue. After several approaches I settled the
problem by creating "class ACErrorTracker" to manage the issue.


Additional Async Client considerations:

The client sketch must always test if the connection is still up at loop()
entry and after the return of any function call, that may have done a
delay() or yield() or any ESPAsyncTCP library family call. For example,
the connection could be lost during a call to _client->write(...). Client
sketches that delete _client as part of their onDisconnect() handler must
be very careful as _client will become invalid after calls to delay(),
yield(), etc.


 */

#include "ESPAsyncTCP.h"
#include "async_config.h"
#include <lwip/err.h>
#include <Arduino.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>

extern "C" {
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
}

void setNonBlocking(int sock_fd) {
    int flags = fcntl(sock_fd, F_GETFL);
    if (flags < 0) {
        perror("Unable to get flags");
        panic();
    }
    if (fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("Unable to set non blocking");
        panic();
    }
}

/*
  Async Client Error Return Tracker
*/
// Assumption: callbacks are never called with err == ERR_ABRT; however,
// they may return ERR_ABRT.

ACErrorTracker::ACErrorTracker(AsyncClient *c) :
        _client(c), _close_error(ERR_OK), _errored(EE_OK) {}


void ACErrorTracker::setCloseError(err_t e) {
    if (e != ERR_OK)
        ASYNC_TCP_DEBUG("setCloseError() to: %s(%ld)\n", _client->errorToString(e), e);
    if (_errored == EE_OK)
        _close_error = e;
}

/**
 * Called mainly by callback routines, called when err is not ERR_OK.
 * This prevents the possiblity of aborting an already errored out
 * connection.
 */
void ACErrorTracker::setErrored(size_t errorEvent) {
    if (EE_OK == _errored)
        _errored = errorEvent;

}

/**
 * Used by callback functions only. Used for proper ERR_ABRT return value
 * reporting. ERR_ABRT is only reported/returned once; thereafter ERR_OK
 * is always returned.
 */
err_t ACErrorTracker::getCallbackCloseError(void) {
    if (EE_OK != _errored)
        return ERR_OK;
    if (ERR_ABRT == _close_error)
        setErrored(EE_ABORTED);
    return _close_error;
}

/*
  Async TCP Client
*/

void callAsyncClientDelayCallback(AsyncClient *obj) {
    obj->onDelayCB();
}

AsyncClient::AsyncClient(int sock_fd) :
        sock_fd{sock_fd}, _connect_cb(0), _connect_cb_arg(0), _discard_cb(0), _discard_cb_arg(0), _sent_cb(0),
        _sent_cb_arg(0),
        _error_cb(0), _error_cb_arg(0), _recv_cb(0), _recv_cb_arg(0), _pb_cb(0), _pb_cb_arg(0), _timeout_cb(0),
        _timeout_cb_arg(0), _poll_cb(0), _poll_cb_arg(0), _pcb_busy(false), _pcb_sent_at(0), _close_pcb(false),
        _ack_pcb(true), _tx_unacked_len(0), _tx_acked_len(0), _tx_unsent_len(0),
        _rx_ack_len(0), _rx_last_packet(0), _rx_since_timeout(0), _ack_timeout(ASYNC_MAX_ACK_TIME), _connect_port(0),
        _recv_pbuf_flags(0), _errorTracker(NULL), prev(NULL), next(NULL) {
    _errorTracker = std::make_shared<ACErrorTracker>(this);

    sockState = 4;
    delayCallbackId = registerOnDelayCallback(reinterpret_cast<void (*)(void *)>(callAsyncClientDelayCallback),
                                              (void *) this);
    if (delayCallbackId < 0) {
        fprintf(stderr, "Failed to bind on delay callback\n");
        panic();
    }

}

AsyncClient::~AsyncClient() {
    close(true);
    _errorTracker->clearClient();
}

inline void clearTcpCallbacks(tcp_pcb *pcb) {
}

bool AsyncClient::connect(IPAddress ip, uint16_t port) {
    fprintf(stderr, "STUB connect(IPAddress ip, uint16_t port)\n");
    return false;
}

bool AsyncClient::connect(const char *host, uint16_t port) {
    fprintf(stderr, "STUB connect(const char *host, uint16_t port)\n");
    return false;
}

AsyncClient &AsyncClient::operator=(const AsyncClient &other) {
    return *this;
}

bool AsyncClient::operator==(const AsyncClient &other) const {
    return sock_fd == other.sock_fd;
}

void AsyncClient::abort() {
    // Notes:
    // 1) _pcb is set to NULL, so we cannot call tcp_abort() more than once.
    // 2) setCloseError(ERR_ABRT) is only done here!
    // 3) Using this abort() function guarantees only one tcp_abort() call is
    //    made and only one CB returns with ERR_ABORT.
    // 4) After abort() is called from _close(), no callbacks with an err
    //    parameter will be called.  eg. _recv(), _error(), _connected().
    //    _close() will reset there CB handlers before calling.
    // 5) A callback to _error(), will set _pcb to NULL, thus avoiding the
    //    of a 2nd call to tcp_abort().
    // 6) Callbacks to _recv() or _connected() with err set, will result in _pcb
    //    set to NULL. Thus, preventing possible calls later to tcp_abort().
    close(true);
    if (sock_fd >= 0) {
        setCloseError(ERR_ABRT);
    }
}

void AsyncClient::close(bool now) {
    _close();
}

void AsyncClient::stop() {
    close(false);
}

bool AsyncClient::free() {
    return true;
}

size_t AsyncClient::write(const char *data) {
    if (data == nullptr)
        return 0;
    return this->write(data, strlen(data));
}

size_t AsyncClient::write(const char *data, size_t size, uint8_t apiflags) {
    size_t will_send = add(data, size, apiflags);

    if (!will_send || !send())
        return 0;
    return will_send;
}

size_t AsyncClient::add(const char *data, size_t size, uint8_t apiflags) {
    if (sock_fd < 0 || size == 0 || data == nullptr)
        return 0;

    size_t sent = ::send(sock_fd, data, size, 0);
    if (sent == -1) {
        if (errno == EWOULDBLOCK) {
            return 0;  // Send later
        } else {
            perror("Failed sending to socket");
            return 0;
        }
    }
    sentBytesForCallback += sent;
    return sent;
}

bool AsyncClient::send() {
    return true;
}

size_t AsyncClient::ack(size_t len) {
    return len;
}

// Private Callbacks

void AsyncClient::_connected(std::shared_ptr<ACErrorTracker> &errorTracker, void *pcb, err_t err) {
    return;
}

void AsyncClient::_close() {
    sockState = 0;
    if (sock_fd >= 0) {
        ::close(sock_fd);
        sock_fd = -1;
    }
    if (delayCallbackId >= 0) {
        deregisterOnDelayCallback(delayCallbackId);
        delayCallbackId = -1;
    }
}

void AsyncClient::_error(err_t err) {
    ASYNC_TCP_DEBUG("_error[%u]:%s err: %s(%ld)\n", getConnectionId(), ((NULL == _pcb) ? " NULL == _pcb!," : ""),
                    errorToString(err), err);
    close(true);
    if (_error_cb)
        _error_cb(_error_cb_arg, this, err);
    if (_discard_cb)
        _discard_cb(_discard_cb_arg, this);
}

void AsyncClient::_sent(std::shared_ptr<ACErrorTracker> &errorTracker, tcp_pcb *pcb, uint16_t len) {
    (void) pcb;
    _rx_last_packet = millis();
    errorTracker->setCloseError(ERR_OK);
    if (_sent_cb) {
        _sent_cb(_sent_cb_arg, this, len, millis());
        if (!errorTracker->hasClient())
            return;
    }
}

void AsyncClient::_recv(std::shared_ptr<ACErrorTracker> &errorTracker, tcp_pcb *pcb, pbuf *pb, err_t err) {
}

void AsyncClient::onDelayCB() {
    while (sentBytesForCallback > 0) {
        // That bitch of a library sends more stuff in the onAck handler, we need to back up the variable so we don't
        // zero it and lock up websockets
        uint64_t sentTemp = sentBytesForCallback;
        sentBytesForCallback = 0;
        _sent(_errorTracker, nullptr, sentTemp);
    }

    char buf[1000];
    ssize_t recvd = ::recv(sock_fd, buf, 1000, 0);
    if (recvd == -1) {
        switch (errno) {
            case EWOULDBLOCK:
                return;
            case ECONNRESET:
                sockState = 0;
                return _s_error(this, ERR_RST);
            case EBADF:
                sockState = 0;
                return _s_error(this, ERR_ABRT);
        }
        perror("Failed to read from socket");
        panic();
    } else if (recvd == 0) {
        // Socket closed
        sockState = 0;
        return _s_error(this, ERR_CLSD);
    }

    if (fakePollLastSentMillis + 100 < millis()) {
        fakePollLastSentMillis = millis();
        _poll(_errorTracker, nullptr);
    }

    if (_recv_cb) {
        _recv_pbuf_flags = 0;
        _recv_cb(_recv_cb_arg, this, buf, recvd);
    }
}

void AsyncClient::_poll(std::shared_ptr<ACErrorTracker> &errorTracker, tcp_pcb *pcb) {
    if (sockState == 4) {
        errorTracker->setCloseError(ERR_OK);
        _poll_cb(_poll_cb_arg, this);
    }
}

void AsyncClient::_dns_found(const ip_addr *ipaddr) {
    if (ipaddr) {
        connect(IPAddress(ipaddr->addr), _connect_port);
    } else {
        if (_error_cb)
            _error_cb(_error_cb_arg, this, -55);
        if (_discard_cb)
            _discard_cb(_discard_cb_arg, this);
    }
}


void AsyncClient::_s_dns_found(const char *name, const ip_addr *ipaddr, void *arg) {
    (void) name;
    reinterpret_cast<AsyncClient *>(arg)->_dns_found(ipaddr);
}

err_t AsyncClient::_s_poll(void *arg, struct tcp_pcb *tpcb) {
    return 0;
}

err_t AsyncClient::_s_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *pb, err_t err) {
    return 0;
}

void AsyncClient::_s_error(void *arg, err_t err) {
    auto *c = reinterpret_cast<AsyncClient *>(arg);
    auto errorTracker = c->getACErrorTracker();
    errorTracker->setCloseError(err);
    errorTracker->setErrored(EE_ERROR_CB);
    fprintf(stderr, "Socket error/event %d: %s; state: %d %s\n", err, c->errorToString(err), c->state(),
            c->stateToString());
    c->_error(err);
}

err_t AsyncClient::_s_sent(void *arg, struct tcp_pcb *tpcb, uint16_t len) {
    return 0;
}

err_t AsyncClient::_s_connected(void *arg, void *tpcb, err_t err) {
    return 0;
}

// Operators

AsyncClient &AsyncClient::operator+=(const AsyncClient &other) {
    if (next == NULL) {
        next = (AsyncClient *) (&other);
        next->prev = this;
    } else {
        AsyncClient *c = next;
        while (c->next != NULL) c = c->next;
        c->next = (AsyncClient *) (&other);
        c->next->prev = c;
    }
    return *this;
}

void AsyncClient::setRxTimeout(uint32_t timeout) {
    fprintf(stderr, "STUB setRxTimeout(%d)\n", timeout);
    _rx_since_timeout = timeout;
}

uint32_t AsyncClient::getRxTimeout() {
    fprintf(stderr, "STUB getRxTimeout()\n");
    return _rx_since_timeout;
}

uint32_t AsyncClient::getAckTimeout() {
    fprintf(stderr, "STUB getAckTimeout()\n");
    return _ack_timeout;
}

void AsyncClient::setAckTimeout(uint32_t timeout) {
    fprintf(stderr, "STUB setAckTimeout(%d)\n", timeout);
    _ack_timeout = timeout;
}

void AsyncClient::setNoDelay(bool nodelay) {
    fprintf(stderr, "STUB setNoDelay(%d)\n", nodelay);
}

bool AsyncClient::getNoDelay() {
    fprintf(stderr, "STUB getNoDelay()\n");
    return true;
}

uint16_t AsyncClient::getMss() {
    fprintf(stderr, "STUB getMss()\n");
    return 1460;
}

uint32_t AsyncClient::getRemoteAddress() {
    fprintf(stderr, "STUB getRemoteAddress()\n");
    return 0x01010101;
}

uint16_t AsyncClient::getRemotePort() {
    fprintf(stderr, "STUB getRemotePort()\n");
    return 42424;
}

uint32_t AsyncClient::getLocalAddress() {
    fprintf(stderr, "STUB getLocalAddress()\n");
    return 0x02020202;
}

uint16_t AsyncClient::getLocalPort() {
    fprintf(stderr, "STUB getLocalPort()\n");
    return 6969;
}

IPAddress AsyncClient::remoteIP() {
    fprintf(stderr, "STUB remoteIP()\n");
    return IPAddress(getRemoteAddress());
}

uint16_t AsyncClient::remotePort() {
    fprintf(stderr, "STUB remotePort()\n");
    return getRemotePort();
}

IPAddress AsyncClient::localIP() {
    fprintf(stderr, "STUB localIP()\n");
    return IPAddress(getLocalAddress());
}

uint16_t AsyncClient::localPort() {
    return getLocalPort();
}


uint8_t AsyncClient::state() {
    fprintf(stderr, "STUB state()\n");
    return sockState;
}

bool AsyncClient::connected() {
    fprintf(stderr, "STUB connected()\n");
    return sockState == 4;
}

bool AsyncClient::connecting() {
    fprintf(stderr, "STUB connecting()\n");
    return sockState > 0 && sockState < 4;
}

bool AsyncClient::disconnecting() {
    fprintf(stderr, "STUB disconnecting()\n");
    return sockState > 4 && sockState < 10;
}

bool AsyncClient::disconnected() {
    fprintf(stderr, "STUB disconnected()\n");
    return sockState == 0 || sockState == 10;
}

bool AsyncClient::freeable() {
    fprintf(stderr, "STUB freeable()\n");
    return sockState == 0 || sockState > 4;
}

bool AsyncClient::canSend() {
    return true;
}


// Callback Setters

void AsyncClient::onConnect(AcConnectHandler cb, void *arg) {
    _connect_cb = cb;
    _connect_cb_arg = arg;

    if (sock_fd >= 0) {
        if (_connect_cb)
            _connect_cb(_connect_cb_arg, this);
    }
}

void AsyncClient::onDisconnect(AcConnectHandler cb, void *arg) {
    _discard_cb = cb;
    _discard_cb_arg = arg;
}

void AsyncClient::onAck(AcAckHandler cb, void *arg) {
    _sent_cb = cb;
    _sent_cb_arg = arg;
}

void AsyncClient::onError(AcErrorHandler cb, void *arg) {
    _error_cb = cb;
    _error_cb_arg = arg;
}

void AsyncClient::onData(AcDataHandler cb, void *arg) {
    _recv_cb = cb;
    _recv_cb_arg = arg;
}

void AsyncClient::onPacket(AcPacketHandler cb, void *arg) {
    _pb_cb = cb;
    _pb_cb_arg = arg;
}

void AsyncClient::onTimeout(AcTimeoutHandler cb, void *arg) {
    _timeout_cb = cb;
    _timeout_cb_arg = arg;
}

void AsyncClient::onPoll(AcConnectHandler cb, void *arg) {
    _poll_cb = cb;
    _poll_cb_arg = arg;
}


size_t AsyncClient::space() {
    return 99999;
    return 0;
}

void AsyncClient::ackPacket(struct pbuf *pb) {
}


const char *AsyncClient::errorToString(err_t error) {
    switch (error) {
        case ERR_OK:
            return "No error, everything OK";
        case ERR_MEM:
            return "Out of memory error";
        case ERR_BUF:
            return "Buffer error";
        case ERR_TIMEOUT:
            return "Timeout";
        case ERR_RTE:
            return "Routing problem";
        case ERR_INPROGRESS:
            return "Operation in progress";
        case ERR_VAL:
            return "Illegal value";
        case ERR_WOULDBLOCK:
            return "Operation would block";
        case ERR_ABRT:
            return "Connection aborted";
        case ERR_RST:
            return "Connection reset";
        case ERR_CLSD:
            return "Connection closed";
        case ERR_CONN:
            return "Not connected";
        case ERR_ARG:
            return "Illegal argument";
        case ERR_USE:
            return "Address in use";
#if defined(LWIP_VERSION_MAJOR) && (LWIP_VERSION_MAJOR > 1)
            case ERR_ALREADY:    return "Already connectioning";
#endif
        case ERR_IF:
            return "Low-level netif error";
        case ERR_ISCONN:
            return "Connection already established";
        case -55:
            return "DNS failed";
        default:
            return "Unknown error";
    }
}

const char *AsyncClient::stateToString() {
    switch (state()) {
        case 0:
            return "Closed";
        case 1:
            return "Listen";
        case 2:
            return "SYN Sent";
        case 3:
            return "SYN Received";
        case 4:
            return "Established";
        case 5:
            return "FIN Wait 1";
        case 6:
            return "FIN Wait 2";
        case 7:
            return "Close Wait";
        case 8:
            return "Closing";
        case 9:
            return "Last ACK";
        case 10:
            return "Time Wait";
        default:
            return "UNKNOWN";
    }
}

/*
  Async TCP Server
*/
AsyncServer::AsyncServer(IPAddress addr, uint16_t port)
        : _port(port), _addr(addr), _connect_cb(nullptr), _connect_cb_arg(nullptr) {}

AsyncServer::AsyncServer(uint16_t port)
        : _port(port), _addr((uint32_t) IPADDR_ANY), _connect_cb(0), _connect_cb_arg(0) {

}

AsyncServer::~AsyncServer() {
    end();
}

void AsyncServer::onClient(AcConnectHandler cb, void *arg) {
    _connect_cb = cb;
    _connect_cb_arg = arg;
}

void AsyncServer::onDelayCB() {
    int client_fd = accept(sock_fd, NULL, NULL);
    if (client_fd == -1) {
        if (errno == EWOULDBLOCK) {
            return;
        }
        perror("Error accepting connection");
        panic();
    }
    setNonBlocking(client_fd);
    auto c = new AsyncClient(client_fd);
    c->onConnect([this](void *arg, AsyncClient *c) {
        _connect_cb(_connect_cb_arg, c);
    }, this);
}

void callAsyncServerDelayCallback(AsyncServer *obj) {
    obj->onDelayCB();
}

void AsyncServer::begin() {
    char buf[20];
    sprintf(buf, "%d.%d.%d.%d", _addr[0], _addr[1], _addr[2], _addr[3]);
    printf("%d.%d.%d.%d port %d\n", _addr[0], _addr[1], _addr[2], _addr[3], _port);

    serv_addr.sin_port = htons(_port);
    inet_aton(buf, &serv_addr.sin_addr);


    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Unable to open server socket");
        panic();
    }
    int option = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int)) == -1) {
        perror("Failed to set reuseaddr");
        panic();
    }
    setNonBlocking(sock_fd);

    if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Unable to bind server socket");
        panic();
    }
    listen(sock_fd, 10);
    char tempIpAddr[20];
    _addr.toString().toCharArray(tempIpAddr, 20, 0);
    fprintf(stderr, "Listening on %s port %d\n", tempIpAddr, _port);
    sockState = 1;
    delayCallbackId = registerOnDelayCallback(reinterpret_cast<void (*)(void *)>(callAsyncServerDelayCallback),
                                              (void *) this);
    if (delayCallbackId < 0) {
        fprintf(stderr, "Failed to bind on delay callback\n");
        panic();
    }
}


void AsyncServer::end() {
    if (delayCallbackId >= 0) {
        deregisterOnDelayCallback(delayCallbackId);
    }
    delayCallbackId = -1;
    if (sock_fd >= 0) {
        ::close(sock_fd);
    }
    sock_fd = -1;
    sockState = 0;
}

void AsyncServer::setNoDelay(bool nodelay) {
}

bool AsyncServer::getNoDelay() {
    return true;
}

uint8_t AsyncServer::status() {
    return sockState;
}

