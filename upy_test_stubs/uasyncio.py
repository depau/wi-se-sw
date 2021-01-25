import asyncio
import io
import sys
from asyncio import *


# Monkey patch real stream reader
def stub_get_extra_info(*a, **kw):
    print(f"STUB uasyncio.StreamReader.get_extra_info({a}, {kw})")
    return "192.168.99.99", 6969


asyncio.StreamReader.get_extra_info = stub_get_extra_info


# Define stub UART stream reader/writer
class Scream(asyncio.StreamReader, asyncio.StreamWriter):
    def __init__(self, *a, **kw):
        self.fread = io.open(sys.stdin.fileno(), 'rb', closefd=False, buffering=0)
        self.fwrite = sys.stdout.buffer

        self.reader = None
        self.writer = None

        pycharm_do_not_remove_the_motherfucking_import = get_event_loop()
        super(Scream, self).__init__()

    async def read(self, bytes: int):
        if not self.reader:
            self.reader = asyncio.StreamReader()
            protocol = asyncio.StreamReaderProtocol(self.reader)
            await get_event_loop().connect_read_pipe(lambda: protocol, self.fread)
        return await self.reader.read(bytes)

    def write(self, payload: bytes):
        if not self.writer:
            return self.fwrite.write(payload)
        self.writer.write(payload)

    async def drain(self):
        if not self.reader:
            return
        if not self.writer:
            w_transport, w_protocol = await get_event_loop() \
                .connect_write_pipe(streams.FlowControlMixin, self.fwrite)
            self.writer = asyncio.StreamWriter(w_transport, w_protocol, self.reader, get_event_loop())
        else:
            await self.writer.drain()


StreamReader = Scream
StreamWriter = Scream
