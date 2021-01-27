import json

import machine
import uasyncio as asyncio

from . import conf
from . import uwebsocket

# client message
CMD_INPUT = b'0'
CMD_RESIZE_TERMINAL = b'1'
CMD_PAUSE = b'2'
CMD_RESUME = b'3'
CMD_JSON_DATA = b'{'

# server message
CMD_OUTPUT = b'0'
CMD_SET_WINDOW_TITLE = b'1'
CMD_SET_PREFERENCES = b'2'


class TTY:
    def __init__(self, token: bytes):
        self.token = token
        self.tty_conf = {
            'baudrate': conf.uart_baud,
            'bits': conf.uart_bits,
            'parity': conf.uart_parity,
            'stop': conf.uart_stop
        }

        self.uart = machine.UART(conf.uart_id)
        self.uart.init(**self.tty_conf)

        self.uart_reader = asyncio.StreamReader(self.uart, {})
        self.uart_writer = asyncio.StreamWriter(self.uart, {})

        self.websockets = []
        self.u2w_loop_alive = False
        self.ping_loop_alive = False

    @property
    def window_title(self):
        parity = self.tty_conf['parity']
        return "{hostname} UART{id} {baud} {bits}{parity}{stop}".format(
            id=conf.uart_id, baud=self.tty_conf['baudrate'], bits=self.tty_conf['bits'], hostname=conf.hostname,
            parity=parity is None and 'N' or parity is 1 and 'O' or parity is 0 and 'E', stop=self.tty_conf['stop']
        ).encode()

    async def u2w_loop(self):
        self.u2w_loop_alive = True
        while len(self.websockets) > 0:
            data = await self.uart_reader.read(8096)
            await self.broadcast(CMD_OUTPUT + data)
        self.u2w_loop_alive = False

    async def broadcast(self, payload: bytes):
        for ws in self.websockets:
            if ws.open:
                try:
                    await ws.send(payload)
                except uwebsocket.ConnectionClosed:
                    self.websockets.remove(ws)

    async def send_initial_message(self, ws: uwebsocket.WebSocket):
        await ws.send(CMD_SET_WINDOW_TITLE + self.window_title)
        await ws.send(CMD_SET_PREFERENCES + json.dumps(conf.ttyd_web_conf).encode())

    async def on_ws_authenticated(self, ws: uwebsocket.WebSocket):
        await self.send_initial_message(ws)
        # Start or add to sender coroutine
        self.websockets.append(ws)
        if not self.u2w_loop_alive:
            asyncio.create_task(self.u2w_loop())
        if not self.ping_loop_alive:
            asyncio.create_task(self.ping_loop())

    async def w2u_loop(self, ws: uwebsocket.WebSocket):
        authenticated = False
        if not self.token:
            authenticated = True
            await self.on_ws_authenticated(ws)

        while ws.open:
            payload = await ws.recv()

            if not authenticated and not payload.startswith(CMD_JSON_DATA):
                await ws.close(uwebsocket.CLOSE_POLICY_VIOLATION)
                break
            elif not authenticated and payload.startswith(CMD_JSON_DATA):
                # noinspection PyBroadException
                try:
                    j = json.loads(payload)
                except Exception:
                    await ws.close(uwebsocket.CLOSE_BAD_DATA)
                    break

                if j.get("AuthToken", None) != self.token.decode():
                    await ws.close(uwebsocket.CLOSE_POLICY_VIOLATION)
                    break
                else:
                    authenticated = True
                    await self.on_ws_authenticated(ws)

            assert authenticated

            if payload.startswith(CMD_INPUT):
                if type(payload) == str:
                    payload = payload.encode()
                self.uart_writer.write(payload[1:])
                await self.uart_writer.drain()

            # No other command is implemented due to technical limitations and limited resources

        if ws in self.websockets:
            self.websockets.remove(ws)

    def add_websocket(self, ws: uwebsocket.WebSocket):
        asyncio.create_task(self.w2u_loop(ws))

    async def ping_loop(self):
        self.ping_loop_alive = True
        while len(self.websockets) > 0:
            for ws in self.websockets:
                if ws.open:
                    await ws.write_frame(uwebsocket.OP_PING)
            await asyncio.sleep(conf.ws_ping_interval)
        self.ping_loop_alive = False

    async def stty(self, **kwargs):
        for key in kwargs:
            if key not in self.tty_conf:
                raise ValueError('Option "{}" is not supported'.format(key))
        tty_conf = self.tty_conf.copy()
        tty_conf.update(kwargs)

        tty_conf['baudrate'] = int(tty_conf['baudrate'])
        tty_conf['bits'] = int(tty_conf['bits'])
        tty_conf['stop'] = int(tty_conf['stop'])
        if tty_conf['parity'] not in (-1, 0, 1):
            raise ValueError('parity can only be -1 (none), 0 (even), 1 (odd)')
        # Make this consistent with the C++ implementation
        if tty_conf['parity'] == -1:
            tty_conf['parity'] = None

        self.tty_conf = tty_conf
        self.uart.init(**tty_conf)
        await self.broadcast(CMD_SET_WINDOW_TITLE + self.window_title)
