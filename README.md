# Ledger Cosmos app
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)
[![CircleCI](https://circleci.com/gh/Zondax/ledger-cosmos.svg?style=shield)](https://circleci.com/gh/Zondax/ledger-cosmos)
[![CodeFactor](https://www.codefactor.io/repository/github/zondax/ledger-cosmos/badge)](https://www.codefactor.io/repository/github/zondax/ledger-cosmos)

This repository contains:

- Ledger Nano S/X Cosmos app
- Specs / Documentation
- C++ unit tests

## Installing

### Cosmos app

The Cosmos app is already available in [Ledger Live](https://www.ledger.com/pages/ledger-live). Our preferred and recommended hardware wallet!

- Open Ledger Live and go to Settings (gear icon on the right):

![](docs/img/cosmos_app1.png)

- Enable developer mode (last option):

![](docs/img/cosmos_app2.png)

- Now go back to manager and search for Cosmos:

![](docs/img/cosmos_app3.png)

# Building

**Please only use a TEST DEVICE!**

**We strongly recommend using Linux as your development environment.**

## Get source
Apart from cloning, be sure you get all the submodules:
```
git submodule update --init --recursive
```

## Dependencies

#### Ledger Nano S

- This project requires Ledger firmware 1.6

- The current repository keeps track of Ledger's SDK but it is possible to override it by changing the git submodule.

#### Docker CE

- Please install docker CE. The instructions can be found here: https://docs.docker.com/install/

#### Ubuntu Dependencies
- Install the following packages:
   ```
   sudo apt update && apt-get -y install build-essential git wget cmake \
  libssl-dev libgmp-dev autoconf libtool
   ```

#### Other dependencies

- You need Python 3. In most cases, `make deps` will be able to install all additional dependencies:

   ```bash
   make deps
   ```

- You also need to install [Conan](https://conan.io/)

   ```bash
   pip install conan
   ```

*Warning*: Some IDEs may not use the same python interpreter or virtual enviroment as the one you used when running `pip`.
If you see conan is not found, check that you installed the package in the same interpreter as the one that launches `cmake`.

# Prepare your development device

   **Please do not use a Ledger device with funds for development purposes.**

   **Have a second device that is used ONLY for development and testing**

   There are a few additional steps that increase reproducibility and simplify development:

**1 - Ensure your device works in your OS**
- In Linux hosts it might be necessary to adjust udev rules, etc. Refer to Ledger documentation: https://support.ledger.com/hc/en-us/articles/115005165269-Fix-connection-issues

**2 - Set a test mnemonic**

All our tests expect the device to be configured with a known test mnemonic.

- Plug your device while pressing the right button

- Your device will show "Recovery" in the screen

- Double click

- Run `make dev_init`. This will take about 2 minutes. The device will be initialized to:

   ```
   PIN: 5555
   Mnemonic: equip will roof matter pink blind book anxiety banner elbow sun young
   ```

**3 - Add a development certificate**

- Plug your device while pressing the right button

- Your device will show "Recovery" in the screen

- Click both buttons at the same time

- Enter your pin if necessary

- Run `make dev_ca`. The device will receive a development certificate to avoid constant manual confirmations.


# Building the Ledger App

The Makefile will build the firmware in a docker container and leave the binary in the correct directory.

- Build

   The following command will build the app firmware inside a container and load to your device:
   ```
   make                # Builds the app
   ```

- Upload to a device
   The following command will upload the application to the ledger. _Warning: The application will be deleted before uploading._
   ```
   make load          # Builds and loads the app to the device
   ```

# Development (building C++ Code / Tests)

This is useful when you want to make changes to libraries, run unit tests, etc. It will build all common libraries and unit tests.

## Building unit tests
While we recommend you configure your preferred development environment, the minimum steps are as follows:

   ```
   mkdir build
   cd build
   cmake .. && make
   ```
   **Run unit tests**
   ```
   export GTEST_COLOR=1 && ctest -VV
   ```

## Specifications

- [APDU Protocol](docs/APDUSPEC.md)
  - [Transaction format](docs/TXSPEC.md)
