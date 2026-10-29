#include "console.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

bool readLine(const string& prompt, string& value) {
    cout << prompt;
    return static_cast<bool>(getline(cin, value));
}

bool readNumber(const string& prompt, int& number) {
    string input;
    if (!readLine(prompt, input)) {
        return false;
    }
    char extra;
    istringstream parser(input);
    if (!(parser >> number) || (parser >> extra)) {
        cout << "Enter a valid number.\n";
        return false;
    }
    return true;
}
