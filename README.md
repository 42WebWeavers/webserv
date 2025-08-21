Webserv Config Parser
This repository contains a simple C++98 configuration parser for a web server project (42 school style).

Key features:

Parses a custom config file format inspired by NGINX.
Supports multiple servers and multiple routes (locations) per server.
Handles settings such as:
Listening port, host, server name
Error pages
Root and index files
Allowed HTTP methods
Directory listing (autoindex)
File upload directory (per route)
CGI paths and extensions
HTTP redirections
Main files:

config.hpp – data structures for server and route configuration.
config.cpp – parser implementation.
main.cpp – example usage: loads config and prints parsed data.
default.conf – example configuration file.
Usage:

Build with:
Run:
Output: parsed config data for all servers and routes.
