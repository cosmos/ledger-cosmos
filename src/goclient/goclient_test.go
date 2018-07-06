/*******************************************************************************
*   (c) 2018 ZondaX GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

// A simple command line tool that outputs json messages representing transactions
// Usage: samples [0-3] [binary|text]
// Note: Use build_samples.sh script to update correctly update dependencies

package main

import (
	"bytes"
	"encoding/hex"
	"fmt"
	secp256k1 "github.com/btcsuite/btcd/btcec"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"github.com/tendermint/ed25519"
	"github.com/tendermint/tendermint/crypto"
	"github.com/zondax/ledger-goclient"
	"testing"
)

//---------------------------------------------------------------
// TESTS
//---------------------------------------------------------------
func Get_Ledger(t *testing.T) (ledger *ledger_goclient.Ledger) {
	ledger, err := ledger_goclient.FindLedger()

	require.Nil(t, err, "Detected error, err: %s\n", err)
	require.NotNil(t, ledger, "Ledger is null")
	return ledger
}

func Test_ListDevices(t *testing.T) {
	ledger_goclient.ListDevices()
}

func Test_FindLedger(t *testing.T) {
	Get_Ledger(t)
}

func Test_LedgerVersion(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true
	version, err := ledger.GetVersion()
	require.Nil(t, err, "Detected error")
	assert.Equal(t, uint8(0xFF), version.AppId, "TESTING MODE NOT ENABLED")
	assert.Equal(t, uint8(0x0), version.Major, "Wrong Major version")
	assert.Equal(t, uint8(0x0), version.Minor, "Wrong Minor version")
	assert.Equal(t, uint8(0x9), version.Patch, "Wrong Patch version")
}

func Test_LedgerSHA256(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	input := []byte{0x56, 0x57, 0x58}
	expected := crypto.Sha256(input)
	answer, err := ledger.Hash(input)

	require.Nil(t, err, "Detected error, err: %s\n", err)
	assert.True(
		t,
		bytes.Equal(expected, answer),
		"unexpected response: %x, expected: %x\n", answer, expected)
}

func Test_LedgerSHA256Chunks(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	const input_size = 600
	input := make([]byte, input_size)
	for i := 0; i < input_size; i++ {
		input[i] = byte(i % 100)
	}
	expected := crypto.Sha256(input)
	answer, err := ledger.Hash(input)

	require.Nil(t, err, "Detected error, err: %s\n", err)
	assert.True(
		t,
		bytes.Equal(expected, answer),
		"unexpected response: %x, expected: %x\n", answer, expected)
}

func Test_PublicKeySECP256K1(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	path := []uint32{44, 60, 0, 0, 0}
	pubKey, err := ledger.GetPublicKeySECP256K1(path)

	require.Nil(t, err, "Detected error, err: %s\n", err)
	assert.Equal(
		t,
		65,
		len(pubKey),
		"Public key has wrong length: %x, expected length: %x\n", pubKey, 65)

	_, err = secp256k1.ParsePubKey(pubKey[:], secp256k1.S256())
	require.Nil(t, err, "Error parsing public key err: %s\n", err)
}

func Test_PublicKeyED25519(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	path := []uint32{44, 60, 0, 0, 0}
	pubKey, err := ledger.GetPublicKeyED25519(path)

	require.Nil(t, err, "Detected error, err: %s\n", err)
	assert.Equal(
		t,
		32,
		len(pubKey),
		"Public key has wrong length: %x, expected length: %x\n", pubKey, 65)
}

func Test_FakePublicKeySECP256K1(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	pubKey, err := ledger.TestGetPublicKeySECP256K1()

	require.Nil(t, err, "Detected error, err: %s\n", err)
	assert.Equal(
		t,
		65,
		len(pubKey),
		"Public key has wrong length: %x, expected length: %x\n", pubKey, 65)

	_, err = secp256k1.ParsePubKey(pubKey[:], secp256k1.S256())
	require.Nil(t, err, "Error parsing public key err: %s\n", err)

	expectedPubKey, err := hex.DecodeString("04bf04526fb497bc22c345f14ff5969a7342d0459b7af1b2b0228e2f6f38f7aedb" +
		"9e2693e2cc8eeabb85ea71ec609edfd4f1a5b968404e33fdecc4ed244cfa55dc")
	require.Nil(t, err)

	assert.Equal(t, expectedPubKey, pubKey)
}

func Test_FakePublicKeyED25519(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	pubKey, err := ledger.TestGetPublicKeyED25519()

	require.Nil(t, err, "Detected error, err: %s\n", err)
	assert.Equal(
		t,
		32,
		len(pubKey),
		"Public key has wrong length: %x, expected length: %x\n", pubKey, 32)

	expectedPubKey, err := hex.DecodeString("6310a04a64842d764dcd1d0af325db65f67e95ad0fb30abd270a0ca0c40b2582")
	require.Nil(t, err)

	assert.Equal(t, expectedPubKey, pubKey)
}

func SignAndVerifySECP256K1(t *testing.T, ledger *ledger_goclient.Ledger, message []byte) {
	signature, err := ledger.TestSignSECP256K1(message)
	require.Nil(t, err, "Detected error during signing message in ledger, err: %s\n", err)

	pubKey, err := ledger.TestGetPublicKeySECP256K1()
	require.Nil(t, err, "Detected error getting public key from ledger, err: %s\n", err)

	pub__, err := secp256k1.ParsePubKey(pubKey[:], secp256k1.S256())
	require.Nil(t, err, "Error parsing public key", err)

	fmt.Printf("Size %d\n", len(signature))

	sig__, err := secp256k1.ParseDERSignature(signature[:], secp256k1.S256())
	require.Nil(t, err, "Error parsing ledger's signature using go's secp256k1 library, signedMsd=%x, err: %s\n", signature, err)

	verified := sig__.Verify(crypto.Sha256(message), pub__)
	require.True(t, verified, "Could not verify the signature=%x", signature)
}

func Test_LedgerSignAndVerifyMessageSECP256K1_Tiny(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	input := make([]byte, 1)
	for i := 0; i < 1; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerifySECP256K1(t, ledger, input)
}

func Test_LedgerSignAndVerifyMessageSECP256K1_Small(t *testing.T) {

	input := make([]byte, 10)
	for i := 0; i < 10; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerifySECP256K1(t, Get_Ledger(t), input)
}

func Test_LedgerSignAndVerifyMessageSECP256K1_Medium(t *testing.T) {

	input := make([]byte, 205)
	for i := 0; i < 205; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerifySECP256K1(t, Get_Ledger(t), input)
}

func Test_LedgerSignAndVerifyMessageSECP256K1_Big(t *testing.T) {

	input := make([]byte, 510)
	for i := 0; i < 510; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerifySECP256K1(t, Get_Ledger(t), input)
}

func SignAndVerifyED25519(t *testing.T, ledger *ledger_goclient.Ledger, message []byte) {
	signature, err := ledger.TestSignED25519(message)
	require.Nil(t, err, "Detected error during signing message in ledger, err: %s\n", err)

	pubKey, err := ledger.TestGetPublicKeyED25519()
	require.Nil(t, err, "Detected error getting public key from ledger, err: %s\n", err)

	require.Equal(t, len(pubKey), ed25519.PublicKeySize, "Invalid public key size")
	require.Equal(t, len(signature), ed25519.SignatureSize, "Invalid signature size")

	var __pub [ed25519.PublicKeySize]byte
	var __signature [ed25519.SignatureSize]byte
	copy(__pub[:], pubKey[0:32])
	copy(__signature[:], signature[0:64])

	require.True(t, ed25519.Verify(&__pub, message, &__signature),
		"Could not verify the signature=%x", signature)
}

func Test_LedgerSignAndVerifyMessageED25519_Tiny(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	input := make([]byte, 1)
	for i := 0; i < 1; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerifyED25519(t, ledger, input)
}

func Test_LedgerSignAndVerifyMessageED25519_Small(t *testing.T) {

	input := make([]byte, 10)
	for i := 0; i < 10; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerifyED25519(t, Get_Ledger(t), input)
}

func Test_LedgerSignAndVerifyMessageED25519_Medium(t *testing.T) {

	input := make([]byte, 205)
	for i := 0; i < 205; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerifyED25519(t, Get_Ledger(t), input)
}

func Test_LedgerSignAndVerifyMessageED25519_Big(t *testing.T) {

	input := make([]byte, 510)
	for i := 0; i < 510; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerifyED25519(t, Get_Ledger(t), input)
}

func Test_TestPublicKeyFromLedgerAndTendermint(t *testing.T) {

	// This is the same private key as the one hard-coded in ledger
	// This one is 64 byte which 32 bytes blob repeated twice
	// In ledger there is only 1 blob of 32 bytes
	// The reason we have 64 bytes is because tendermint APIs take 64 bytes
	// but only use first 32
	privateKey := [64]byte{
		0x75, 0x56, 0x0e, 0x4d, 0xde, 0xa0, 0x63, 0x05,
		0xc3, 0x6e, 0x2e, 0xb5, 0xf7, 0x2a, 0xca, 0x71,
		0x2d, 0x13, 0x4c, 0xc2, 0xa0, 0x59, 0xbf, 0xe8,
		0x7e, 0x9b, 0x5d, 0x55, 0xbf, 0x81, 0x3b, 0xd4,
		0x75, 0x56, 0x0e, 0x4d, 0xde, 0xa0, 0x63, 0x05,
		0xc3, 0x6e, 0x2e, 0xb5, 0xf7, 0x2a, 0xca, 0x71,
		0x2d, 0x13, 0x4c, 0xc2, 0xa0, 0x59, 0xbf, 0xe8,
		0x7e, 0x9b, 0x5d, 0x55, 0xbf, 0x81, 0x3b, 0xd4,
	}
	tendermintPubKey := ed25519.MakePublicKey(&privateKey)

	ledger := Get_Ledger(t)
	ledger.Logging = true
	ledgerPubKey, _ := ledger.TestGetPublicKeyED25519()

	require.True(t, bytes.Equal(tendermintPubKey[:], ledgerPubKey), "Public keys for the same private key should match between ledger and tendermint APIs")
}
