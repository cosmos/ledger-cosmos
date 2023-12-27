/** ******************************************************************************
 *  (c) 2018-2023 Zondax GmbH
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

import Zemu, { ClickNavigation, TouchNavigation } from '@zondax/zemu'
// @ts-ignore
import { CosmosApp } from '@zondax/ledger-cosmos-js'
import { defaultOptions, DEVICE_MODELS, AMINO_JSON_TX } from './common'

import { Keplr, StdSignDoc } from "@keplr-wallet/types";

// @ts-ignore
import secp256k1 from 'secp256k1/elliptic'
// @ts-ignore
import crypto from 'crypto'
import { ButtonKind, IButton } from '@zondax/zemu/dist/types'

jest.setTimeout(10000)


// https://github.com/chainapsis/keplr-wallet/blob/dd487d2a041e2a0ebff99b1cc633bc84a9eef916/packages/cosmos/src/adr-36/amino.ts#L87
function makeADR36AminoSignDoc(
    signer: string,
    data: string | Uint8Array,
  ): StdSignDoc {
    if (typeof data === 'string') {
      data = Buffer.from(data).toString('base64');
    } else {
      data = Buffer.from(data).toString('base64');
    }

    return {
        account_number: '0',
        chain_id: '',
        fee: {
            amount: [],
            gas: '0',
        },
        memo: '',
        msgs: [
            {
                type: 'sign/MsgSignData',
                value: {
                    data,
                    signer,
                },
            },
        ],
        sequence: '0',
    };
  }

describe('Keplr', function () {
  test.concurrent.each(DEVICE_MODELS)('sign keplr', async function (m) {
    const sim = new Zemu(m.path)
    try {
        await sim.start({ ...defaultOptions, model: m.name })
        const app = new CosmosApp(sim.getTransport())

        const nonce = crypto.randomBytes(16);
        const signArbitraryMessage = makeADR36AminoSignDoc("cosmos1wkd9tfm5pqvhhaxq77wv9tvjcsazuaykwsld65", nonce)

        const path = [44, 118, 0, 0, 0]
        const tx = Buffer.from(JSON.stringify(signArbitraryMessage), "utf-8")
        const hrp = 'cosmos'

        // console.log(tx)
        sim.log("CHECK JSON!!!!!")
        sim.log(tx.toString())
        sim.log("-----------------------------------------------------------------------")

        // get address / publickey
        const respPk = await app.getAddressAndPubKey(path, hrp)
        expect(respPk.return_code).toEqual(0x9000)
        expect(respPk.error_message).toEqual('No errors')
        console.log(respPk)

        // do not wait here..
        const signatureRequest = app.sign(path, tx, hrp, AMINO_JSON_TX)

        // Wait until we are not in the main menu
        await sim.waitUntilScreenIsNot(sim.getMainMenuSnapshot())
        await sim.compareSnapshotsAndApprove('.', `${m.prefix.toLowerCase()}-sign_arbitrary`)

        const resp = await signatureRequest
        console.log(resp)

        expect(resp.return_code).toEqual(0x9000)
        expect(resp.error_message).toEqual('No errors')
        expect(resp).toHaveProperty('signature')

    } finally {
      await sim.close()
    }
  })
})
