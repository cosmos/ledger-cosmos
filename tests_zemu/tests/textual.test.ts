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

import Zemu, { ClickNavigation, TouchNavigation, isTouchDevice } from '@zondax/zemu'
import { CosmosApp } from '@zondax/ledger-cosmos-js'
import { defaultOptions, DEVICE_MODELS, tx_sign_textual, TEXTUAL_TX } from './common'
import { secp256k1 } from '@noble/curves/secp256k1'
import { sha256 } from '@noble/hashes/sha256'
import { keccak_256 } from '@noble/hashes/sha3'
import { ButtonKind, IButton, SwipeDirection } from '@zondax/zemu/dist/types'

jest.setTimeout(90000)

// Textual mode is not available for NanoS
const TEXTUAL_MODELS = DEVICE_MODELS.filter(m => m.name !== 'nanos')

describe('Textual', function () {
  // eslint-disable-next-line jest/expect-expect
  test.concurrent.each(TEXTUAL_MODELS)('can start and stop container', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(TEXTUAL_MODELS)('sign basic textual', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = [44, 118, 0, 0, 0]
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-textual-sign_basic`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')
      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const hash = sha256(tx)
      const signature = secp256k1.Signature.fromDER(resp.signature)
      const pk = secp256k1.ProjectivePoint.fromHex(respPk.compressed_pk)
      const signatureOk = secp256k1.verify(signature, hash, pk.toRawBytes())
      expect(signatureOk).toEqual(true)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(TEXTUAL_MODELS)('sign basic textual expert', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Change to expert mode so we can skip fields
      await sim.toggleExpertMode()

      const path = [44, 118, 0, 0, 0]
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-textual-sign_basic_expert`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')
      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const hash = sha256(tx)
      const signature = secp256k1.Signature.fromDER(resp.signature)
      const pk = secp256k1.ProjectivePoint.fromHex(respPk.compressed_pk)
      const signatureOk = secp256k1.verify(signature, hash, pk.toRawBytes())
      expect(signatureOk).toEqual(true)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(TEXTUAL_MODELS)('sign basic textual eth', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Enable expert to allow sign with eth path
      await sim.toggleExpertMode()

      const path = [44, 60, 0, 0, 0]
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'inj'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-textual-sign_basic_eth`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')
      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const msgHash = keccak_256(tx)
      const signature = secp256k1.Signature.fromDER(resp.signature)
      const pk = secp256k1.ProjectivePoint.fromHex(respPk.compressed_pk)
      const signatureOk = secp256k1.verify(signature, msgHash, pk.toRawBytes())
      expect(signatureOk).toEqual(true)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(TEXTUAL_MODELS)('sign basic textual eth warning ', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = [44, 60, 0, 0, 0]
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'inj'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      let nav = undefined
      if (isTouchDevice(m.name)) {
        const okButton: IButton = {
          x: 200,
          y: 540,
          delay: 0.25,
          direction: SwipeDirection.NoSwipe,
        }
        nav = new TouchNavigation(m.name, [ButtonKind.ConfirmYesButton])
        nav.schedule[0].button = okButton
      } else {
        nav = new ClickNavigation([1, 0])
      }
      await sim.navigate('.', `${m.prefix.toLowerCase()}-textual-sign_basic_eth_warning`, nav.schedule)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x6984)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(TEXTUAL_MODELS)('sign basic textual evmos ', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Enable expert to allow sign with eth path
      await sim.toggleExpertMode()

      const path = [44, 60, 0, 0, 0]
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'evmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      const last_index = await sim.navigateUntilText(
        '.',
        `${m.prefix.toLowerCase()}-textual-sign_basic_evmos`,
        sim.startOptions.approveKeyword,
        false,
        false,
      )

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')
      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const msgHash = keccak_256(tx)
      const signature = secp256k1.Signature.fromDER(resp.signature)
      const pk = secp256k1.ProjectivePoint.fromHex(respPk.compressed_pk)
      const signatureOk = secp256k1.verify(signature, msgHash, pk.toRawBytes())
      expect(signatureOk).toEqual(true)
    } finally {
      await sim.close()
    }
  })
})
