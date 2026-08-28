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
import {
  defaultOptions,
  DEVICE_MODELS,
  example_tx_str_basic,
  example_tx_str_basic2,
  ibc_denoms,
  AMINO_JSON_TX,
  setWithdrawAddress,
  cliGovDeposit,
  example_tx_str_msgMultiSend,
  example_tx_str_msgMultiSendAndSend,
  big_transaction,
  wasm_execute_contract_boundary_test,
  babylonWrappedDelegate,
  babylonWrappedUndelegate,
} from './common'

// @ts-ignore
import secp256k1 from 'secp256k1/elliptic'
// @ts-ignore
import crypto from 'crypto'
import { ButtonKind, IButton, SwipeDirection } from '@zondax/zemu/dist/types'
import { getTouchElement } from "@zondax/zemu/dist/buttons";

jest.setTimeout(120000)

describe('Amino', function () {
  // eslint-disable-next-line jest/expect-expect
  test.concurrent.each(DEVICE_MODELS)('can start and stop container', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('sign basic normal', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), 'utf-8')
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic`)

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

  test.concurrent.each(DEVICE_MODELS)('sign basic, omitted HRP after rejected get_addr', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), 'utf-8')

      // Prime the global HRP with a rejected mixed-case GET_ADDR on the default
      // Cosmos path. An upper-case HRP is not a valid bech32 prefix, so
      // checkChainConfig() refuses it outright - but extractHRP() runs first,
      // so the mixed-case HRP is still written into the shared global buffer
      // before the request fails.
      const rejected = app.getAddressAndPubKey(path, 'CoSmOs')
      await expect(rejected).rejects.toMatchObject({ returnCode: 0x698c })

      // A later SIGN that omits the HRP must still behave as a self-contained
      // default-Cosmos flow and reach review, rather than reusing the leftover
      // HRP and aborting before review.
      const signatureRequest = app.sign(path, tx, undefined, AMINO_JSON_TX)

      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic_omitted_hrp`)

      const resp = await signatureRequest
      expect(resp).toHaveProperty('signature')

      // Fetch the pubkey with a valid HRP and verify the produced signature.
      const respPk = await app.getAddressAndPubKey(path, 'cosmos')
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

  test.concurrent.each(DEVICE_MODELS)('sign rejects a malformed HRP on the generic Cosmos path', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), 'utf-8')

      // SIGN declares its HRP in the first chunk (extractHDPath_HRP), the second
      // call site of the same chain-config check. A NUL-smuggled 'inj' has to be
      // refused here as well, before any of the transaction reaches review --
      // otherwise the device signs for the Cosmos 118' key while the host
      // believes it is talking to Injective.
      await expect(app.sign(path, tx, 'inj\u0000X', AMINO_JSON_TX)).rejects.toMatchObject({
        returnCode: 0x698c,
        errorMessage: 'Chain config not supported',
      })

      // Uppercase is refused on the same call site, and now with the
      // chain-config status word rather than a generic execution error.
      await expect(app.sign(path, tx, 'COSMOS', AMINO_JSON_TX)).rejects.toMatchObject({
        returnCode: 0x698c,
        errorMessage: 'Chain config not supported',
      })

      // The device is left idle by both rejections, not stuck mid-transaction:
      // a following well-formed request is accepted rather than answered with
      // COMMAND_NOT_ALLOWED.
      const resp = await app.getAddressAndPubKey(path, 'akash')
      expect(resp.bech32_address.startsWith('akash1')).toBe(true)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('sign basic normal2', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic2))
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic2`)

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

  test.concurrent.each(DEVICE_MODELS)('sign basic with extra fields', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic))
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic_extra_fields`)

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

  test.concurrent.each(DEVICE_MODELS)('ibc denoms', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(ibc_denoms))
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-ibc_denoms`)

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

  test.concurrent.each(DEVICE_MODELS)('SetWithdrawAddress', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(setWithdrawAddress))
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-setWithdrawAddress`)

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

  test.concurrent.each(DEVICE_MODELS)('CLIGovDeposit', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(cliGovDeposit))
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-govDeposit`)

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

  // A MsgMultiSend batched with a MsgSend: both messages and all of their
  // fields (including the Send's amount, from and to) must appear on the review
  // screens. Runs in normal (non-expert) mode on the default chain.
  test.concurrent.each(DEVICE_MODELS)('MsgMultiSendAndSend', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_msgMultiSendAndSend))
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-msgMultiSendAndSend`)

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

  test.concurrent.each(DEVICE_MODELS)('MsgMultisend', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Activate expert mode
      await sim.toggleExpertMode()

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_msgMultiSend))
      const hrp = 'cosmos'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-msgMultiSend`)

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

  // Test for string length boundary issue - certain field length combinations
  // should not cause "Data is Invalid" errors
  test.concurrent.each(DEVICE_MODELS)('wasm execute contract boundary test', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Activate expert mode
      await sim.toggleExpertMode()

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(wasm_execute_contract_boundary_test))
      const hrp = 'neutron'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-wasm_boundary_test`)

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

  test.concurrent.each(DEVICE_MODELS)('SetWithdrawAddress-eth', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Change to expert mode so we can skip fields
      await sim.toggleExpertMode()

      const path = "m/44'/60'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(setWithdrawAddress))
      const hrp = 'inj'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-setWithdrawAddress-eth`)

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

  test.concurrent.each(DEVICE_MODELS)('sign basic normal Eth', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Enable expert to allow sign with eth path
      await sim.toggleExpertMode()

      const path = "m/44'/60'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), 'utf-8')
      const hrp = 'inj'

      // check with invalid HRP
      const errorRespPk = app.getAddressAndPubKey(path, 'forbiddenHRP')
      await expect(errorRespPk).rejects.toMatchObject({
        returnCode: 0x698c,
        errorMessage: 'Chain config not supported',
      })

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic_eth`)

      const resp = await signatureRequest
      console.log(resp)

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      console.log(respPk)

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

  // Babylon x/epoching wrapped staking messages. The wrapped MsgDelegate is
  // nested under a "msg" key; the device must render the inner Type, Amount,
  // Delegator and Validator as reviewable fields (not a raw JSON blob).
  test.concurrent.each(DEVICE_MODELS)('babylon wrapped delegate', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(babylonWrappedDelegate), 'utf-8')
      const hrp = 'bbn'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-babylon_wrapped_delegate`)

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

  test.concurrent.each(DEVICE_MODELS)('babylon wrapped undelegate', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/118'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(babylonWrappedUndelegate), 'utf-8')
      const hrp = 'bbn'

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      expect(respPk).toHaveProperty('compressed_pk')
      expect(respPk).toHaveProperty('bech32_address')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-babylon_wrapped_undelegate`)

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

  test.concurrent.each(DEVICE_MODELS)('sign basic normal Eth no expert', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...defaultOptions, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = "m/44'/60'/0'/0/0"
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), 'utf-8')
      const hrp = 'inj'
      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, hrp)
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, hrp)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      let nav = undefined
      if (isTouchDevice(m.name)) {
        const confirmButton: IButton = getTouchElement(m.name, ButtonKind.ConfirmYesButton)
        nav = new TouchNavigation(m.name, [ButtonKind.ConfirmYesButton]);
        nav.schedule[0].button = confirmButton;
      } else {
        nav = new ClickNavigation([1, 0]);
      }

      // Start navigation without await
      sim.navigateAndCompareSnapshots('.', `${m.prefix.toLowerCase()}-sign_basic_eth_warning`, nav.schedule)

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
})
