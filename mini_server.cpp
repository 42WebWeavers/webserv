/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_server.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prutkows <prutkows@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 19:03:34 by prutkows          #+#    #+#             */
/*   Updated: 2025/07/20 18:38:14 by prutkows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <cstdlib>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

// http://localhost:8080

static void print_error(const char *msg)
{
	const char *err_str = strerror(errno);
	std::string output = std::string(msg) + ": " + err_str + "\n";
	write(2, output.c_str(), output.size());
}

int setupServerSocket(int port)
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		print_error("socket");
		return -1;
	}

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
	{
		print_error("bind");
		close(server_fd);
		return -1;
	}
	std::cout << "Server listening on port " << port << std::endl;
	return server_fd;
}

void handleClient(int client_fd)
{
	char buffer[8192];
	ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
	if (bytes_read <= 0)
	{
		print_error("read");
		close(client_fd);
		return;
	}
	buffer[bytes_read] = '\0';
	std::string raw(buffer);

	HttpRequest request;
	if (!request.parse(raw))
	{
		const char *response =
			"HTTP/1.1 400 Bad Request\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: 54\r\n"
			"\r\n"
			"<html><body><h1>400 - Bad Request</h1></body></html>";

		write(client_fd, response, std::strlen(response));
		close(client_fd);
		return;
	}

	// HttpResponse response(200, "OK");
	// response.setHeader("Content-Type", "text/html");
	// response.setBody("<html><body><h1>Request received</h1></body></html>");

	HttpResponse response;

	if (request.getMethod() == "GET" && request.getPath() == "/")
	{
		response.setStatus(200, "OK");
		response.setHeader("Content-Type", "text/html");
		response.setBody("<html><body><h1>Welcome to the root!</h1></body></html>");
	}
	else if (request.getMethod() == "GET" && request.getPath() == "/hello")
	{
		response.setStatus(200, "OK");
		response.setHeader("Content-Type", "text/html");
		response.setBody("<html><body><h1>Hello, world!</h1></body></html>");
	}
	else if (request.getMethod() == "POST" && request.getPath() == "/echo")
	{
		response.setStatus(200, "OK");
		response.setHeader("Content-Type", "text/plain");
		response.setBody("Received POST with body:\n" + request.getBody());
	}
	else
	{
		response.setStatus(404, "Not Found");
		response.setHeader("Content-Type", "text/html");
		response.setBody("<html><body><h1>404 - Not Found</h1></body></html>");
	}

	std::string out = response.toString();

	std::cout << request.toString() << std::endl;
	write(client_fd, out.c_str(), out.size());
	close(client_fd);
}

int main()
{
	int server_fd = setupServerSocket(8080);
	if (server_fd < 0)
		return 1;

	if (listen(server_fd, 10) < 0)
	{
		print_error("listen");
		close(server_fd);
		return 1;
	}

	sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	while (true)
	{
		int client_fd = accept(server_fd, (sockaddr *)&addr, &addrlen);
		if (client_fd < 0)
		{
			print_error("accept");
			continue;
		}
		handleClient(client_fd);
	}
	close(server_fd);
	return 0;
}

// int main()
// {
// 	int server_fd;
// 	int client_fd;
// 	sockaddr_in addr;
// 	socklen_t addrlen = sizeof(addr);

// 	// AF_INET = IPv4
// 	// SOCK_STREAM = Specifies type for the socket will use TCP
// 	// 0 means you must use default protocol
// 	// PF_INET instead AF_INET?
// 	server_fd = socket(AF_INET, SOCK_STREAM, 0);
// 	if (server_fd == -1)
// 	{
// 		print_error("socket");
// 		return 1;
// 	}
// 	// addr structure defines properties
// 	addr.sin_family = AF_INET;		   // address's familly
// 	addr.sin_addr.s_addr = INADDR_ANY; // listening all local IP 0.0.0.0
// 	addr.sin_port = htons(8080);	   // listening port

// 	// assigning an address to a socket
// 	// a pointer to a struct sockaddr_in can be cast to a pointer
// 	// to a struct sockaddr and vice-versa!
// 	if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
// 	{
// 		print_error("bind");
// 		return 1;
// 	}
// 	// waiting call limit equal 10
// 	if (listen(server_fd, 10) < 0)
// 	{
// 		print_error("listen");
// 		return 1;
// 	}

// 	std::cout << "Server listening on the 8080 port " << std::endl;

// 	client_fd = accept(server_fd, (sockaddr *)&addr, &addrlen);
// 	if (client_fd < 0)
// 	{
// 		print_error("accept");
// 		return 1;
// 	}

// 	char buffer[1024] = {0};
// 	read(client_fd, buffer, sizeof(buffer) - 1);
// 	std::cout << "Recieved request:\n"
// 			  << buffer << std::endl;

// 	const char *response =
// 		"HTTP/1.1 200 OK\r\n"
// 		"Content-Type: text/html\r\n"
// 		"Content-Length: 46\r\n"
// 		"\r\n"
// 		"<html><body><h1>Welcome with mini_server!</h1></body></html>";

// 	write(client_fd, response, strlen(response));
// 	close(client_fd);
// 	close(server_fd);

// 	return 0;
// }

/*
// (IPv4 only--see struct sockaddr_in6 for IPv6)
struct sockaddr_in {
	short int sin_family; // Address family, AF_INET
	unsigned short int sin_port; // Port number
	struct in_addr sin_addr; // Internet address
	unsigned char sin_zero[8]; // Same size as struct sockaddr
sin_port };
*/
