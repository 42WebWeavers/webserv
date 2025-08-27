/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IHttpHandle.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prutkows <prutkows@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 19:04:52 by prutkows          #+#    #+#             */
/*   Updated: 2025/08/23 00:04:55 by prutkows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IHTTPHANDLE_HPP
#define IHTTPHANDLE_HPP

#include <cstddef>
#include <string>

// this is is simple API for me
struct ParseDecision {
  bool headers_ok;
  bool has_body;
  bool chunked;
  size_t body_expected;  // if known (Content-Length)
  int status_when_error; // np. 400/413/431
  ParseDecision()
      : headers_ok(false), has_body(false), chunked(false), body_expected(0),
        status_when_error(0) {}
};

class IHttpHandle {
public:
  virtual ~IHttpHandle() {}
  virtual void attachClientContext(int client_fd) = 0;
  virtual void reset() = 0;
  virtual ParseDecision onHeaders(const std::string &raw_headers) = 0;
  virtual void onBodyData(const char *data, size_t n) = 0;
  virtual bool buildResponse(std::string &out_buf, bool &keep_alive,
                             bool &close_after_write) = 0;
};
#endif
