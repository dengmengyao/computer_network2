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
#define LENGTH  8185//���ݳ���
#define HEADER 7//ͷ������
#define LENGTHALL  8192//���ݰ�����
const int MAX = 200000000;//�ڴ�����󻺴����ݰ���С
SOCKADDR_IN serverAddress, clientAddress;
char sendclose[2];//�رշ��Ͷ�
char recvclose[2];
char sendconnection[2];//����ն�����
char recvconnection[2];
char Message[MAX];
int length = 0;
int total;
int cwnd = 0;
int maxWin = 7;//��󴰿ڳ���
int acknum = 0;
struct bag {//����������ݽṹ
	char syn;
	char ack[2];
	char fin;
	char check;
	char size[2];
	char package[10000];
};

bag Package(char* Message, int len) {//�����ݰ����з�װ
	bag Bag;
	Bag.fin = Message[1];//FIN����ʾ�Ƿ�Ϊ���һ����
	Bag.ack[0] = Message[2]; Bag.ack[1] = Message[3];//ack
	Bag.size[0] = Message[4]; Bag.size[1] = Message[5];//���Ĵ�С
	Bag.syn = Message[6];//SYN
	Bag.check = Message[0];//У���
	for (int i = 0; i < len; i++)
		Bag.package[i] = Message[i + 7];//������Ϣ
	return Bag;
}
unsigned char CheckPackage(char* arry, int length)//����У��͵ĺ�����һ����8λУ����
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
	return ~retu;//����
}


