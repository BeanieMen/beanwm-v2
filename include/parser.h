#pragma once
#include <string>

std::string trim(const std::string& s);
int parseIntOrDefault(const std::string& value, int fallback);
bool parseConfigLine(const std::string& line, std::string& key, std::string& value);
