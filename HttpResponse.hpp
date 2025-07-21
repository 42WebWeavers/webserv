/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prutkows <prutkows@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/18 17:07:30 by prutkows          #+#    #+#             */
/*   Updated: 2025/07/20 17:52:14 by prutkows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>
#include <map>

class HttpResponse
{
private:
	int statusCode;
	std::string statusMessage;
	std::map<std::string, std::string> headers;
	std::string body;

public:
	HttpResponse(int code = 200, const std::string &message = "OK");

	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &content);
	void setStatus(int code, const std::string &message);

	std::string toString() const;
};

#endif
