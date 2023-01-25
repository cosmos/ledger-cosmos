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
 import { DEFAULT_OPTIONS, DEVICE_MODELS, tx_sign_textual, TEXTUAL_TX } from './common'
 
 // @ts-ignore
 import secp256k1 from 'secp256k1/elliptic'
 // @ts-ignore
 import crypto from 'crypto'
 
 jest.setTimeout(60000)
 
 describe('Textual', function () {
   // eslint-disable-next-line jest/expect-expect
   test.each(DEVICE_MODELS)('can start and stop container', async function (m) {
     const sim = new Zemu(m.path)
     try {
       await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
     } finally {
       await sim.close()
     }
   })
 
   test.each(DEVICE_MODELS)('sign basic textual', async function (m) {
     const sim = new Zemu(m.path)
     try {
       await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
       const app = new CosmosApp(sim.getTransport())
 
       const path = [44, 118, 0, 0, 0]
       const tx = Buffer.from(tx_sign_textual, 'hex')
 
       // get address / publickey
       const respPk = await app.getAddressAndPubKey(path, 'cosmos')
       expect(respPk.return_code).toEqual(0x9000)
       expect(respPk.error_message).toEqual('No errors')
       console.log(respPk)
 
       // do not wait here..
       const signatureRequest = app.sign(path, tx, TEXTUAL_TX)
 
       // Wait until we are not in the main menu
       await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
       await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-textual-sign_basic`)
 
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


   test.each(DEVICE_MODELS)('sign basic textual expert', async function (m) {
    const sim = new Zemu(m.path)
    try {
      await sim.start({ ...DEFAULT_OPTIONS, model: m.name })
      const app = new CosmosApp(sim.getTransport())

      // Change to expert mode so we can skip fields
      await sim.clickRight()
      await sim.clickBoth()
      await sim.clickLeft()

      const path = [44, 118, 0, 0, 0]
      const tx = Buffer.from(tx_sign_textual, 'hex')

      // get address / publickey
      const respPk = await app.getAddressAndPubKey(path, 'cosmos')
      expect(respPk.return_code).toEqual(0x9000)
      expect(respPk.error_message).toEqual('No errors')
      console.log(respPk)

      // do not wait here..
      const signatureRequest = app.sign(path, tx, TEXTUAL_TX)

      // Wait until we are not in the main menu
      await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
      await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-textual-sign_basic_expert`)

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
 })
 