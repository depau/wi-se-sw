import sys

import wi_se

_status_messages = {
    101: "Switching Protocols",
    200: "OK",
    400: "Bad Request",
    401: "Unauthorized",
    404: "Not Found",
    406: "Not Acceptable",
    500: "Internal Server Error",
}


class HTTPRequest:
    def __init__(self, method: str, path: str, httpv: str, query: dict, segment: str, headers: dict):
        self.method = method
        self.path = path
        self.httpv = httpv
        self.query = query
        self.segment = segment
        self.headers = headers

    @classmethod
    async def parse(cls, reader) -> 'HTTPRequest':
        method, path, httpv = (await reader.readline()).decode().split(' ')

        query, query_raw, segment = {}, None, None
        if '?' in path:
            path, query_raw = path.split("?", 1)
        if query_raw and '#' in query_raw:
            query_raw, segment = query_raw.split("#", 1)

        if query_raw:
            query = {key: val for key, val in
                     (item.split('=', 1) for item in query_raw.split("&"))}

        headers = {}
        lastline = None
        while True:
            line = (await reader.readline()).decode().strip()
            if line == lastline == '':
                # Body is coming
                break
            lastline = line

            if ":" not in line:
                # Not an header, skip
                continue

            name, value = line.split(":")
            name = name.strip().lower()
            value = value.strip()

            headers[name] = value

        return cls(method, path, httpv, query, segment, headers)


class HTTPResponse:
    def __init__(self, status: int = 200, http_version="HTTP/1.1", headers=None, status_mesg=None, body=None):
        self.status = status
        self.http_version = http_version
        self.headers = headers if headers else {}
        self.status_mesg = status_mesg
        self.body = body

        if 'Server' not in self.headers and 'server' not in self.headers:
            self.headers['Server'] = \
                'Wi-Se Snek v' + wi_se.VERSION + \
                ' {}/v{}'.format(sys.implementation.name, '.'.join(sys.implementation.version))
        if 'Connection' not in self.headers and 'connection' not in self.headers:
            self.headers['Connection'] = "close"

    async def write_into(self, writer):
        writer.write(
            "{} {}{}".format(self.http_version, self.status, (" " + self.status_mesg) if self.status_mesg else ''))
        await writer.drain()

        for key, val in self.headers.items():
            writer.write("{}: {}\r\n".format(key, val))
            await writer.drain()
        writer.write('\r\n')
        await writer.drain()

        if self.body:
            chunk_size = 256
            for i in range(0, len(self.body), chunk_size):
                writer.write(self.body[i:i + chunk_size] + b'\n')
                await writer.drain()

    @classmethod
    async def not_found(cls, writer):
        await cls(404).write_into(writer)

    @classmethod
    async def bad_request(cls, writer):
        await cls(400).write_into(writer)

    @classmethod
    async def unauthorized(cls, writer):
        await cls(401).write_into(writer)

    @classmethod
    async def not_acceptable(cls, writer):
        await cls(406).write_into(writer)
