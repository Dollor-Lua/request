#include <string>
#include <vector>
#include <iostream>

#include "../include/request.hpp"
#include "utils.hpp"

using namespace std;
vector<string> handler(string method, string location)
{
    vector<string> result{"", "text/plain"};

    if (method == "GET")
    {
        string trueloc = "tests/web";
        trueloc += location;

        vector<string> bases = utils::split(trueloc, ".");

        if (bases.size() == 1)
        {
            trueloc += ".html";
        }

        string file = utils::readfile(trueloc);

        result.clear();

        result.push_back(file);
        result.push_back("text/" + utils::split(trueloc, ".")[1]);
    }

    return result;
}

int main()
{
    // bool request::serve(host = 127.0.0.1, port = 3000, request_handler = empty_handler)
    // all arguments are optional
    bool success = request::serve("127.0.0.1", 3000, handler);
    return success;
}