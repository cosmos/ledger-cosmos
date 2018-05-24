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
	"fmt"
	secp256k1 "github.com/btcsuite/btcd/btcec"
	sdk "github.com/cosmos/cosmos-sdk/types"
	"github.com/cosmos/cosmos-sdk/x/bank"
	"github.com/cosmos/cosmos-sdk/x/ibc"
	"github.com/cosmos/cosmos-sdk/x/stake"
	"github.com/tendermint/ed25519"
	"github.com/tendermint/go-crypto"
	"github.com/zondax/ledger-goclient"
	"os"
	"strconv"
	"encoding/hex"
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

func GetExampleTxs() []sdk.StdSignMsg {
	return []sdk.StdSignMsg{
		sdk.StdSignMsg{"test-chain-1", []int64{1}, sdk.NewStdFee(10000, sdk.Coin{"photon", 5}), bank.MsgSend{
			Inputs: []bank.Input{
				{
					Address: crypto.Address([]byte("input")),
					Coins:   sdk.Coins{{"atom", 10}},
				},
			},
			Outputs: []bank.Output{
				{
					Address: crypto.Address([]byte("output")),
					Coins:   sdk.Coins{{"atom", 10}},
				},
			},
		}},
		sdk.StdSignMsg{"test-chain-2", []int64{2}, sdk.NewStdFee(10000, sdk.Coin{"photon", 10}), stake.MsgUnbond{
			DelegatorAddr: sdk.Address([]byte("delegator")),
			CandidateAddr: sdk.Address([]byte("candidate")),
			Shares:        "100",
		}},
		sdk.StdSignMsg{"test-chain-3", []int64{3}, sdk.NewStdFee(5000, sdk.Coin{"photon", 25}), ibc.IBCTransferMsg{ibc.IBCPacket{
			SrcAddr:   sdk.Address([]byte("source")),
			DestAddr:  sdk.Address([]byte("dest")),
			Coins:     sdk.Coins{sdk.Coin{"steak", 5}},
			SrcChain:  "cosmos-hub",
			DestChain: "peggy",
		}}},
	}
}

func GetMessages() []bank.MsgSend {
	return []bank.MsgSend{
		// Simple address, 1 input, 1 output
		bank.MsgSend{
			Inputs: []bank.Input{
				{
					Address: crypto.Address([]byte("input")),
					Coins:   sdk.Coins{{"atom", 10}},
					//Sequence: 1,
				},
			},
			Outputs: []bank.Output{
				{
					Address: crypto.Address([]byte("output")),
					Coins:   sdk.Coins{{"atom", 10}},
				},
			},
		},

		// Real public key, 1 input, 1 output
		bank.MsgSend{
			Inputs: []bank.Input{
				{
					Address: crypto.Address(crypto.GenPrivKeySecp256k1().PubKey().Bytes()),
					Coins:   sdk.Coins{{"atom", 1000000}},
					//Sequence: 1,
				},
			},
			Outputs: []bank.Output{
				{
					Address: crypto.Address(crypto.GenPrivKeySecp256k1().PubKey().Bytes()),
					Coins:   sdk.Coins{{"atom", 1000000}},
				},
			},
		},

		// Simple address, 2 inputs, 2 outputs
		bank.MsgSend{
			Inputs: []bank.Input{
				{
					Address: crypto.Address([]byte("input")),
					Coins:   sdk.Coins{{"atom", 10}},
					//Sequence: 1,
				},
				{
					Address: crypto.Address([]byte("anotherinput")),
					Coins:   sdk.Coins{{"atom", 50}},
					//Sequence: 1,
				},
			},
			Outputs: []bank.Output{
				{
					Address: crypto.Address([]byte("output")),
					Coins:   sdk.Coins{{"atom", 10}},
				},
				{
					Address: crypto.Address([]byte("anotheroutput")),
					Coins:   sdk.Coins{{"atom", 50}},
				},
			},
		},

		// Simple address, 2 inputs, 2 outputs, 2 coins
		bank.MsgSend{
			Inputs: []bank.Input{
				{
					Address: crypto.Address([]byte("input")),
					Coins:   sdk.Coins{{"atom", 10}, {"bitcoin", 20}},
					//Sequence: 1,
				},
				{
					Address: crypto.Address([]byte("anotherinput")),
					Coins:   sdk.Coins{{"atom", 50}, {"bitcoin", 60}, {"ethereum", 70}},
					//Sequence: 1,
				},
			},
			Outputs: []bank.Output{
				{
					Address: crypto.Address([]byte("output")),
					Coins:   sdk.Coins{{"atom", 10}, {"bitcoin", 20}},
				},
				{
					Address: crypto.Address([]byte("anotheroutput")),
					Coins:   sdk.Coins{{"atom", 50}, {"bitcoin", 60}, {"ethereum", 70}},
				},
			},
		},
	}
}

