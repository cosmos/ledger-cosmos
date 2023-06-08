/** ******************************************************************************
 *  (c) 2018-2022 Zondax GmbH
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
// @ts-ignore
import { CosmosApp } from '@zondax/ledger-cosmos-js'
import { DEFAULT_OPTIONS, DEVICE_MODELS, example_tx_str_basic, example_tx_str_basic2, ibc_denoms } from './common'

// @ts-ignore
import secp256k1 from 'secp256k1/elliptic'
// @ts-ignore
import crypto from 'crypto'

jest.setTimeout(90000)

describe('Standard', function () {
  // eslint-disable-next-line jest/expect-expect
  test.concurrent.each(DEVICE_MODELS)('can start and stop container', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('main menu', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      expect(await sim.navigateAndCompareSnapshots('.', `${m.prefix.toLowerCase()}-mainmenu`, [1, 0, 0, 4, -5])).toEqual(true)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('get app version', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())
      const resp = await app.getVersion()

      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')
      expect(resp).toHaveProperty('test_mode')
      expect(resp).toHaveProperty('major')
      expect(resp).toHaveProperty('minor')
      expect(resp).toHaveProperty('patch')
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('get address', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Derivation path. First 3 items are automatically hardened!
      const path = [44, 118, 5, 0, 3]
      const resp = await app.getAddressAndPubKey(path, 'cosmos')

      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')

      expect(resp).toHaveProperty('bech32_address')
      expect(resp).toHaveProperty('compressed_pk')

      expect(resp.bech32_address).toEqual('cosmos1wkd9tfm5pqvhhaxq77wv9tvjcsazuaykwsld65')
      expect(resp.compressed_pk.length).toEqual(33)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('show address', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Derivation path. First 3 items are automatically hardened!
      const path = [44, 118, 5, 0, 3]
      const respRequest = app.showAddressAndPubKey(path, 'cosmos')
      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-show_address`)

      const resp = await respRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')

      expect(resp).toHaveProperty('bech32_address')
      expect(resp).toHaveProperty('compressed_pk')

      expect(resp.bech32_address).toEqual('cosmos1wkd9tfm5pqvhhaxq77wv9tvjcsazuaykwsld65')
      expect(resp.compressed_pk.length).toEqual(33)
    } finally {
      await sim.close()
    }
  })

    test.concurrent.each(DEVICE_MODELS)('show Eth address', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Derivation path. First 3 items are automatically hardened!
      const path = [44, 60, 0, 0, 1]
      const hrp = 'inj'

      // check with invalid HRP
      const errorRespPk = await app.getAddressAndPubKey(path, 'cosmos')
      expect(errorRespPk.return_code).toEqual(0x6986)
      expect(errorRespPk.error_message).toEqual('Transaction rejected')

      const respRequest = app.showAddressAndPubKey(path, hrp)
      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-show_eth_address`)

      const resp = await respRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')

      expect(resp).toHaveProperty('bech32_address')
      expect(resp).toHaveProperty('compressed_pk')

      expect(resp.compressed_pk.length).toEqual(33)

      // Verify address
      const secp256k1 = require("secp256k1");
      const keccak = require("keccak256");
      const { bech32 } = require("bech32");

      // Take the compressed pubkey and verify that the expected address can be computed
      const uncompressPubKeyUint8Array = secp256k1.publicKeyConvert(resp.compressed_pk, false).subarray(1);
      const ethereumAddressBuffer = Buffer.from(keccak(Buffer.from(uncompressPubKeyUint8Array))).subarray(-20);
      const eth_address = bech32.encode(hrp, bech32.toWords(ethereumAddressBuffer)); // "cosmos15n2h0lzvfgc8x4fm6fdya89n78x6ee2fm7fxr3"

      expect(resp.bech32_address).toEqual(eth_address)
      expect(resp.bech32_address).toEqual('inj15n2h0lzvfgc8x4fm6fdya89n78x6ee2f3h7z3f')
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('show address HUGE', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Derivation path. First 3 items are automatically hardened!
      const path = [44, 118, 2147483647, 0, 4294967295]
      const resp = await app.showAddressAndPubKey(path, 'cosmos')
      console.log(resp)

      expect(resp.return_code).toEqual(0x6985)
      expect(resp.error_message).toEqual('Conditions not satisfied')
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('show address HUGE Expert', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Activate expert mode
      await sim.toggleExpertMode();

      // Derivation path. First 3 items are automatically hardened!
      const path = [44, 118, 2147483647, 0, 4294967295]
      const respRequest = app.showAddressAndPubKey(path, 'cosmos')

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-show_address_huge`)

      const resp = await respRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')

      expect(resp).toHaveProperty('bech32_address')
      expect(resp).toHaveProperty('compressed_pk')

      expect(resp.bech32_address).toEqual('cosmos1ex7gkwwmq4vcgdwcalaq3t20pgwr37u6ntkqzh')
      expect(resp.compressed_pk.length).toEqual(33)
    } finally {
      await sim.close()
    }
  })

  test.concurrent.each(DEVICE_MODELS)('sign basic normal', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = [44, 118, 0, 0, 0]
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), "utf-8")

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, 'cosmos')
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')
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

  test.concurrent.each(DEVICE_MODELS)('sign basic normal2', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = [44, 118, 0, 0, 0]
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic2), "utf-8")

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, 'cosmos')
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic2`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')

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
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = [44, 118, 0, 0, 0]
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), "utf-8")

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, 'cosmos')
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic_extra_fields`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')
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
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = [44, 118, 0, 0, 0]
      const tx = Buffer.from(JSON.stringify(ibc_denoms), "utf-8")


      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, 'cosmos')
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-ibc_denoms`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')
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

    test.concurrent.each(DEVICE_MODELS)('sign basic normal Eth', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Enable expert to allow sign with eth path
      await sim.toggleExpertMode();

      const path = [44, 60, 0, 0, 0]
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), "utf-8")

      // check with invalid HRP
      const errorRespPk = await app.getAddressAndPubKey(path, 'forbiddenHRP')
      expect(errorRespPk.return_code).toEqual(0x6986)
      expect(errorRespPk.error_message).toEqual('Transaction rejected')

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, 'inj')
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_basic_eth`)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x9000)
      expect(resp.error_message).toEqual('No errors')

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

  test.concurrent.each(DEVICE_MODELS)('sign basic normal Eth no expert', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      const path = [44, 60, 0, 0, 0]
      const tx = Buffer.from(JSON.stringify(example_tx_str_basic), "utf-8")

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, 'inj')
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.navigateAndCompareSnapshots('.', `${m.prefix.toLowerCase()}-sign_basic_eth_warning`, [1,0], false)

      const resp = await signatureRequest
      console.log(resp)

      expect(resp.return_code).toEqual(0x6984)
    } finally {
      await sim.close()
    }
  })
})
