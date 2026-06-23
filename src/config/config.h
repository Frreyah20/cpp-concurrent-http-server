#pragma once

#include <string>

struct Config
{
    int port;
    int workers;
    std::string root;
};

extern Config config;

bool loadConfig(const std::string& filename);
