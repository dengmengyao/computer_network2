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
#define WIN 7  //ȷ��������������
char message[200000000];
char sendclose[2];//�رշ��Ͷ�
char recvclose[2];
char sendconnection[2];//����ն�����
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
struct bag {//����������ݽṹ
	char syn;
	char ack[2];
	char fin;
	char check;
	char size[2];
	char package[16377];
};

bag Package(char* Message, int len) {//�����ݰ����з�װ
	bag Bag;
	Bag.fin = Message[1];//FIN����ʾ�Ƿ�Ϊ���һ����
	Bag.ack[0] = Message[2];Bag.ack[1] = Message[3];//ack
	Bag.size[0] = Message[4];Bag.size[1] = Message[5];//���Ĵ�С
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
		unsigned int comop= check % (1 << 8);
		check = dived + comop;
		retu = check;
	}
	return ~retu;//����
}

// ���ӵ�������
void ConnectToServer()
{
	int addr = sizeof(ClientAddress);
	while (1)
	{
		sendconnection[0] = SEQ1;//SEQ
		sendconnection[1] = ACK1;//ACK
		sendto(Send, sendconnection, 2, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
		cout << "���Ͷ˽��е�һ������ָ�" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
		int begin_time = clock();
		bool flag = 1;//��־�Ƿ��ͳɹ�
		while (recvfrom(Send, recvconnection, 2, 0, (sockaddr*)&ServerAddress, &addr) == SOCKET_ERROR);
		sendconnection[0] = SEQ1 + 1;
		sendconnection[1] = ACK1 + 1;
		if (flag==1 && recvconnection[0] == SEQ1 && recvconnection[1] == ACK1 + 1)
		{
			cout << "���Ͷ˽��յ��ڶ�������ָ�" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
			sendto(Send, sendconnection, 2, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
			cout << "���Ͷ˷��͵���������ָ�" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
			break;
		}
		else {
			cout << "���Ͷ˽��յ����������ָ������½�������!" << endl;
			break;
		}
	}
}

// �رտͻ��ˣ����л��ֲ���
void CloseClient(SOCKET client)
{
	sendclose[0] = WAVE;sendclose[1] = ACK;
	int addr = sizeof(ClientAddress);
	while (1)
	{
        bool flag = 1;
		sendto(client, sendclose, 2, 0, (sockaddr*)&ServerAddress, addr);
		cout << "���Ͷ˷��͵�һ�λ���." << " SEQ=" << sendclose[0] << " ACK=" << sendclose[1] << endl;
		int begin_time = clock();
		while (recvfrom(client, recvclose, 2, 0, (sockaddr*)&ServerAddress, &addr) == SOCKET_ERROR)
		{
			int end_time = clock();
			if (end_time - begin_time > 2500)
			{
				flag = 0;
				cout << "��ʱ��" << endl;
				break;
			}
		}
		if (flag==1 && recvclose[0] == ACK && recvclose[1] == WAVE + 1)
		{
			cout << "�Ͽ����ӡ�" << " SEQ=" << recvclose[0] << " ACK=" << recvclose[1] << endl;
			break;
		}
		else
			cout << "δ�յ��ظ������½��л��ֲ���!" << endl;
	}
}

// �����ļ�����
void SendFileName(string name, int size)
{
	char* Name = new char[size + 1];
	for (int i = 0; i < size; i++)
		Name[i] = name[i];
	Name[size] = '#';//��ʶ�������һλ��������ն�ʶ��
	sendto(Send, Name, size + 1, 0, (sockaddr*)&ServerAddress, sizeof(ServerAddress));
	delete Name;
}
// �������ݰ�
void Sendpackage( char* msgs, int length, int index, int last)
{
	char* Messages = new char[length + CheckSum];
	if (last)
		Messages[1] = LAST;

	else
		Messages[1] = NOTLAST;
	Messages[2] = index / 128;//���ݱ��ı��
	Messages[3] = index % 128;
	Messages[4] = length / 128;//���ݱ��Ĵ�С
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
	cout << "���ڷ������ݰ������:" << index << "  У���Ϊ:" << v << "  ACKΪ��" << ack1 << "  SYNΪ��";
	cout<< Bag1.syn << "   finΪ:" << Bag1.fin << "   ���ݰ���СΪ;" << int(Bag1.size[0])*128+int(Bag1.size[1])<<endl;
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
			//�ۼ�ȷ�ϻ���
			if (recvifsuccess[0] == 1) {//ÿ���յ���WIN���������ACKȷ�ϣ��˴�ȷ�ϰ�����ACK����Ѿ������İ�����
				Pkgack[ack1] = 1;
				Wins -= WIN;
				ack1 = int(recvifsuccess[1]) * 128 + int(recvifsuccess[2]);
				SendSuccesspackage = int(recvifsuccess[3]);
				cout << int(recvifsuccess[1]) * 128 + int(recvifsuccess[2]) - WIN+1 << "��" << "��" << int(recvifsuccess[1]) * 128 + int(recvifsuccess[2]) << "�Ű��ɹ�������!";
				cout << "���һ����ACKΪ:" << ack1 << endl;
				success = 1;
				break;
			}
			else {//������ն˷�����������ô���Ͷ˻���ݽ��յ�����Ϣ���·��ʹ���İ�
				int ack0 = int(Bag1.ack[0]) * 128 + Bag1.ack[1];
				Wins -= WIN;
				if (int(recvifsuccess[3]) != 0)
					SendSuccesspackage = int(recvifsuccess[3]);
				else
					SendSuccesspackage = 0;
				cout <<"��������"<< ack1 << "�Ű�δ���ɹ�����!" << endl;
				cout << "���·��͸ð�������а�!" << endl;
				//������İ�֮������а����·���һ�鱣֤����ɹ�
				success = 0;
				break;
			}
		}
	}
	delete Messages;
}
// ������Ϣ
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
			cout << "�ļ�������!" << endl;
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
	while (1) {//ÿ���������ʹ��ڴ�С��������Ϣ�����ݷ��ص�sendsuccesspackage��ǰ�϶�����
		SendSuccesspackage = 0;//��¼����ȷ������û�з�������������ݰ�����
		for (int i = 0; i < WIN && i + WinPlace < package_num; i++)
			Sendpackage(message + (i + WinPlace) * LENGTH, (i + WinPlace) == package_num - 1 ? len - (package_num - 1) * LENGTH : LENGTH, (i + WinPlace), (i + WinPlace) == package_num - 1);
		WinPlace += SendSuccesspackage;//���ݳɹ��������͵���Ϣ������ǰ��������
		if (WinPlace >= package_num)
			break;
	}
	clock_t timeend = clock();
	double time = (double)(timeend - timestart) / CLOCKS_PER_SEC;
	cout << "��ʱΪ��" << time << "s" << endl;
	cout << "������Ϊ��" << len * 8 / time / 1024 / 1024 << "Mbps" << endl;
	cout << len << endl;
}
int main()
{
	//��ʼ��ͻ��˽�������
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)//
	{
		cout << "��������:" << WSAGetLastError() << endl;
		return -1;
	}
	Send = socket(AF_INET, SOCK_DGRAM, 0);
	ServerAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(Por);
	cout << "��ʼ����ն�����!" << endl;
	ConnectToServer();
	cout << "�ɹ�����!" << endl;
	string name;
	cin >> name;
	SendFileName(name, name.length());
	Sendmessage( name, name.length());
	cout << "�ɹ�������Ϣ!" << endl;
	CloseClient(Send);
	WSACleanup();
	return 0;
}