void ConnectToServer() {//����ն�����
	int addr = sizeof(serverAddress);
	while (1)
	{
		sendconnection[0] = SEQ1;//SEQ
		sendconnection[1] = ACK1;//ACK
		sendto(Send, sendconnection, 2, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
		cout << "���Ͷ˽��е�һ������ָ�" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
		int begin_time = clock();
		bool flag = 1;//��־�Ƿ��ͳɹ�
		while (recvfrom(Send, recvconnection, 2, 0, (sockaddr*)&serverAddress, &addr) == SOCKET_ERROR);
		sendconnection[0] = SEQ1 + 1;
		sendconnection[1] = ACK1 + 1;
		if (flag == 1 && recvconnection[0] == SEQ1 && recvconnection[1] == ACK1 + 1)
		{
			cout << "���Ͷ˽��յ��ڶ�������ָ�" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
			sendto(Send, sendconnection, 2, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
			cout << "���Ͷ˷��͵���������ָ�" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
			break;
		}
		else {
			cout << "���Ͷ˽��յ����������ָ������½�������!" << endl;
			break;
		}
	}
}
void CloseClient() {//�Ͽ�����
	sendclose[0] = WAVE; sendclose[1] = ACK;
	int addr = sizeof(clientAddress);
	while (1)
	{
		bool flag = 1;
		sendto(Send, sendclose, 2, 0, (sockaddr*)&serverAddress, addr);
		cout << "���Ͷ˷��͵�һ�λ���." << " SEQ=" << sendclose[0] << " ACK=" << sendclose[1] << endl;
		int begin_time = clock();
		while (recvfrom(Send, recvclose, 2, 0, (sockaddr*)&serverAddress, &addr) == SOCKET_ERROR)
		{
			int end_time = clock();
			if (end_time - begin_time > 2500)
			{
				flag = 0;
				cout << "��ʱ��" << endl;
				break;
			}
		}
		if (flag == 1 && recvclose[0] == ACK && recvclose[1] == WAVE + 1)
		{
			cout << "�Ͽ����ӡ�" << " SEQ=" << recvclose[0] << " ACK=" << recvclose[1] << endl;
			break;
		}
		else
			cout << "δ�յ��ظ������½��л��ֲ���!" << endl;
	}
}




void sendPackage(char* msgs, int lens, int index, int last, int status)//��װ���ݰ����д���
{
	char* sendMessage = new char[lens + HEADER];

	if (last == 1)
		sendMessage[1] = LAST;//������һ�����ݰ�

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
	bag Bag1 = Package(sendMessage, lens);//��װ
	string str0;

	if (status == 0)
		str0 = "��ʱΪ������״̬��";
	else
		str0 = "��ʱΪӵ������״̬��";

	sendto(Send, sendMessage, lens + HEADER, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
	cout << "���ڷ������ݰ������:" << index << "  У���Ϊ:" << v << "  ACKΪ��" << ack1 << "  SYNΪ��";
	cout << Bag1.syn << "   finΪ:" << Bag1.fin << "   ���ݰ���СΪ;" << int(Bag1.size[0]) * 128 + int(Bag1.size[1]) << "   " << str0 << endl;
}

char* Sendname(string name) {//�����ļ�����
	char* SendName = new char[name.length() + 1];
	for (int i = 0; i < name.length(); i++)
		SendName[i] = name[i];
	SendName[name.length()] = '#';//���ļ����浽���ͻ�����

	ifstream fin(name.c_str(), ifstream::binary);
	if (!fin)
		cout << "�ļ�������! " << endl;

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
	bool status = 0;// 0 ��ʾ������״̬�� 1 ��ʾӵ��״̬
	int sureall = 0;
	int before = -1, current;
	while (true)
	{
		//�жϵ�ǰ������״̬
		if (cwnd == 0) //�жϵ�ǰ������״̬�����ݴ��ڴ�С���е���
			cwnd = 1;
		else if (cwnd < maxWin && status == 0)
		{
			if (cwnd * 2 > maxWin) {//ת��Ϊӵ��״̬
				status = 1;
				cwnd += 1;
			}
			else
				cwnd *= 2;//����������״̬
		}
		else if (status == 1)
			cwnd++;
		printf("��ǰ���ڴ�СΪ: %d\n", cwnd);
		int i;
		for (i = 0; i < cwnd && i + sureall < total; i++)//������Ϣ
		{
			int pkgnumber = i + sureall;
			if (pkgnumber == total - 1)
				sendPackage(Message + pkgnumber * LENGTH, length - (total - 1) * LENGTH, pkgnumber, true, status);
			else
				sendPackage(Message + pkgnumber * LENGTH, LENGTH, pkgnumber, false, status);
		}
		int begin_time = clock(), end_time;
		while (1)//���ն˷�����Ϣ
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
					cout << "�����˶�������ʱ�յ�������ACK��";
					cout << "�ڴ�ȷ�ϱ��ĺţ�" << before << "  ʵ��ȷ�ϱ��ĺţ�" << current << endl;
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
			sureall = before + 1;//�ۻ�ȷ��֮ǰ�ı��ģ����������ش��������ڣ���Э�����ܸ���Ч��
			acknum = 0; cwnd = 0; status = 0;
			if (i + sureall >= total)
				break;
		}
	}
	char sendEnd[3];//������Ϣ����ʾ��Ϣ�����ѽ���
	sendEnd[0] = 'f';
	sendEnd[1] = 'i';
	sendEnd[2] = 'n';

	end = clock();
	sendto(Send, sendEnd, 3, 0, (sockaddr*)&serverAddress, sizeof(serverAddress));
	int totaltime = end - begin;
	cout << "����ʱ�䣺" << double(totaltime / 1000) << "s" << endl;;
	cout << "�����ʣ�" << length * 8 / totaltime << "bps" << endl;;
}

int main()
{
	cout << "������Ҫ���͵��ļ�����";//���뷢�͵��ļ���
	string name;
	cin >> name;
	
	//�������Ͷ�
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup ����: " << WSAGetLastError() << endl;
		return 0;
	}
	Send = socket(AF_INET, SOCK_DGRAM, 0);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(1233);

	ConnectToServer();//���ӵ����ն�
	cout << "�ɹ����ӵ����նˣ�" << endl;
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