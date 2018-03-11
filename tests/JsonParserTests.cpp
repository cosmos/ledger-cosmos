// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "lib/JsonParser.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <jsmn.h>
#include <lib/JsonParser.h>

namespace
{
    std::string exec(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
        if (!pipe) throw std::runtime_error("popen() failed!");
        while (!feof(pipe.get())) {
            if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
                result += buffer.data();
        }
        return result;
    }

TEST(JsonParserTest, Empty)
{
    parsed_json_t parserData = {0};
    ParseJson(&parserData, "");

    EXPECT_FALSE(parserData.CorrectFormat);
    EXPECT_EQ(0, parserData.NumberOfTokens);
}

TEST(JsonParserTest, SinglePrimitive)
{
    parsed_json_t parserData = {0};
    ParseJson(&parserData, "EMPTY");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(1, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
}

TEST(JsonParserTest, KeyValuePrimitives)
{
    parsed_json_t parserData = {0};
    ParseJson(&parserData, "KEY : VALUE");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(2, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_PRIMITIVE);
}

TEST(JsonParserTest, SingleString)
{
    parsed_json_t parserData = {0};
    ParseJson(&parserData, "\"EMPTY\"");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(1, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_STRING);
}

TEST(JsonParserTest, KeyValueStrings)
{
    parsed_json_t parserData = {0};
    ParseJson(&parserData, "\"KEY\" : \"VALUE\"");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(2, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_STRING);
}

TEST(JsonParserTest, SimpleArray)
{
    parsed_json_t parserData = {0};
    ParseJson(&parserData, "LIST : [1, 2, 3, 4]");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(6, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_ARRAY);
    EXPECT_TRUE(parserData.Tokens[2].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[3].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[4].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[5].type == jsmntype_t::JSMN_PRIMITIVE);
}

TEST(JsonParserTest, MixedArray)
{
    parsed_json_t parserData = {0};
    ParseJson(&parserData, "LIST : [1, \"Text\", 3, \"Another text\"]");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(6, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_ARRAY);
    EXPECT_TRUE(parserData.Tokens[2].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[3].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[4].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[5].type == jsmntype_t::JSMN_STRING);
}

TEST(JsonParserTest, SimpleObject)
{
        parsed_json_t parserData = {0};
        ParseJson(&parserData, "vote : "
                "{ "
                "\"key\" : \"value\", "
                "\"another key\" : { "
                "\"inner key\" : \"inner value\", "
                "\"total\":123 }"
                "}");

        EXPECT_TRUE(parserData.CorrectFormat);
        EXPECT_EQ(10, parserData.NumberOfTokens);
        EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
        EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_OBJECT);
        EXPECT_TRUE(parserData.Tokens[2].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.Tokens[3].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.Tokens[4].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.Tokens[5].type == jsmntype_t::JSMN_OBJECT);
        EXPECT_TRUE(parserData.Tokens[6].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.Tokens[7].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.Tokens[8].type == jsmntype_t::JSMN_STRING);
        EXPECT_TRUE(parserData.Tokens[9].type == jsmntype_t::JSMN_PRIMITIVE);
    }

    static bool Compare(const char* input, jsmntok_t inputToken, const char* reference)
    {
        return strncmp(input + inputToken.start, reference, inputToken.end - inputToken.start) == 0;
    }

