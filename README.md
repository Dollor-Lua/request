# Request

Hi, welcome to Request! A C++ library for server TCP/IP connections. Request is built to simplify HTTP servers
with one single function.

## Supported Operating Systems

Request will probably support all operating systems (there is code for UNIX systems), although the UNIX system code
is untested and I have no motivation to test it. If you find bugs with it and end up fixing it, send a pull request,
I'll check it out, and accept it if it makes UNIX work correctly.

## Usage

To use request, you first need to include the header file.
For basic usage:

```cpp
// include request
#include "request.hpp"

int main() {
    // start a new server on 127.0.0.1:3000
    bool success = request::serve("127.0.0.1", 3000);
    return success;
}
```

If you wish to handle GET/PULL/DESTROY/PATCH/etc requests,
you can handle them with the third argument provided by `request::server`:

```cpp
// include basic std libs
#include <string>
#include <vector>
#include <iostream>

// include request
#include "request.hpp"

using namespace std;
vector<string> handler(string method, string location)
{
    // result always has to be returned with two arguments, {data, http_text_type}
    vector<string> result{"hello, world!", "text/plain"};

    // ex: "Method: GET, For: '/'"
    cout << "Method: " << method << ", For: '" << location << "'" << endl;

    // return our result so request knows what to do.
    return result;
}

int main() {
    // start a new server on 127.0.0.1:3000
    bool success = request::serve("127.0.0.1", 3000, handler);
    return success;
}
```

The serve function has all parameters as optional, so running `request::serve()` is valid.

You can find a basic webserver example in the `/tests/` directory.

### Full Api Documentation

`bool request::serve(?ip = 127.0.0.1, ?port = 3000, ?callback)` - hosts a server on `ip` with port number `port`. When a client tries to access the server, the callback `callback` is called if provided.

#### Developer Only

`vector<string> request::__empty_func(method, location)` - the default function for handling client requests. Accepts a method and location just like the callback parameter of `request::serve`.
