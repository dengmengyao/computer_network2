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
#define WAVE '0'
#define ACK '1'
#define ACKS WAVE + 1
#define LAST 'Y'
#define NOTLAST 'N'
#define SYN 'Y'
SOCKET Receive, Send;
#define LENGTH  8185//数据长度
#define HEADER 7//头部长度
#define LENGTHALL  8192//数据包长度
const int MAX = 200000000;//内存中最大缓存数据包大小
SOCKADDR_IN serverAddress, clientAddress;
char sendclose[2];//关闭发送端
char recvclose[2];
char sendconnection[2];//与接收端连接
char recvconnection[2];
char Message[MAX];
int length = 0;
int total;
int cwnd = 0;
int maxWin = 7;//最大窗口长度
int acknum = 0;
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


void ConnectToServer() {//与接收端连接
	int addr = sizeof(serverAddress);
	while (1)
	{
		sendconnection[0] = SEQ1;//SEQ
		sendconnection[1] = ACK1;//ACK
		sendto(Send, sendconnection, 2, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
		cout << "发送端进行第一次握手指令。" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
		int begin_time = clock();
		bool flag = 1;//标志是否发送成功
		while (recvfrom(Send, recvconnection, 2, 0, (sockaddr*)&serverAddress, &addr) == SOCKET_ERROR);
		sendconnection[0] = SEQ1 + 1;
		sendconnection[1] = ACK1 + 1;
		if (flag == 1 && recvconnection[0] == SEQ1 && recvconnection[1] == ACK1 + 1)
		{
			cout << "发送端接收到第二次握手指令。" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
			sendto(Send, sendconnection, 2, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
			cout << "发送端发送第三次握手指令。" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
			break;
		}
		else {
			cout << "发送端接收到错误的握手指令，请重新进行连接!" << endl;
			break;
		}
	}
}
void CloseClient() {//断开连接
	sendclose[0] = WAVE; sendclose[1] = ACK;
	int addr = sizeof(clientAddress);
	while (1)
	{
		bool flag = 1;
		sendto(Send, sendclose, 2, 0, (sockaddr*)&serverAddress, addr);
		cout << "发送端发送第一次挥手." << " SEQ=" << sendclose[0] << " ACK=" << sendclose[1] << endl;
		int begin_time = clock();
		while (recvfrom(Send, recvclose, 2, 0, (sockaddr*)&serverAddress, &addr) == SOCKET_ERROR)
		{
			int end_time = clock();
			if (end_time - begin_time > 2500)
			{
				flag = 0;
				cout << "超时！" << endl;
				break;
			}
		}
		if (flag == 1 && recvclose[0] == ACK && recvclose[1] == WAVE + 1)
		{
			cout << "断开连接。" << " SEQ=" << recvclose[0] << " ACK=" << recvclose[1] << endl;
			break;
		}
		else
			cout << "未收到回复，重新进行挥手操作!" << endl;
	}
}




void sendPackage(char* msgs, int lens, int index, int last, int status)//封装数据包进行传输
{
	char* sendMessage = new char[lens + HEADER];

	if (last == 1)
		sendMessage[1] = LAST;//标记最后一个数据包

	else sendMessage[1] = NOTLAST;
	sendMessage[2] = index / 128; sendMessage[3] = index % 128;
	sendMessage[4] = lens / 128; sendMessage[5] = lens % 128;
	sendMessage[6] = SYN;

	for (int i = 0; i < lens; i++)
		sendMessage[i + HEADER] = msgs[i];

	int v = sendMessage[0];
	if (v < 0)
		v = v + 255;
	int ack1 = sendMessage[2] * 128 + sendMessage[3];

	sendMessage[0] = CheckPackage(sendMessage + 1, lens + HEADER - 1);
	bag Bag1 = Package(sendMessage, lens);//封装
	string str0;

	if (status == 0)
		str0 = "此时为慢启动状态。";
	else
		str0 = "此时为拥塞避免状态。";

	sendto(Send, sendMessage, lens + HEADER, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
	cout << "正在发送数据包，编号:" << index << "  校验和为:" << v << "  ACK为：" << ack1 << "  SYN为：";
	cout << Bag1.syn << "   fin为:" << Bag1.fin << "   数据包大小为;" << int(Bag1.size[0]) * 128 + int(Bag1.size[1]) << "   " << str0 << endl;
}

char* Sendname(string name) {//发送文件名称
	char* SendName = new char[name.length() + 1];
	for (int i = 0; i < name.length(); i++)
		SendName[i] = name[i];
	SendName[name.length()] = '#';//将文件保存到发送缓冲区

	ifstream fin(name.c_str(), ifstream::binary);
	if (!fin)
		cout << "文件不存在! " << endl;

	unsigned char ch = fin.get();
	while (fin)
	{
		Message[length] = ch;
		length++;
		ch = fin.get();
	}
	fin.close();
	return SendName;
}

void SendMessage() {
	int begin = clock(), end;
	bool status = 0;// 0 表示慢启动状态， 1 表示拥塞状态
	int sureall = 0;
	int before = -1, current;
	while (true)
	{
		//判断当前所处的状态
		if (cwnd == 0) //判断当前所处的状态并根据窗口大小进行调整
			cwnd = 1;
		else if (cwnd < maxWin && status == 0)
		{
			if (cwnd * 2 > maxWin) {//转换为拥塞状态
				status = 1;
				cwnd += 1;
			}
			else
				cwnd *= 2;//保持慢启动状态
		}
		else if (status == 1)
			cwnd++;
		printf("当前窗口大小为: %d\n", cwnd);
		int i;
		for (i = 0; i < cwnd && i + sureall < total; i++)//发送消息
		{
			int pkgnumber = i + sureall;
			if (pkgnumber == total - 1)
				sendPackage(Message + pkgnumber * LENGTH, length - (total - 1) * LENGTH, pkgnumber, true, status);
			else
				sendPackage(Message + pkgnumber * LENGTH, LENGTH, pkgnumber, false, status);
		}
		int begin_time = clock(), end_time;
		while (1)//接收端反馈消息
		{
			int clientlen = sizeof(clientAddress);
			bool flag = 1;
			char feedback[3];
			while (recvfrom(Send, feedback, 3, 0, (sockaddr*)&serverAddress, &clientlen) == SOCKET_ERROR)
			{
				end_time = clock();
				if (end_time - begin_time > 200)
				{
					flag = 0;
					break;
				}
			}
			if (flag && feedback[0] == 1)
			{
				current = feedback[1] * 128 + feedback[2];
				if (before + 1 == current) {
					acknum += 1; before += 1;
				}
				else {
					cout << "发生了丢包，此时收到了乱序ACK，";
					cout << "期待确认报文号：" << before << "  实际确认报文号：" << current << endl;
				}
			}
			else if (flag == 0) {
				break;
			}
		}
		if (acknum == cwnd)
		{
			acknum = 0;
			sureall += cwnd;
		}
		else
		{
			sureall = before + 1;//累积确认之前的报文，这样避免重传整个窗口，让协议性能更高效。
			acknum = 0; cwnd = 0; status = 0;
			if (i + sureall >= total)
				break;
		}
	}
	char sendEnd[3];//发送消息，表示消息传输已结束
	sendEnd[0] = 'f';
	sendEnd[1] = 'i';
	sendEnd[2] = 'n';

	end = clock();
	sendto(Send, sendEnd, 3, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
	int totaltime = end - begin;
	cout << "发送时间：" << double(totaltime / 1000) << "s" << endl;;
	cout << "吞吐率：" << length * 8 / totaltime << "bps" << endl;;
}

int main()
{
	cout << "请输入要发送的文件名：";//输入发送的文件名
	string name;
	cin >> name;
	
	//启动发送端
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup 错误: " << WSAGetLastError() << endl;
		return 0;
	}
	Send = socket(AF_INET, SOCK_DGRAM, 0);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(1233);

	ConnectToServer();//连接到接收端
	cout << "成功连接到接收端！" << endl;
	char* SendName=Sendname(name);
	int temps = 1;
	setsockopt(Send, SOL_SOCKET, SO_RCVTIMEO, (char*)&temps, sizeof(temps));
	sendto(Send, SendName, name.length() + 1, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
	delete SendName;
	total = length / LENGTH + (length % LENGTH != 0);
	SendMessage();
	CloseClient();
	closesocket(Send);
	closesocket(Receive);
	WSACleanup();
	return 0;
}