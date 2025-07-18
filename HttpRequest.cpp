/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prutkows <prutkows@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 19:04:52 by prutkows          #+#    #+#             */
/*   Updated: 2025/07/18 16:57:13 by prutkows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include <sstream>
#include <cstdlib>

bool HttpRequest::parse(const std::string &raw)
{
	std::istringstream stream(raw);
	std::string line;

	if (!std::getline(stream, line))
		return false;

	if (!line.empty() && line.back() == '\r')
		line.pop_back();

	std::istringstream startLine(line);
	startLine >> method >> path >> version;

	if (method.empty() || path.empty() || version.empty())
		return false;

	// Headers
	while (std::getline(stream, line))
	{
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		if (line.empty())
			break;

		size_t pos = line.find(':');
		if (pos != std::string::npos)
		{
			std::string key = line.substr(0, pos);
			std::string value = line.substr(pos + 1);
			while (!value.empty() && std::isspace(value[0]))
				value.erase(0, 1);
			headers[key] = value;
		}
	}

	if (version == "HTTP/1.1" && headers.find("Host") == headers.end())
		return false;

	if (headers.count("Content-Length"))
	{
		char *endptr;
		int len = std::strtol(headers["Content-Length"].c_str(), &endptr, 10);
		if (*endptr != '\0' || len < 0 || len > 10 * 1024 * 1024) // max 10MB
			return false;

		body.resize(len);
		stream.read(&body[0], len);
	}

	valid = true;
	return true;
}

std::string HttpRequest::getHeader(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = headers.find(key);
	if (it != headers.end())
		return it->second;
	return "";
}

std::string HttpRequest::toString() const
{
	std::ostringstream out;
	out << "Method: " << method << "\n";
	out << "Path: " << path << "\n";
	out << "Version: " << version << "\n";
	out << "Headers:\n";
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		out << "  " << it->first << ": " << it->second << "\n";
	out << "Body: " << (body.empty() ? "<empty>" : body) << "\n";
	out << "Valid: " << (valid ? "true" : "false") << "\n";
	return out.str();
}
