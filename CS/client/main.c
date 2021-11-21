#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#define MAX_BUF_SIZE 65535//���ܺͷ��ͻ�������С
int main(int argc,char *argv[])
{
    //��ʼ��winsock2����
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);

    //����TCP�׽���
    SOCKET TCPSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    //����TCP Socket��ַ�ṹ
    SOCKADDR_IN TCPServer;
    TCPServer.sin_family = AF_INET;
    TCPServer.sin_addr.S_un.S_addr = inet_addr(argv[1]);//����ķ�����IP��ַ
    TCPServer.sin_port = htons((u_short)atoi(argv[2]));//����ķ�����TCP�˿ں�

    connect(TCPSocket,(SOCKADDR *)&TCPServer,sizeof(TCPServer));//�����������������tcp����

    //tip��UDP�˿ں�������TCP�ɹ����ٴӷ�������ȡ
    //�����������TCP���ӻ�ȡUDP�˿ڣ����������֡�START��
    int ByteReceived = 0;//�����ֽ���
    const char Clientbuf[MAX_BUF_SIZE];//���ܺͷ��ͻ�����ָ��
    char portnum[5];//���յ�UDP�˿ں��ַ���������Ҫ��atoiת��������
    u_short ServerUDPPort;//����UDP�˿ں�
    ByteReceived = recv(TCPSocket,Clientbuf,sizeof(Clientbuf),0);
    memcpy(portnum,Clientbuf,5);//����UDP�˿ں��ַ��������˿ں��ַ����̶�ռ��5���ֽڣ�
    ServerUDPPort = (u_short)atoi(portnum);//UDP�˿ں�

    //����UDP�׽���
    SOCKET UDPSocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    //����UDP Socket��ַ�ṹ
    SOCKADDR_IN UDPServer;
    UDPServer.sin_family = AF_INET;
    UDPServer.sin_addr.S_un.S_addr = inet_addr(argv[1]);//����ķ�����IP��ַ
    UDPServer.sin_port = htons(ServerUDPPort);//ǰ���recv�õ��ķ�����UDP�˿ں�
    if(strcmp("START",Clientbuf+5)!=0)
    {
        int e = WSAGetLastError();
        return -1;//��֤����˷����ġ�START����ͬ��Ϊ0˵������������ɹ�
    }

    while(TRUE)
    {
        //��ʾ�û����룬ѡ����Ӧ����TCP����UDP��
        int UserChoice = 0;
        printf("1��TCP\t���õ�Server��ϵͳʱ�䣩\n2��UDP\t��Server���Թ��ܣ�\n");
        scanf("%d",&UserChoice);
        switch(UserChoice)
        {
        case 1:
            strcpy(Clientbuf,"GET CUR TIME");
            send(TCPSocket,Clientbuf,strlen(Clientbuf),0);//�ж��ٷ�����
            recv(TCPSocket,Clientbuf,sizeof(Clientbuf),0);//��󻺴����s
            break;
        case 2:
            printf("��������Ϣ���ݣ�\n");
            getchar();
            scanf("%s",Clientbuf);
            sendto(UDPSocket,Clientbuf,strlen(Clientbuf),0,(SOCKADDR *)&UDPServer,sizeof(SOCKADDR));
            recvfrom(UDPSocket,Clientbuf,sizeof(Clientbuf),0,(SOCKADDR *)&UDPServer,sizeof(SOCKADDR));
            break;
        case 3:
            closesocket(TCPSocket);
            closesocket(UDPSocket);
            WSACleanup();
            return;
        default :
            printf("�������\n");
            break;
        }

        printf("recv��%s\n",Clientbuf);
    }
    return 0;
}
