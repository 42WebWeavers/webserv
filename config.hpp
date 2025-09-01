//Copyright [2025] <Antoni Gorski> <agorski@student.42warsaw.pl>

#pragma once

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::vector<std::string> allow_methods;
    bool autoindex;
    std::string index;
    std::string return_path;
    std::string upload_dir;
    std::vector<std::string> cgi_path;
    std::vector<std::string> cgi_ext;
    std::string root;
    LocationConfig() : autoindex(false) {}
};

struct ServerConfig {
    int listen_port;
    std::string server_name;
    std::string host;
    std::string root;
    std::string index;
    std::map<int, std::string> error_pages;
    std::map<std::string, LocationConfig> locations;
    ServerConfig() : listen_port(80), host("0.0.0.0") {}
};

class Config {
private:
    std::vector<ServerConfig> servers;

    void parseLines(std::ifstream& file);

public:
    Config(const std::string& filename);

    const std::vector<ServerConfig>& getServers() const;
};
