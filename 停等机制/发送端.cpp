#pragma comment(lib, "Ws2_32.lib")
#include<iostream>
#include<WinSock2.h>
#include<string>
#include<string.h>
#include<time.h>
#include<fstream>
#include<stdio.h>
#include<vector>
using namespace std;
SOCKET client;
SOCKADDR_IN serverAddr, clientAddr;
char SEQ1 = '0';
char ACK1 ='0';
#define WAVE1 '0'
#define ACKW1 '1'
#define ACKW2 WAVE1 + 1
#define LENGTH 16378
#define CheckSum 16384 - LENGTH
#define LAST 'A'
#define NOTLAST 'B'
#define TEST 'C'
bool QUIT = true;
bool No1 = 1;
int ack1 = 0;
char message[200000000];
// checksum检查包
unsigned char PkgCheck(char* arr, int len)//计算校验和的函数，一共有8位校验码
{
	if (len == 0)
		return ~(0);
	unsigned char ret = arr[0];
	for (int i = 1; i < len; i++)
	{
		unsigned int tmp = ret + (unsigned char)arr[i];
		tmp = tmp / (1 << 8) + tmp % (1 << 8);
		ret = tmp;
	}
	return ~ret;//反码
}
// 发包
void Sendpackage(char* msg, int len, int index, int last,int u)
{
	char* buffer = new char[len + CheckSum];
	if (last)
	{
		buffer[1] = LAST;
	}
	else
	{
		buffer[1] = NOTLAST;
	}
	buffer[2] = index / 128;//数据报的标号
	buffer[3] = index % 128;
	buffer[4] = len / 128;//数据报的大小
	buffer[5] = len % 128;

	ack1 = int(buffer[2]) * 128 + buffer[3];

	for (int i = 0; i < len; i++)
	{
		buffer[i + CheckSum] = msg[i];
	}
	while (1)//发送文件具体信息，每次发送部分
	{
		buffer[0] = PkgCheck(buffer + 1, len + CheckSum - 1);
		u = buffer[0];
		if (u < 0)
			u = 255 + u;
		sendto(client, buffer, LENGTH + CheckSum, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
		int begin_time = clock();
		char recv[3];
		int clientlen = sizeof(clientAddr);
		while (recvfrom(client, recv, 3, 0, (sockaddr*)&serverAddr, &clientlen) == SOCKET_ERROR)
		{//超时重传，若超时未收到ACK，则进行重传
			int over_time = clock();
			if (over_time - begin_time > 500)
			{
				break;
			}
		}
		//停等机制，收到ACK后再发送下一条
		if ((int(recv[0])*128+int(recv[1]))==ack1)
		{
			if (index != recv[0] * 128 + recv[1])
				continue;
			cout <<"消息已送到   "<<"校验码="<< u << " ACK=" << ack1 << "   ";
			
			break;
		}
		else//确认重传机制
		{
			cout << "发生错误，重新传送  " <<endl;
			ack1 = int(buffer[2]) * 128 + int(buffer[3]);
		}
	}
	delete buffer;
}
// 发送消息
void Sendmessage(string filename, int size)
{
	int len = 0;
	if (filename == "quit")
	{
		QUIT = false;
		return;
	}
	else
	{
		ifstream fin(filename.c_str(), ifstream::binary);
		if (!fin)
		{
			cout << "文件不存在!" << endl;
			return;
		}
		unsigned char ch = fin.get();
		while (fin)
		{
			message[len] = ch;
			len++;
			ch = fin.get();
		}
		fin.close();
	}
	int package_num = len / LENGTH + (len % LENGTH != 0);
	static int index = 0;
	int u=0;
	clock_t timestart = clock();
	for (int i = 0; i < package_num; i++)
	{
		Sendpackage(message + i * LENGTH, i == package_num - 1 ? len - (package_num - 1) * LENGTH : LENGTH, index++, i == package_num - 1,u);
		if (i % 1== 0)
		{
			printf("已经传输:%.2f%%\n", (float)i / package_num * 100);
		}
	}
	clock_t timeend = clock();
	double time= (double)(timeend - timestart) / CLOCKS_PER_SEC;
	cout << "耗时为：" << time << "s"<<endl;
	cout << "吞吐率为：" << len *8/ time /1024/1024<< "Mbps" << endl;
	cout << len << endl;
}
// 连接到服务器
void ConnectToServer()
{
	while (1)
	{
		char send[2];
		send[0] = SEQ1;//初始化SEQ
		send[1] = ACK1;//初始化ACK
		sendto(client, send, 2, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
		cout << "发送端进行第一次握手指令。" << " SEQ=" << send[0] << " ACK=" << send[1] << endl;
		int begin_time = clock();
		bool flag = 1;//标志是否发送成功
		char recv[2];
		int clientlen = sizeof(clientAddr);
		while (recvfrom(client, recv, 2, 0, (sockaddr*)&serverAddr, &clientlen) == SOCKET_ERROR)
		{
			int over_time = clock();
			if (over_time - begin_time > 500)//判断是否超时
			{
				flag = 0;//输出发送失败
				cout << "发送失败\n请重新发送" << endl;
				break;
			}
		}
		if (flag && recv[0] == SEQ1 && recv[1] == ACK1+1)
		{
			cout << "发送端接收到第二次握手指令。" << " SEQ=" << recv[0] << " ACK=" << recv[1] << endl;
			send[0] = SEQ1+1;//SEQ和ACK加一表示握手成功
			send[1] = ACK1+1;
			sendto(client, send, 2, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
			cout << "发送端发送第三次握手指令。" << " SEQ=" << send[0] << " ACK=" << send[1] << endl;
			break;
		}
	}
}
// 发送文件名称
void SendName(string name, int size)
{
	char* Name = new char[size + 1];
	for (int i = 0; i < size; i++)
	{
		Name[i] = name[i];
	}
	Name[size] = '#';//标识名称最后一位，方便接收端识别
	sendto(client, Name, size + 1, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
	delete Name;
}
// 关闭客户端，进行挥手操作
void CloseClient()
{
	while (1)
	{
		char send[2];
		send[0] = WAVE1;
		send[1] = ACKW1;
		sendto(client, send, 2, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
		cout << "发送端发送第一次挥手." << " SEQ=" << send[0] << " ACK=" << send[1] << endl;
		int begin_time = clock();
		bool flag = 1;
		char recv[2];
		int clientlen = sizeof(clientAddr);
		while (recvfrom(client, recv, 2, 0, (sockaddr*)&serverAddr, &clientlen) == SOCKET_ERROR)
		{
			int over_time = clock();
			if (over_time - begin_time > 500)
			{
				flag = 0;
				cout << "超时！" << endl;
				break;
			}
		}
		if (flag && recv[0] == ACKW1 && recv[1] == WAVE1 + 1)
		{
			cout << "断开连接。" << " SEQ=" << recv[0] << " ACK=" << recv[1] << endl;
			break;
		}
	}
}
int main()
{
	//开始与客户端进行连接
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)//
	{
		cout << "打开错误:" << WSAGetLastError() << endl;
		return -1;
	}
	client = socket(AF_INET, SOCK_DGRAM, 0);

	if (client == SOCKET_ERROR)
	{
		cout << "Socket 错误:" << WSAGetLastError() << endl;
		return -1;
	}
	int Port = 1234;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(Port);
	cout << "开始与接收端连接!" << endl;


	ConnectToServer();
	cout << "成功连接!" << endl;
	int time_out = 1;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(time_out));


	string name;
	cin >> name;
	SendName(name, name.length());
	Sendmessage(name, name.length());
	cout << "成功发送消息!" << endl;
	
	CloseClient();
	WSACleanup();
	return 0;
}
