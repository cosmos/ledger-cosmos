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
	"os"
	"strconv"
	"github.com/tendermint/go-crypto"
	sdk "github.com/cosmos/cosmos-sdk/types"
	"github.com/cosmos/cosmos-sdk/x/bank"
	"github.com/zondax/ledger-goclient"
	"bytes"
)

func PrintSampleFunc(message bank.SendMsg, output string) {

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

func GetMessages() ([]bank.SendMsg) {
	return []bank.SendMsg{
		// Simple address, 1 input, 1 output
		bank.SendMsg{
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
		bank.SendMsg{
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
		bank.SendMsg{
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
		bank.SendMsg{
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


func get_checksum(buffer []byte) uint64 {
	var checksum uint64 = 0
	for _, element := range buffer {
		checksum += uint64(element)
	}
	return checksum
}

func main() {
	ledger, err := ledger_goclient.FindLedger()

	if err != nil {
		fmt.Printf("Ledger NOT Found\n")
		fmt.Print(err.Error())
	} else {
		ledger.Logging = true

		fmt.Printf("\n************ Version\n")
		version, err := ledger.GetVersion()
		if err != nil {
			fmt.Printf("Could not get version. Error: %s", err)
			os.Exit(1)
		}

		fmt.Printf("Ledger. Version %d.%d.%d\n", version.Major, version.Minor, version.Patch)

		fmt.Printf("\n************ Short Echo\n")

		input := []byte{0x56}
		expected := []byte{0x56}

		answer, err := ledger.Echo(input)
		if err == nil {
			if !bytes.Equal(answer, expected) {
				fmt.Fprintf(os.Stderr, "unexpected response: %x, expected: %x\n", answer, expected)
				os.Exit(1)
			}
		} else {
			fmt.Printf("Error: %s\n", err)
			os.Exit(1)
		}

		fmt.Printf("\n************ Chunked Echo\n")

		input = make([]byte, 500)
		for i := 0; i < 500; i++ {
			input[i] = byte(i%100)
		}

		answer, err = ledger.Echo(input)
		if err == nil {
			if !bytes.Equal(answer, input[:64]) {
				fmt.Fprintf(os.Stderr, "unexpected response: %x\n", answer)
				os.Exit(1)
			}
		} else {
			fmt.Printf("Error: %s\n", err)
			os.Exit(1)
		}

		fmt.Printf("\n************ SHA256\n")

		input = []byte{0x56, 0x57, 0x58}
		expected = crypto.Sha256(input)

		answer, err = ledger.Hash(input)
		if err == nil {
			if !bytes.Equal(answer, expected) {
				fmt.Fprintf(os.Stderr, "unexpected response: %x\n", answer)
				os.Exit(1)
			}
		} else {
			fmt.Printf("Error: %s\n", err)
			os.Exit(1)
		}

		fmt.Printf("\n************ SHA256 for chunks\n")

		const input_size = 600
		input = make([]byte, input_size)
		for i := 0; i < input_size; i++ {
			input[i] = byte(i%100)
		}
		expected = crypto.Sha256(input)

		answer, err = ledger.Hash(input)
		if err == nil {
			if !bytes.Equal(answer, expected) {
				fmt.Fprintf(os.Stderr, "unexpected response: %x\n", answer)
				os.Exit(1)
			}
		} else {
			fmt.Printf("Error: %s\n", err)
			os.Exit(1)
		}

		//fmt.Printf("\n************ GetPK\n")
		//
		//input = []byte{0x56, 0x57, 0x58}
		//expected = crypto.Sha256(input)
		//
		//answer, err = ledger.GetPKDummy()
		//if err == nil {
		//	if !bytes.Equal(answer, expected) {
		//		fmt.Fprintf(os.Stderr, "unexpected response: %x\n", answer)
		//		os.Exit(1)
		//	}
		//} else {
		//	fmt.Printf("Error: %s\n", err)
		//	os.Exit(1)
		//}

		fmt.Printf("\n************ GetPublicKey\n")

		pubKey, err := ledger.GetPublicKey()

		if err == nil {
			fmt.Printf("Public key for the derivation path 44'/60'/0'/0/0 equals %x\n", pubKey)
		} else {
			fmt.Printf("Error: %s\n", err)
			os.Exit(1)
		}

		fmt.Printf("\n************ Waiting for signature message 1..\n")

		messages := GetMessages()
		transactionData := messages[0].GetSignBytes()
		fmt.Printf("messages[0] checksum: %d\n", get_checksum(transactionData));
		signedMsg, err := ledger.Sign(transactionData)

		if err == nil {
			fmt.Printf("Signed msg: %x\n", signedMsg)
			fmt.Printf("Signed msg checksum: %d\n", get_checksum(signedMsg));

		} else {
			fmt.Printf("Error: %s\n", err)
			os.Exit(1)
		}

		fmt.Printf("\n************ Waiting for signature message 2..\n")

		transactionData = messages[1].GetSignBytes()
		fmt.Printf("messages[1] checksum: %d\n", get_checksum(transactionData));

		signedMsg, err = ledger.Sign(transactionData)

		if err == nil {
			fmt.Printf("Signed msg: %x\n", signedMsg)
			fmt.Printf("Signed msg checksum: %d\n", get_checksum(signedMsg));
		} else {
			fmt.Printf("Error: %s\n", err)
			os.Exit(1)
		}

		fmt.Printf("\n************ Waiting for signature message 3..\n")

		transactionData = messages[2].GetSignBytes()
		fmt.Printf("messages[2] checksum: %d\n", get_checksum(transactionData));

		signedMsg, err = ledger.Sign(transactionData)

		if err == nil {
			fmt.Printf("Signed msg: %x\n", signedMsg)
			fmt.Printf("Signed msg checksum: %d\n", get_checksum(signedMsg));
		} else {
			fmt.Printf("Error: %s\n", err)
			os.Exit(1)
		}
	}
}
