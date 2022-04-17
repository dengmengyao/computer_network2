#pragma comment(lib, "Ws2_32.lib")
#include<iostream>
#include<WinSock2.h>
#include<string>
#include<string.h>
#include<fstream>
using namespace std;
SOCKET server;
SOCKADDR_IN serverAddr, clientAddr;
char SEQ1 = '0';
char ACK1 = '0';
#define WAVE1 '0'
#define ACKW1 '1'
#define ACKW2 WAVE1 + 1
#define LENGTH 16378
#define CheckSum 16384 - LENGTH
#define LAST 'A'
#define NOTLAST 'B'
string name;
char message[200000000];
int ack1 = 0;
static char pindex[2];		// 用于标志已经收到了哪个包
// checksum检查包
unsigned char PkgCheck(char* arr, int len)//计算校验和的函数
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
// 等待连接
void WaitConnection()
{
	while (1)
	{
		char recv[2];
		int clientlen = sizeof(clientAddr);
		while (recvfrom(server, recv, 2, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
		if (recv[0] != SEQ1)
			continue;
		cout << "接收端接收到第一次握手指令。" << " SEQ="<<recv[0]<<" ACK="<<recv[1]<<endl;
		bool flag = 1;//判断是否连接成功
		while (1)
		{
			memset(recv, 0, 2);
			char send[2];
			send[0] = SEQ1;
			send[1] = ACK1+1;
			sendto(server, send, 2, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
			cout << "接收端发送第二次握手指令。" << " SEQ=" << send[0] << " ACK=" << send[1]<<endl;
			while (recvfrom(server, recv, 2, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
			cout << "接收端接收到第三次握手指令。" << " SEQ=" << recv[0] << " ACK=" << recv[1] << endl;
			if (recv[0] == SEQ1)
				continue;
			if (recv[0] != SEQ1+1 || recv[1] != ACK1+1)
			{
				cout << "连接失败\n请重启发送端" << endl;
				flag = 0;
			}
			break;
		}
		if (!flag)
			continue;
		break;
	}
}

// 等待断开连接
void WaitDisconnection()
{
	while (1)
	{
		char recv[2];
		int clientlen = sizeof(clientAddr);
		while (recvfrom(server, recv, 2, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
		if (recv[0] != WAVE1)
			continue;
		cout << "接收端收到第一次挥手." << " SEQ=" << recv[0] << " ACK=" << recv[1] << endl;
		char send[2];
		send[0] = ACKW1;
		send[1] = WAVE1+1;
		sendto(server, send, 2, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
		break;
	}
	cout << "接收端断开连接。" << endl;
}
// 接收消息
void Recvmessage()//没成功进行一次传输，ACK都会加一，若ACK未变化，表明传输失败，需要进行重传
{
	int len = 0;
	while (1)
	{
		char recv[LENGTH + CheckSum];
		memset(recv, '\0', LENGTH + CheckSum);
		int length;
		while (1)
		{
			int clientlen = sizeof(clientAddr);
			while (recvfrom(server, recv, LENGTH + CheckSum, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
			length = recv[4] * 128 + recv[5];
			char send[3];
			memset(send, '\0', 3);
			int t = recv[0];
			if (t < 0)
				t = 255 + t;
			cout << "成功接收到消息！ 校验码=" << t << " ACK=" << (int(recv[2]) * 128 + int(recv[3])) <<endl;
			// 检查ACK
			
			if ((int(recv[2]) * 128 + int(recv[3])) == (ack1))
			{   //如果接收到消息，对消息进行接收
				ack1 = int(recv[2]) * 128 + int(recv[3])+1;
				// 这一个包是不是顺序的下一个包
				//停等机制
				if (PkgCheck(recv, length + CheckSum) == 0)
				{
					send[0] = recv[2];
					send[1] = recv[3];
				}
				else
				{
					send[0] = -1;
					send[1] = -1;
					sendto(server, send, 3, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
					continue;
				}
				sendto(server, send, 3, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
				break;
			}
			else//发生错误，等待发送端重新进行传输
			{
				cout << "发生错误!" << " ACK应该为="<< int(recv[2]) * 128 + int(recv[3])+1 <<endl;
				continue;
			}
		}
		for (int i = CheckSum; i < length + CheckSum; i++)
		{
			message[len] = recv[i];
			len++;
		}
		if (recv[1] == LAST)
			break;
	}
	ofstream fout(name.c_str(), ofstream::binary);
	for (int i = 0; i < len; i++)
		fout << message[i];
	fout.close();
}
// 接收文件名称
void RecvName()
{
	char Name[100];
	int clientlen = sizeof(clientAddr);
	while (recvfrom(server, Name, 100, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
	for (int i = 0; Name[i] != '#'; i++)//如果没有到最后一位，则一直存储名称
	{
		name += Name[i];
	}
}
int main()
{
	//开始接收与发送端连接
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "打开错误:" << WSAGetLastError() << endl;
		return -1;
	}
	server = socket(AF_INET, SOCK_DGRAM, 0);

	if (server == SOCKET_ERROR)
	{
		cout << "Socket 错误:" << WSAGetLastError() << endl;
		return -1;
	}
	int Port = 1234;
	serverAddr.sin_addr.s_addr = htons(INADDR_ANY);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(Port);

	if (bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cout << "Bind错误:" << WSAGetLastError() << endl;
		return -1;
	}
	cout << "开始与发送端连接!" << endl;


	WaitConnection();
	cout << "成功与发送端连接!" << endl;

	RecvName();
	Recvmessage();
	cout << "成功接收消息!" << endl;
	
	WaitDisconnection();
	closesocket(server);
	WSACleanup();
	return 0;
}