/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prutkows <prutkows@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 17:07:33 by prutkows          #+#    #+#             */
/*   Updated: 2025/07/20 17:59:29 by prutkows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpResponse.hpp"
#include <sstream>

HttpResponse::HttpResponse(int code, const std::string &message)
	: statusCode(code), statusMessage(message) {}

void HttpResponse::setHeader(const std::string &key, const std::string &value)
{
	headers[key] = value;
}

void HttpResponse::setBody(const std::string &content)
{
	body = content;
	headers["Content-Length"] = std::to_string(body.size());
}

void HttpResponse::setStatus(int code, const std::string &message)
{
	statusCode = code;
	statusMessage = message;
}

std::string HttpResponse::toString() const
{
	std::ostringstream response;

	response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";

	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		response << it->first << ": " << it->second << "\r\n";

	response << "\r\n";

	response << body;

	return response.str();
}
