//Copyright [2025] <Antoni Gorski> <agorski@student.42warsaw.pl>

#include "config.hpp"
#include <iostream>

int main() {
    try {
        Config config("default.conf");
        const std::vector<ServerConfig>& servers = config.getServers();

        std::cout << "Number of servers: " << servers.size() << std::endl;
        for (size_t i = 0; i < servers.size(); ++i) {
            const ServerConfig& srv = servers[i];
            std::cout << "\nServer #" << i+1 << std::endl;
            std::cout << "  listen_port: " << srv.listen_port << std::endl;
            std::cout << "  server_name: " << srv.server_name << std::endl;
            std::cout << "  host: " << srv.host << std::endl;
            std::cout << "  root: " << srv.root << std::endl;
            std::cout << "  index: " << srv.index << std::endl;
            std::cout << "  error_pages: ";
            for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin(); it != srv.error_pages.end(); ++it)
                std::cout << it->first << "->" << it->second << " ";
            std::cout << std::endl;

            std::cout << "  Locations/routes: " << srv.locations.size() << std::endl;
            for (std::map<std::string, LocationConfig>::const_iterator loc = srv.locations.begin(); loc != srv.locations.end(); ++loc) {
                std::cout << "    Path: " << loc->first << std::endl;
                const LocationConfig& lc = loc->second;
                std::cout << "      allow_methods: ";
                for (size_t j = 0; j < lc.allow_methods.size(); ++j)
                    std::cout << lc.allow_methods[j] << " ";
                std::cout << std::endl;
                std::cout << "      autoindex: " << (lc.autoindex ? "on" : "off") << std::endl;
                std::cout << "      index: " << lc.index << std::endl;
                std::cout << "      return_path: " << lc.return_path << std::endl;
                std::cout << "      upload_dir: " << lc.upload_dir << std::endl;
                std::cout << "      root: " << lc.root << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Config error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}