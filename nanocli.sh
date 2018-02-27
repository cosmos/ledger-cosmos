#!/usr/bin/env bash

SCRIPT_DIR=$(cd `dirname $0` && pwd)

handle_config()
{
    os_string="$(uname -s)"
    case "${os_string}" in
        Linux*)
            sudo apt-get install libusb-1.0.0 libudev-dev
            pip2 install --upgrade setuptools
            pip2 install -U ledgerblue==0.1.15 ecpy==0.8.2
            ;;
        Darwin*)
            brew install libusb
            pip install --upgrade setuptools --user python
            pip install -U ledgerblue==0.1.15 ecpy==0.8.2 --user python
            ;;
        *)
            echo "OS not recognized"
            ;;
    esac

}

handle_make()
{
    # This function works in the scope of the container
    DOCKER_IMAGE=zondax/builder_bolos
    BOLOS_SDK=/project/deps/nanos-secure-sdk
    BOLOS_ENV=/opt/bolos

    docker run -it --rm \
            -e BOLOS_SDK=${BOLOS_SDK} \
            -e BOLOS_ENV=${BOLOS_ENV} \
            -e USERID=$UID \
            -u `id -u` \
            -v $(pwd):/project \
            ${DOCKER_IMAGE} \
            make -C /project/src/ledger $1
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
    make)       handle_make $2;;
    config)     handle_config;;
    load)       handle_load;;
    delete)     handle_delete;;
    upgrade)    handle_delete && handle_load;;
    *)
        echo "ERROR. Valid commands: make, config, load, delete"
        ;;
esac
