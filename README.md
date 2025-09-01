# WebServ Project

## Bookmarks for reference

### HTTP Protocol standard
[HTTP1.1/RFC9112](https://www.rfc-editor.org/info/rfc9112)<br>
Minimal read:<br>
- Section 1 (Intro)
- Section 5 (Request)
- Section 6 (Response)
- Section 9 (Methods like GET, POST, PUT)

### Guide to Network Programming
[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split/index.html)
by Brian "Beej Jorgensen" Hall

### Common Gateway Interface
[Common Gateway Interface Standard](https://www.rfc-editor.org/info/rfc3875)<br>
[CGI Wikipedia](https://en.wikipedia.org/wiki/Common_Gateway_Interface)<br>
[Tutorial with CGI example](https://www.tutorialspoint.com/cplusplus/cpp_web_programming.htm)<br>

### NGINX - open source software for web serving, reverse proxying, caching, load balancing, media streaming, and more.
[What is NGINX?](https://www.f5.com/glossary/nginx)<br>
[NGINX official docs](https://nginx.org/en/docs/)<br>

### Docker - solely for the prototyping on Campus computers
??

## Project information

# Webserv Config Parser

This repository contains a simple C++98 configuration parser for a web server project.

## Key features:

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

## Main files:

config.hpp – data structures for server and route configuration.
config.cpp – parser implementation.
main.cpp – example usage: loads config and prints parsed data.
default.conf – example configuration file.
Usage:

Build with: makeWebserv Config Parser
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
Run:Webserv Config Parser
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
Run:Webserv Config Parser
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

Build with: make
Run: ./WebServer
Output: parsed config data for all servers and routes. Write data on standard output.

# or use the following command:
```bash
g++ -Wall -Wextra -Werror -std=c++98 main.cpp config.cpp -o WebServer

### Authors
Antoni Górski<br>
Paweł Rutkowski<br>
Zyta Słowiańska<br>

### Meetings calendar
| Date       | Antoni Górski | Paweł Rutkiewicz | Zyta Słowiańska |
|------------|:-------------:|:----------------:|:---------------:|
| 2025-06-13 |      ✅       |        ✅        |       ✅        |
| 2025-06-20 |      ✅       |        ✅        |       ❌        |
| 2025-06-27 |      ✅       |        ✅        |       ✅        |
| 2025-07-04 |      ✅       |        ✅        |       ✅        |
| 2025-07-15 |      ✅       |        ✅        |       ✅        |
| 2025-07-22 |      ✅       |        ✅        |       ❌        |

*Mark presence with ✅ and absence with ❌. Add more rows as needed.*
