
package main

import (
	"fmt"
	"os"
	"strconv"
	crypto "github.com/tendermint/go-crypto" // You have to manually switch to develop branch
	sdk "github.com/cosmos/cosmos-sdk/types" // You have to manually switch to adrian/tx_encoding branch
	"github.com/cosmos/cosmos-sdk/x/bank"    // You have to manually switch to adrian/tx_encoding branch
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
	return []bank.SendMsg {

		// Simple address, 1 input, 1 output
		bank.SendMsg{
			Inputs: []bank.Input{
				{
					Address:  crypto.Address([]byte("input")),
					Coins:    sdk.Coins{{"atom", 10}},
					Sequence: 1,
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
					Address:  crypto.Address(crypto.GenPrivKeyEd25519().PubKey().Bytes()),
					Coins:    sdk.Coins{{"atom", 1000000}},
					Sequence: 1,
				},
			},
			Outputs: []bank.Output{
				{
					Address: crypto.Address(crypto.GenPrivKeyEd25519().PubKey().Bytes()),
					Coins:   sdk.Coins{{"atom", 1000000}},
				},
			},
		},

		// Simple address, 2 inputs, 2 outputs
        bank.SendMsg{
            Inputs: []bank.Input{
                {
                    Address:  crypto.Address([]byte("input")),
                    Coins:    sdk.Coins{{"atom", 10}},
                    Sequence: 1,
                },
                {
                    Address:  crypto.Address([]byte("anotherinput")),
                    Coins:    sdk.Coins{{"atom", 50}},
                    Sequence: 1,
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
                    Address:  crypto.Address([]byte("input")),
                    Coins:    sdk.Coins{{"atom", 10},{"bitcoint", 20}},
                    Sequence: 1,
                },
                {
                    Address:  crypto.Address([]byte("anotherinput")),
                    Coins:    sdk.Coins{{"atom", 50},{"bitcoint", 60},{"ethereum", 70}},
                    Sequence: 1,
                },
            },
            Outputs: []bank.Output{
                {
                    Address: crypto.Address([]byte("output")),
                    Coins:    sdk.Coins{{"atom", 10},{"bitcoint", 20}},
                },
                {
                    Address: crypto.Address([]byte("anotheroutput")),
                    Coins:    sdk.Coins{{"atom", 50},{"bitcoint", 60},{"ethereum", 70}},
                },
            },
        },
	}
}



func main() {

	messages := GetMessages()

	// Parse the args
	sampleIndex, sampleOutput, returnCode := ParseArgs(len(messages))
	if returnCode != 0 {
		os.Exit(returnCode)
	}

	// Print the selected sample
	PrintSampleFunc(messages[sampleIndex], sampleOutput)
}
