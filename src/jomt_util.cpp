#include "jomt_util.hpp"
#include <string>

using namespace jomt::utils;

std::string strings::trim(std::string data)
{
    if (data.empty())
        return data;
    
    return ltrim(rtrim(data));
}

std::string strings::ltrim(std::string data)
{   
    if(data.empty()) 
        return data;

    size_t ini = data.find_first_not_of(' ');
    data = ini != std::string::npos ? data.erase(0, ini) : data.erase(0);
    return data;
}

std::string strings::rtrim(std::string data)
{
    if (data.empty())
        return data;

    size_t end = data.find_last_not_of(' ');
    data = end != std::string::npos ? data.erase(end + 1) : data.erase(0);
    return data;
}