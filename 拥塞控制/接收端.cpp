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
char SEQ1 = '0';
char ACK1 = '0';
#define WAVE1 '0'
#define ACKW1 '1'
#define ACKW2 WAVE1 + 1
#define LAST 'Y'
#define NOTLAST 'N'
SOCKET Receive, Send;
#define LENGTH 8185//数据长度
#define HEADER 7//头部长度
#define LENGTHALL 8192//数据包长度
const int MAX = 200000000;//内存中最大缓存数据包大小
SOCKADDR_IN serverAddress, clientAddress;
char sendclose[2];//关闭发送端
char recvclose[2];
char sendconnection[2];//与接收端连接
char recvconnection[2];
string name;
int MessageLength[MAX];//每个数据包的长度
char message[MAX];
struct bag {//定义包的数据结构
	char syn;
	char ack[2];
	char fin;
	char check;
	char size[2];
	char package[10000];
};

bag Package(char* Message, int len) {//对数据包进行封装
	bag Bag;
	Bag.fin = Message[1];//FIN，表示是否为最后一个包
	Bag.ack[0] = Message[2]; Bag.ack[1] = Message[3];//ack
	Bag.size[0] = Message[4]; Bag.size[1] = Message[5];//包的大小
	Bag.syn = Message[6];//SYN
	Bag.check = Message[0];//校验和
	for (int i = 0; i < len; i++)
		Bag.package[i] = Message[i + 7];//具体信息
	return Bag;
}
unsigned char CheckPackage(char* arry, int length)//计算校验和的函数，一共有8位校验码
{
	if (length == 0)
		return ~(0);
	unsigned char retu = arry[0];
	unsigned int check;
	for (int i = 1; i < length; i++)
	{
		check = retu + (unsigned char)arry[i];
		unsigned int dived = check / (1 << 8);
		unsigned int comop = check % (1 << 8);
		check = dived + comop;
		retu = check;
	}
	return ~retu;//反码
}

void Recvname() {//接收文件名称
	char names[100];
	int clientlen = sizeof(clientAddress);
	bool flag = 1;
	int begin_time = clock();
	while (recvfrom(Receive, names, 100, 0, (sockaddr*)&clientAddress, &clientlen) == SOCKET_ERROR)
	{
		int over_time = clock();
		if (over_time - begin_time > 200)
		{
			flag = 0;
			cout << "未收到文件名。" << endl;
			continue;
		}
	}
	if (flag)
	{
		for (int i = 0; names[i] != '#'; i++)
			name += names[i];
	}
}

void WaitConnection() {//等待连接
	while (1)
	{
		int len = sizeof(clientAddress);
		while (recvfrom(Receive, recvconnection, 2, 0, (sockaddr*)&clientAddress, &len) == SOCKET_ERROR);
		if (recvconnection[0] != SEQ1)
			continue;
		else {
			cout << "接收端接收到第一次握手指令。" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
			bool flag = 1;//判断是否连接成功
			sendconnection[0] = SEQ1; sendconnection[1] = ACK1 + 1;
			while (1)
			{
				memset(recvconnection, 0, 2);
				sendto(Receive, sendconnection, 2, 0, (sockaddr*)&clientAddress, sizeof(clientAddress));
				cout << "接收端发送第二次握手指令。" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
				while (recvfrom(Receive, recvconnection, 2, 0, (sockaddr*)&clientAddress, &len) == SOCKET_ERROR);
				if (recvconnection[0] == SEQ1)
					continue;
				else if (recvconnection[0] != SEQ1 + 1 || recvconnection[1] != ACK1 + 1)
				{
					cout << "连接失败\n请重启发送端" << endl;
					flag = 0;
				}
				else {
					cout << "接收端接收到第三次握手指令。" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
					break;
				}
				break;
			}
			if (flag != 1)
				continue;
			else
				break;
		}
	}
}
void WaitDisconnection() {//断开连接
	int len = sizeof(clientAddress);
	while (1)
	{
		sendclose[0] = ACKW1; sendclose[1] = WAVE1 + 1;
		while (recvfrom(Receive, recvclose, 2, 0, (sockaddr*)&clientAddress, &len) == SOCKET_ERROR);
		if (recvclose[0] != WAVE1) {
			cout << "接收端未收到第一次挥手，请重试！" << endl;
			continue;
		}
		else {
			cout << "接收端收到第一次挥手." << " SEQ=" << recvclose[0] << " ACK=" << recvclose[1] << endl;
			sendto(Receive, sendclose, 2, 0, (sockaddr*)&clientAddress, sizeof(clientAddress));
			break;
		}
	}
	cout << "接收端断开连接。" << endl;
}