func GetStdSignMessages() ([]sdk.StdSignMsg) {
	return []sdk.StdSignMsg{

		sdk.StdSignMsg{
			ChainID:   "1",
			Sequences: []int64{2},
			Fee:       sdk.StdFee{Amount: sdk.Coins{sdk.Coin{Denom: "GBP", Amount: 100}, sdk.Coin{Denom: "GBP", Amount: 100}}, Gas: 2},
			Msg:       GetMessages()[0],
		}};
}

func testTendermintED25519(messages []bank.SendMsg, ledger *ledger_goclient.Ledger) {
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

func testED25519(messages []bank.MsgSend, ledger *ledger_goclient.Ledger) {
	//// Now the same with ed25519
	for i := 0; i < len(messages); i++ {
		fmt.Printf("\nMessage %d - Please Sign..\n", i)
		message := messages[i].GetSignBytes()

		path := []uint32{44, 60, 0, 0, 0}

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

func testSECP256K1(messages []bank.MsgSend, ledger *ledger_goclient.Ledger) {
	for i := 0; i < len(messages); i++ {
		fmt.Printf("\nMessage %d - Please Sign..\n", i)
		message := messages[i].GetSignBytes()
		fmt.Printf("[GetPK] Message: %s", message)

		path := []uint32{44, 60, 0, 0, 0}

		signature, err := ledger.SignSECP256K1(path, message)
		if err != nil {
			fmt.Printf("[Sign] Error: %s\n", err)
			os.Exit(1)
		}

		pubKey, err := ledger.GetPublicKeySECP256K1(path)

		if err != nil {
			fmt.Printf("[GetPK] Error: %s\n", err)
			os.Exit(1)
		}

		pub__, err := secp256k1.ParsePubKey(pubKey[:], secp256k1.S256())
		if err != nil {
			fmt.Printf("[ParsePK] Error: %s\n", err)
			os.Exit(1)
		}

		sig__, err := secp256k1.ParseDERSignature(signature[:], secp256k1.S256())
		if err != nil {
			fmt.Printf("[ParseSig] Error: %s\n", err)
			os.Exit(1)
		}

		verified := sig__.Verify(crypto.Sha256(message), pub__)
		if !verified {
			fmt.Printf("[VerifySig] Error verifying signature\n", err)
			os.Exit(1)
		}

		fmt.Printf("Message %d - Valid signature\n", i)
	}
}

func testSECP256K1_StdSignMsg(messages []sdk.StdSignMsg, ledger *ledger_goclient.Ledger) {
	for i := 0; i < len(messages); i++ {
		fmt.Printf("\nMessage %d - Please Sign..\n", i)
		message := messages[i].Bytes()

		path := []uint32{44, 60, 0, 0, 0}

		fmt.Printf("\nMessage: %s\n", message)
		signature, err := ledger.SignSECP256K1_StdSignMsg(path, message)
		if err != nil {
			fmt.Printf("[Sign] Error: %s\n", err)
			os.Exit(1)
		}

		pubKey, err := ledger.GetPublicKeySECP256K1(path)

		if err != nil {
			fmt.Printf("[GetPK] Error: %s\n", err)
			os.Exit(1)
		}

		pub__, err := secp256k1.ParsePubKey(pubKey[:], secp256k1.S256())
		if err != nil {
			fmt.Printf("[ParsePK] Error: %s\n", err)
			os.Exit(1)
		}

		sig__, err := secp256k1.ParseDERSignature(signature[:], secp256k1.S256())
		if err != nil {
			fmt.Printf("[ParseSig] Error: %s\n", err)
			os.Exit(1)
		}

		verified := sig__.Verify(crypto.Sha256(message), pub__)
		if !verified {
			fmt.Printf("[VerifySig] Error verifying signature\n", err)
			os.Exit(1)
		}

		fmt.Printf("Message %d - Valid signature\n", i)
	}
}

func main() {
	ledger, err := ledger_goclient.FindLedger()

	if err != nil {
		fmt.Printf("Error: %s", err.Error())
		fmt.Printf("\nUSB devices found:\n")
		ledger_goclient.ListDevices()
	} else {
		ledger.Logging = true

		// WORKING: Sign standard signature message using ledger (SECP256K1)
		//testSECP256K1_StdSignMsg(GetStdSignMessages(), ledger)

		// WORKING: Sign transaction message using ledger (SECP256K1)
		//testSECP256K1(GetMessages(), ledger)

		// WORKING: Sign transaction message using tendermint sdk (ED25519)
		testTendermintED25519(GetMessages(), ledger)

		// FIXME, sign transaction message using ledger (ED25519)
		//testED25519(GetMessages(), ledger)
	}
}
