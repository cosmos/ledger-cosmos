/** ******************************************************************************
 *  (c) 2018 - 2026 Zondax AG
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************* */

import Zemu from '@zondax/zemu'
import { AMINO_JSON_TX, defaultOptions, DEVICE_MODELS } from './common'

jest.setTimeout(180000)

const CLA = 0x55

const INS_GET_VERSION = 0x00
const INS_SIGN_SECP256K1 = 0x02
const INS_GET_ADDR_SECP256K1 = 0x04

const P1_INIT = 0x00
const P1_ADD = 0x01
const P1_LAST = 0x02
const P1_BAD = 0x03

const SW_OK = 0x9000
const SW_WRONG_LENGTH = 0x6700
const SW_DATA_INVALID = 0x6984
const SW_COMMAND_NOT_ALLOWED = 0x6986
const SW_INVALID_P1P2 = 0x6b00
const SW_INS_NOT_SUPPORTED = 0x6d00
const SW_CLA_NOT_SUPPORTED = 0x6e00

// Accept every status word we assert on, so a wrong-but-known code surfaces as
// a readable expectation failure instead of a TransportStatusError.
const ACCEPTED_STATUS = [
  SW_OK,
  SW_WRONG_LENGTH,
  SW_DATA_INVALID,
  SW_COMMAND_NOT_ALLOWED,
  SW_INVALID_P1P2,
  SW_INS_NOT_SUPPORTED,
  SW_CLA_NOT_SUPPORTED,
]

// m/44'/118'/0'/0/0 as five little-endian uint32 words, the layout extractHDPath
// memcpy's straight into hdPath.
const HDPATH = Buffer.from('2c00008076000080000000800000000000000000', 'hex')

// Arbitrary SignDoc fragment. This suite never completes a transaction, so the
// payload only has to be non-empty.
const CHUNK = Buffer.from('7b226163636f756e745f6e756d626572223a223022', 'hex')

const GET_ADDR_ARGS = Buffer.concat([Buffer.from([6]), Buffer.from('cosmos'), HDPATH])

// Zemu wraps the transport in a proxy that raises TransportError for any status
// word other than 0x9000, regardless of the accepted-status list handed to the
// underlying transport. This suite asserts on rejections, so unwrap it back into
// a plain status word.
async function sw(transport: any, cla: number, ins: number, p1: number, p2: number, data: Buffer): Promise<number> {
  try {
    const resp = await transport.send(cla, ins, p1, p2, data, ACCEPTED_STATUS)
    return resp.readUInt16BE(resp.length - 2)
  } catch (e: any) {
    if (typeof e?.statusCode === 'number') {
      return e.statusCode
    }
    throw e
  }
}

const hex = (v: number) => `0x${v.toString(16)}`

describe('APDU state machine', function () {
  // One container per device: every assertion below is a pure APDU exchange, and
  // the sequence is ordered so each step leaves the state the next one needs.
  // Spinning up a simulator per assertion would mean 45 containers and starves
  // the runner into start-up timeouts.
  test.concurrent.each(DEVICE_MODELS)('chunk sequencing is enforced', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const t = sim.getTransport()
      const check = (actual: number, expected: number, what: string) =>
        expect(`${what} -> ${hex(actual)}`).toEqual(`${what} -> ${hex(expected)}`)

      // --- device idle: nothing may be appended to a flow that never started ---
      check(await sw(t, CLA, INS_SIGN_SECP256K1, P1_ADD, AMINO_JSON_TX, CHUNK), SW_COMMAND_NOT_ALLOWED, 'add without init')
      check(
        await sw(t, CLA, INS_SIGN_SECP256K1, P1_LAST, AMINO_JSON_TX, CHUNK),
        SW_COMMAND_NOT_ALLOWED,
        'last without init',
      )

      // --- dispatcher rejects malformed requests ---
      check(await sw(t, CLA, INS_SIGN_SECP256K1, P1_BAD, AMINO_JSON_TX, CHUNK), SW_INVALID_P1P2, 'unknown payload type')
      check(await sw(t, CLA, 0x99, 0x00, 0x00, CHUNK), SW_INS_NOT_SUPPORTED, 'unknown instruction')
      check(await sw(t, 0x44, INS_GET_VERSION, 0x00, 0x00, CHUNK), SW_CLA_NOT_SUPPORTED, 'unknown class')

      // One payload byte: past the OFFSET_DATA guard, short of the five uint32
      // words extractHDPath needs.
      check(
        await sw(t, CLA, INS_SIGN_SECP256K1, P1_INIT, AMINO_JSON_TX, Buffer.from([0x00])),
        SW_WRONG_LENGTH,
        'truncated hd path',
      )
      // No payload at all: rx stops exactly at OFFSET_DATA.
      check(
        await sw(t, CLA, INS_SIGN_SECP256K1, P1_INIT, AMINO_JSON_TX, Buffer.alloc(0)),
        SW_DATA_INVALID,
        'empty init payload',
      )

      // --- a flow is now in progress and owns the device ---
      check(await sw(t, CLA, INS_SIGN_SECP256K1, P1_INIT, AMINO_JSON_TX, HDPATH), SW_OK, 'init')
      check(
        await sw(t, CLA, INS_SIGN_SECP256K1, P1_INIT, AMINO_JSON_TX, HDPATH),
        SW_COMMAND_NOT_ALLOWED,
        'second init cannot restart a flow in progress',
      )
      check(
        await sw(t, CLA, INS_GET_ADDR_SECP256K1, 0x00, 0x00, GET_ADDR_ARGS),
        SW_COMMAND_NOT_ALLOWED,
        'get address while receiving',
      )
      check(await sw(t, CLA, INS_SIGN_SECP256K1, P1_ADD, AMINO_JSON_TX, CHUNK), SW_OK, 'add after init')

      // --- an unrelated error must abort a half-received transaction ---
      // The buffered chunks were accepted under the HD path and HRP that INIT
      // installed, so an aborted flow must not be resumable against new ones.
      // handleApdu keeps resetting TX_STATE_RECEIVING on error and only holds
      // the state while a review is on screen; this pins that split.
      check(await sw(t, CLA, INS_SIGN_SECP256K1, P1_BAD, AMINO_JSON_TX, CHUNK), SW_INVALID_P1P2, 'error during receive')
      check(
        await sw(t, CLA, INS_SIGN_SECP256K1, P1_ADD, AMINO_JSON_TX, CHUNK),
        SW_COMMAND_NOT_ALLOWED,
        'add after an aborted flow',
      )
    } finally {
      await sim.close()
    }
  })
})
