#ifndef CONSOLE_H
#define CONSOLE_H

#include <string>

bool readLine(const std::string& prompt, std::string& value);
bool readNumber(const std::string& prompt, int& number);

#endif
