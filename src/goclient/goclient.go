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
	"encoding/hex"
	"fmt"
	secp256k1 "github.com/btcsuite/btcd/btcec"
	sdk "github.com/cosmos/cosmos-sdk/types"
	"github.com/cosmos/cosmos-sdk/x/auth"
	"github.com/cosmos/cosmos-sdk/x/bank"
	"github.com/cosmos/cosmos-sdk/x/ibc"
	"github.com/cosmos/cosmos-sdk/x/stake"
	"github.com/tendermint/ed25519"
	"github.com/tendermint/tendermint/crypto"
	"github.com/zondax/ledger-goclient"
	"os"
	"strconv"
)

func PrintSampleFunc(message bank.MsgSend, output string) {

	res := message.GetSignBytes()

	if output == "binary" {
		fmt.Print(res)
	} else if output == "text" {
		fmt.Print(string(res))
	}
}

func ParseArgs(numberOfSamples int) (int, string, int) {
	argsWithoutProg := os.Args[1:]
	if len(argsWithoutProg) < 2 {
		fmt.Println("Not enought arguments")
		fmt.Printf("USAGE: samples SampleNumber[0-%d] OutputType[\"binary|text\"]\n", numberOfSamples-1)

		return 0, "", -1
	}
	sampleIndex, err := strconv.Atoi(argsWithoutProg[0])
	if err != nil {
		fmt.Println("Could not parse argument " + err.Error())
		return 0, "", -1
	}
	if sampleIndex < 0 && sampleIndex >= numberOfSamples {
		fmt.Printf("Number must be betweem 0-%d\n", numberOfSamples-1)
		return 0, "", -1
	}
	sampleOutput := argsWithoutProg[1]
	if sampleOutput != "binary" && sampleOutput != "text" {
		fmt.Println("Wrong OutputType, only binary|text allowed")
		return 0, "", -1
	}
	return sampleIndex, sampleOutput, 0
}

func GetExampleTxs() []auth.StdSignMsg {
	return []auth.StdSignMsg{

		auth.StdSignMsg{"test-chain-1", int64(0), int64(1), auth.NewStdFee(10000, sdk.Coin{"photon", sdk.NewInt(5)}), []sdk.Msg{bank.MsgSend{
			Inputs: []bank.Input{
				{
					Address: sdk.AccAddress([]byte("input")),
					Coins:   sdk.Coins{{"atom", sdk.NewInt(10)}},
				},
			},
			Outputs: []bank.Output{
				{
					Address: sdk.AccAddress([]byte("output")),
					Coins:   sdk.Coins{{"atom", sdk.NewInt(10)}},
				},
			},
		}}, "testmemo"},
		// Long message to test writing to nvram
		auth.StdSignMsg{"test-chain-1,test-chain-1,test-chain-1,test-chain-1,test-chain-1,test-chain-1", int64(0), int64(1), auth.NewStdFee(10000, sdk.Coin{"photonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphotonphoton", sdk.NewInt(5)}), []sdk.Msg{bank.MsgSend{
			Inputs: []bank.Input{
				{
					Address: sdk.AccAddress([]byte("input_1")),
					Coins:   sdk.Coins{{"atom_A", sdk.NewInt(10)}},
				},
				{
					Address: sdk.AccAddress([]byte("input_2")),
					Coins:   sdk.Coins{{"atom_B", sdk.NewInt(20)}},
				},
				{
					Address: sdk.AccAddress([]byte("input_3")),
					Coins:   sdk.Coins{{"atom_C", sdk.NewInt(30)}},
				},
				{
					Address: sdk.AccAddress([]byte("input_4")),
					Coins:   sdk.Coins{{"atom_D", sdk.NewInt(40)}},
				},
				{
					Address: sdk.AccAddress([]byte("input_5")),
					Coins:   sdk.Coins{{"atom_E", sdk.NewInt(50)}},
				},
			},
			Outputs: []bank.Output{
				{
					Address: sdk.AccAddress([]byte("output")),
					Coins:   sdk.Coins{{"atom", sdk.NewInt(10)}},
				},
				{
					Address: sdk.AccAddress([]byte("output")),
					Coins:   sdk.Coins{{"atom", sdk.NewInt(10)}},
				},
				{
					Address: sdk.AccAddress([]byte("output")),
					Coins:   sdk.Coins{{"atom", sdk.NewInt(10)}},
				},
				{
					Address: sdk.AccAddress([]byte("output")),
					Coins:   sdk.Coins{{"atom", sdk.NewInt(10)}},
				},
				{
					Address: sdk.AccAddress([]byte("output")),
					Coins:   sdk.Coins{{"atom", sdk.NewInt(10)}},
				},
			},
		}}, "testmemo"},
		auth.StdSignMsg{"test-chain-2", int64(0), int64(2), auth.NewStdFee(10000, sdk.Coin{"photon", sdk.NewInt(10)}), []sdk.Msg{stake.MsgBeginUnbonding{
			DelegatorAddr: sdk.AccAddress([]byte("delegator")),
			ValidatorAddr: sdk.AccAddress([]byte("candidate")),
			SharesAmount:  sdk.NewRat(100),
		}}, "testmemo"},
		auth.StdSignMsg{"test-chain-3", int64(1), int64(3), auth.NewStdFee(5000, sdk.Coin{"photon", sdk.NewInt(25)}), []sdk.Msg{ibc.IBCTransferMsg{ibc.IBCPacket{
			SrcAddr:   sdk.AccAddress([]byte("source")),
			DestAddr:  sdk.AccAddress([]byte("dest")),
			Coins:     sdk.Coins{sdk.Coin{"steak", sdk.NewInt(5)}},
			SrcChain:  "cosmos-hub",
			DestChain: "peggy",
		}}}, "testmemo"},
	}
}

