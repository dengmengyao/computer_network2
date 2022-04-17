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
#define LENGTH 8185//���ݳ���
#define HEADER 7//ͷ������
#define LENGTHALL 8192//���ݰ�����
const int MAX = 200000000;//�ڴ�����󻺴����ݰ���С
SOCKADDR_IN serverAddress, clientAddress;
char sendclose[2];//�رշ��Ͷ�
char recvclose[2];
char sendconnection[2];//����ն�����
char recvconnection[2];
string name;
int MessageLength[MAX];//ÿ�����ݰ��ĳ���
char message[MAX];
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

void Recvname() {//�����ļ�����
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
			cout << "δ�յ��ļ�����" << endl;
			continue;
		}
	}
	if (flag)
	{
		for (int i = 0; names[i] != '#'; i++)
			name += names[i];
	}
}

void WaitConnection() {//�ȴ�����
	while (1)
	{
		int len = sizeof(clientAddress);
		while (recvfrom(Receive, recvconnection, 2, 0, (sockaddr*)&clientAddress, &len) == SOCKET_ERROR);
		if (recvconnection[0] != SEQ1)
			continue;
		else {
			cout << "���ն˽��յ���һ������ָ�" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
			bool flag = 1;//�ж��Ƿ����ӳɹ�
			sendconnection[0] = SEQ1; sendconnection[1] = ACK1 + 1;
			while (1)
			{
				memset(recvconnection, 0, 2);
				sendto(Receive, sendconnection, 2, 0, (sockaddr*)&clientAddress, sizeof(clientAddress));
				cout << "���ն˷��͵ڶ�������ָ�" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
				while (recvfrom(Receive, recvconnection, 2, 0, (sockaddr*)&clientAddress, &len) == SOCKET_ERROR);
				if (recvconnection[0] == SEQ1)
					continue;
				else if (recvconnection[0] != SEQ1 + 1 || recvconnection[1] != ACK1 + 1)
				{
					cout << "����ʧ��\n���������Ͷ�" << endl;
					flag = 0;
				}
				else {
					cout << "���ն˽��յ�����������ָ�" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
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
void WaitDisconnection() {//�Ͽ�����
	int len = sizeof(clientAddress);
	while (1)
	{
		sendclose[0] = ACKW1; sendclose[1] = WAVE1 + 1;
		while (recvfrom(Receive, recvclose, 2, 0, (sockaddr*)&clientAddress, &len) == SOCKET_ERROR);
		if (recvclose[0] != WAVE1) {
			cout << "���ն�δ�յ���һ�λ��֣������ԣ�" << endl;
			continue;
		}
		else {
			cout << "���ն��յ���һ�λ���." << " SEQ=" << recvclose[0] << " ACK=" << recvclose[1] << endl;
			sendto(Receive, sendclose, 2, 0, (sockaddr*)&clientAddress, sizeof(clientAddress));
			break;
		}
	}
	cout << "���ն˶Ͽ����ӡ�" << endl;
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
			//������Ϣ
			int length = receiveMessage[4] * 128 + receiveMessage[5];
			bag bag1 = Package(receiveMessage, length);

			number = bag1.ack[0] * 128 + bag1.ack[1];
			lens = bag1.size[0] * 128 + bag1.size[1];
			char receiveSuccess[3];
			memset(receiveSuccess, '\0', 3);
			unsigned int cks = CheckPackage(receiveMessage, lens + HEADER);// ���û�г�ʱ����ACK����ȷ���������ȷ
			if (flag && cks == 0)
			{
				receiveSuccess[0] = 1;
				receiveSuccess[1] = bag1.ack[0]; receiveSuccess[2] = bag1.ack[1];
				int t = bag1.check;
				if (t < 0)
					t = t + 255;
				sendto(Receive, receiveSuccess, 3, 0, (sockaddr*)&clientAddress, sizeof(clientAddress));

				cout << "�ɹ����յ���Ϣ�� У���" << t << " ACK=" << (int(bag1.ack[0]) * 128 + int(bag1.ack[1]));
				cout << "   SYNΪ��" << bag1.syn << "   finΪ:" << bag1.fin;
				cout << "  У���Ϊ��" << t << "   ������ǰ�ƶ�" << endl;
				break;
			}
			else
			{
				if (receiveMessage[0] == 'f' && receiveMessage[1] == 'i' && receiveMessage[2] == 'n')
				{//����״̬
					cout << "�����ļ�������ϡ�" << endl;
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
		// �յ��İ����ܻ�������ģ��������Ҫȷ������λ�ã���ʱҪ�������ڵ�ACK�����ݽ��д洢
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
	//�������ն�
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup ����" << WSAGetLastError() << endl;
		return 0;
	}
	Receive = socket(AF_INET, SOCK_DGRAM, 0);

	if (Receive == SOCKET_ERROR)
	{
		cout << "�׽��ִ���" << WSAGetLastError() << endl;
		return 0;
	}

	//serverAddress.sin_addr.s_addr = htons(INADDR_ANY);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(1234);

	if (bind(Receive, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		cout << "�˿ڴ���" << WSAGetLastError() << endl;
		return 0;
	}
	WaitConnection();
	cout << "�ɹ����ӵ����Ͷˣ�" << endl;
	Recvname();
	RecvMessage();
	WaitDisconnection();//�ȴ����Ͷ˶Ͽ�����
	cout << "���Ͷ˶Ͽ����ӡ�" << endl;
	closesocket(Receive);
	closesocket(Send);
	WSACleanup();
	return 0;
}