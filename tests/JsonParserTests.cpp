// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include "gtest/gtest.h"
#include "lib/JsonParser.h"

namespace
{

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

TEST(JsonParserTest, SimpleObject)
{
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

}
