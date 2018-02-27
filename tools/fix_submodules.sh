#!/usr/bin/env bash
SCRIPT_DIR=$(cd `dirname $0` && pwd)
rm -rf .git/modules
rm -rf deps/*
git submodule update --init --recursive