    TEST(JsonParserTest, ParseSendMsg_SingleInputSingleOutput)
    {
        parsed_json_t parsedMessage = {0};
        //std::string result = exec("../tools/samples 0 text");
        const char* sendMsgSample = R"({"_df":"3CAAA78D13BAE0","_v":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10}],"sequence":1}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":10}]}]}})";

        ParseJson(&parsedMessage, sendMsgSample);

        EXPECT_EQ(1, parsedMessage.NumberOfInputs);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Sequence], "1"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Address], "696E707574"));
        EXPECT_EQ(1, parsedMessage.Inputs[0].NumberOfCoins);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Coins[0].Amount], "10"));

        EXPECT_EQ(1, parsedMessage.NumberOfOutputs);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Address], "6F7574707574"));
        EXPECT_EQ(1, parsedMessage.Outputs[0].NumberOfCoins);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Coins[0].Amount], "10"));
    }

    TEST(JsonParserTest, ParseSendMsg_TwoInputsTwoOutputs)
    {
        parsed_json_t parsedMessage = {0};
        //std::string result = exec("../tools/samples 2 text");
        const char* sendMsgSample = R"({"_df":"3CAAA78D13BAE0","_v":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10}],"sequence":1},{"address":"616E6F74686572696E707574","coins":[{"denom":"atom","amount":50}],"sequence":1}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":10}]},{"address":"616E6F746865726F7574707574","coins":[{"denom":"atom","amount":50}]}]}})";

        ParseJson(&parsedMessage, sendMsgSample);

        EXPECT_EQ(2, parsedMessage.NumberOfInputs);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Address], "696E707574"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Address], "616E6F74686572696E707574"));
        EXPECT_EQ(1, parsedMessage.Inputs[0].NumberOfCoins);
        EXPECT_EQ(1, parsedMessage.Inputs[1].NumberOfCoins);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Sequence], "1"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Sequence], "1"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Coins[0].Amount], "10"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Coins[0].Amount], "50"));

        EXPECT_EQ(2, parsedMessage.NumberOfOutputs);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Address], "6F7574707574"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Address], "616E6F746865726F7574707574"));
        EXPECT_EQ(1, parsedMessage.Outputs[0].NumberOfCoins);
        EXPECT_EQ(1, parsedMessage.Outputs[1].NumberOfCoins);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Coins[0].Amount], "10"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Coins[0].Amount], "50"));
    }

    TEST(JsonParserTest, ParseSendMsg_TwoInputsTwoOutputsMultipleCoins)
    {
        parsed_json_t parsedMessage = {0};
        //std::string result = exec("../tools/samples 3 text");
        const char* sendMsgSample = R"({"_df":"3CAAA78D13BAE0","_v":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10},{"denom":"bitcoin","amount":20}],"sequence":1},{"address":"616E6F74686572696E707574","coins":[{"denom":"atom","amount":50},{"denom":"bitcoin","amount":60},{"denom":"ethereum","amount":70}],"sequence":17}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":11},{"denom":"bitcoint","amount":21}]},{"address":"616E6F746865726F7574707574","coins":[{"denom":"atom","amount":51},{"denom":"bitcoint","amount":61},{"denom":"ethereum","amount":71}]}]}})";

        ParseJson(&parsedMessage, sendMsgSample);

        EXPECT_EQ(2, parsedMessage.NumberOfInputs);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Address], "696E707574"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Address], "616E6F74686572696E707574"));
        EXPECT_EQ(2, parsedMessage.Inputs[0].NumberOfCoins);
        EXPECT_EQ(3, parsedMessage.Inputs[1].NumberOfCoins);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Sequence], "1"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Sequence], "17"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Coins[0].Amount], "10"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Coins[1].Denum], "bitcoin"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[0].Coins[1].Amount], "20"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Coins[0].Amount], "50"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Coins[1].Denum], "bitcoin"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Coins[1].Amount], "60"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Coins[2].Denum], "ethereum"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Inputs[1].Coins[2].Amount], "70"));

        EXPECT_EQ(2, parsedMessage.NumberOfOutputs);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Address], "6F7574707574"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Address], "616E6F746865726F7574707574"));
        EXPECT_EQ(2, parsedMessage.Outputs[0].NumberOfCoins);
        EXPECT_EQ(3, parsedMessage.Outputs[1].NumberOfCoins);
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Coins[0].Amount], "11"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Coins[1].Denum], "bitcoint")); // typo on purpose
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[0].Coins[1].Amount], "21"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Coins[0].Denum], "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Coins[0].Amount], "51"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Coins[1].Denum], "bitcoint")); // typo on purpose
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Coins[1].Amount], "61"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Coins[2].Denum], "ethereum"));
        EXPECT_TRUE( Compare(sendMsgSample, parsedMessage.Tokens[parsedMessage.Outputs[1].Coins[2].Amount], "71"));
    }
}
