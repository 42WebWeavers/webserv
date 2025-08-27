/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: prutkows <prutkows@student.42warsaw.pl>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/07/16 19:04:52 by prutkows          #+#    #+#             */
/*   Updated: 2025/08/26 12:19:46 by prutkows         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <cstddef>
#include <ctime>
#include <map>
#include <string>
#include <sys/poll.h>
#include <vector>

class IHttpHandle;

struct Connection {
  // client FD
  int fd;

  // buffers I/O
  std::string input_buf;
  std::string output_buf;
  size_t output_pos;

  size_t body_expected;
  size_t body_received;

  // managment life connection
  bool keep_alive;
  bool close_after_write;

  // timeouts
  time_t last_activity;

  // pointer to httpHandle, Zyta part.
  IHttpHandle *httpHandle;

  // mask for poll()
  short poll_events;

  // simple automation state
  enum ConnState {
    RECV_HEADERS,
    RECV_BODY,
    PROCESSING,
    SENDING,
    CLOSING
  } state;

  bool closed;

  Connection(int fd_)
      : fd(fd_), output_pos(0), body_expected(0), body_received(0),
        keep_alive(false), close_after_write(false),
        last_activity(std::time(NULL)), httpHandle(0), poll_events(POLLIN),
        state(RECV_HEADERS), closed(false) {}
};

struct CgiPipe {
  // FD with CGI process (stdout/stderr), which i read non blocking
  int fd;
  int client_fd;
  bool is_stderr;
  short poll_events;
  bool closed;
  time_t last_activity;

  CgiPipe()
      : fd(-1), client_fd(-1), is_stderr(false), poll_events(0), closed(false),
        last_activity(0) {}
};

class Server {
public:
  Server(int timeout_ms = 30000)
      : timeout_ms(timeout_ms), running(false), pfds_dirty(false) {}

  ~Server() {}

  // setup
  void addListenSocket(int fd);                            // ✅
  void addCgiPipes(int out_fd, int err_fd, int client_fd); // ✅

  // main
  void run(); // ✅

  // helpers
  static void print_error(const char *msg);            // ✅
  static bool set_nonblock_cloexec(int fd);            // ✅
  static struct pollfd make_pfd(int fd, short events); // ✅

private:
  // blocking copy server
  Server(const Server &src);
  Server &operator=(const Server &rhs);

  // state of server
  std::vector<int> listen_fds;
  std::map<int, Connection> connections;
  std::map<int, CgiPipe> cgi_pipes;
  std::vector<struct pollfd> pfds;
  int timeout_ms;
  bool running;
  bool pfds_dirty;

  // event handling
  void handleAccept(int listenFd); // ✅
  void handleClientRead(Connection &c);
  void handleClientWrite(Connection &c);       // ✅
  void resumeCgiPipesForClient(int client_fd); // ✅
  void handleCgiRead(CgiPipe &p);              // ✅

  // Closing / cleanup
  void close_connection(int client_fd); // ✅
  void close_cgi_pipe(int pipe_fd);     // ✅

  // updates masks poll()
  void updatePollMask(Connection &c); // ✅
  void updatePollMask(CgiPipe &p);    // ✅
  void rebuildPollfdsIfDirty();       // ✅

  static bool has_error(short revents); // POLLERR|POLLHUP|POLLNVAL ✅
};

#endif
