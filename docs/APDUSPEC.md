# Cosmos App - Ledger Nano S
## General structure

The general structure of commands and responses is as follows:

#### Commands

| Field   | Type     | Content                | Note |
| :------ | :------- | :--------------------- | ---- |
| CLA     | byte (1) | Application Identifier | 0x55 |
| INS     | byte (1) | Instruction ID         |      |
| P1      | byte (1) | Parameter 1            |      |
| P2      | byte (1) | Parameter 2            |      |
| L       | byte (1) | Bytes in payload       |      |
| PAYLOAD | byte (L) | Payload                |      |

#### Response

| Field   | Type     | Content     | Note                     |
| ------- | -------- | ----------- | ------------------------ |
| ANSWER  | byte (?) | Answer      | depends on the command   |
| SW1-SW2 | byte (2) | Return code | see list of return codes |

#### Return codes

| Return code | Description                               |
| ----------- | ----------------------------------------- |
| 0x6400      | Execution Error                           |
| 0x6982      | Empty buffer                              |
| 0x6983      | Output buffer too small                   |
| 0x6986      | Command not allowed                       |
| 0x6984      | Data Invalid (More info on error message) |
| 0x6988      | Transaction data exceeds buffer capacity  |
| 0x698A      | Wrong HRP Length                          |
| 0x698B      | Invalid HD path coin value                |
| 0x698C      | Chain Config not supported                |
| 0x698D      | Expert Mode required for Eth chain        |
| 0x6D00      | INS not supported                         |
| 0x6E00      | CLA not supported                         |
| 0x6F00      | Unknown                                   |
| 0x9000      | Success                                   |

---------

## Command definition

### GET_VERSION

#### Command

| Field | Type     | Content                | Expected |
| ----- | -------- | ---------------------- | -------- |
| CLA   | byte (1) | Application Identifier | 0x55     |
| INS   | byte (1) | Instruction ID         | 0x00     |
| P1    | byte (1) | Parameter 1            | ignored  |
| P2    | byte (1) | Parameter 2            | ignored  |
| L     | byte (1) | Bytes in payload       | 0        |

#### Response

| Field   | Type     | Content          | Note                            |
| ------- | -------- | ---------------- | ------------------------------- |
| CLA     | byte (1) | Test Mode        | 0xFF means test mode is enabled |
| MAJOR   | byte (1) | Version Major    |                                 |
| MINOR   | byte (1) | Version Minor    |                                 |
| PATCH   | byte (1) | Version Patch    |                                 |
| LOCKED  | byte (1) | Device is locked |                                 |
| TARGET_ID  | byte (4) | Device ID     | Identifier for NanoS/SP/X/ or Stax  |
| SW1-SW2 | byte (2) | Return code      | see list of return codes        |

--------------

### INS_GET_ADDR

#### Command

| Field      | Type           | Content                        | Expected       |
| ---------- | -------------- | ------------------------------ | -------------- |
| CLA        | byte (1)       | Application Identifier         | 0x55           |
| INS        | byte (1)       | Instruction ID                 | 0x04           |
| P1         | byte (1)       | Display address/path on device | 0x00 No        |
|            |                |                                | 0x01 Yes       |
| P2         | byte (1)       | Parameter 2                    | ignored        |
| L          | byte (1)       | Bytes in payload               | (depends)      |
| HRP_LEN    | byte(1)        | Bech32 HRP Length              | 1<=HRP_LEN<=83 |
| HRP        | byte (HRP_LEN) | Bech32 HRP                     |                |
| Path[0]    | byte (4)       | Derivation Path Data           | 44             |
| Path[1]    | byte (4)       | Derivation Path Data           | 118 / 60 / declared |
| Path[2]    | byte (4)       | Derivation Path Data           | ?              |
| Path[3]    | byte (4)       | Derivation Path Data           | ?              |
| Path[4]    | byte (4)       | Derivation Path Data           | ?              |

First three items in the derivation path will be hardened automatically hardened

##### Accepted coin types and HRPs

`Path[1]` accepts 118 (the generic Cosmos path), 60 (the Ethereum-style family) and any coin type
declared in the app's chain configuration — currently 1200 (Gonka). Any other value is rejected with
`APDU_CODE_INVALID_HD_PATH_COIN_VALUE`.

The HRP is then resolved against that same configuration:

- 118 accepts any HRP, except one declared for a coin type of its own — a chain that owns a coin
  type is reachable only through it, so it has a single account rather than one per path.
- Every other coin type — 60 and any coin type the chain configuration declares — accepts only the
  HRP declared for it.

Any other pair is rejected with `APDU_CODE_CHAIN_CONFIG_NOT_SUPPORTED`.

#### Response

| Field   | Type      | Content               | Note                     |
| ------- | --------- | --------------------- | ------------------------ |
| PK      | byte (33) | Compressed Public Key |                          |
| ADDR    | byte (65) | Bech 32 addr          |                          |
| SW1-SW2 | byte (2)  | Return code           | see list of return codes |

### INS_SIGN

#### Command

| Field | Type     | Content                | Expected  |
| ----- | -------- | ---------------------- | --------- |
| CLA   | byte (1) | Application Identifier | 0x55      |
| INS   | byte (1) | Instruction ID         | 0x02      |
| P1    | byte (1) | Payload desc           | 0 = init  |
|       |          |                        | 1 = add   |
|       |          |                        | 2 = last  |
| P2    | byte (1) | Transaction Format     | 0 = json  |
|       |          |                        | 1 = textual |
| L     | byte (1) | Bytes in payload       | (depends) |

The first packet/chunk includes only the derivation path and HRP.
Sending the HRP is optional on the generic Cosmos path (118) only, where it defaults to `cosmos`; it
will be mandatory in a future version. On every other coin type — 60 and any coin type the chain
configuration declares — the HRP names the chain being signed for, so omitting it is rejected with
`APDU_CODE_INVALID_HD_PATH_COIN_VALUE`.

All other packets/chunks should contain message to sign

*First Packet*

| Field      | Type     | Content                | Expected  |
| ---------- | -------- | ---------------------- | --------- |
| Path[0]    | byte (4)       | Derivation Path Data           | 44             |
| Path[1]    | byte (4)       | Derivation Path Data           | 118 / 60 / declared |
| Path[2]    | byte (4)       | Derivation Path Data           | ?              |
| Path[3]    | byte (4)       | Derivation Path Data           | ?              |
| Path[4]    | byte (4)       | Derivation Path Data           | ?              |
| HRP_LEN    | byte(1)        | Bech32 HRP Length              | 1<=HRP_LEN<=83 |
| HRP        | byte (HRP_LEN) | Bech32 HRP                     |                |

*Other Chunks/Packets*

| Field   | Type     | Content         | Expected |
| ------- | -------- | --------------- | -------- |
| Message | bytes... | Message to Sign |          |

#### Response

| Field   | Type      | Content     | Note                     |
| ------- | --------- | ----------- | ------------------------ |
| SIG     | byte (variable) | Signature   |                          |
| SW1-SW2 | byte (2)  | Return code | see list of return codes |

The signature data is DER encoded. The returned bytes have the following structure.

```
0x30 <length of whole message> <0x02> <length of R> <R> 0x2 <length of S> <S>
```

--------------
