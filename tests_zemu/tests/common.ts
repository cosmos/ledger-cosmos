/** ******************************************************************************
 *  (c) 2021-2022 Zondax GmbH
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
import { DEFAULT_START_OPTIONS, DeviceModel } from '@zondax/zemu'

const Resolve = require('path').resolve

export const AMINO_JSON_TX = 0x0
export const TEXTUAL_TX = 0x1
export const APP_SEED = 'equip will roof matter pink blind book anxiety banner elbow sun young'

const APP_PATH_S = Resolve('../app/output/app_s.elf')
const APP_PATH_X = Resolve('../app/output/app_x.elf')
const APP_PATH_SP = Resolve('../app/output/app_s2.elf')

export const DEFAULT_OPTIONS = {
  ...DEFAULT_START_OPTIONS,
  logging: true,
  custom: `-s "${APP_SEED}"`,
  pressDelay: 1500,
  X11: false,
}

export const DEVICE_MODELS: DeviceModel[] = [
  { name: 'nanos', prefix: 'S', path: APP_PATH_S },
  { name: 'nanox', prefix: 'X', path: APP_PATH_X },
  { name: 'nanosp', prefix: 'SP', path: APP_PATH_SP },
]

export const tx_sign_textual = '92a20168436861696e20696402686d792d636861696ea2016e4163636f756e74206e756d626572026131a2016853657175656e6365026132a301674164647265737302782d636f736d6f7331756c6176336873656e7570737771666b77327933737570356b677471776e767161386579687304f5a3016a5075626c6963206b657902781f2f636f736d6f732e63727970746f2e736563703235366b312e5075624b657904f5a3026d5075624b6579206f626a656374030104f5a401634b657902785230324542204444374620453446442045423736204443384120323035452046363544203739304320443330452038413337203541354320323532382045423341203932334120463146422034443739203444030204f5a102781e54686973207472616e73616374696f6e206861732031204d657373616765a3016d4d6573736167652028312f312902781c2f636f736d6f732e62616e6b2e763162657461312e4d736753656e640301a2026e4d736753656e64206f626a6563740302a3016c46726f6d206164647265737302782d636f736d6f7331756c6176336873656e7570737771666b77327933737570356b677471776e76716138657968730303a3016a546f206164647265737302782d636f736d6f7331656a726634637572327779366b667572673966326a707070326833616665356836706b6835740303a30166416d6f756e74026731302041544f4d0303a1026e456e64206f66204d657373616765a201644d656d6f0278193e20e29a9befb88f5c7532363942e29a9befb88f2020202020a2016446656573026a302e3030322041544f4da30169476173206c696d697402673130302730303004f5a3017148617368206f66207261772062797465730278403963303433323930313039633237306232666661396633633066613535613039306330313235656265663838316637646135333937386462663933663733383504f5'

export const example_tx_str_basic = {
  account_number: '108',
  chain_id: 'cosmoshub-4',
  fee: {
    amount: [
      {
        amount: '600',
        denom: 'uatom',
      },
    ],
    gas: '200000',
  },
  memo: '',
  msgs: [
    {
      type: 'cosmos-sdk/MsgWithdrawDelegationReward',
      value: {
        delegator_address: 'cosmos1w34k53py5v5xyluazqpq65agyajavep2rflq6h',
        validator_address: 'cosmosvaloper1kn3wugetjuy4zetlq6wadchfhvu3x740ae6z6x',
      },
    },
    {
      type: 'cosmos-sdk/MsgWithdrawDelegationReward',
      value: {
        delegator_address: 'cosmos1w34k53py5v5xyluazqpq65agyajavep2rflq6h',
        validator_address: 'cosmosvaloper1sjllsnramtg3ewxqwwrwjxfgc4n4ef9u2lcnj0',
      },
    },
  ],
  sequence: '106',
}

export const example_tx_str_expert = {
  account_number: '108',
  chain_id: 'cosmoshub-2',
  fee: {
    amount: [
      {
        amount: '600',
        denom: 'uatom',
      },
    ],
    gas: '200000',
  },
  memo: '',
  msgs: [
    {
      type: 'cosmos-sdk/MsgWithdrawDelegationReward',
      value: {
        delegator_address: 'cosmos1kky4yzth6gdrm8ga5zlfwhav33yr7hl87jycah',
        validator_address: 'cosmosvaloper1kn3wugetjuy4zetlq6wadchfhvu3x740ae6z6x',
      },
    },
    {
      type: 'cosmos-sdk/MsgWithdrawDelegationReward',
      value: {
        delegator_address: 'cosmos1kky4yzth6gdrm8ga5zlfwhav33yr7hl87jycah',
        validator_address: 'cosmosvaloper1sjllsnramtg3ewxqwwrwjxfgc4n4ef9u2lcnj0',
      },
    },
  ],
  sequence: '106',
}

export const example_tx_str_combined = {
  account_number: '108',
  chain_id: 'cosmoshub-4',
  fee: {
    amount: [
      {
        amount: '600',
        denom: 'uatom',
      },
    ],
    gas: '200000',
  },
  memo: '',
  msgs: [
    {
      type: 'cosmos-sdk/MsgWithdrawDelegationReward',
      value: {
        delegator_address: 'cosmos1w34k53py5v5xyluazqpq65agyajavep2rflq6h',
        validator_address: 'cosmosvaloper1648ynlpdw7fqa2axt0w2yp3fk542junl7rsvq6',
      },
    },
    {
      type: 'cosmos-sdk/MsgDelegate',
      value: {
        amount: {
          amount: '20139397',
          denom: 'uatom',
        },
        delegator_address: 'cosmos1w34k53py5v5xyluazqpq65agyajavep2rflq6h',
        validator_address: 'cosmosvaloper1648ynlpdw7fqa2axt0w2yp3fk542junl7rsvq6',
      },
    },
  ],
  sequence: '106',
}

export const example_tx_str_basic2 = {
  account_number: '482',
  chain_id: 'cosmoshub-4',
  fee: {
    amount: [],
    gas: '10000000',
  },
  memo: '',
  msgs: [
    {
      type: 'somechain/MsgNew',
      value: {
        coins: [
          {
            amount: '20139397',
            asset: 'uatom',
          },
        ],
        memo: 'memo_text_goes_here',
        signer: 'cosmos1w34k53py5v5xyluazqpq65agyajavep2rflq6h',
      },
    },
  ],
  sequence: '6',
}

export const example_tx_str_basic_extra_fields = {
  account_number: '108',
  chain_id: 'cosmoshub-4',
  extra_field: 'empty',
  fee: {
    amount: [
      {
        amount: '600',
        denom: 'uatom',
      },
    ],
    gas: '200000',
  },
  foo: 'bar',
  memo: '',
  msgs: [
    {
      type: 'cosmos-sdk/MsgWithdrawDelegationReward',
      value: {
        delegator_address: 'cosmos1w34k53py5v5xyluazqpq65agyajavep2rflq6h',
        validator_address: 'cosmosvaloper1kn3wugetjuy4zetlq6wadchfhvu3x740ae6z6x',
      },
    },
    {
      type: 'cosmos-sdk/MsgWithdrawDelegationReward',
      value: {
        delegator_address: 'cosmos1w34k53py5v5xyluazqpq65agyajavep2rflq6h',
        validator_address: 'cosmosvaloper1sjllsnramtg3ewxqwwrwjxfgc4n4ef9u2lcnj0',
      },
    },
  ],
  sequence: '106',
  unknown_field: 123456
}

export const ibc_denoms = {
  account_number: "0",
  chain_id: "cosmoshub-4",
  fee: {
    amount: [
      {
        "amount": '5',
        "denom": 'uatom',
      }
    ],
    gas: '10000',
  },
  memo: "testmemo",
  msgs: [
    {
      inputs: [
        {
          address: "cosmosaccaddr1d9h8qat5e4ehc5",
          coins: [
            {
              amount: '10',
              denom: 'ibc/27394FB092D2ECCD56123C74F36E4C1F926001CEADA9CA97EA622B25F41E5EB2'
            }
          ]
        }
      ],
      outputs: [
        {
          address: 'cosmosaccaddr1da6hgur4wse3jx32',
          coins: [
            {
              amount: '10',
              denom: 'ibc/27394FB092D2ECCD56123C74F36E4C1F926001CEADA9CA97EA622B25F41E5EB2'
            }
          ]
        }
      ]
    }
  ],
  sequence: '1'
}

export const setWithdrawAddress = {
    account_number: '8',
    chain_id: 'testing',
    fee: {
      amount: [
        {
          amount: '5000',
          denom: 'uatom'
        }
      ],
      gas: '200000'
    },
    memo: '',
    msgs: [
      {
        type: 'cosmos-sdk/MsgSetWithdrawAddress',
        value: {
          delegator_address: 'cosmos1hr9x0sjvel6z3vt9qny8sdd5gnnlgk0p69d6cv',
          withdraw_address: 'cosmos12d64j98tjjpqkx70r08aspc4nvntqp2w6wr2de'
        }
      },
      {
        type: 'cosmos-sdk/MsgWithdrawDelegationReward',
        value: {
          delegator_address: 'cosmos1hr9x0sjvel6z3vt9qny8sdd5gnnlgk0p69d6cv',
          validator_address: 'cosmosvaloper13dr26wdygna3s8fdl5tlc45m2le2ydyddxzj49'
        }
      }
    ],
    sequence: '7'
  }

  export const cliGovDeposit = {
    account_number: '8',
    chain_id: 'my-chain',
    fee: {
      amount: [],
      gas: '200000'
    },
    memo: 'A B C',
    msgs: [
      {
        type: 'cosmos-sdk/MsgDeposit',
        value: {
          amount: [{
            amount: '10',
            denom: 'stake',
          }],
          depositor: 'cosmos1xl2256vdh0j68khz9wq88hnyqcq0f5f4za2480',
          proposal_id: '1'
        }
      },
    ],
    sequence: '2'
  }
