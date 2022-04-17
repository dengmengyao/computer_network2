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
#define WIN 7  //ȷ��������������
int pcknum = 0;
int Winnum = 0;
static char FirstLost[2];// ���ڱ�־�Ѿ��յ����ĸ���
string name;
char message[200000000];
char sendclose[2];//�رս��ն�
char recvclose[2];
char sendconnection[2];//�뷢�Ͷ�����
char recvconnection[2];
int ack1 = 0;
int Por = 1234;
struct bag {//�������ݰ������ݽṹ
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
		unsigned int comop = check % (1 << 8);
		check = dived + comop;
		retu = check;
	}
	return ~retu;//����
}
// �ȴ�����

void WaitConnection()
{
	while (1)
	{
		int len = sizeof(ClientAddress);
		while (recvfrom(Receive, recvconnection, 2, 0, (sockaddr*)&ClientAddress, &len) == SOCKET_ERROR);
		if (recvconnection[0] != SEQ1)
			continue;
		else {
			cout << "���ն˽��յ���һ������ָ�" << " SEQ=" << recvconnection[0] << " ACK=" << recvconnection[1] << endl;
			bool flag = 1;//�ж��Ƿ����ӳɹ�
			sendconnection[0] = SEQ1;sendconnection[1] = ACK1 + 1;
			while (1)
			{
				memset(recvconnection, 0, 2);
				sendto(Receive, sendconnection, 2, 0, (sockaddr*)&ClientAddress, sizeof(ClientAddress));
				cout << "���ն˷��͵ڶ�������ָ�" << " SEQ=" << sendconnection[0] << " ACK=" << sendconnection[1] << endl;
				while (recvfrom(Receive, recvconnection, 2, 0, (sockaddr*)&ClientAddress, &len) == SOCKET_ERROR);
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
			if (flag!=1)
				continue;
			else
			    break;
		}
	}
}

//�ȴ��Ͽ�����
void WaitDisconnection(SOCKET server)
{
	int len = sizeof(ClientAddress);
	while (1)
	{
		sendclose[0] = ACKW1;sendclose[1] = WAVE1 + 1;
		while (recvfrom(server, recvclose, 2, 0, (sockaddr*)&ClientAddress, &len) == SOCKET_ERROR);
		if (recvclose[0] != WAVE1) {
			cout << "���ն�δ�յ���һ�λ��֣������ԣ�" << endl;
			continue;
		}
		else {
			cout << "���ն��յ���һ�λ���." << " SEQ=" << recvclose[0] << " ACK=" << recvclose[1] << endl;
			sendto(server, sendclose, 2, 0, (sockaddr*)&ClientAddress, sizeof(ClientAddress));
			break;
		}
	}
	cout << "���ն˶Ͽ����ӡ�" << endl;
}
// ������Ϣ
void Recvmessage()//û�ɹ�����һ�δ��䣬ACK�����һ����ACKδ�仯����������ʧ�ܣ���Ҫ�����ش�
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
		char* recvmessage = new char[LENGTH + CheckSum];//���ն˵Ļ�����
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
				//û�г�ʱ��ACK��ȷ
				cout << "ACK=" << ack1 << "  У���Ϊ��" << t << endl;
			    ack1 = int(Bag1.ack[0]) * 128 + int(Bag1.ack[1]) + 1;
			    Winnum++;
				sendsuccess++;
				//���ն˽����ۼ�ȷ�ϻ��ƣ��յ��˶���������ȷ�ϣ�ȷ���Ƿ�
			    if (Winnum == WIN || Bag1.fin == LAST) {
				    cout << "�ɹ����յ����ϴ��ڴ�С��������Ϣ�� ���һ����Ϣ��У���" << t << " ACK=" << (int(Bag1.ack[0]) * 128 + int(Bag1.ack[1]));
					cout << "   SYNΪ��"<<Bag1.syn << "   finΪ:" << Bag1.fin << "   �˴δ��������ݴ�СΪ;" << sendsuccess*(int(Bag1.size[0]) * 128 + int(Bag1.size[1]));
					cout << "  У���Ϊ��" << t <<"   ������ǰ�ƶ�" << endl;
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
			//��������˶�������ᷢ���ۼ�ȷ�ϵ���Ϣ
			//���ͽ��ն˶�ʧ�ĵ�һ���������
			else {
				cout << "��������!" << endl;
				Winnum++;
				if (FirstLost[0] == -1) {//��¼�µ�һ����ʧ�İ�
					FirstLost[0] = Bag.ack[0];FirstLost[1] = Bag.ack[1];
					sends = sendsuccess;//��¼����ȷ����δ����������İ�����
					sendsuccess = 0;
				}
				if (Winnum == WIN || recvmessage[1] == LAST) {
					sendSuccessRecv[0] = 0;
					sendSuccessRecv[1] = FirstLost[0];sendSuccessRecv[2] = FirstLost[1];
					sendSuccessRecv[3] = sends;
					sendto(Receive, sendSuccessRecv, 4, 0, (sockaddr*)&ClientAddress, sizeof(ClientAddress));
					cout << "��������!" << " ����ʼ�İ�Ϊ��" << int(Bag1.ack[0]) * 128 + int(Bag1.ack[1]) + 1 ;
					cout << "   �˴γɹ�����������ݰ�����Ϊ:" << sends << endl;
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
		//ȷ���Ƿ�Ϊ���һ����
		if (recvmessage[1] != NOTLAST) 
			break;
	}
	if (pcklen == pcknum)
		cout << "û�а���ʧ" << endl;
	ofstream out(name.c_str(), ofstream::binary);
	for (int i = 0; i < place; i++)
		out << message[i];
	out.close();
}
// �����ļ�����
void RecvFileName()
{
	char Name[100];
	int clientlen = sizeof(ClientAddress);
	while (recvfrom(Receive, Name, 100, 0, (sockaddr*)&ClientAddress, &clientlen) == SOCKET_ERROR);
	for (int i = 0; Name[i] != '#'; i++)//���û�е����һλ����һֱ�洢����
		name += Name[i];
}
int main()
{
	//��ʼ�����뷢�Ͷ�����
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "��������:" << WSAGetLastError() << endl;
		return -1;
	}
	Receive = socket(AF_INET, SOCK_DGRAM, 0);
	ServerAddress.sin_addr.s_addr = htons(INADDR_ANY);
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_port = htons(Por);
	if (bind(Receive, (SOCKADDR*)&ServerAddress, sizeof(ServerAddress)) == SOCKET_ERROR)
	{
		cout << "Bind����:" << WSAGetLastError() << endl;
		return -1;
	}
	cout << "��ʼ�뷢�Ͷ�����!" << endl;
	WaitConnection();
	cout << "�ɹ��뷢�Ͷ�����!" << endl;
	RecvFileName();
	Recvmessage();
	cout << "�ɹ�������Ϣ!" << endl;
	WaitDisconnection(Receive);
	closesocket(Receive);
	WSACleanup();
	return 0;
}