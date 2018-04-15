#!/usr/bin/env bash
go get "github.com/tendermint/go-crypto"
go get "github.com/cosmos/cosmos-sdk/types"
go get "github.com/zondax/ledger-goclient"
git --git-dir=$GOPATH/src/github.com/cosmos/cosmos-sdk/.git --work-tree=$GOPATH/src/github.com/cosmos/cosmos-sdk/ checkout master
git --git-dir=$GOPATH/src/github.com/tendermint/go-crypto/.git --work-tree=$GOPATH/src/github.com//tendermint/go-crypto/ checkout master
go build samples.go