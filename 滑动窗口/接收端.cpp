#pragma comment(lib, "Ws2_32.lib")
#include<iostream>
#include<WinSock2.h>
#include<string>
#include<string.h>
#include<fstream>
#include<ctime>
using namespace std;
SOCKET Receive;
SOCKADDR_IN ServerAddress, ClientAddress;
char SEQ1 = '0';
char ACK1 = '0';
#define WAVE1 '0'
#define ACKW1 '1'
#define ACKW2 WAVE1 + 1
#define LENGTH 16377
#define CheckSum 7
#define LAST 'Y'
#define NOTLAST 'N'
#define SYN 'Y'
#define WIN 7  //确定滑动窗口数量
int pcknum = 0;
int Winnum = 0;
static char FirstLost[2];// 用于标志已经收到了哪个包
string name;
char message[200000000];
char sendclose[2];//关闭接收端
char recvclose[2];
char sendconnection[2];//与发送端连接
char recvconnection[2];
int ack1 = 0;
int Por = 1234;
struct bag {//定义数据包的数据结构
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
		unsigned int comop = check % (1 << 8);
		check = dived + comop;
		retu = check;
	}
	return ~retu;//反码
}
// 等待连接

void WaitConnection()
{
	while (1)
	{
		int len = sizeof(ClientAddress);
		while (recvfrom(Receive, recvconnection, 2, 0, (sockaddr*)&ClientAddress, &len) == SOCKET_ERROR);
		if (recvconnection[0] != SEQ1)
			continue;
		else {
			cout << "接收端接收到第一次握手指令。" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
			bool flag = 1;//判断是否连接成功
			sendconnection[0] = SEQ1;sendconnection[1] = ACK1 + 1;
			while (1)
			{
				memset(recvconnection, 0, 2);
				sendto(Receive, sendconnection, 2, 0, (sockaddr*)&ClientAddress, sizeof(ClientAddress));
				cout << "接收端发送第二次握手指令。" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
				while (recvfrom(Receive, recvconnection, 2, 0, (sockaddr*)&ClientAddress, &len) == SOCKET_ERROR);
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
			if (flag!=1)
				continue;
			else
			    break;
		}
	}
}

//等待断开连接
void WaitDisconnection(SOCKET server)
{
	int len = sizeof(ClientAddress);
	while (1)
	{
		sendclose[0] = ACKW1;sendclose[1] = WAVE1 + 1;
		while (recvfrom(server, recvclose, 2, 0, (sockaddr*)&ClientAddress, &len) == SOCKET_ERROR);
		if (recvclose[0] != WAVE1) {
			cout << "接收端未收到第一次挥手，请重试！" << endl;
			continue;
		}
		else {
			cout << "接收端收到第一次挥手." << " SEQ=" << recvclose[0] << " ACK=" << recvclose[1] << endl;
			sendto(server, sendclose, 2, 0, (sockaddr*)&ClientAddress, sizeof(ClientAddress));
			break;
		}
	}
	cout << "接收端断开连接。" << endl;
}
// 接收消息
void Recvmessage()//没成功进行一次传输，ACK都会加一，若ACK未变化，表明传输失败，需要进行重传
{
	int clientlen = sizeof(ClientAddress);
	char length[2];
	while (recvfrom(Receive, length, 2, 0, (sockaddr*)&ClientAddress, &clientlen) == SOCKET_ERROR);
	int pcklen = length[0] * 128 + length[1];
	FirstLost[0] = -1;
	FirstLost[1] = -1;
	int place = 0;
	int lengths;
	int sendsuccess = 0;
	int sends;
	while (1)
	{
		char* recvmessage = new char[LENGTH + CheckSum];//接收端的缓冲区
		bag Bag;
		while (1)
		{
			int clientlen = sizeof(ClientAddress);
			bool flag = 1;
			while (recvfrom(Receive, recvmessage, LENGTH + CheckSum, 0, (sockaddr*)&ClientAddress, &clientlen) == SOCKET_ERROR){}
			lengths = recvmessage[4] * 128 + recvmessage[5];
			char sendSuccessRecv[4];
			bag Bag1 = Package(recvmessage, lengths);
			Bag = Bag1;
			memset(sendSuccessRecv, '\0', 4);
			int t = Bag1.check;
			if (t < 0)
				t = 255 + t;
			if (flag == 1 && CheckPackage(recvmessage, lengths + CheckSum) == 0 && (int(Bag1.ack[0]) * 128 + int(Bag1.ack[1])) == (ack1)) {
				//没有超时且ACK正确
				cout << "ACK=" << ack1 << "  校验和为：" << t << endl;
			    ack1 = int(Bag1.ack[0]) * 128 + int(Bag1.ack[1]) + 1;
			    Winnum++;
				sendsuccess++;
				//接收端进行累计确认机制，收到了多个包后进行确认，确认是否
			    if (Winnum == WIN || Bag1.fin == LAST) {
				    cout << "成功接收到符合窗口大小条数的消息！ 最后一条消息的校验和" << t << " ACK=" << (int(Bag1.ack[0]) * 128 + int(Bag1.ack[1]));
					cout << "   SYN为："<<Bag1.syn << "   fin为:" << Bag1.fin << "   此次传输总数据大小为;" << sendsuccess*(int(Bag1.size[0]) * 128 + int(Bag1.size[1]));
					cout << "  校验和为：" << t <<"   窗口向前移动" << endl;
				    Winnum = 0;
				    sendSuccessRecv[0] = 1;
					sendSuccessRecv[1] = Bag1.ack[0];sendSuccessRecv[2] = Bag1.ack[1];
					sendSuccessRecv[3] = sendsuccess;
				    sendto(Receive, sendSuccessRecv, 4, 0, (sockaddr*)&ClientAddress, sizeof(ClientAddress));
					sendsuccess = 0;
			    }
			    pcknum++;
			    break;
		     }
			//如果出现了丢包，则会发送累计确认的信息
			//发送接收端丢失的第一个包的序号
			else {
				cout << "发生丢包!" << endl;
				Winnum++;
				if (FirstLost[0] == -1) {//记录下第一个丢失的包
					FirstLost[0] = Bag.ack[0];FirstLost[1] = Bag.ack[1];
					sends = sendsuccess;//记录下正确接收未发生乱序传输的包个数
					sendsuccess = 0;
				}
				if (Winnum == WIN || recvmessage[1] == LAST) {
					sendSuccessRecv[0] = 0;
					sendSuccessRecv[1] = FirstLost[0];sendSuccessRecv[2] = FirstLost[1];
					sendSuccessRecv[3] = sends;
					sendto(Receive, sendSuccessRecv, 4, 0, (sockaddr*)&ClientAddress, sizeof(ClientAddress));
					cout << "发生错误!" << " 错误开始的包为：" << int(Bag1.ack[0]) * 128 + int(Bag1.ack[1]) + 1 ;
					cout << "   此次成功按序传输的数据包数量为:" << sends << endl;
					break;
				}
				Winnum = 0;
			}
			sendsuccess = 0;
		}
		for (int i = CheckSum; i < lengths + CheckSum; i++)
		{
			message[place] = Bag.package[i];
			place++;
		}
		//确定是否为最后一个包
		if (recvmessage[1] != NOTLAST) 
			break;
	}
	if (pcklen == pcknum)
		cout << "没有包丢失" << endl;
	ofstream out(name.c_str(), ofstream::binary);
	for (int i = 0; i < place; i++)
		out << message[i];
	out.close();
}
// 接收文件名称
void RecvFileName()
{
	char Name[100];
	int clientlen = sizeof(ClientAddress);
	while (recvfrom(Receive, Name, 100, 0, (sockaddr*)&ClientAddress, &clientlen) == SOCKET_ERROR);
	for (int i = 0; Name[i] != '#'; i++)//如果没有到最后一位，则一直存储名称
		name += Name[i];
}
int main()
{
	//开始接收与发送端连接
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "发生错误:" << WSAGetLastError() << endl;
		return -1;
	}
	Receive = socket(AF_INET, SOCK_DGRAM, 0);
	ServerAddress.sin_addr.s_addr = htons(INADDR_ANY);
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(Por);
	if (bind(Receive, (SOCKADDR*)&ServerAddress, sizeof(ServerAddress)) == SOCKET_ERROR)
	{
		cout << "Bind错误:" << WSAGetLastError() << endl;
		return -1;
	}
	cout << "开始与发送端连接!" << endl;
	WaitConnection();
	cout << "成功与发送端连接!" << endl;
	RecvFileName();
	Recvmessage();
	cout << "成功接收消息!" << endl;
	WaitDisconnection(Receive);
	closesocket(Receive);
	WSACleanup();
	return 0;
}