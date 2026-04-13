#pragma once
#include"function.h"

class Task
{
public:
	Task(int fd, string orders, string username, int Dir)
		:fd(fd),orders(orders),username(username),Dir(Dir)
	{

	}

	int fd;//客户端socket
	string orders;//客户端发送的命令
	string username;//客户端登录用户名
	int Dir;//用户所在目录号
};

class WorkQue
{
public:
	WorkQue()
	{
		mutex = PTHREAD_MUTEX_INITIALIZER;
	}
	void insertTask(const Task& task)
	{
		deq.push_back(task);
	}
	int size()
	{
		return deq.size();
	}
	Task getTask()
	{
		Task ans = deq.front();
		deq.pop_front();
		return ans;
	}

	pthread_mutex_t mutex;
	deque<Task>deq;//存放accept接受的fd
};
