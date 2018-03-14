#!/usr/bin/env python
from __future__ import print_function

import binascii
from ledgerblue.comm import getDongle
from ledgerblue.commException import CommException


# Sends json message to ledger preceded with a simple header made of 4 bytes written in hex ABCD, where:
#   A is a fixed magic number 0x80
#   B is an instruction code, here 0x01 (json message)
#   C is an index of the sent chunk (json messages bigger than 250 characters are split into multiple chunks)
#   D is a total number of chunks
def send(dongle, cmd, params=[]):
    chunk_size = 250
    chunks = [params[x:x + chunk_size] for x in range(0, len(params), chunk_size)]

    chunk_index = 0
    for chunk in chunks:
        try:
            chunk_index = chunk_index + 1
            cmd_str = "80{0:02x}{1:02x}{2:02x}".format(cmd, chunk_index, len(chunks))
            for p in chunk:
                cmd_str = cmd_str + "{0:02x}".format(p)

            print("Sending message: " + cmd_str)
            dongle.exchange(binascii.unhexlify(cmd_str))

        except CommException as e:
            print("COMMEXC: ", e)
        except Exception as e:
            print("COMMEXC: ", e)


def get_test_message():
    import subprocess

    # Use samples tool to produce different type of messages
    # To create samples tool, run 'go build tools/samples.go'

    # json = subprocess.check_output(['./samples', '1', 'text'])

    # Alternatively use this for a simple json message
    #json = r"""{"_df":"3CAAA78D13BAE0","_v":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10}],"sequence":1}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":10}]}]}}"""
    # or this for more complicated one
    json = r"""{"_df":"3CAAA78D13BAE0","_v":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10},{"denom":"bitcoint","amount":20}],"sequence":1},{"address":"616E6F74686572696E707574","coins":[{"denom":"atom","amount":50},{"denom":"bitcoint","amount":60},{"denom":"ethereum","amount":70}],"sequence":1}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":10},{"denom":"bitcoint","amount":20}]},{"address":"616E6F746865726F7574707574","coins":[{"denom":"atom","amount":50},{"denom":"bitcoint","amount":60},{"denom":"ethereum","amount":70}]}]}}"""

    return json


def main():
    try:
        dongle = getDongle(True)

        json = get_test_message()

        buffer = []
        for j in json:
            buffer.append(ord(j))

        print("JSON Buffer Size: {}".format(len(buffer)))
        send(dongle, 1, buffer)

    except CommException as e:
        print(e)
        quit()


if __name__ == '__main__':
    main()
