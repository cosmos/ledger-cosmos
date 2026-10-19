/** ******************************************************************************
 *  (c) 2018 - 2024 Zondax AG
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

import Zemu, { isTouchDevice } from '@zondax/zemu'
// @ts-ignore
import CosmosApp from '@zondax/ledger-cosmos-js'
import { defaultOptions, DEVICE_MODELS, AMINO_JSON_TX } from './common'

// @ts-ignore
import secp256k1 from 'secp256k1/elliptic'
// @ts-ignore
import crypto from 'crypto'

jest.setTimeout(300000)

const PATH = "m/44'/118'/0'/0/0"
const HRP = 'cosmos'

// The review paginates a value by counting characters, while the screen fills a
// fixed number of lines. Those two only agree while no glyph is wider than the
// per-character width the budget assumed. A value built out of wide glyphs is
// what pulls them apart, so that is what this signs: 'W' and 'M' are the widest
// characters the review font draws, at 31px and 29px against a 22px assumption.
const WIDE_PADDING = 'WaMb'.repeat(30)

const WASM_TX_WITH_WIDE_VALUE = {
  account_number: '0',
  chain_id: 'cosmoshub-4',
  fee: { amount: [{ amount: '5', denom: 'uatom' }], gas: '10000' },
  memo: '',
  msgs: [
    {
      type: 'wasm/MsgExecuteContract',
      value: {
        contract: 'cosmos1abc',
        funds: [],
        msg: { note: WIDE_PADDING, send: { amount: '1000000000000', denom: 'ustake' } },
        sender: 'cosmos1abc',
      },
    },
  ],
  sequence: '1',
}

// Rebuild the text the device actually drew for one review field. Events arrive
// as the individual runs of text NBGL painted, so a field is its key event
// followed by its value lines, until the next key or the page furniture.
const PAGE_FURNITURE = ['Reject', 'Fee', 'Gas', 'Sign', 'Hold', 'Swipe', 'Review']

function readFieldFromScreens(events: Array<{ text: string }>, fieldKey: string) {
  const lines: string[] = []
  let sawEllipsis = false

  for (let i = 0; i < events.length; i++) {
    if (!events[i].text.startsWith(fieldKey)) continue
    for (let j = i + 1; j < events.length; j++) {
      const text = events[j].text
      if (text.startsWith('msgs/') || PAGE_FURNITURE.some(p => text.startsWith(p))) break
      // NBGL appends this when it clipped a line and threw the remainder away
      if (text === '...' || text === '…') {
        sawEllipsis = true
        continue
      }
      lines.push(text)
    }
  }

  return { displayed: lines.join(''), sawEllipsis }
}

describe('Review integrity', function () {
  // The character/line mismatch only exists on the touch review path; the nano
  // models paginate through a different backend entirely.
  const touchModels = DEVICE_MODELS.filter(m => isTouchDevice(m.name))

  test.concurrent.each(touchModels)(
    'review shows every character of a wide-glyph value ($name)',
    async function (m) {
      const sim = new Zemu(m.path)
      try {
        await sim.start({ ...defaultOptions, model: m.name })
        const app = new CosmosApp(sim.getTransport())

        const respPk = await app.getAddressAndPubKey(PATH, HRP)
        expect(respPk).toHaveProperty('compressed_pk')

        await sim.deleteEvents()

        const tx = Buffer.from(JSON.stringify(WASM_TX_WITH_WIDE_VALUE))
        const signatureRequest = app.sign(PATH, tx, HRP, AMINO_JSON_TX)

        await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
        await sim.navigateUntilText('.', '', 'Hold to sign', true, false, 0, 180000, true)

        const resp = await signatureRequest
        expect(resp).toHaveProperty('signature')

        // What the signature actually covers, as the app renders it into a value
        const signedValue = JSON.stringify(WASM_TX_WITH_WIDE_VALUE.msgs[0].value.msg)

        const events = await sim.getEvents()
        const { displayed, sawEllipsis } = readFieldFromScreens(events, 'msgs/value/msg')

        // The whole point of a review screen: nothing may be signed that was not
        // shown. A mismatch here means characters reached no page while staying
        // inside the signed document.
        expect(displayed).toEqual(signedValue)
        expect(sawEllipsis).toBe(false)

        // And confirm the device really did sign that document
        const msgHash = Uint8Array.from(crypto.createHash('sha256').update(tx).digest())
        const signature = secp256k1.signatureImport(Uint8Array.from(resp.signature))
        expect(secp256k1.ecdsaVerify(signature, msgHash, Uint8Array.from(respPk.compressed_pk))).toEqual(true)
      } finally {
        await sim.close()
      }
    },
  )
})
