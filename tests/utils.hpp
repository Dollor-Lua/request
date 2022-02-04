#include <string>
#include <vector>
#include <fstream>

using namespace std;

namespace utils
{
    string readfile(string path);

    vector<string> split(string s, string delimiter);
}