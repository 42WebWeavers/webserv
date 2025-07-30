/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mini_server.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prutkows <prutkows@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 19:03:34 by prutkows          #+#    #+#             */
/*   Updated: 2025/07/30 21:28:43 by prutkows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstddef>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cctype>
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

bool set_nonblock_cloexec(int fd)
{
	if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0)
	{
		print_error("fcntl(F_SETFD)");
		return false;
	}
	// get actuall flags
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
	{
		print_error("fcntl(F_GETFL)");
		return false;
	}
	// set actuall flags and O_NONBLOCK
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		print_error("fcntl(F_SETFL)");
		return false;
	}
	return true;
}

int setupServerSocket(int port)
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		print_error("socket");
		return -1;
	}
	if (!set_nonblock_cloexec(server_fd))
	{
		close(server_fd);
		return -1;
	}

	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(server_fd, (sockaddr *)&addr, sizeof(addr)) < 0)
	{
		print_error("bind");
		close(server_fd);
		return -1;
	}
	if (listen(server_fd, SOMAXCONN) < 0)
	{
		print_error("listen");
		close(server_fd);
		return -1;
	}
	std::cout << "Server listening on port " << port << std::endl;
	return server_fd;
}

// NEW
struct Connection
{
	int fd;
	std::string	input_buf;
	std::string	output_buf;
	size_t	output_pos = 0;
	bool header_parsed = false;
	size_t header_end = 0;
	size_t content_length = 0;
	bool	header_ready = false;
	bool	keep_alive = false;
};
static size_t extract_content_length(const std::string& headers)
{
	const std::string key = "Content-Length:";
	size_t pos = headers.find(key);
	if (pos == std::string::npos)
		return 0;
	pos += key.size();
	while (pos < headers.size() && std::isspace(static_cast<unsigned char>(headers[pos])))
		pos++;
	size_t end = headers.find("\r\n", pos);
	if (end == std::string::npos)
		end = headers.size();
	std::string value = headers.substr(pos, end - pos);
	char* str_end = NULL;
	unsigned long result = std::strtoul(value.c_str(), &str_end, 10);
	if (str_end == value.c_str())
		return 0;

	return static_cast<size_t>(result);
}

void close_client(std::vector<pollfd>& fds,std::map<int,Connection>& clients,size_t idx)
{
	int fd = fds[idx].fd;
	close(fd);
	clients.erase(fd);
	fds.erase(fds.begin() + idx);
}

void accept_new(std::vector<pollfd>& fds, std::map<int, Connection>& clients, int server_fd)
{
	sockaddr_in addr;
	socklen_t len = sizeof(addr);
	while (true)
	{
		int client_fd = accept(server_fd, (sockaddr*)&addr, &len);
		if (client_fd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			print_error("accept");
			break;
		}
		if (!set_nonblock_cloexec(client_fd))
		{
			close(client_fd);
			continue;
		}
		clients[client_fd] = {
			client_fd,
			"",
			"",
			0,
			false,
			0,
			0,
			false,
			false
		};
		fds.push_back({client_fd, POLLIN, 0});
	}
}

void prepare_response(Connection& connect)
{
	HttpRequest  req;
	HttpResponse res;
	if (!req.parse(connect.input_buf)) {
		res.setStatus(400,"Bad Request");
		res.setHeader("Content-Type","text/html");
		res.setBody("<h1>400 Bad Request</h1>");
		connect.keep_alive = false;
	}
	else
	{
		std::string conn = req.getHeader("Connection");
		if (req.getVersion() == "HTTP/1.1")
			connect.keep_alive = (conn != "close");
		else
			connect.keep_alive = (conn == "keep-alive");

		if (req.getMethod()=="GET" && req.getPath()=="/") {
			res.setStatus(200,"OK");
			res.setHeader("Content-Type","text/html");
			res.setBody("<h1>Main side</h1>");
		} else if (req.getMethod()=="GET" && req.getPath()=="/hello") {
			res.setStatus(200,"OK");
			res.setHeader("Content-Type","text/html");
			res.setBody("<h1>Hello!</h1>");
		} else if (req.getMethod()=="POST" && req.getPath()=="/echo") {
			res.setStatus(200,"OK");
			res.setHeader("Content-Type","text/plain");
			res.setBody("Echo:\n"+req.getBody());
		} else
		{
			res.setStatus(404,"Not Found");
			res.setHeader("Content-Type","text/html");
			res.setBody("<h1>404 Not Found</h1>");
		}
	}
	res.setHeader("Connection", connect.keep_alive?"keep-alive":"close");
	connect.output_buf = res.toString();
	connect.output_pos = 0;
}

void handle_read(pollfd& p, Connection& connect)
{
	char buffer[4096];
	while (true)
	{
		ssize_t n = read(connect.fd, buffer, sizeof(buffer));
		if (n > 0)
			connect.input_buf.append(buffer, n);
		else if (n == 0)
		{
			p.events = 0;
			break;
		}
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;
		else
		{
			print_error("read");
			p.events = 0;
			break;
		}
	}
	if(!connect.header_parsed)
	{
		size_t pos = connect.input_buf.find("\r\n\r\n");
		if (pos != std::string::npos)
		{
			connect.header_parsed = true;
			connect.header_end = pos + 4;
			std::string hdr = connect.input_buf.substr(0, connect.header_end);
			connect.content_length = extract_content_length(hdr);
		}
	}
	if (connect.header_parsed)
	{
		if (connect.input_buf.size() >= connect.header_end + connect.content_length) {
			connect.header_ready = true;
		}
    }
	if (connect.header_ready)
	{
		prepare_response(connect);
		p.events = POLLIN | POLLOUT;
	}
}

void handle_write(pollfd& p, Connection& connect)
{
	while (connect.output_pos < connect.output_buf.size())
	{
		ssize_t n = write(connect.fd,
					connect.output_buf.data() + connect.output_pos,
					connect.output_buf.size() - connect.output_pos);

		if (n > 0)
			connect.output_pos += n;
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
			break;
		else
		{
			print_error("write");
			connect.keep_alive = false;
			p.events = 0;
			break;
		}
	}
	if (connect.output_pos >= connect.output_buf.size())
	{
		if (connect.keep_alive)
		{
			connect.input_buf.clear();
			connect.output_buf.clear();
			connect.output_pos = 0;
			connect.header_parsed = false;
			connect.header_end = 0;
			connect.content_length = 0;
			connect.header_ready = false;
			p.events = POLLIN;
		} else
			p.events = 0;
	}
}

int main()
{
	signal(SIGPIPE, SIG_IGN);
	int server_fd = setupServerSocket(8080);
	if (server_fd < 0)
		return 1;
	std::vector<pollfd>	fds;
	std::map<int, Connection> clients;
	fds.push_back({server_fd, POLLIN, 0});

	while (true)
	{
		int n = poll(fds.data(), fds.size(), 5000);
		if (n < 0)
		{
			if (errno == EINTR)
				continue;
			print_error("poll");
			break;
		}
		if (fds[0].revents & POLLIN)
			accept_new(fds, clients, server_fd);

		for (size_t i = 1; i < fds.size(); i++)
		{
			struct pollfd &p = fds[i];
			std::map<int, Connection>::iterator it = clients.find(p.fd);
			if (it == clients.end())
				continue;
			Connection &connect = it->second;

			if (p.revents & (POLLHUP | POLLERR))
			{
				close_client(fds, clients, i--);
				continue;
			}
			if ((p.revents & POLLIN) && !connect.header_ready)
				handle_read(p,connect);
			if ((p.revents & POLLOUT) && connect.header_ready)
				handle_write(p, connect);
			if (p.events == 0)
				close_client(fds, clients, i--);
		}
	}

	close(server_fd);
	return 0;
}

// void handleClient(int client_fd)
// {
// 	const size_t BUF_SIZE = 4096;
// 	char buf[BUF_SIZE];
// 	std::vector<char> buffer;

// 	while(true)
// 	{
// 		ssize_t n = read(client_fd, buf, BUF_SIZE);
// 		if (n > 0)
// 			buffer.insert(buffer.end(), buf, buf + n);
// 		else if (n == 0)
// 		{
// 			close(client_fd);
// 			return;
// 		}
// 		else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
// 			break;
// 		else
// 		{
// 			print_error("read");
// 			close(client_fd);
// 			return;
// 		}
// 	}

// 	std::string raw(buffer.data(), buffer.size());


// 	HttpRequest request;
// 	if (!request.parse(raw))
// 	{
// 		const char *response =
// 			"HTTP/1.1 400 Bad Request\r\n"
// 			"Content-Type: text/html\r\n"
// 			"Content-Length: 54\r\n"
// 			"\r\n"
// 			"<html><body><h1>400 - Bad Request</h1></body></html>";

// 		write(client_fd, response, std::strlen(response));
// 		close(client_fd);
// 		return;
// 	}

// 	HttpResponse response;

// 	if (request.getMethod() == "GET" && request.getPath() == "/")
// 	{
// 		response.setStatus(200, "OK");
// 		response.setHeader("Content-Type", "text/html");
// 		response.setBody("<html><body><h1>Welcome to the root!</h1></body></html>");
// 	}
// 	else if (request.getMethod() == "GET" && request.getPath() == "/hello")
// 	{
// 		response.setStatus(200, "OK");
// 		response.setHeader("Content-Type", "text/html");
// 		response.setBody("<html><body><h1>Hello, world!</h1></body></html>");
// 	}
// 	else if (request.getMethod() == "POST" && request.getPath() == "/echo")
// 	{
// 		response.setStatus(200, "OK");
// 		response.setHeader("Content-Type", "text/plain");
// 		response.setBody("Received POST with body:\n" + request.getBody());
// 	}
// 	else
// 	{
// 		response.setStatus(404, "Not Found");
// 		response.setHeader("Content-Type", "text/html");
// 		response.setBody("<html><body><h1>404 - Not Found</h1></body></html>");
// 	}

// 	std::string out = response.toString();

// 	std::cout << request.toString() << std::endl;
// 	write(client_fd, out.c_str(), out.size());
// 	close(client_fd);
// }

// int main()
// {
// 	int server_fd = setupServerSocket(8080);
// 	if (server_fd < 0)
// 		return 1;

// 	if (listen(server_fd, 10) < 0)
// 	{
// 		print_error("listen");
// 		close(server_fd);
// 		return 1;
// 	}

// 	sockaddr_in addr;
// 	socklen_t addrlen = sizeof(addr);

// 	std::vector<struct pollfd> fds;
// 	fds.push_back({server_fd, POLLIN, 0});

// 	while (true)
// 	{
// 		for (struct pollfd &p : fds)
// 			p.revents = 0;
// 		int ret = poll(fds.data(), fds.size(), 1000);
// 		if (ret > 0)
// 		{
// 			if (fds[0].revents & POLLIN)
// 			{
// 				int client_fd = accept(server_fd, (sockaddr *)&addr, &addrlen);
// 				if (client_fd < 0)
// 				{
// 					if (errno == EAGAIN || errno == EWOULDBLOCK)
// 					{
// 						usleep(1000);
// 						continue;
// 					}
// 					print_error("accept");
// 					continue;
// 				}
// 				if (!set_nonblock_cloexec((client_fd)))
// 				{
// 					close(client_fd);
// 					continue;
// 				}

// 				fds.push_back({client_fd, POLLIN, 0});
// 			}
// 			for (size_t i = 1; i < fds.size(); )
// 			{
// 				if (fds[i].revents & POLLIN)
// 				{
// 					handleClient(fds[i].fd);
// 					close(fds[i].fd);
// 					fds.erase(fds.begin() + i);
// 				}
// 				else
// 					i++;
// 			}
// 		}
// 	}
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
