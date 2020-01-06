/*
	SIC LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
	Copyright(c) 2013 by LG Electronics Inc.

	This program is free software; you can redistribute it and/or 
	modify it under the terms of the GNU General Public License
	version 2 as published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of 
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
	GNU General Public License for more details.
*/ 

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "log.h"
#include "../../include/logm_kapi.h"

#if 0
#define debug(fmt, arg...)	printf ("%s.%d: " fmt, __func__, __LINE__, ##arg)
#else
#define debug(fmt, arg...)	do{}while(0)
#endif

static FILE *pid_fd = NULL;

typedef int (*polls_callback_t)(int fd, int revents, void *arg);

struct
{
	struct pollfd pd;
	polls_callback_t cb;
	void *cb_arg;
} *polls;
int polls_num;

static void error (char *str)
{
	perror (str);
	exit (1);
}

int polls_add (int fd, int events,
		polls_callback_t cb, void *cb_arg)
{
	int index;

	debug ("add %d on %d\n", fd, polls_num);

	index = polls_num;
	polls_num ++;

	polls = realloc (polls, sizeof(*polls)*polls_num);
	polls[index].pd.fd = fd;
	polls[index].pd.events = events;
	polls[index].pd.revents = 0;
	polls[index].cb = cb;
	polls[index].cb_arg = cb_arg;

	return 0;
}

int polls_del (int fd)
{
	int a;

	debug ("del %d\n", fd);

	for (a=0; a<polls_num; a++)
	{
		if (polls[a].pd.fd == fd)
		{
			polls_num --;
			for (; a<polls_num; a++)
				polls[a] = polls[a+1];
			return 0;
		}
	}

	error ("unknown fd");
	return -1;
}

int polls_wait_and_call (void)
{
	int ret;
	int a;
	int num;
	struct pollfd pds[polls_num];

	num = polls_num;

	for (a=0; a<num; a++)
		pds[a] = polls[a].pd;

	debug ("waiting...\n");
	ret = poll (pds, num, -1);
	if (ret < 0)
		error ("polls:poll");
	debug ("got %d events...\n", ret);

	for (a=0; a<num; a++)
	{
		if (pds[a].revents != 0)
		{
			debug ("call callback on %d(fd:%d), event 0x%x\n",
					a, pds[a].fd, pds[a].revents);
			polls[a].cb (pds[a].fd, pds[a].revents, polls[a].cb_arg);
		}
	}

	return 0;
}

int client_event (int fd, int revents, void *arg)
{
	debug ("event.\n");

	if (revents & POLLIN)
	{
		ssize_t readed;
		char buf[1024];

		readed = read (fd, buf, sizeof(buf)-1);
		if (readed < 0)
		{
			debug ("no data. %s\n", strerror (errno));
		}
		else
		{
			buf[readed] = 0;
			debug ("got %d bytes\n", readed);
			//debug ("got : %s\n", buf);
		}
	}

	if (revents & POLLRDHUP)
	{
		close (fd);
		polls_del (fd);
	}

	return 0;
}

int server_socket (int portno)
{
	struct sockaddr_in serv_addr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		error("server:socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
				sizeof(serv_addr)) < 0) 
		error("server:bind");
	listen(sockfd,5);

	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
		error ("F_SETFL");

	return sockfd;
}

int server_event (int fd, int revents, void *arg)
{
	struct sockaddr_in cli_addr;
	socklen_t clilen;
	int new;

	debug ("event.\n");

	clilen = sizeof(cli_addr);
	new = accept(fd, (struct sockaddr *) &cli_addr, 
			&clilen);
	if (new< 0) 
		error("server:accept");

	polls_add (new, POLLIN|POLLRDHUP, client_event, NULL);

	return 0;
}

int file_read_and_brocast_event (int fd, int revents, void *arg)
{
	debug ("event.\n");

	if (revents & POLLIN)
	{
		ssize_t readed;
		int a;
		char *buf;
		int len = 512;
		int printed = 0;

		buf = malloc (len);

read_again:
		if (buf == NULL)
		{
			error ("file_read_and_brocast_event:malloc");
			return -1;
		}

		errno = 0;
		readed = read (fd, buf, len);
		if (readed < 0)
		{
			if (errno == ENOMEM)
			{
				free(buf);
				len *= 2;
				buf = malloc (len);
				goto read_again;
			}
			error ("file_read_and_brocast_event:read");
		}

		debug ("got %d bytes from fd %d\n", readed, fd);

		for (a=0; a<polls_num; a++)
		{
			if (polls[a].cb == &client_event)
			{
				int fd = polls[a].pd.fd;

				write (fd, buf, readed);
				printed ++;
			}
		}

		if (buf[0] && !printed)
		{
			for (a=0; a<readed; a++)
			{
				if (buf[a] == '\n')
					fwrite ("\r", 1, 1, stdout);
				fwrite (buf+a, 1, 1, stdout);
			}
		}

		free (buf);
	}

	return 0;
}

int logfunnel_open (void)
{
	extern int log_opendev (int mode);
	int fd;
	int ret;

	fd = log_opendev (O_RDONLY);
	if (fd < 0)
		error ("logfunnel:open");

	ret = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (ret < 0)
		error ("logfunnel:F_SETFL");

	return fd;
}

int logm_open(void)
{
	int ret;
	int fd;
	unsigned int arg = 0;

	fd =open("/dev/lg/logm", O_RDONLY | O_TRUNC);
	if(fd < 0)
		error("logm:open");

	ret = fcntl(fd, F_SETFL, O_NONBLOCK);
	if (ret < 0)
		error ("logfunnel:F_SETFL");

	ret = ioctl(fd, LOGM_IOW_PATH_CTRL, (unsigned int)&arg);
	if (ret < 0)
		printf ("LOGM_IOW_PATH_CTRL failed\n");

	return fd;
}

int main (int argc, char **argv)
{
	int opt;
	int foreground;
	int port;
	pid_t pid;

	foreground = 0;
	port = 2012;
	while ((opt = getopt (argc, argv, "fp:")) != -1)
	{
		switch (opt)
		{
		case 'f':
			foreground = 1;
			break;

		case 'p':
			port = atoi (optarg);
			break;

		default:
			printf (
					"%s -f -p <port number>\n"
					"  -f : foreground. do not daemonize.\n"
					"  -p <port number>\n"
					"     : set listening port number. default:%d\n"
					, argv[0], port);

			return 1;
		}
	}

	if (!foreground)
		daemon (1, 1);

	polls_add (server_socket (port), POLLIN, server_event, NULL);
	polls_add (logfunnel_open (), POLLIN, file_read_and_brocast_event, NULL);
	polls_add (logm_open (), POLLIN, file_read_and_brocast_event, NULL);

	pid = getpid();
	pid_fd = fopen("/tmp/logpipe.pid", "w+");
	if(!pid_fd)
		error(" fail open logpipe.pid");
	else
	{
		fprintf(pid_fd, "pid : %d\n", pid);
		fclose(pid_fd);
	}

	while (1)
		polls_wait_and_call ();

	return 0; 
}