func testTendermintED25519(messages []bank.MsgSend, ledger *ledger_goclient.Ledger) {
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

	for i := 0; i < len(messages); i++ {
		fmt.Printf("\nMessage %d - Please Sign..\n", i)
		message := messages[i].GetSignBytes()

		fmt.Printf("Private key=%s\n", hex.EncodeToString(privateKey[:32]))
		pubKey := ed25519.MakePublicKey(&privateKey)
		fmt.Printf("Public key=%s\n", hex.EncodeToString(privateKey[32:]))
		fmt.Printf("Public key=%s\n", hex.EncodeToString(pubKey[:]))

		// Print ledger test public key for comparison only
		pubKeyEd, _ := ledger.TestGetPublicKeyED25519()
		fmt.Printf("ED Public key=%s\n", hex.EncodeToString(pubKeyEd))

		signature := ed25519.Sign(&privateKey, message)
		verified := ed25519.Verify(pubKey, message, signature)
		if !verified {
			fmt.Printf("[VerifySig] Error verifying signature\n")
			os.Exit(1)
		}

		fmt.Printf("Message %d - Valid signature\n", i)
	}
}

func testED25519(messages []auth.StdSignMsg, ledger *ledger_goclient.Ledger) {
	//// Now the same with ed25519
	for i := 0; i < len(messages); i++ {
		fmt.Printf("\nMessage %d - Please Sign..\n", i)
		message := messages[i].Bytes()

		path := []uint32{44, 60, 0, 0, 0}

		fmt.Printf("\nMessage: %s\n", message)

		pubKey, err := ledger.GetPublicKeyED25519(path)
		if err != nil {
			fmt.Printf("[GetPK] Error: %s\n", err)
			os.Exit(1)
		}
		if len(pubKey) != ed25519.PublicKeySize {
			fmt.Printf("[Sign] Invalid public key size\n")
			os.Exit(1)
		}

		signature, err := ledger.SignED25519(path, message)
		if err != nil {
			fmt.Printf("[Sign] Error: %s\n", err)
			os.Exit(1)
		}
		if len(signature) != ed25519.SignatureSize {
			fmt.Printf("[Sign] Invalid signature size\n")
			os.Exit(1)
		}

		var __pub [ed25519.PublicKeySize]byte
		var __signature [ed25519.SignatureSize]byte
		copy(__pub[:], pubKey[0:32])
		copy(__signature[:], signature[0:64])

		verified := ed25519.Verify(&__pub, message, &__signature)
		if !verified {
			fmt.Printf("[VerifySig] Error verifying signature\n")
			os.Exit(1)
		}

		fmt.Printf("Message %d - Valid signature\n", i)
	}
}

