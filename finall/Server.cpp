/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prutkows <prutkows@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 19:04:52 by prutkows          #+#    #+#             */
/*   Updated: 2025/08/26 12:39:38 by prutkows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "IHttpHandle.hpp"

#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

static const size_t MAX_HEADER_SIZE = 8192;
static const size_t MAX_INPUT_BUFFER = 8 * 1024 * 1024;
static const size_t MAX_OUTPUT_BUFFER = 8 * 1024 * 1024;

static const size_t CGI_PAUSE_AT = MAX_OUTPUT_BUFFER;
static const size_t CGI_RESUME_AT = MAX_OUTPUT_BUFFER / 2;

// helper minimal response
static void build_minimal_ok(std::string &out, bool &keep_alive,
                             bool &close_after_write) {
  const char *body = "webserv: OK\n";
  std::ostringstream oss;
  oss << "HTTP/1.1 200 OK\r\n"
      << "Content-Length: " << std::strlen(body) << "\r\n"
      << "Content-Type: text/plain\r\n"
      << "Connection: close\r\n"
      << "\r\n";
  out = oss.str();
  out += body;
  keep_alive = false;
  close_after_write = true;
}

void Server::addListenSocket(int fd) {
  if (fd < 0)
    return;
  set_nonblock_cloexec(fd);
  listen_fds.push_back(fd);
  pfds_dirty = true;
}

void Server::addCgiPipes(int out_fd, int err_fd, int client_fd) {
  if (out_fd >= 0) {
    set_nonblock_cloexec(out_fd);
    CgiPipe p;
    p.fd = out_fd;
    p.client_fd = client_fd;
    p.is_stderr = false;
    p.poll_events = POLLIN;
    p.last_activity = std::time(NULL);
    cgi_pipes[out_fd] = p;
  }
  if (err_fd >= 0) {
    set_nonblock_cloexec(err_fd);
    CgiPipe p;
    p.fd = err_fd;
    p.client_fd = client_fd;
    p.is_stderr = true;
    p.poll_events = POLLIN;
    p.last_activity = std::time(NULL);
    cgi_pipes[err_fd] = p;
  }
  pfds_dirty = true;
}

void Server::run() {
  std::signal(SIGPIPE, SIG_IGN);

  running = true;
  while (running) {
    rebuildPollfdsIfDirty();

    //!!!
    const nfds_t n = static_cast<nfds_t>(pfds.size());
    struct pollfd *ptr = n ? &pfds.front() : (struct pollfd *)0;
    int rv = poll(ptr, n, timeout_ms);
    // int rv = poll(&pfds[0], pfds.empty() ? 0 : (nfds_t)pfds.size(),
    // timeout_ms);
    if (rv < 0) {
      if (errno == EINTR)
        continue;
      print_error("poll");
      break;
    }
    // iteration over pfds
    for (size_t i = 0; i < pfds.size(); i++) {
      struct pollfd p = pfds[i];
      if (p.fd < 0)
        continue;

      // check listen
      bool is_listen = false;
      for (size_t k = 0; k < listen_fds.size(); k++)
        if (listen_fds[k] == p.fd) {
          is_listen = true;
          break;
        }
      if (is_listen) {
        if (has_error(p.revents)) {
          print_error("listen fd error");
          close(p.fd);
          listen_fds.erase(std::remove(listen_fds.begin(), listen_fds.end(),
                                       listen_fds.end()));
          pfds_dirty = true;
          continue;
        }
        if (p.revents & POLLIN)
          handleAccept(p.fd);
        continue;
      }

      // check client connection
      std::map<int, Connection>::iterator itc = connections.find(p.fd);
      if (itc != connections.end()) {
        Connection &c = itc->second;

        if (has_error(p.revents)) {
          close_connection(p.fd);
          continue;
        }
        if (p.revents & POLLIN)
          handleClientRead(c);
        if (p.revents & POLLOUT)
          handleClientWrite(c);
        continue;
      }

      // check CGI pipe
      std::map<int, CgiPipe>::iterator itp = cgi_pipes.find(p.fd);
      if (itp != cgi_pipes.end()) {
        CgiPipe &cp = itp->second;

        if (has_error(p.revents)) {
          close_cgi_pipe(p.fd);
          continue;
        }
        if (p.revents & POLLIN)
          handleCgiRead(cp);
        continue;
      }
    }
    // connection timeouts
    time_t now = std::time(NULL);
    for (std::map<int, Connection>::iterator it = connections.begin();
         it != connections.end();) {
      std::map<int, Connection>::iterator cur = it++;
      Connection &c = cur->second;
      // timeout_ms is in ms, last_activity is in seconds
      if ((now - c.last_activity) * 1000 > (long)timeout_ms)
        close_connection(cur->first);
    }
  }
}

void Server::print_error(const char *msg) {
  const char *err_str = strerror(errno);
  std::string output = std::string(msg) + ": " + err_str + "\n";
  write(2, output.c_str(), output.size());
}

// I'm not sure if the F_GETFL and F_SETFD are comparible with subject
bool Server::set_nonblock_cloexec(int fd) {
#ifdef __linux__
  int fl = fcntl(fd, F_GETFL, 0);
  if (fl == -1)
    fl = 0;
  if (fcntl(fd, F_SETFL, fl | O_NONBLOCK) < 0) {
    print_error("fcntl(F_SETFL)");
    return false;
  }

  int fdfl = fcntl(fd, F_GETFD, 0);
  if (fdfl == -1)
    fdfl = 0;
  if (fcntl(fd, F_SETFD, fdfl | FD_CLOEXEC) < 0) {
    print_error("fcntl(F_SETFD)");
    return false;
  }
#else
  if (fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
    print_error("fcntl(F_SETFD)");
    return false;
  }
  if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
    print_error("fcntl(F_SETFL)");
    return false;
  }
#endif
  return true;
}

struct pollfd Server::make_pfd(int fd, short events) {
  struct pollfd p;
  p.fd = fd;
  p.events = events;
  p.revents = 0;
  return p;
}

void Server::handleAccept(int listenFd) {
  for (;;) {
    int client_fd = accept(listenFd, 0, 0);
    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        break;
      print_error("accept");
      break;
    }
    set_nonblock_cloexec(client_fd);
    connections[client_fd] = Connection(client_fd);
    pfds_dirty = true;
  }
}
void Server::resumeCgiPipesForClient(int client_fd) {
  for (std::map<int, CgiPipe>::iterator it = cgi_pipes.begin();
       it != cgi_pipes.end(); ++it) {
    CgiPipe &p = it->second;
    if (p.client_fd == client_fd && p.poll_events == 0) {
      p.poll_events = POLLIN;
      pfds_dirty = true;
    }
  }
}

void Server::handleClientRead(Connection &c) {
  const size_t IO_BUDGET_IN = 64 * 1024;
  size_t got = 0;
  char buf[4096];
  for (;;) {
    ssize_t n = read(c.fd, buf, sizeof(buf));
    if (n > 0) {
      c.input_buf.append(buf, static_cast<size_t>(n));
      c.last_activity = std::time(NULL);
      got += (size_t)n;
      // memory protected
      if (c.input_buf.size() > MAX_INPUT_BUFFER) {
        close_connection(c.fd);
        return;
      }
      // state transition condition
      if (c.state == Connection::RECV_HEADERS) {
        // searching for the end of the headers
        size_t p = std::string::npos;
        if (c.input_buf.size() <= MAX_HEADER_SIZE)
          p = c.input_buf.find("\r\n\r\n");
        else {
          // too big headers - break
          close_connection(c.fd); // error 431
          return;
        }
        if (p != std::string::npos) {
          const size_t headers_len = p + 4;
          const std::string raw_headers = c.input_buf.substr(0, headers_len);

          bool has_body = false;
          bool chunked = false;
          size_t expected = 0;

          // Zyta part impelemntation
          if (c.httpHandle) {
            // example implementation
            // ParseDecision d = c.httpHandle->onHeaders(raw_headers);
            // has_body = d.has_body;
            // chunked = d.chunked;
            // expected = d.body_expected;
            // if(d.headers_ok == false)
            //   error code

            // temporary without Zyta part
            has_body = false;
            chunked = false;
            expected = 0;
          } else
            has_body = false;

          // next condition
          if (has_body) {
            c.state = Connection::RECV_BODY;
            c.body_expected = expected;
            c.body_received = 0;
          } else
            c.state = Connection::PROCESSING;

          // erase headers, leave body
          c.input_buf.erase(0, headers_len);
        }
      }
      // recieve body
      if (c.state == Connection::RECV_BODY) {
        if (c.httpHandle) {
          // example implementation
          c.httpHandle->onBodyData(c.input_buf.data(), c.input_buf.size());

          c.body_received += c.input_buf.size();
          c.input_buf.clear();
        } else {
          c.body_received += c.input_buf.size();
          c.input_buf.clear();
        }

        // Content-Length is known and reached
        if (c.body_expected > 0 && c.body_received >= c.body_expected)
          c.state = Connection::PROCESSING;
      }
      // building response
      if (c.state == Connection::PROCESSING) {
        bool produced = false;
        bool keep = false;
        bool close_after = false;

        if (c.httpHandle) {
          // example implementation
          // produced = c.httpHandle->buildResponse(c.output_buf, keep,
          // close_after);
        }

        if (!c.httpHandle) {
          build_minimal_ok(c.output_buf, keep, close_after);
          produced = true;
        }

        if (produced) {
          c.keep_alive = keep;
          c.close_after_write = close_after;
          c.state = Connection::SENDING;
          updatePollMask(c); // turn POLLOUT
          pfds_dirty = true;
          break;
        }
      }
      if (got >= IO_BUDGET_IN)
        break;
      continue;
    } else if (n == 0) {
      close_connection(c.fd);
      return;
    }
    break;
  }
}

void Server::handleClientWrite(Connection &c) {
  const size_t IO_BUDGET = 64 * 1024; // 64KB on one iteration
  size_t sent = 0;

  while (c.output_pos < c.output_buf.size()) {
    size_t left = c.output_buf.size() - c.output_pos;
    size_t chunk = left;
    if (sent + chunk > IO_BUDGET)
      chunk = IO_BUDGET - sent;

    ssize_t n = write(c.fd, c.output_buf.data() + c.output_pos, chunk);

    if (n > 0) {
      c.output_pos += (size_t)n;
      c.last_activity = std::time(NULL);
      sent += (size_t)n;

      // if cgi pause
      if (c.output_buf.size() - c.output_pos <= CGI_RESUME_AT) {
        resumeCgiPipesForClient(c.fd);
      }
      if (sent >= IO_BUDGET)
        break;
      continue;
    } else if (n < 0)
      break;
  }

  // has everything been sent?
  if (c.output_pos >= c.output_buf.size()) {
    c.output_buf.clear();
    c.output_pos = 0;

    if (c.close_after_write) {
      close_connection(c.fd);
      return;
    }
    // Keep-Alive - back to next request on the socket
    c.state = Connection::RECV_HEADERS;
    // c.keep_alive = true;
    c.close_after_write = false;

    updatePollMask(c); // POLLIN
    pfds_dirty = true;
    // resume cgi
    resumeCgiPipesForClient(c.fd);
  }
}

void Server::handleCgiRead(CgiPipe &p) {
  char buf[4096];
  for (;;) {
    ssize_t n = read(p.fd, buf, sizeof(buf));
    if (n > 0) {
      p.last_activity = std::time(NULL);
      std::map<int, Connection>::iterator itc = connections.find(p.client_fd);
      if (itc == connections.end()) {
        close_cgi_pipe(p.fd);
        return;
      }
      Connection &c = itc->second;

      // stderr with cgi to server
      if (p.is_stderr) {
        (void)write(2, buf, (size_t)n);
        continue;
      }

      ssize_t room = (MAX_OUTPUT_BUFFER > c.output_buf.size())
                         ? (MAX_OUTPUT_BUFFER - c.output_buf.size())
                         : 0;
      if ((size_t)n > room) {
        if (room > 0) {
          c.output_buf.append(buf, room);
          c.last_activity = std::time(NULL);
        }
        p.poll_events = 0;
        pfds_dirty = true;
        return;
      }

      // HTTP layer(Zyta) must filter header CGI !!!
      c.output_buf.append(buf, static_cast<size_t>(n));
      c.last_activity = std::time(NULL);

      if (c.output_buf.size() >= CGI_PAUSE_AT) {
        p.poll_events = 0;
        pfds_dirty = true;
        return;
      }

      if (c.state != Connection::SENDING) {
        c.state = Connection::SENDING;
        updatePollMask(c);
        pfds_dirty = true;
      }
      continue;
    } else if (n == 0) {
      // EOF with CGI -> close pipe
      close_cgi_pipe(p.fd);

      bool any_left = false;
      for (std::map<int, CgiPipe>::iterator it = cgi_pipes.begin();
           it != cgi_pipes.end(); ++it)
        if (it->second.client_fd == p.client_fd) {
          any_left = true;
          break;
        }
      if (!any_left) {
        std::map<int, Connection>::iterator itc = connections.find(p.client_fd);
        if (itc != connections.end()) {
          updatePollMask(itc->second);
          pfds_dirty = true;
        }
      }
      return;
    } else
      break;
  }
}

void Server::close_connection(int client_fd) {
  std::map<int, Connection>::iterator it = connections.find(client_fd);
  if (it != connections.end()) {
    close(client_fd);
    connections.erase(it);
    pfds_dirty = true;
  }
  // closing all pipe CGI
  for (std::map<int, CgiPipe>::iterator ip = cgi_pipes.begin();
       ip != cgi_pipes.end();) {
    std::map<int, CgiPipe>::iterator cur = ip++;
    if (cur->second.client_fd == client_fd) {
      close(cur->first);
      cgi_pipes.erase(cur);
      pfds_dirty = true;
    }
  }
}

void Server::close_cgi_pipe(int pipe_fd) {
  std::map<int, CgiPipe>::iterator it = cgi_pipes.find(pipe_fd);
  if (it != cgi_pipes.end()) {
    close(pipe_fd);
    cgi_pipes.erase(it);
    pfds_dirty = true;
  }
}

void Server::updatePollMask(Connection &c) {
  if (connections.find(c.fd) == connections.end())
    return;

  short ev = 0;
  if (c.state == Connection::RECV_HEADERS || c.state == Connection::RECV_BODY)
    ev |= POLLIN;
  if (c.state == Connection::SENDING && c.output_pos < c.output_buf.size())
    ev |= POLLOUT;
  c.poll_events = ev;
}

void Server::updatePollMask(CgiPipe &p) {
  if (cgi_pipes.find(p.fd) == cgi_pipes.end())
    return;
  p.poll_events = POLLIN;
}

void Server::rebuildPollfdsIfDirty() {
  if (!pfds_dirty)
    return;

  pfds.clear();
  // listen FDs
  for (size_t i = 0; i < listen_fds.size(); i++)
    pfds.push_back((make_pfd(listen_fds[i], POLLIN)));
  // Client connections
  for (std::map<int, Connection>::iterator it = connections.begin();
       it != connections.end(); ++it) {
    const Connection &c = it->second;
    pfds.push_back(make_pfd(c.fd, c.poll_events));
  }
  // CGI pipes
  for (std::map<int, CgiPipe>::iterator it = cgi_pipes.begin();
       it != cgi_pipes.end(); ++it) {
    const CgiPipe &p = it->second;
    pfds.push_back(make_pfd(p.fd, p.poll_events));
  }
  pfds_dirty = false;
}

bool Server::has_error(short revents) {
  return (revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
}
