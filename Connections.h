// ChildList.h
// Copyright 2018/05/26 Robin.Rowe@CinePaint.org
// License MIT open source

#ifndef Connections_h
#define Connections_h

#ifdef __cplusplus

#include <map>
#include <deque>
#include "run-command.h"

static int addrcmp(const struct sockaddr_storage *s1,const struct sockaddr_storage *s2);

inline
bool operator<(const sockaddr_storage& left,const sockaddr_storage& right)
{	return addrcmp(&left,&right) < 0;
}

class Connections
{	static const int UNLIMITED = 0;
	int maxConnections;
	std::map<struct sockaddr_storage,struct child_process> connections;
	std::deque<std::pair<struct sockaddr_storage,struct child_process> > connectionsMore;
	bool CheckConnections();	
	void CloseOneConnection();
public:
	Connections()
	:	maxConnections(32)
	{}
	int GetMaxConnections() const
	{	return maxConnections;
	}
	void SetMaxConnections(int maxConnections)
	{	if(maxConnections <= 0)
		{	this->maxConnections = UNLIMITED;
			return;
		}
		this->maxConnections = maxConnections;
	}
	int GetConnectionsCount() const
	{	return connections.size() + connectionsMore.size();
	}
	void AddConnection(child_process *cld,sockaddr *addr,socklen_t addrlen);
	int CloseDeadConnections();
};

extern "C" {
#endif

void SetMaxConnections(int maxConnections);
void AddConnection(struct child_process *cld, struct sockaddr *addr, socklen_t addrlen);
int CloseDeadConnections();

#ifdef __cplusplus
}
#endif

#endif
