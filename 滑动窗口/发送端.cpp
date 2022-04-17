#pragma comment(lib, "Ws2_32.lib")
#include<iostream>
#include<WinSock2.h>
#include<string>
#include<string.h>
#include<time.h>
#include<fstream>
#include<stdio.h>
using namespace std;
SOCKET Send;
SOCKADDR_IN ServerAddress, ClientAddress;
char SEQ1 = '0';
char ACK1 = '0';
#define WAVE '0'
#define ACK '1'
#define ACKS WAVE + 1
#define LENGTH 16377
#define CheckSum 7
#define LAST 'Y'
#define NOTLAST 'N'
#define SYN 'Y'
#define WIN 7  //确定滑动窗口数量
char message[200000000];
char sendclose[2];//关闭发送端
char recvclose[2];
char sendconnection[2];//与接收端连接
char recvconnection[2];
bool QUIT = true;
int Wins = 0;
bool Pkgack[100000];
int len = 0;
bool success;
int SendSuccesspackage=0;
int num = len / LENGTH + (len % LENGTH != 0);
int ack1 = 0;
int Por = 1234;
struct bag {//定义包的数据结构
	char syn;
	char ack[2];
	char fin;
	char check;
	char size[2];
	char package[16377];
};

bag Package(char* Message, int len) {//对数据包进行封装
	bag Bag;
	Bag.fin = Message[1];//FIN，表示是否为最后一个包
	Bag.ack[0] = Message[2];Bag.ack[1] = Message[3];//ack
	Bag.size[0] = Message[4];Bag.size[1] = Message[5];//包的大小
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
		unsigned int comop= check % (1 << 8);
		check = dived + comop;
		retu = check;
	}
	return ~retu;//反码
}

