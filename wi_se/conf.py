import sys

# Monkey patch built-ins to run in CPython for testing
if sys.implementation.name != 'micropython':
    import builtins
    builtins.const = lambda x: x

from binascii import b2a_base64
from wi_se_conf import *

try:
    http_basic_auth = "Basic " + b2a_base64(http_basic_auth.encode()).decode().strip()
except NameError:
    pass