void RecvMessage() {
	int check = 1;
	int lens, number = 0;// , ackbefore = -1;
	int clientlen = sizeof(clientAddress);
	while (true)
	{
		char receiveMessage[LENGTHALL];
		memset(receiveMessage, '\0', sizeof receiveMessage);
		while (true)
		{
			clientlen = sizeof(clientAddress);
			int begin = clock();
			bool flag = true;
			while (recvfrom(Receive, receiveMessage, LENGTHALL, 0, (sockaddr*)&clientAddress, &clientlen) == SOCKET_ERROR) {}
			//接收消息
			int length = receiveMessage[4] * 128 + receiveMessage[5];
			bag bag1 = Package(receiveMessage, length);

			number = bag1.ack[0] * 128 + bag1.ack[1];
			lens = bag1.size[0] * 128 + bag1.size[1];
			char receiveSuccess[3];
			memset(receiveSuccess, '\0', 3);
			unsigned int cks = CheckPackage(receiveMessage, lens + HEADER);// 如果没有超时，且ACK码正确，差错检查正确
			if (flag && cks == 0)
			{
				receiveSuccess[0] = 1;
				receiveSuccess[1] = bag1.ack[0]; receiveSuccess[2] = bag1.ack[1];
				int t = bag1.check;
				if (t < 0)
					t = t + 255;
				sendto(Receive, receiveSuccess, 3, 0, (sockaddr*)&clientAddress, sizeof(clientAddress));

				cout << "成功接收到消息！ 校验和" << t << " ACK=" << (int(bag1.ack[0]) * 128 + int(bag1.ack[1]));
				cout << "   SYN为：" << bag1.syn << "   fin为:" << bag1.fin;
				cout << "  校验和为：" << t << "   窗口向前移动" << endl;
				break;
			}
			else
			{
				if (receiveMessage[0] == 'f' && receiveMessage[1] == 'i' && receiveMessage[2] == 'n')
				{//结束状态
					cout << "接收文件内容完毕。" << endl;
					int length = 0;
					int pkgnum;
					for (pkgnum = 0; MessageLength[pkgnum] != '\0'; pkgnum++)
						length += MessageLength[pkgnum];
					ofstream fout(name.c_str(), ofstream::binary);
					for (int i = 0; i < length; i++)
						fout << message[i];
					fout.close();
					check = 0;
					break;
				}

				receiveSuccess[0] = 0;
				receiveSuccess[1] = receiveMessage[2]; receiveSuccess[2] = receiveMessage[3];
				sendto(Receive, receiveSuccess, 3, 0, (sockaddr*)&clientAddress, sizeof(clientAddress));
				continue;
			}
		}
		// 收到的包可能会是乱序的，因此我们要确定他的位置，此时要根据现在的ACK对数据进行存储
		int curpkgno = LENGTH * (receiveMessage[2] * 128 + receiveMessage[3]);
		MessageLength[receiveMessage[2] * 128 + receiveMessage[3]] = lens;
		for (int i = HEADER; i < lens + HEADER; i++)
		{
			message[curpkgno + i - HEADER] = receiveMessage[i];
		}
		if (check == 0)
			break;
	}
}

int main()
{
	//启动接收端
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup 错误：" << WSAGetLastError() << endl;
		return 0;
	}
	Receive = socket(AF_INET, SOCK_DGRAM, 0);

	if (Receive == SOCKET_ERROR)
	{
		cout << "套接字错误：" << WSAGetLastError() << endl;
		return 0;
	}

	//serverAddress.sin_addr.s_addr = htons(INADDR_ANY);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(1234);

	if (bind(Receive, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cout << "端口错误：" << WSAGetLastError() << endl;
		return 0;
	}
	WaitConnection();
	cout << "成功连接到发送端！" << endl;
	Recvname();
	RecvMessage();
	WaitDisconnection();//等待发送端断开连接
	cout << "发送端断开连接。" << endl;
	closesocket(Receive);
	closesocket(Send);
	WSACleanup();
	return 0;
}