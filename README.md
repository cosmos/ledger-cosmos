# ledger-cosmos

[![CircleCI](https://circleci.com/gh/cosmos/ledger-cosmos/tree/master.svg?style=svg)](https://circleci.com/gh/cosmos/ledger-cosmos/tree/master)

This project is work in progress. Some aspects are subject to change.

# Get source
Apart from cloning, be sure you get all submodules
```
git submodule update --init --recursive
```

# Dependencies
This project requires ledger firmware 1.4.1

## Ledger python tools

Ledger firmware 1.4.1 requires, ledgerblue 0.1.17. Unfortunately, the package is still not available in pypi. For this reason, it is necessary to install it directly from source. In most cases, `nanocli.sh` should be able to install all dependencies: 

```bash
./nanocli.sh config
```
It is possible that due to recent changes to the firmware/SDK some additional steps might be required.

## CircleCI CLI

Installing the CircleCI CLI is recommended. Follow the instructions here:

https://circleci.com/docs/2.0/local-cli/#installing-the-circleci-local-cli-on-macos-and-linux-distros

## Ubuntu
Install the following packages:
```
sudo apt update && apt-get -y install build-essential git sudo wget cmake libssl-dev libgmp-dev autoconf libtool
```

# Building

Depending on the purpose, there are different ways in which this project can be built.
## Local x64 code + tests
This is useful when you want to make changes to libraries, run unit tests, etc. It will build all common libraries and unit tests.

#### Compile
```
cmake . && make
```
#### Run unit tests
```
export GTEST_COLOR=1 && ctest -VV
```
## Continous Integration Image - Ubuntu 16.04
This is similar to the previous approach, however, it will build in a docker image identical to what CircleCI uses. This provides a clean, reproducible environment. It also can be helpful to debug CI issues.
```
circleci build
```

## BOLOS / Ledger firmware
This approach will build the cosmos' ledger app in a Docker image for BOLOS. The resulting firmware will be available in the host. This can be later uploaded as described below.
#### Build
The following command will build the app firmware inside a container. All output will be available to the host.
```
./nanocli.sh make
```
#### Delete app from ledger
The following command will delete the application from the ledger. 
```
./nanocli.sh delete
```
#### Load app to ledger
The following command will upload the application to the ledger. _Warning: The application must be deleted before uploading._
```
./nanocli.sh load
```

# Additional Scripts

There are a few additional scripts that could be useful.
#### Build json samples
Use go to build json test samples for different test scenarios. 
```
./tools/build_samples.sh
```
#### Python test script
In addition to unit tests, it is possible to run an integration test using this script.

- Build and upload the ledger app
- Start the ledger app
- Run the following command:
```
./tools/test.py
```
- The app should change to a "View/Sign transaction" mode.
