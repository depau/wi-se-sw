#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

if __name__ == "__main__":
    iters = -1
    if len(sys.argv) > 1:
        iters = int(sys.argv[1])

    try:
        nums = b'.'.join(map(lambda x: ("%02X" % x).encode() ,range(256)))

        while iters == -1 or iters > 0:
            sys.stdout.buffer.write(nums)
            if iters != -1:
                iters -= 1

    except KeyboardInterrupt:
        pass
