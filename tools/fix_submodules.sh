#!/usr/bin/env bash
SCRIPT_DIR=$(cd $(dirname $0) && pwd)
rm -rf ${SCRIPT_DIR}/../.git/modules
rm -rf ${SCRIPT_DIR}/../deps/*
git submodule update --init --recursive
