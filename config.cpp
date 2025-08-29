//Copyright [2025] <Antoni Gorski> <agorski@student.42warsaw.pl>

#include "config.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream> // opcjonalnie, jeśli potrzebujesz diagnostyki

void trim(std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if(start == std::string::npos) {
        s.clear();
        return;
    }
    size_t end = s.find_last_not_of(" \t\r\n");
    s = s.substr(start, end - start + 1);
    if (!s.empty() && s[s.size() - 1] == ';')
        s.erase(s.size() - 1);
}

void Config::parseLines(std::ifstream& file) {
    std::string line;
    ServerConfig currentServer;
    LocationConfig currentLocation;
    std::string currentLocationPath;
    bool inServer = false;
    bool inLocation = false;

    while (std::getline(file, line)) {
        trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "server") {
            if (inServer) {
                if (inLocation)
                    currentServer.locations[currentLocationPath] = currentLocation;
                servers.push_back(currentServer);
                currentServer = ServerConfig();
                currentLocation = LocationConfig();
                currentLocationPath.clear();
                inLocation = false;
            }
            inServer = true;
        }
        else if (key == "listen") {
            int p; iss >> p; currentServer.listen_port = p;
        }
        else if (key == "server_name") {
            std::string sn; iss >> sn; currentServer.server_name = sn;
        }
        else if (key == "host") {
            std::string h; iss >> h; currentServer.host = h;
        }
        else if (key == "root" && !inLocation) {
            std::string r; iss >> r; currentServer.root = r;
        }
        else if (key == "index" && !inLocation) {
            std::string idx; iss >> idx; currentServer.index = idx;
        }
        else if (key == "error_page") {
            int code; std::string val; iss >> code >> val;
            currentServer.error_pages[code] = val;
        }
        else if (key == "location") {
            if (inLocation)
                currentServer.locations[currentLocationPath] = currentLocation;
            iss >> currentLocationPath;
            currentLocation = LocationConfig();
            inLocation = true;
        }
        else if (inLocation) {
            if (key == "allow_methods") {
                currentLocation.allow_methods.clear();
                std::string m;
                while (iss >> m)
                    currentLocation.allow_methods.push_back(m);
            }
            else if (key == "autoindex") {
                std::string val; iss >> val;
                currentLocation.autoindex = (val == "on");
            }
            else if (key == "index") {
                std::string idx; iss >> idx;
                currentLocation.index = idx;
            }
            else if (key == "return") {
                std::string ret; iss >> ret;
                currentLocation.return_path = ret;
            
            }
            else if (key == "upload") {
                std::string upload_dir; iss >> upload_dir;
                currentLocation.upload_dir = upload_dir;
            }
            else if (key == "cgi_path") {
                currentLocation.cgi_path.clear();
                std::string p;
                while (iss >> p)
                    currentLocation.cgi_path.push_back(p);
            }
            else if (key == "cgi_ext") {
                currentLocation.cgi_ext.clear();
                std::string ext;
                while (iss >> ext)
                    currentLocation.cgi_ext.push_back(ext);
            }
            else if (key == "root") {
                std::string r; iss >> r;
                currentLocation.root = r;
            }
        }
    }

    if (inServer) {
        if (inLocation)
            currentServer.locations[currentLocationPath] = currentLocation;
        servers.push_back(currentServer);
    }
}

Config::Config(const std::string& filename) {
    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file");
    parseLines(file);
}

const std::vector<ServerConfig>& Config::getServers() const {
    return servers;
}