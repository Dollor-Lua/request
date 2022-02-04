#include "utils.hpp"

using namespace std;

namespace utils
{
    string readfile(string path)
    {
        string line;
        string total;
        ifstream myfile(path);
        if (myfile.is_open())
        {
            while (getline(myfile, line))
                total += line;
            myfile.close();
        }

        return total;
    }

    vector<string> split(string s, string delimiter)
    {
        size_t pos = 0;
        string token;

        vector<string> parts;
        while ((pos = s.find(delimiter)) != string::npos)
        {
            token = s.substr(0, pos);
            s.erase(0, pos + delimiter.length());
            parts.push_back(token);
        }

        parts.push_back(s);
        return parts;
    }
}