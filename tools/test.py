#!/usr/bin/env python
from __future__ import print_function

import binascii
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException

try:
    dongle = getDongle(True)
except CommException as e:
    print(e)
    quit()


def send(cmd, params=[]):
    try:
        cmd_str = "80{0:02x}".format(cmd)
        for p in params:
            cmd_str = cmd_str + "{0:02x}".format(p)

        print(binascii.unhexlify(cmd_str))
        return dongle.exchange(binascii.unhexlify(cmd_str))

    except CommException as e:
        print("COMMEXC: ", e)
    except Exception as e:
        print("COMMEXC: ", e)


import subprocess
json = subprocess.check_output(['./samples', '0', 'text'])
buffer = []
for j in json:
    buffer.append(ord(j))
print(len(buffer))
send(1, buffer)
