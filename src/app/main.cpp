// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include <iostream>


extern "C"
{
#include <lib/JsonParser.h>
}

int main()
{
    ParsedMessage data;
    ParseJson(&data, "Topic: \"Hello json world\"");

    std::cout << "Number of found tokens: " << data.NumberOfTokens << std::endl;

    return 0;
}