#!/usr/bin/env bash

SCRIPT_DIR=$(cd `dirname $0` && pwd)

handle_config()
{
    os_string="$(uname -s)"
    case "${os_string}" in
        Linux*)
            apt-get install libusb-1.0.0
            apt-get install libudev-dev
            ;;
        Darwin*)
            brew install python3
            brew install libusb
            ;;
        *)
            echo "OS not recognized"
            ;;
    esac

    pip3 install ledgerblue==0.1.15
}

handle_build()
{
    # This function works in the scope of the container
    DOCKER_IMAGE=zondax/builder_bolos
    BOLOS_SDK=/root/project/deps/nanos-secure-sdk
    BOLOS_ENV=/opt/bolos

    docker run -it --rm \
            -e BOLOS_SDK=${BOLOS_SDK} \
            -e BOLOS_ENV=${BOLOS_ENV} \
            -v $(pwd):/root/project \
            ${DOCKER_IMAGE} \
            make -C /root/project/src/ledger $1
}

handle_load()
{
    # This function works in the scope of the host
    export BOLOS_SDK=${SCRIPT_DIR}/deps/nanos-secure-sdk
    export BOLOS_ENV=/opt/bolos
    make -C ${SCRIPT_DIR}/src/ledger load
}

handle_delete()
{
    # This function works in the scope of the host
    export BOLOS_SDK=${SCRIPT_DIR}/deps/nanos-secure-sdk
    export BOLOS_ENV=/opt/bolos
    make -C ${SCRIPT_DIR}/src/ledger delete
}

case "$1" in
    build)      handle_build;;
    config)     handle_config;;
    load)       handle_load;;
    delete)     handle_delete;;
    upgrade)    handle_delete && handle_load;;
    *)
        echo "ERROR. Valid commands: make, config, load, delete"
        ;;
esac