// 连接到服务器
void ConnectToServer()
{
	int addr = sizeof(ClientAddress);
	while (1)
	{
		sendconnection[0] = SEQ1;//SEQ
		sendconnection[1] = ACK1;//ACK
		sendto(Send, sendconnection, 2, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
		cout << "发送端进行第一次握手指令。" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
		int begin_time = clock();
		bool flag = 1;//标志是否发送成功
		while (recvfrom(Send, recvconnection, 2, 0, (sockaddr*)&ServerAddress, &addr) == SOCKET_ERROR);
		sendconnection[0] = SEQ1 + 1;
		sendconnection[1] = ACK1 + 1;
		if (flag==1 && recvconnection[0] == SEQ1 && recvconnection[1] == ACK1 + 1)
		{
			cout << "发送端接收到第二次握手指令。" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
			sendto(Send, sendconnection, 2, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
			cout << "发送端发送第三次握手指令。" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
			break;
		}
		else {
			cout << "发送端接收到错误的握手指令，请重新进行连接!" << endl;
			break;
		}
	}
}

// 关闭客户端，进行挥手操作
void CloseClient(SOCKET client)
{
	sendclose[0] = WAVE;sendclose[1] = ACK;
	int addr = sizeof(ClientAddress);
	while (1)
	{
        bool flag = 1;
		sendto(client, sendclose, 2, 0, (sockaddr*)&ServerAddress, addr);
		cout << "发送端发送第一次挥手." << " SEQ=" << sendclose[0] << " ACK=" << sendclose[1] << endl;
		int begin_time = clock();
		while (recvfrom(client, recvclose, 2, 0, (sockaddr*)&ServerAddress, &addr) == SOCKET_ERROR)
		{
			int end_time = clock();
			if (end_time - begin_time > 2500)
			{
				flag = 0;
				cout << "超时！" << endl;
				break;
			}
		}
		if (flag==1 && recvclose[0] == ACK && recvclose[1] == WAVE + 1)
		{
			cout << "断开连接。" << " SEQ=" << recvclose[0] << " ACK=" << recvclose[1] << endl;
			break;
		}
		else
			cout << "未收到回复，重新进行挥手操作!" << endl;
	}
}

// 发送文件名称
void SendFileName(string name, int size)
{
	char* Name = new char[size + 1];
	for (int i = 0; i < size; i++)
		Name[i] = name[i];
	Name[size] = '#';//标识名称最后一位，方便接收端识别
	sendto(Send, Name, size + 1, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
	delete Name;
}
// 发送数据包
void Sendpackage( char* msgs, int length, int index, int last)
{
	char* Messages = new char[length + CheckSum];
	if (last)
		Messages[1] = LAST;

	else
		Messages[1] = NOTLAST;
	Messages[2] = index / 128;//数据报的标号
	Messages[3] = index % 128;
	Messages[4] = length / 128;//数据报的大小
	Messages[5] = length % 128;
	Messages[6] = SYN;
	ack1 = int(Messages[2]) * 128 + Messages[3];
	for (int i = 0; i < length; i++)
		Messages[i + CheckSum] = msgs[i];
	Messages[0] = CheckPackage(Messages + 1, length + CheckSum - 1);
	int v = Messages[0];
	if (v < 0)
		v = v + 255;
	bag Bag1=Package(Messages, length);
	sendto(Send, Messages, LENGTH + CheckSum, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
	//cout << v << "  " << ack1 <<" "<<Wins <<endl;
	cout << "正在发送数据包，编号:" << index << "  校验和为:" << v << "  ACK为：" << ack1 << "  SYN为：";
	cout<< Bag1.syn << "   fin为:" << Bag1.fin << "   数据包大小为;" << int(Bag1.size[0])*128+int(Bag1.size[1])<<endl;
	Wins++;
	if (Wins == WIN||Bag1.fin==LAST) {
		int servlen = sizeof(ServerAddress);
		char recvifsuccess[4];
		memset(recvifsuccess, '\0', 4);
		while (1)
		{
			int begin = clock();
			bool flag = 1;
			while (recvfrom(Send, recvifsuccess, 4, 0, (sockaddr*)&ServerAddress, &servlen) == SOCKET_ERROR) {
				int end = clock();
				if (end - begin > 500) {
					flag = 0;
					break;
				}
			}
			//累计确认机制
			if (recvifsuccess[0] == 1) {//每接收到了WIN个包后进行ACK确认，此次确认包含了ACK码和已经发到的包序列
				Pkgack[ack1] = 1;
				Wins -= WIN;
				ack1 = int(recvifsuccess[1]) * 128 + int(recvifsuccess[2]);
				SendSuccesspackage = int(recvifsuccess[3]);
				cout << int(recvifsuccess[1]) * 128 + int(recvifsuccess[2]) - WIN+1 << "号" << "到" << int(recvifsuccess[1]) * 128 + int(recvifsuccess[2]) << "号包成功被接收!";
				cout << "最后一个包ACK为:" << ack1 << endl;
				success = 1;
				break;
			}
			else {//如果接收端反馈丢包，那么发送端会根据接收到的信息重新发送错误的包
				int ack0 = int(Bag1.ack[0]) * 128 + Bag1.ack[1];
				Wins -= WIN;
				if (int(recvifsuccess[3]) != 0)
					SendSuccesspackage = int(recvifsuccess[3]);
				else
					SendSuccesspackage = 0;
				cout <<"发生错误，"<< ack1 << "号包未被成功接收!" << endl;
				cout << "重新发送该包后的所有包!" << endl;
				//将错误的包之后的所有包重新发送一遍保证传输成功
				success = 0;
				break;
			}
		}
	}
	delete Messages;
}
// 发送消息
void Sendmessage(string name, int size)
{
	memset(Pkgack, false, sizeof(Pkgack) / sizeof(bool));
	if (name == "quit")
		return;
	else
	{
		ifstream in(name.c_str(), ifstream::binary);
		if (!in)
		{
			cout << "文件不存在!" << endl;
			return;
		}
		unsigned char ch = in.get();
		while (in)
		{
			message[len] = ch;
			len++;
			ch = in.get();
		}
		in.close();
	}
	int package_num = len / LENGTH + (len % LENGTH != 0);
	static int WinPlace = 0;
	clock_t timestart = clock();
	char sendsize[2];
	sendsize[0] = package_num / 128;sendsize[1] = package_num % 128;
	sendto(Send, sendsize, 2, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
	while (1) {//每次连续发送窗口大小数量的消息，根据返回的sendsuccesspackage向前拖动窗口
		SendSuccesspackage = 0;//记录下正确传输且没有发生乱序传输的数据包个数
		for (int i = 0; i < WIN && i + WinPlace < package_num; i++)
			Sendpackage(message + (i + WinPlace) * LENGTH, (i + WinPlace) == package_num - 1 ? len - (package_num - 1) * LENGTH : LENGTH, (i + WinPlace), (i + WinPlace) == package_num - 1);
		WinPlace += SendSuccesspackage;//根据成功按续发送的消息数量向前滑动窗口
		if (WinPlace >= package_num)
			break;
	}
	clock_t timeend = clock();
	double time = (double)(timeend - timestart) / CLOCKS_PER_SEC;
	cout << "耗时为：" << time << "s" << endl;
	cout << "吞吐率为：" << len * 8 / time / 1024 / 1024 << "Mbps" << endl;
	cout << len << endl;
}
int main()
{
	//开始与客户端进行连接
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)//
	{
		cout << "发生错误:" << WSAGetLastError() << endl;
		return -1;
	}
	Send = socket(AF_INET, SOCK_DGRAM, 0);
	ServerAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(Por);
	cout << "开始与接收端连接!" << endl;
	ConnectToServer();
	cout << "成功连接!" << endl;
	string name;
	cin >> name;
	SendFileName(name, name.length());
	Sendmessage( name, name.length());
	cout << "成功发送消息!" << endl;
	CloseClient(Send);
	WSACleanup();
	return 0;
}
