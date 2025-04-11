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
// @ts-ignore
import CosmosApp from '@zondax/ledger-cosmos-js'
import { defaultOptions, DEVICE_MODELS, tx_sign_textual, TEXTUAL_TX } from './common'
// @ts-ignore
import secp256k1 from 'secp256k1/elliptic'
// @ts-ignore
import crypto from 'crypto'
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

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-textual-sign_basic`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const hash = crypto.createHash('sha256')
      const msgHash = Uint8Array.from(hash.update(tx).digest())

      const signatureDER = resp.signature
      const signature = secp256k1.signatureImport(Uint8Array.from(signatureDER))

      const pk = Uint8Array.from(respPk.compressed_pk)

      const signatureOk = secp256k1.ecdsaVerify(signature, msgHash, pk)
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

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-textual-sign_basic_expert`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const hash = crypto.createHash('sha256')
      const msgHash = Uint8Array.from(hash.update(tx).digest())

      const signatureDER = resp.signature
      const signature = secp256k1.signatureImport(Uint8Array.from(signatureDER))

      const pk = Uint8Array.from(respPk.compressed_pk)

      const signatureOk = secp256k1.ecdsaVerify(signature, msgHash, pk)
      expect(signatureOk).toEqual(true)
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

      const path = "m/44'/60'/0'/0/0"
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'evmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
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

      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const sha3 = require('js-sha3')
      const msgHash = Buffer.from(sha3.keccak256(tx), 'hex')

      const signatureDER = resp.signature
      const signature = secp256k1.signatureImport(Uint8Array.from(signatureDER))

      const pk = Uint8Array.from(respPk.compressed_pk)

      const signatureOk = secp256k1.ecdsaVerify(signature, msgHash, pk)
      expect(signatureOk).toEqual(true)
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

      const path = "m/44'/60'/0'/0/0"
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'evmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
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

      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const sha3 = require('js-sha3')
      const msgHash = Buffer.from(sha3.keccak256(tx), 'hex')

      const signatureDER = resp.signature
      const signature = secp256k1.signatureImport(Uint8Array.from(signatureDER))

      const pk = Uint8Array.from(respPk.compressed_pk)

      const signatureOk = secp256k1.ecdsaVerify(signature, msgHash, pk)
      expect(signatureOk).toEqual(true)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(TEXTUAL_MODELS)('sign basic textual eth warning', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/60'/0'/0/0"
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'inj'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      let nav = undefined;
      if (isTouchDevice(m.name)) {
        const okButton: IButton = {
          x: 200,
          y: 540,
          delay: 0.25,
          direction: SwipeDirection.NoSwipe,
        };
        nav = new TouchNavigation(m.name, [
          ButtonKind.ConfirmYesButton,
        ]);
        nav.schedule[0].button = okButton;
      } else {
        nav = new ClickNavigation([1, 0]);
      }

      // Start navigation without await
      sim.navigateAndCompareSnapshots('.', `${m.prefix.toLowerCase()}-textual-sign_basic_eth_warning`, nav.schedule);

      // Handle both errors
      try {
        await signatureRequest
        throw new Error('Expected sign to fail')
      } catch (error: any) {
        // First error from ledger-js
        expect(error.message).toBe('Data is invalid')
        
        // Wait a bit to ensure the second error is caught
        await new Promise(resolve => setTimeout(resolve, 1000))
        
        // Second error after navigation
        try {
          await signatureRequest
          throw new Error('Expected second error')
        } catch (error2: any) {
          expect(error2.message).toBe('Data is invalid')
        }
      }
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(TEXTUAL_MODELS)('sign basic textual eth warning', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/60'/0'/0/0"
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'inj'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
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

      // Start navigation without await
      sim.navigateAndCompareSnapshots('.', `${m.prefix.toLowerCase()}-textual-sign_basic_eth_warning`, nav.schedule)

      // Handle both errors
      try {
        await signatureRequest
        throw new Error('Expected sign to fail')
      } catch (error: any) {
        // First error from ledger-js
        expect(error.message).toBe('Data is invalid')

        // Wait a bit to ensure the second error is caught
        await new Promise(resolve => setTimeout(resolve, 1000))

        // Second error after navigation
        try {
          await signatureRequest
          throw new Error('Expected second error')
        } catch (error2: any) {
          expect(error2.message).toBe('Data is invalid')
        }
      }
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(TEXTUAL_MODELS)('sign basic textual eth ', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Enable expert to allow sign with eth path
      await sim.toggleExpertMode()

      const path = "m/44'/60'/0'/0/0"
      const tx = Buffer.from(tx_sign_textual, 'hex')
      const hrp = 'inj'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-textual-sign_basic_eth`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp).toHaveProperty('signature')

      // Now verify the signature
      const sha3 = require('js-sha3')
      const msgHash = Buffer.from(sha3.keccak256(tx), 'hex')

      const signatureDER = resp.signature
      const signature = secp256k1.signatureImport(Uint8Array.from(signatureDER))

      const pk = Uint8Array.from(respPk.compressed_pk)

      const signatureOk = secp256k1.ecdsaVerify(signature, msgHash, pk)
      expect(signatureOk).toEqual(true)
    } finally {
      await sim.close()
    }
  })
})
