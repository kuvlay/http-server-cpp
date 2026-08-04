#include "file.hpp"

#include <fstream>
#include <iostream>
#include <iterator>

bool readFile(const std::string &filepath, std::string &body)
{
    std::cout << filepath << '\n';
    std::ifstream file(filepath);
    if (!file.is_open())
        return false;

    body.assign(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>());

    return true;
}

bool writeFile(const std::string &filepath, const std::string &body)
{
    std::cout << "write file called\n";
    std::ofstream file(filepath);
    if (!file.is_open())
        return false;

    file << body;
    return true;
}