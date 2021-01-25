import hashlib
import json
import os
from binascii import b2a_base64

import uasyncio as asyncio

from . import conf, html
from . import ufirewall
from . import uhttp
from . import uttyd
from . import uwebsocket


async def close_streams(*streams):
    for stream in streams:
        stream.close()
    for stream in streams:
        await stream.wait_closed()


class Server:
    def __init__(self):
        if getattr(conf, 'http_basic_auth'):
            self.token = b2a_base64(os.urandom(10))[:-1]
        else:
            self.token = b''

        self.tty = uttyd.TTY(self.token)
        self.websockets = []

    async def serve(self):
        await asyncio.start_server(self.on_http_connection, conf.http_listen, conf.http_port)

    async def on_http_connection(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        ip, port = reader.get_extra_info('peername')

        if not ufirewall.is_allowed(ip):
            print("IP not allowed: {}".format(ip))
            await close_streams(writer)
            return

        req = await uhttp.HTTPRequest.parse(reader)

        if getattr(conf, 'http_basic_auth'):
            if not req.check_basic_auth(conf.http_basic_auth):
                await uhttp.HTTPResponse.unauthorized(writer, realm="Wi-Se")
                await close_streams(writer)
                return

        if req.method in ('GET', 'POST') and req.path == "/stty":
            await self.handle_http_stty(req, reader, writer)
        elif req.method == "GET":
            if req.path == "/ws":
                if await self.websocket_handshake(req, writer):
                    await self.handle_uart_socket(reader, writer)
                    return  # Do not close stream!

            if req.path == "/token":
                await self.handle_http_token(req, writer)

            elif req.path == "/" or req.path == "/index.html":
                await self.handle_http_index(req, writer)

            else:
                await uhttp.HTTPResponse.not_found(writer)

        elif req.method == "POST":
            await uhttp.HTTPResponse.not_found(writer)

        else:
            await uhttp.HTTPResponse.bad_request(writer)

        await close_streams(writer)

    @staticmethod
    async def websocket_handshake(req: uhttp.HTTPRequest, writer: asyncio.StreamWriter) -> bool:
        if 'upgrade' not in req.headers.get('connection', '').lower() or \
                req.headers.get('upgrade', '').lower() != 'websocket' or \
                'sec-websocket-key' not in req.headers or \
                req.headers.get('sec-websocket-protocol', '') != 'tty':
            await uhttp.HTTPResponse.bad_request(writer)
            return False
        ws_accept = b2a_base64(
            hashlib.sha1(
                (req.headers['sec-websocket-key'].encode()) + b"258EAFA5-E914-47DA-95CA-C5AB0DC85B11")
                .digest()) \
            .decode().strip()
        await uhttp.HTTPResponse(
            status=101,
            headers={
                'Connection': 'upgrade',
                'Upgrade': 'websocket',
                'Sec-WebSocket-Protocol': 'tty',
                'Sec-WebSocket-Version': 13,
                'Sec-WebSocket-Accept': ws_accept,
            }
        ).write_into(writer)
        return True

    @staticmethod
    async def handle_http_index(req: uhttp.HTTPRequest, writer: asyncio.StreamWriter):
        if 'gzip' not in req.headers.get('accept-encoding', 'gzip'):
            await uhttp.HTTPResponse.not_acceptable(writer)
            return

        await uhttp.HTTPResponse(200, body=html.index_gz, headers={
            'Content-Type': 'text/html;charset=utf-8',
            'Content-Encoding': 'gzip'
        }).write_into(writer)

    async def handle_http_stty(self, req: uhttp.HTTPRequest, reader: asyncio.StreamReader,
                               writer: asyncio.StreamWriter):
        if req.method == 'POST':
            body = await reader.read(1000)
            try:
                j = json.loads(body)
                await self.tty.stty(**j)
            except Exception as exc:
                await uhttp.HTTPResponse(400, body=str(exc), headers={
                    'Content-Type': 'text/plain'
                }).write_into(writer)
                return

        await uhttp.HTTPResponse(
            200, body=json.dumps(self.tty.tty_conf), headers={'Content-Type': 'application/json;charset=utf-8'}) \
            .write_into(writer)

    async def handle_uart_socket(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        websocket = uwebsocket.WebSocket(reader, writer)
        self.tty.add_websocket(websocket)

    async def handle_http_token(self, req: uhttp.HTTPRequest, writer: asyncio.StreamWriter):
        await uhttp.HTTPResponse(body=b'{"token": "' + self.token + b'"}', headers={
            'Content-Type': 'application/json;charset=utf-8'
        }).write_into(writer)
