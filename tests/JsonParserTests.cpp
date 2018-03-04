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
    JsonParserData parserData;
    ParseJson(&parserData, "");

    EXPECT_FALSE(parserData.CorrectFormat);
    EXPECT_EQ(0, parserData.NumberOfTokens);
}

TEST(JsonParserTest, SinglePrimitive)
{
    JsonParserData parserData;
    ParseJson(&parserData, "EMPTY");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(1, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
}

TEST(JsonParserTest, KeyValuePrimitives)
{
    JsonParserData parserData;
    ParseJson(&parserData, "KEY : VALUE");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(2, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_PRIMITIVE);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_PRIMITIVE);
}

TEST(JsonParserTest, SingleString)
{
    JsonParserData parserData;
    ParseJson(&parserData, "\"EMPTY\"");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(1, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_STRING);
}

TEST(JsonParserTest, KeyValueStrings)
{
    JsonParserData parserData;
    ParseJson(&parserData, "\"KEY\" : \"VALUE\"");

    EXPECT_TRUE(parserData.CorrectFormat);
    EXPECT_EQ(2, parserData.NumberOfTokens);
    EXPECT_TRUE(parserData.Tokens[0].type == jsmntype_t::JSMN_STRING);
    EXPECT_TRUE(parserData.Tokens[1].type == jsmntype_t::JSMN_STRING);
}

TEST(JsonParserTest, SimpleArray)
{
    JsonParserData parserData;
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
    JsonParserData parserData;
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

TEST(JsonParserTest, SimpleObject) {
        JsonParserData parserData;
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
        return strncmp(input + inputToken.start, reference, inputToken.size);
    }

    TEST(JsonParserTest, ParseSendMsg_SingleInputSingleOutput)
    {
        JsonParserData parserData;
        //std::string result = exec("../tools/samples 0 text");
        const char* sendMsgSample = R"({"_df":"3CAAA78D13BAE0","_v":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10}],"sequence":1}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":10}]}]}})";

        ParseJson(&parserData, sendMsgSample);

        EXPECT_EQ(1, GetNumberOfInputs(&parserData));
        EXPECT_EQ(1, GetNumberOfOutputs(&parserData));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputAddressToken(&parserData, 0), "696E707574"));
        EXPECT_EQ(1, GetInputNumberOfCoins(&parserData, 0));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinDenomToken(&parserData, 0, 0), "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinAmountToken(&parserData, 0, 0), "10"));
    }

    TEST(JsonParserTest, ParseSendMsg_TwoInputsTwoOutputs)
    {
        JsonParserData parserData;
        //std::string result = exec("../tools/samples 2 text");
        const char* sendMsgSample = R"({"_df":"3CAAA78D13BAE0","_v":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10}],"sequence":1},{"address":"616E6F74686572696E707574","coins":[{"denom":"atom","amount":50}],"sequence":1}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":10}]},{"address":"616E6F746865726F7574707574","coins":[{"denom":"atom","amount":50}]}]}})";

        ParseJson(&parserData, sendMsgSample);

        EXPECT_EQ(2, GetNumberOfInputs(&parserData));
        EXPECT_EQ(2, GetNumberOfOutputs(&parserData));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputAddressToken(&parserData, 0), "696E707574"));
        EXPECT_EQ(1, GetInputNumberOfCoins(&parserData, 0));
        EXPECT_EQ(1, GetInputNumberOfCoins(&parserData, 1));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinDenomToken(&parserData, 0, 0), "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinAmountToken(&parserData, 0, 0), "10"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinDenomToken(&parserData, 1, 0), "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinAmountToken(&parserData, 1, 0), "50"));
    }

    TEST(JsonParserTest, ParseSendMsg_TwoInputsTwoOutputsMultipleCoins)
    {
        JsonParserData parserData;
        //std::string result = exec("../tools/samples 3 text");
        const char* sendMsgSample = R"({"_df":"3CAAA78D13BAE0","_v":{"inputs":[{"address":"696E707574","coins":[{"denom":"atom","amount":10},{"denom":"bitcoint","amount":20}],"sequence":1},{"address":"616E6F74686572696E707574","coins":[{"denom":"atom","amount":50},{"denom":"bitcoint","amount":60},{"denom":"ethereum","amount":70}],"sequence":1}],"outputs":[{"address":"6F7574707574","coins":[{"denom":"atom","amount":10},{"denom":"bitcoint","amount":20}]},{"address":"616E6F746865726F7574707574","coins":[{"denom":"atom","amount":50},{"denom":"bitcoint","amount":60},{"denom":"ethereum","amount":70}]}]}})";

        ParseJson(&parserData, sendMsgSample);

        EXPECT_EQ(2, GetNumberOfInputs(&parserData));
        EXPECT_EQ(2, GetNumberOfOutputs(&parserData));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputAddressToken(&parserData, 0), "696E707574"));
        EXPECT_EQ(2, GetInputNumberOfCoins(&parserData, 0));
        EXPECT_EQ(3, GetInputNumberOfCoins(&parserData, 1));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinDenomToken(&parserData, 0, 0), "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinAmountToken(&parserData, 0, 0), "10"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinDenomToken(&parserData, 0, 1), "bitcoin"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinAmountToken(&parserData, 0, 1), "20"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinDenomToken(&parserData, 1, 0), "atom"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinAmountToken(&parserData, 1, 0), "50"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinDenomToken(&parserData, 1, 1), "bitcoin"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinAmountToken(&parserData, 1, 1), "60"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinDenomToken(&parserData, 1, 2), "ethereum"));
        EXPECT_TRUE( Compare(sendMsgSample, GetInputCoinAmountToken(&parserData, 1, 2), "70"));
    }
}
