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
static char pindex[2];		// ���ڱ�־�Ѿ��յ����ĸ���
// checksum����
unsigned char PkgCheck(char* arr, int len)//����У��͵ĺ���
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
// �ȴ�����
void WaitConnection()
{
	while (1)
	{
		char recv[2];
		int clientlen = sizeof(clientAddr);
		while (recvfrom(server, recv, 2, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
		if (recv[0] != SEQ1)
			continue;
		cout << "���ն˽��յ���һ������ָ�" << " SEQ="<<recv[0]<<" ACK="<<recv[1]<<endl;
		bool flag = 1;//�ж��Ƿ����ӳɹ�
		while (1)
		{
			memset(recv, 0, 2);
			char send[2];
			send[0] = SEQ1;
			send[1] = ACK1+1;
			sendto(server, send, 2, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
			cout << "���ն˷��͵ڶ�������ָ�" << " SEQ=" << send[0] << " ACK=" << send[1]<<endl;
			while (recvfrom(server, recv, 2, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
			cout << "���ն˽��յ�����������ָ�" << " SEQ=" << recv[0] << " ACK=" << recv[1] << endl;
			if (recv[0] == SEQ1)
				continue;
			if (recv[0] != SEQ1+1 || recv[1] != ACK1+1)
			{
				cout << "����ʧ��\n���������Ͷ�" << endl;
				flag = 0;
			}
			break;
		}
		if (!flag)
			continue;
		break;
	}
}

// �ȴ��Ͽ�����
void WaitDisconnection()
{
	while (1)
	{
		char recv[2];
		int clientlen = sizeof(clientAddr);
		while (recvfrom(server, recv, 2, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
		if (recv[0] != WAVE1)
			continue;
		cout << "���ն��յ���һ�λ���." << " SEQ=" << recv[0] << " ACK=" << recv[1] << endl;
		char send[2];
		send[0] = ACKW1;
		send[1] = WAVE1+1;
		sendto(server, send, 2, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
		break;
	}
	cout << "���ն˶Ͽ����ӡ�" << endl;
}
// ������Ϣ
void Recvmessage()//û�ɹ�����һ�δ��䣬ACK�����һ����ACKδ�仯����������ʧ�ܣ���Ҫ�����ش�
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
			cout << "�ɹ����յ���Ϣ�� У����=" << t << " ACK=" << (int(recv[2]) * 128 + int(recv[3])) <<endl;
			// ���ACK
			
			if ((int(recv[2]) * 128 + int(recv[3])) == (ack1))
			{   //������յ���Ϣ������Ϣ���н���
				ack1 = int(recv[2]) * 128 + int(recv[3])+1;
				// ��һ�����ǲ���˳�����һ����
				//ͣ�Ȼ���
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
			else//�������󣬵ȴ����Ͷ����½��д���
			{
				cout << "��������!" << " ACKӦ��Ϊ="<< int(recv[2]) * 128 + int(recv[3])+1 <<endl;
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
// �����ļ�����
void RecvName()
{
	char Name[100];
	int clientlen = sizeof(clientAddr);
	while (recvfrom(server, Name, 100, 0, (sockaddr*)&clientAddr, &clientlen) == SOCKET_ERROR);
	for (int i = 0; Name[i] != '#'; i++)//���û�е����һλ����һֱ�洢����
	{
		name += Name[i];
	}
}
int main()
{
	//��ʼ�����뷢�Ͷ�����
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "�򿪴���:" << WSAGetLastError() << endl;
		return -1;
	}
	server = socket(AF_INET, SOCK_DGRAM, 0);

	if (server == SOCKET_ERROR)
	{
		cout << "Socket ����:" << WSAGetLastError() << endl;
		return -1;
	}
	int Port = 1234;
	serverAddr.sin_addr.s_addr = htons(INADDR_ANY);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(Port);

	if (bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cout << "Bind����:" << WSAGetLastError() << endl;
		return -1;
	}
	cout << "��ʼ�뷢�Ͷ�����!" << endl;


	WaitConnection();
	cout << "�ɹ��뷢�Ͷ�����!" << endl;

	RecvName();
	Recvmessage();
	cout << "�ɹ�������Ϣ!" << endl;
	
	WaitDisconnection();
	closesocket(server);
	WSACleanup();
	return 0;
}