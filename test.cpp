
#include <cerrno>
#include <poll.h>
#include <sys/_types/_ssize_t.h>
#include <sys/poll.h>
#include <vector>

// struct pollfd
// {
// 	int fd;
// 	short events;
// 	short revents;
// };

int main()
{
	// struct pollfd fds[1];
	// fds[0].fd = server_fd;
	// fds[0].events = POLLIN;
	// fds[0].revents = 0;

	// int ret = poll(fds, 1, 1000);
	// if (ret)
	std::vector<char> buffer;
	while (true)
	{
		ssize_t n = read(fd, buf, buf_size);
		if (n > 0)
			buffer .insert(buffer.end(), buf, buf + n);
		else if (n == 0)
		{
			close(fd);
			break;
		}
		else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			break;

	}


	return 0;
}