func testSECP256K1Message(message []byte, ledger *ledger_goclient.Ledger, i int) {

	path := []uint32{44, 60, 0, 0, 0}

	fmt.Printf("\nMessage: %s\n", message)

	signature, err := ledger.SignSECP256K1(path, message)
	if err != nil {
		fmt.Printf("[Sign] Error: %s\n", err)
		return
	}

	pubKey, err := ledger.GetPublicKeySECP256K1(path)

	if err != nil {
		fmt.Printf("[GetPK] Error: %s\n", err)
		return
	}

	pub__, err := secp256k1.ParsePubKey(pubKey[:], secp256k1.S256())
	if err != nil {
		fmt.Printf("[ParsePK] Error: %s\n", err)
		return
	}

	sig__, err := secp256k1.ParseDERSignature(signature[:], secp256k1.S256())
	if err != nil {
		fmt.Printf("[ParseSig] Error: %s\n", err)
		return
	}

	verified := sig__.Verify(crypto.Sha256(message), pub__)
	if !verified {
		fmt.Printf("[VerifySig] Error verifying signature\n", err)
		return
	}

	fmt.Printf("Message %d - Valid signature\n", i)
}
func testSECP256K1(messages []auth.StdSignMsg, ledger *ledger_goclient.Ledger) {
	fmt.Printf("\nTesting valid messages:\n")
	for i := 0; i < len(messages); i++ {
		fmt.Printf("\nMessage %d - Please Sign..\n", i)
		message := messages[i].Bytes()
		testSECP256K1Message(message, ledger, i)
	}

	fmt.Printf("\nTesting invalid messages:\n")
	var invalid_msg_whitespaces = []byte("{\"sequence\":1,\"alt_bytes\"   :    null,\"chain_id\":\"test-chain-1\",\"fee_bytes\":{\"amount\":[{\"amount\":5,\"denom\":\"photon\"}],\"gas\":10000},\"msg_bytes\":{\"inputs\":[{\"address\":\"696E707574\",\"coins\":[{\"amount\":10,\"denom\":\"atom\"}]}],\"outputs\":[{\"address\":\"6F7574707574\",\"coins\":[{\"amount\":10,\"denom\":\"atom\"}]}]}}")
	testSECP256K1Message(invalid_msg_whitespaces, ledger, 0)
	var invalid_msg_not_sorted_dictionaries = []byte("{\"sequence\":1,\"alt_bytes\":null,\"chain_id\":\"test-chain-1\",\"fee_bytes\":{\"amount\":[{\"amount\":5,\"denom\":\"photon\"}],\"gas\":10000},\"msg_bytes\":{\"inputs\":[{\"address\":\"696E707574\",\"coins\":[{\"amount\":10,\"denom\":\"atom\"}]}],\"outputs\":[{\"address\":\"6F7574707574\",\"coins\":[{\"amount\":10,\"denom\":\"atom\"}]}]}}")
	testSECP256K1Message(invalid_msg_not_sorted_dictionaries, ledger, 1)
}

func main() {
	ledger, err := ledger_goclient.FindLedger()

	if err != nil {
		fmt.Printf("Error: %s", err.Error())
		fmt.Printf("\nUSB devices found:\n")
		ledger_goclient.ListDevices()
	} else {
		ledger.Logging = true

		testSECP256K1(GetExampleTxs(), ledger)
		//testED25519(GetExampleTxs(), ledger)
	}
}
