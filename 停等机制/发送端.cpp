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
// checksum����
unsigned char PkgCheck(char* arr, int len)//����У��͵ĺ�����һ����8λУ����
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
	return ~ret;//����
}
// ����
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
	buffer[2] = index / 128;//���ݱ��ı��
	buffer[3] = index % 128;
	buffer[4] = len / 128;//���ݱ��Ĵ�С
	buffer[5] = len % 128;

	ack1 = int(buffer[2]) * 128 + buffer[3];

	for (int i = 0; i < len; i++)
	{
		buffer[i + CheckSum] = msg[i];
	}
	while (1)//�����ļ�������Ϣ��ÿ�η��Ͳ���
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
		{//��ʱ�ش�������ʱδ�յ�ACK��������ش�
			int over_time = clock();
			if (over_time - begin_time > 500)
			{
				break;
			}
		}
		//ͣ�Ȼ��ƣ��յ�ACK���ٷ�����һ��
		if ((int(recv[0])*128+int(recv[1]))==ack1)
		{
			if (index != recv[0] * 128 + recv[1])
				continue;
			cout <<"��Ϣ���͵�   "<<"У����="<< u << " ACK=" << ack1 << "   ";
			
			break;
		}
		else//ȷ���ش�����
		{
			cout << "�����������´���  " <<endl;
			ack1 = int(buffer[2]) * 128 + int(buffer[3]);
		}
	}
	delete buffer;
}
// ������Ϣ
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
			cout << "�ļ�������!" << endl;
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
			printf("�Ѿ�����:%.2f%%\n", (float)i / package_num * 100);
		}
	}
	clock_t timeend = clock();
	double time= (double)(timeend - timestart) / CLOCKS_PER_SEC;
	cout << "��ʱΪ��" << time << "s"<<endl;
	cout << "������Ϊ��" << len *8/ time /1024/1024<< "Mbps" << endl;
	cout << len << endl;
}
// ���ӵ�������
void ConnectToServer()
{
	while (1)
	{
		char send[2];
		send[0] = SEQ1;//��ʼ��SEQ
		send[1] = ACK1;//��ʼ��ACK
		sendto(client, send, 2, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
		cout << "���Ͷ˽��е�һ������ָ�" << " SEQ=" << send[0] << " ACK=" << send[1] << endl;
		int begin_time = clock();
		bool flag = 1;//��־�Ƿ��ͳɹ�
		char recv[2];
		int clientlen = sizeof(clientAddr);
		while (recvfrom(client, recv, 2, 0, (sockaddr*)&serverAddr, &clientlen) == SOCKET_ERROR)
		{
			int over_time = clock();
			if (over_time - begin_time > 500)//�ж��Ƿ�ʱ
			{
				flag = 0;//�������ʧ��
				cout << "����ʧ��\n�����·���" << endl;
				break;
			}
		}
		if (flag && recv[0] == SEQ1 && recv[1] == ACK1+1)
		{
			cout << "���Ͷ˽��յ��ڶ�������ָ�" << " SEQ=" << recv[0] << " ACK=" << recv[1] << endl;
			send[0] = SEQ1+1;//SEQ��ACK��һ��ʾ���ֳɹ�
			send[1] = ACK1+1;
			sendto(client, send, 2, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
			cout << "���Ͷ˷��͵���������ָ�" << " SEQ=" << send[0] << " ACK=" << send[1] << endl;
			break;
		}
	}
}
// �����ļ�����
void SendName(string name, int size)
{
	char* Name = new char[size + 1];
	for (int i = 0; i < size; i++)
	{
		Name[i] = name[i];
	}
	Name[size] = '#';//��ʶ�������һλ��������ն�ʶ��
	sendto(client, Name, size + 1, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
	delete Name;
}
// �رտͻ��ˣ����л��ֲ���
void CloseClient()
{
	while (1)
	{
		char send[2];
		send[0] = WAVE1;
		send[1] = ACKW1;
		sendto(client, send, 2, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
		cout << "���Ͷ˷��͵�һ�λ���." << " SEQ=" << send[0] << " ACK=" << send[1] << endl;
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
				cout << "��ʱ��" << endl;
				break;
			}
		}
		if (flag && recv[0] == ACKW1 && recv[1] == WAVE1 + 1)
		{
			cout << "�Ͽ����ӡ�" << " SEQ=" << recv[0] << " ACK=" << recv[1] << endl;
			break;
		}
	}
}
int main()
{
	//��ʼ��ͻ��˽�������
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)//
	{
		cout << "�򿪴���:" << WSAGetLastError() << endl;
		return -1;
	}
	client = socket(AF_INET, SOCK_DGRAM, 0);

	if (client == SOCKET_ERROR)
	{
		cout << "Socket ����:" << WSAGetLastError() << endl;
		return -1;
	}
	int Port = 1234;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(Port);
	cout << "��ʼ����ն�����!" << endl;


	ConnectToServer();
	cout << "�ɹ�����!" << endl;
	int time_out = 1;
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(time_out));


	string name;
	cin >> name;
	SendName(name, name.length());
	Sendmessage(name, name.length());
	cout << "�ɹ�������Ϣ!" << endl;
	
	CloseClient();
	WSACleanup();
	return 0;
}
