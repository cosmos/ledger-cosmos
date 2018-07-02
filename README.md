# Ledger-cosmos
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![CircleCI](https://circleci.com/gh/cosmos/ledger-cosmos/tree/master.svg?style=shield)](https://circleci.com/gh/cosmos/ledger-cosmos/tree/master)

This is a prototype of the Ledger Nano S app for Tendermint/Cosmos. 

It is work in progress and subject to further modifications and testing.

**WARNING: DO NOT USE THIS APP IN A LEDGER WITH REAL FUNDS.**

This repo includes:

- Ledger Nano S app/firmware
- C++ unit tests
- Golang client test

# Demo Firmware

Continuous integration generates a demo.zip. This includes firmware plus a shell script that installs the firmware. 

**WARNING**: Again! Remember to use this ONLY in a ledger without fund and only for test purposes.

# More info

[Build instructions](./docs/BUILD.md)

**Specifications**

- [APDU Protocol](./docs/PROTOSPEC.md)
- [Transaction format](./docs/TXSPEC.md)
- [User interface](./docs/UISPEC.md)
