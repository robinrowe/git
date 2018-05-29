// Connections.cpp
// Copyright 2018/05/26 Robin.Rowe@CinePaint.org
// License MIT open source

#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <signal.h>
#include <syslog.h>
#include <sched.h>
#include <sys/wait.h>
#include "git-compat-util.h"
#include "run-command.h"
#include "Connections.h"

static Connections connections;
void loginfo(const char *err, ...);

void SetMaxConnections(unsigned maxConnections)
{	connections.SetMaxConnections(maxConnections);
}

void AddConnection(struct child_process *cld, struct sockaddr *addr, socklen_t addrlen)
{	connections.AddConnection(cld,addr,addrlen);
}

int CloseDeadConnections()
{	return connections.CloseDeadConnections();
}

bool Connections::CheckConnections()
{	if(UNLIMITED == GetMaxConnections())
	{	return true;
	}
	if(GetConnectionsCount() < GetMaxConnections()) 
	{	return true;
	}
	CloseOneConnection();
	if(GetConnectionsCount() < GetMaxConnections()) 
	{	return true;
	}
	return false;
}

static int addrcmp(const struct sockaddr_storage *s1,const struct sockaddr_storage *s2)
{	const struct sockaddr *sa1 = (const struct sockaddr*) s1;
	const struct sockaddr *sa2 = (const struct sockaddr*) s2;
	if(sa1->sa_family != sa2->sa_family)
	{	return sa1->sa_family - sa2->sa_family;
	}
	if(sa1->sa_family == AF_INET)
	{	return memcmp(&((struct sockaddr_in *)s1)->sin_addr,
					  &((struct sockaddr_in *)s2)->sin_addr,
					  sizeof(struct in_addr));
	}
#ifndef NO_IPV6
	if(sa1->sa_family == AF_INET6)
	{	return memcmp(&((struct sockaddr_in6 *)s1)->sin6_addr,
					  &((struct sockaddr_in6 *)s2)->sin6_addr,
					  sizeof(struct in6_addr));
	}
#endif
	return 0;
}

void Connections::AddConnection(child_process *cld,sockaddr *addr,socklen_t addrlen)
{	std::pair<sockaddr_storage,child_process> data;
	memcpy(&(data.first),addr,addrlen);
	data.second = *cld; 
	auto p = connections.find(data.first);
	if(connections.end() == p)
	{	connections[data.first]=data.second;
		return;
	}
	connectionsMore.push_front(data);
}

void Connections::CloseOneConnection()
{	if(0 == GetConnectionsCount())
	{	return;
	}
	if(connectionsMore.empty())
	{	kill(connections.begin()->second.pid,SIGTERM);
		sleep(1);// Performance bug?
		return;
	}
	auto p = connectionsMore.back();
	connectionsMore.pop_back();
	kill(p.second.pid, SIGTERM);
	sleep(1);  // Performance bug? give it some time to die 
}

int Connections::CloseDeadConnections()
{	int count = 0;
	int status;
	pid_t pid;
	{	auto conn = connections.begin();
		while(conn != connections.end())
		{	if((pid = waitpid(conn->second.pid,&status,WNOHANG)) <= 1)
			{	conn++;
				continue;
			}
			const char *dead = "";
			if(status)
			{	dead = " (with error)";
			}
			loginfo("[%" PRIuMAX "] Disconnected%s", (uintmax_t)pid, dead);
			/* remove the child */
			child_process_clear(&conn->second);
			auto zombie = conn;
			conn++;
			connections.erase(zombie);
			count++;
	}	}
	{	auto conn = connectionsMore.begin();
		while(conn != connectionsMore.end())
		{	if((pid = waitpid(conn->second.pid,&status,WNOHANG)) <= 1)
			{	conn++;
				continue;
			}
			const char *dead = "";
			if(status)
			{	dead = " (with error)";
			}
			loginfo("[%" PRIuMAX "] Disconnected%s", (uintmax_t)pid, dead);
			/* remove the child */
			child_process_clear(&conn->second);
			auto zombie = conn;
			conn++;
			connectionsMore.erase(zombie);
			count++;
	}	}
	return count;
}
