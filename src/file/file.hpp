#pragma once

#include <string>

bool readFile(const std::string &filepath, std::string &body);
bool writeFile(const std::string &filepath, const std::string &body);