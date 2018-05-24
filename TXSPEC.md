Transaction Specification
-------------------------

### Format

Transactions passed to the Ledger device will be in the following format. The Ledger device MUST accept any transaction (valid as below) in this format.

```json
{
  "chain_id": {string},
  "sequences": [{number}, {number}, ...],
  "fee_bytes": {
    "amount": [{"amount": {number}, "denom": {string}}, ...],
    "gas": {number}
  },
  "msg_bytes": {arbitrary},
  "alt_bytes": {arbitrary}
}
```

`msg_bytes` and `alt_bytes` are arbitary JSON.

#### Examples

```json
{"alt_bytes":null,"chain_id":"test-chain-1","fee_bytes":{"amount":[{"amount":5,"denom":"photon"}],"gas":10000},"msg_bytes":{"inputs":[{"address":"696E707574","coins":[{"amount":10,"denom":"atom"}]}],"outputs":[{"address":"6F7574707574","coins":[{"amount":10,"denom":"atom"}]}]},"sequences":[1]}
```

```json
{"alt_bytes":null,"chain_id":"test-chain-2","fee_bytes":{"amount":[{"amount":10,"denom":"photon"}],"gas":10000},"msg_bytes":{"shares":"100"},"sequences":[2]}
```

#### Display Logic

The Ledger device SHOULD pick a suitable display representation for the transaction.

The key type (secp256k1 / ed25519), `chain_id`, `sequences`, and `fee_bytes` should be displayed in that order, each on their own page, autoscrolling if necessary.

`msg_bytes` and `alt_bytes` should be displayed according to the following recursive logic:

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

starting at level 0, e.g. `display(msg_bytes, 0)` and `display(alt_bytes, 0)`.

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
