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
    chunk_size = 250
    chunks = [params[x:x+chunk_size] for x in xrange(0, len(params), chunk_size)]

    chunk_index = 0
    for chunk in chunks:
        try:
            chunk_index = chunk_index + 1
            cmd_str = "80{0:02x}{1:02x}{2:02x}".format(cmd, chunk_index, len(chunks))
            for p in chunk:
                cmd_str = cmd_str + "{0:02x}".format(p)

            print("Sending message: " + binascii.unhexlify(cmd_str))
            dongle.exchange(binascii.unhexlify(cmd_str))

        except CommException as e:
            print("COMMEXC: ", e)
        except Exception as e:
            print("COMMEXC: ", e)


import subprocess
json = subprocess.check_output(['./samples', '1', 'text'])
buffer = []
for j in json:
    buffer.append(ord(j))
print(len(buffer))
send(1, buffer)
