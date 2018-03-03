#!/usr/bin/env bash
go get "github.com/tendermint/go-crypto"
go get "github.com/cosmos/cosmos-sdk/types"
git --git-dir=$GOPATH/src/github.com/cosmos/cosmos-sdk/.git --work-tree=$GOPATH/src/github.com/cosmos/cosmos-sdk/ checkout remotes/origin/adrian/tx_encoding
git --git-dir=$GOPATH/src/github.com/tendermint/go-crypto/.git --work-tree=$GOPATH/src/github.com//tendermint/go-crypto/ checkout develop
go build samples.go