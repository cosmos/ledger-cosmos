Transaction Specification
-------------------------
Two types of transaction formats are supported by the Cosmos App, Json format and Textual format.

### JSON Format

Transactions passed to the Ledger device will be in the following format. The Ledger device MUST accept any transaction (valid as below) in this format.

```json
{
  "account_number": {number},
  "chain_id": {string},
  "fee": {
    "amount": [{"amount": {number}, "denom": {string}}, ...],
    "gas": {number}
  },
  "memo": {string},
  "msgs": [{arbitrary}],
  "sequence": {number}
}
```

`msgs` is a list of messages, which are arbitrary JSON structures.

#### Examples

```json
{
  "account_number": "123",
  "chain_id": "cosmoshub-4",
  "fee": {
    "amount": [{"amount": "4000", "denom": "uatom"}, ...],
    "gas": "40000"
  },
  "memo": "this is a comment",
  "msgs": [{arbitrary}],
  "sequence": "42"
}
```

Note, all the `{number}` values must be passed as string.

#### Display Logic

The Ledger device SHOULD pick a suitable display representation for the transaction.

The key type (secp256k1 / ed25519), `chain_id`, `account_number`, `sequence`, `fee`, and `memo` should be displayed in that order, each on their own page, autoscrolling if necessary.

`msgs` should be iterated through and each displayed according to the following recursive logic:

```
display (json, level)
  if level == 2
    show value as json-encoded string
  else
    switch typeof(json) {
      case object:
        for (key, value) in object:
          show key
          display(value, level + 1)
      case array:
        for element in array:
          display(element, level + 1)
      otherwise:
        show value as json-encoded string
    }
```

starting at level 0, e.g. `display(msgs[0], 0)`.

### Validation

The Ledger device MUST validate that supplied JSON is valid. Our JSON specification is a subset of [RFC 7159](https://tools.ietf.org/html/rfc7159) - invalid RFC 7159 JSON is invalid Ledger JSON, but not all valid RFC 7159 JSON is valid Ledger JSON.

We add the following two rules:
- No spaces or newlines can be included, other than escaped in strings
- All dictionaries must be serialized in lexicographical key order

This serves to prevent signature compatibility issues among different client libraries.

This is equivalent to the following Python snippet:

```python
import json

def ledger_validate(json_str):
  obj = json.loads(json_str)
  canonical = json.dumps(obj, sort_keys = True, separators = (',', ':'))
  return canonical == json_str

assert ledger_validate('{"a":2,"b":3}')
assert ledger_validate('{"a ":2,"b":3}')
assert not ledger_validate('{"a":2,\n"b":3}')
assert not ledger_validate('{"b":2,"a":3}')
assert not ledger_validate('{"a" : 2 }')
```

### Textual Format

In Textual Format, a transaction is rendered into a textual representation, which is then sent to Ledger device, where transmitted data can be simply decoded into legible text.

The textual representation is a sequence of screens. Each screen is meant to be displayed in its entirety. This textual representation must be encoded using CBOR deterministic encoding (RFC 8949).

A screen consists of a text string, an indentation, and the expert flag,represented as an integer-keyed map. All entries are optional and MUST be omitted from the encoding if empty, zero, or false. Text defaults to the empty string, indent defaults to zero, and expert defaults to false.
```json
screen = {
  text_key: tstr,
  indent_key: uint,
  expert_key: bool,
}
````
To keep the enconding small on the CBOR the entry keys are represented by small integers.
```
text_key = 1
indent_key = 2
expert_key = 3
```

#### Examples
Textual Representation
```
Chain id: my-chain
Account number: 1
Sequence: 2 
Public key: cosmos.crypto.secp256k1.PubKey
> PubKey object
>> Key: Auvdf+T963bciiBe9l15DNMOijdaXCUo6zqSOvH7TXlN
Transaction: 1 Messages
> Message (1/1): cosmos.bank.v1beta1.MsgSend
> From address: cosmos1ulav3hsenupswqfkw2y3sup5kgtqwnvqa8eyhs
> To address: cosmos1ejrf4cur2wy6kfurg9f2jppp2h3afe5h6pkh5t
> Amount: 10 ATOM
End of Messages
Fees: 0.002 uatom
Gas limit: 100'000
Hash of raw bytes: e237dc2e3f8f0b1d0310e1cd0b0ab5fc4a59e4bc3454d67eb65c3fdb0fa599c9
````

CBOR Envelope
```
[
    {1: "Chain id: my-chain"},
    {1: "Account number: 1"},
    {1: "Sequence: 2"},
    {1: "Public key: cosmos.crypto.secp256k1.PubKey", 3: true},
    {1: "PubKey object", 2: 1, 3: true},
    {1: "Key: Auvdf+T963bciiBe9l15DNMOijdaXCUo6zqSOvH7TXlN", 2: 2, 3: true},
    {1: "Transaction: 1 Messages"},
    {1: "Message (1/1): cosmos.bank.v1beta1.MsgSend", 2: 1},
    {1: "From address: cosmos1ulav3hsenupswqfkw2y3sup5kgtqwnvqa8eyhs", 2: 1},
    {1: "To address: cosmos1ejrf4cur2wy6kfurg9f2jppp2h3afe5h6pkh5t", 2: 1},
    {1: "Amount: 10 ATOM", 2: 1},
    {1: "End of Messages"},
    {1: "Fees: 0.002 uatom"},
    {1: "Gas limit: 100'000", 3: true},
    { 1: "Hash of raw bytes: e237dc2e3f8f0b1d0310e1cd0b0ab5fc4a59e4bc3454d67eb65c3fdb0fa599c9", 3: true,},
]
```
CBOR Encoded
```
8fa10172436861696e2069643a206d792d636861696ea101714163636f756e74206e756d6265723a2031a1016b53657175656e63653a2032a201782a5075626c6963206b65793a20636f736d6f732e63727970746f2e736563703235366b312e5075624b657903f5a3016d5075624b6579206f626a656374020103f5a30178314b65793a2041757664662b54393633626369694265396c3135444e4d4f696a64615843556f367a71534f76483754586c4e020203f5a101775472616e73616374696f6e3a2031204d65737361676573a201782a4d6573736167652028312f31293a20636f736d6f732e62616e6b2e763162657461312e4d736753656e640201a201783b46726f6d20616464726573733a20636f736d6f7331756c6176336873656e7570737771666b77327933737570356b677471776e76716138657968730201a2017839546f20616464726573733a20636f736d6f7331656a726634637572327779366b667572673966326a707070326833616665356836706b6835740201a2016f416d6f756e743a2031302041544f4d0201a1016f456e64206f66204d65737361676573a10171466565733a20302e303032207561746f6da20172476173206c696d69743a203130302730303003f5a201785348617368206f66207261772062797465733a206532333764633265336638663062316430333130653163643062306162356663346135396534626333343534643637656236356333666462306661353939633903f5
```

The CBOR envelope is decoded using Intel TinyCbor library. Each CBOR container size is verified and the text string is slip into title and content to later be displayed in the Ledger Screen.