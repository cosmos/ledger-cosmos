import { DeviceModel } from '@zondax/zemu'

const Resolve = require('path').resolve

export const APP_SEED = 'equip will roof matter pink blind book anxiety banner elbow sun young'

const APP_PATH_S = Resolve('../app/output/app_s.elf')
const APP_PATH_X = Resolve('../app/output/app_x.elf')

export const models: DeviceModel[] = [
  { name: 'nanos', prefix: 'S', path: APP_PATH_S },
  { name: 'nanox', prefix: 'X', path: APP_PATH_X },
]

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

const example_tx_str_expert = {
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

const example_tx_str_combined = {
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
