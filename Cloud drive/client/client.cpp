#include"function.h"
#include"md5.h"

int main()
{
	int ret;
	int dataLen = 0;
	char buf[1000] = { 0 };
	Packet packet;
	bool flag;
	int Dir = 0;
	string username;
	string password;
	/**************连接服务器socket********************/
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	ERROR_CHECK(sockfd, -1, "socket");
	struct sockaddr_in sockinfo;
	bzero(&sockinfo, sizeof(sockinfo));
	sockinfo.sin_addr.s_addr = htonl(INADDR_ANY);
	sockinfo.sin_port = htons(PORT);
	sockinfo.sin_family = AF_INET;
	ret = connect(sockfd, (sockaddr*)&sockinfo, sizeof(sockinfo));
	ERROR_CHECK(ret, -1, "connect");
	cout << "connect success" << endl;
	/******************连接服务器成功，进入界面：选择 注册 OR 登陆**********************/
console://当注册或登录失败时重新返回这个界面
	cout << "选择登录或者注册:" << endl
		<< "1.登录\t\t\t"
		<< "2.注册\t\t\t"
		<< "3.退出"
		<< endl;
	string order;
	int choice;
	do
	{
		cin >> choice;
		switch (choice)
		{
		case 1:
		{
			cout << "请输入用户名：" << endl;
			cin >> username;
			cout << "请输入密码：" << endl;
			cin >> password;
			order = "logIn " + username;
			packet.dataLen = strlen(order.c_str());
			memcpy(packet.buf, order.c_str(), packet.dataLen);
			sendCycle(sockfd, &packet, 4 + packet.dataLen);
			/*接收服务器查询结果，有无此用户*/
			recvCycle(sockfd, &flag, 1);
			if (flag == false)
			{
				cout << "用户不存在!" << endl
					<< endl;
				goto console;

			}
			/*接收盐值*/
			bzero(buf, sizeof(buf));
			recvCycle(sockfd, &dataLen, 4);
			recvCycle(sockfd, buf, dataLen);
			/*将接收的盐值使用crypt函数加密后在发送*/
			char* cipher = crypt(password.c_str(), buf);
			packet.dataLen = strlen(cipher);
			memcpy(packet.buf, cipher, packet.dataLen);
			sendCycle(sockfd, &packet, 4 + packet.dataLen);
			/*判断最后是否连接成功*/
			recvCycle(sockfd, &flag, 1);
			if (flag == true)
			{
				cout << "验证成功！" <<endl;
				goto client_system;
			}
			else
			{
				cout << "验证失败！账号或密码有误！" << endl;
				goto console;
			}
			break;
		}
		case 2:
		{
			/*进行注册操作*/
			cout << "请输入用户名：" << endl;
			cin >> username;
			cout << "请输入密码：" << endl;
			cin >> password;
			order = "signIn " + username + " " + password;
			packet.dataLen = strlen(order.c_str());
			memcpy(packet.buf, order.c_str(), packet.dataLen);
			sendCycle(sockfd, &packet, 4 + packet.dataLen);
			/*判断是否已经有相同的用户*/
			recvCycle(sockfd, &flag, 1);
			if (flag == true)
			{
				cout << "注册成功!" << endl
					<< endl;
				goto console;

			}
			else
			{
				cout << "注册失败！用户名已经存在！" << endl
					<< endl;
				goto console;
			}
			break;
		}
		case 3:
		{
			order = "quit";
			packet.dataLen = strlen(order.c_str());
			memcpy(packet.buf, order.c_str(), packet.dataLen);
			sendCycle(sockfd, &packet, 4 + packet.dataLen);
			exit(0);
		}
		default:
		{
			cout << "输入错误！重新输入！" << endl
				<< endl;
			break;
		}
		}
	} while (1);
	
	/*进入客户端系统*/
client_system:
	while (1)
	{
		/*读取命令*/
		cout << username << "@client:";
		fflush(stdout);
		bzero(&packet, sizeof(packet));
		packet.dataLen = read(STDIN_FILENO, packet.buf, sizeof(packet.buf));
		ERROR_CHECK(packet.dataLen, -1, "read");

		/*分析标准输入端读取的命令*/
		string orders(packet.buf);
		stringstream ss(orders);
		string order, name, order2;
		ss >> order >> name >> order2;

		/**************ls****************/
		if (order == "ls")
		{
			sendCycle(sockfd, &packet, 4+packet.dataLen);

			packet.dataLen = strlen(username.c_str());
			memcpy(packet.buf, username.c_str(), packet.dataLen);
			sendCycle(sockfd, &packet, 4 + packet.dataLen);

			sendCycle(sockfd, &Dir, 4);
			/*读取服务器返回的信息*/
			bzero(buf, sizeof(buf));
			recvCycle(sockfd, &dataLen, 4);
			recvCycle(sockfd, buf, dataLen);
			cout << "-------所有文件------------" << endl;
			cout << buf;
			cout << "----------所有文件-----------" << endl;
		}
		/***********mkdir**************/
		else if (order == "mkdir")
		{
			sendCycle(sockfd, &packet, 4 + packet.dataLen);

			packet.dataLen = strlen(username.c_str());
			memcpy(packet.buf, username.c_str(), packet.dataLen);
			sendCycle(sockfd, &packet, 4 + packet.dataLen);

			sendCycle(sockfd, &Dir, 4);
			/*读取服务器返回的信息*/
			recvCycle(sockfd, &flag, 1);
			if (flag)
			{
				cout << "创建目录成功！" << endl;
			}
			else
			{
				cout << "创建失败！有同名目录！" << endl;
			}
		}
		/**************rmdir****************/
		else if (order == "rmdir")
		{
			sendCycle(sockfd, &packet, 4 + packet.dataLen);

			packet.dataLen = strlen(username.c_str());
			memcpy(packet.buf, username.c_str(), packet.dataLen);
			sendCycle(sockfd, &packet, 4 + packet.dataLen);

			sendCycle(sockfd, &Dir, 4);

			recvCycle(sockfd, &flag, 1);
			if (flag)
			{
				cout << "删除成功！" << endl;
			}
			else
			{
				cout << "删除失败！没有该文件" << endl;
			}
		}
		/*******************cd*****************/
		else if (order == "cd")
		{
			if (Dir == 0 && ".." == name)
			{
				cout << "已经到达根目录" << endl;
			}
			else if ("." == name)
			{
				/*do nothing*/
			}
			else
			{
				sendCycle(sockfd, &packet, 4 + packet.dataLen);

				packet.dataLen = strlen(username.c_str());
				memcpy(packet.buf, username.c_str(), packet.dataLen);
				sendCycle(sockfd, &packet, 4 + packet.dataLen);

				sendCycle(sockfd, &Dir, 4);

				recvCycle(sockfd, &flag, 1);
				if (flag)
				{
					recvCycle(sockfd, &Dir, 4);
				}
				else
				{
					cout << "没有" << name << "文件！" << endl;
				}
			}
		}
		/****************rm****************/
		else if (order == "rm")
		{
			sendCycle(sockfd, &packet, 4 + packet.dataLen);

			packet.dataLen = strlen(username.c_str());
			memcpy(packet.buf, username.c_str(), packet.dataLen);
			sendCycle(sockfd, &packet, 4 + packet.dataLen);

			sendCycle(sockfd, &Dir, 4);

			recvCycle(sockfd, &flag, 1);
			if (flag)
			{
				cout << "删除成功！" << endl;
			}
			else
			{
				cout << "删除失败！没有该文件！" << endl;
			}
		}
		/******************puts*******************/
		else if (order == "puts")
		{
			/*启动多线程，帮助上传*/
			LoadTask task;
			task.order = orders;
			task.username = username;
			task.Dir = Dir;
			pthread_t pth1;
			pthread_create(&pth1, NULL, upLoad, &task);
			pthread_join(pth1, NULL);
		}
		/******************gets********************/
		else if (order == "gets")
		{
			/*启动多线程，帮助下载*/
			/*断点续传功能*/
			struct stat statbuf;
			ret = stat(name.c_str(), &statbuf);
			if (-1 == ret)
			{
				orders = order + " " + name + " 0";
			}
			else
			{
				orders = order + " " + name + " " + to_string(statbuf.st_size);
			}
			LoadTask task;
			task.order = orders;
			task.username = username;
			task.Dir = Dir;
			pthread_t pth2;
			pthread_create(&pth2, NULL, downLoad, &task);
			pthread_join(pth2, NULL);
		}
		else if (order == "quit")
		{
			sendCycle(sockfd, &packet, 4 + packet.dataLen);
		}
		else
		{
			cout << "Wrong Command!" << endl;
		}
	}

	close(sockfd);
	return 0;
}
