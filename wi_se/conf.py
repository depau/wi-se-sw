try:
    from binascii import b2a_base64
except ImportError:
    from ubinascii import b2a_base64

from wi_se_conf import *

if vars().get('http_basic_auth', None):
    http_basic_auth = "Basic " + b2a_base64(http_basic_auth.encode()).decode().strip()
