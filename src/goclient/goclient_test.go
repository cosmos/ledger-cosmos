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
	"bytes"
	"testing"
	"github.com/stretchr/testify/assert"
)

//---------------------------------------------------------------
// TESTS
//---------------------------------------------------------------
func Get_Ledger(t *testing.T) (ledger *ledger_goclient.Ledger) {
	ledger, err := ledger_goclient.FindLedger()
	assert.Nil(t, err, "Detected error, err: %s\n", err)
	assert.NotNil(t, ledger, "Ledger is null")
	return ledger
}

func Test_FindLedger(t *testing.T) {
	Get_Ledger(t)
}

func Test_LedgerVersion(t *testing.T) {
	ledger := Get_Ledger(t)
	version, err := ledger.GetVersion()
	assert.Nil(t, err, "Detected error")
	assert.Equal(t, version.AppId, uint8(0x55), "Wrong AppId version")
	assert.Equal(t, version.Major, uint8(0x0), "Wrong Major version")
	assert.Equal(t, version.Minor, uint8(0x0), "Wrong Minor version")
	assert.Equal(t, version.Patch, uint8(0x4), "Wrong Patch version")
}

func Test_LedgerShortEcho(t *testing.T) {
	ledger := Get_Ledger(t)

	input := []byte{0x56}
	expected := []byte{0x56}

	answer, err := ledger.Echo(input)
	assert.Nil(t, err, "Detected error, err: %s\n", err)
	assert.True(
		t,
		bytes.Equal(answer, expected),
		"unexpected response: %x, expected: %x\n", answer, expected)
}

func Test_LedgerEchoChunks(t *testing.T) {
	ledger := Get_Ledger(t)

	input := []byte{0x56}
	expected := []byte{0x56}

	answer, err := ledger.Echo(input)
	assert.Nil(t, err, "Detected error, err: %s\n", err)
	assert.True(
		t,
		bytes.Equal(answer, expected),
		"unexpected response: %x, expected: %x\n", answer, expected)

	input = make([]byte, 500)
	for i := 0; i < 500; i++ {
		input[i] = byte(i%100)
	}
	answer, err = ledger.Echo(input)

	assert.Nil(t, err, "Detected error, err: %s\n", err)
	assert.True(
		t,
		bytes.Equal(answer, input[:64]),
		"unexpected response: %x, expected: %x\n", answer, input[:64])
}

func Test_LedgerSHA256(t *testing.T) {
	ledger := Get_Ledger(t)
	input := []byte{0x56, 0x57, 0x58}
	expected := crypto.Sha256(input)
	answer, err := ledger.Hash(input)

	assert.Nil(t, err, "Detected error, err: %s\n", err)
	assert.True(
		t,
		bytes.Equal(answer, expected),
		"unexpected response: %x, expected: %x\n", answer, expected)
}

func Test_LedgerSHA256Chunks(t *testing.T) {
	ledger := Get_Ledger(t)
	const input_size = 600
	input := make([]byte, input_size)
	for i := 0; i < input_size; i++ {
		input[i] = byte(i%100)
	}
	expected := crypto.Sha256(input)
	answer, err := ledger.Hash(input)

	assert.Nil(t, err, "Detected error, err: %s\n", err)
	assert.True(
		t,
		bytes.Equal(answer, expected),
		"unexpected response: %x, expected: %x\n", answer, expected)
}

func Test_LedgerPublicKey(t *testing.T) {
	ledger := Get_Ledger(t)
	pubKey, err := ledger.GetTestPublicKey()

	assert.Nil(t, err, "Detected error, err: %s\n", err)
	assert.Equal(
		t,
		len(pubKey),
		65,
		"Public key has wrong length: %x, expected length: %x\n", pubKey, 65)
}