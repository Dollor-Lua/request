#include <string>
#include <vector>
#include <functional>
#include <iostream>

using namespace std;
namespace request
{
    inline const vector<string> __empty_func(string a, string b)
    {
        cout << a << " was handled by default callback! location '" << b << "'. Provide a function as the final argument to handle requests." << endl;
        return vector<string>{"", "text/plain"};
    };

    bool serve(string ip = "127.0.0.1", int port = 3000, function<vector<string>(string, string)> _requestCallback = __empty_func);
}