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
	"github.com/tendermint/go-crypto"
	"github.com/zondax/ledger-goclient"
	secp256k1 "github.com/btcsuite/btcd/btcec"
	"bytes"
	"testing"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"encoding/hex"
	"fmt"
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
	assert.Equal(t, uint8(0x6), version.Patch, "Wrong Patch version")
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

func Test_LedgerPublicKeyReal(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	path := []uint32{44, 60, 0, 0, 0}
	pubKey, err := ledger.GetPublicKey(path)

	require.Nil(t, err, "Detected error, err: %s\n", err)
	assert.Equal(
		t,
		65,
		len(pubKey),
		"Public key has wrong length: %x, expected length: %x\n", pubKey, 65)

	_, err = secp256k1.ParsePubKey(pubKey[:], secp256k1.S256())
	require.Nil(t, err, "Error parsing public key err: %s\n", err)
}

func Test_LedgerPublicKeyTest(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	pubKey, err := ledger.GetTestPublicKey()

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

func SignAndVerify(t *testing.T, ledger *ledger_goclient.Ledger, message []byte) {
	signedMsg, err := ledger.SignTest(message)
	require.Nil(t, err, "Detected error during signing message in ledger, err: %s\n", err)

	pubKey, err := ledger.GetTestPublicKey()
	require.Nil(t, err, "Detected error getting public key from ledger, err: %s\n", err)

	pub__, err := secp256k1.ParsePubKey(pubKey[:], secp256k1.S256())
	require.Nil(t, err, "Error parsing public key", err)

	fmt.Printf("Size %d\n", len(signedMsg))

	sig__, err := secp256k1.ParseDERSignature(signedMsg[:], secp256k1.S256())
	require.Nil(t, err, "Error parsing ledger's signature using go's secp256k1 library, signedMsd=%x, err: %s\n", signedMsg, err)

	verified := sig__.Verify(crypto.Sha256(message), pub__)
	require.True(t, verified, "Could not verify the signature, signedMsd=%x", signedMsg)
}

func Test_LedgerSignAndVerifyMessage_Tiny(t *testing.T) {
	ledger := Get_Ledger(t)
	ledger.Logging = true

	input := make([]byte, 1)
	for i := 0; i < 1; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerify(t, ledger, input)
}

func Test_LedgerSignAndVerifyMessage_Small(t *testing.T) {

	input := make([]byte, 10)
	for i := 0; i < 10; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerify(t, Get_Ledger(t), input)
}

func Test_LedgerSignAndVerifyMessage_Medium(t *testing.T) {

	input := make([]byte, 205)
	for i := 0; i < 205; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerify(t, Get_Ledger(t), input)
}

func Test_LedgerSignAndVerifyMessage_Big(t *testing.T) {

	input := make([]byte, 510)
	for i := 0; i < 510; i++ {
		input[i] = byte(i % 255)
	}

	SignAndVerify(t, Get_Ledger(t), input)
}
