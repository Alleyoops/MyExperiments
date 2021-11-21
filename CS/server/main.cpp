//#include <iostream>
//#include <windows.h>
//#include <winsock2.h>
//#include <time.h>
//#include <winnt.h>


#include <stdio.h>
#include <WinSock2.h>
#include <ctime>




#define MAX_CLIENT 10
#define MAX_BUF_SIZE 65535
#define UDP_SRV_PORT 8848

typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct in_addr in_addr;
typedef struct sockaddr sockaddr;

typedef struct
{
    SOCKET socket;
    sockaddr_in addr;
} TcpThreadParam;


long TcpClientCount=0;

DWORD WINAPI TcpServeThread(LPVOID  lpParam)
{
    SOCKET TcpSocket =((TcpThreadParam*)lpParam)->socket;
    SOCKADDR_IN TcpClientAddr=((TcpThreadParam*)lpParam)->addr;
    char ServerTCPBuf[1024];
    sprintf(ServerTCPBuf,"%5d%s",UDP_SRV_PORT,"START");
    send(TcpSocket,ServerTCPBuf,strlen(ServerTCPBuf),0);
    int TCPBytesReceived;
    time_t CurSysTime;
    while(TRUE)
    {
        memset(ServerTCPBuf,'\0',sizeof(ServerTCPBuf));
        TCPBytesReceived=recv(TcpSocket,ServerTCPBuf,sizeof(ServerTCPBuf),0);
        if(TCPBytesReceived==0||TCPBytesReceived==SOCKET_ERROR)   break;
        if(strcmp(ServerTCPBuf,"GET CUR TIME")!=0)
        {
            printf("TCP: %s\n",ServerTCPBuf);
            continue;
        }
        time(&CurSysTime);
        memset(ServerTCPBuf,'\0',sizeof(ServerTCPBuf));
        strftime(ServerTCPBuf,sizeof(ServerTCPBuf),"%Y-%m-%d %H:%M:%S",localtime(&CurSysTime));
        printf("TCP: get cur timeing\n");
        send(TcpSocket,ServerTCPBuf,strlen(ServerTCPBuf),0);

    }

    closesocket(TcpSocket);
    InterlockedDecrement(&TcpClientCount);
}


DWORD WINAPI UdpServer(LPVOID  lpParam)
{
    SOCKET UDPSrvSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    char hostname[256];
    gethostname(hostname,sizeof(hostname));
    hostent * pHostent =gethostbyname(hostname);
    // 枚举本地ip
    //for(int i =0; i<3; i++)
    //{
     //   in_addr ip = *(in_addr*)pHostent->h_addr_list[i];
     //   printf("%d.%d.%d.%d\n",(ip.s_addr<<24)>>24,(ip.s_addr<<16)>>24,(ip.s_addr<<8)>>24,ip.s_addr>>24);

    //}

    SOCKADDR_IN UDPSrvAddr;
    memset(&UDPSrvAddr,0,sizeof(SOCKADDR_IN));
    UDPSrvAddr.sin_family=AF_INET;
    UDPSrvAddr.sin_port=htons(UDP_SRV_PORT);
    UDPSrvAddr.sin_addr=*(in_addr*)pHostent->h_addr_list[2];
    bind(UDPSrvSocket,(sockaddr*)&UDPSrvAddr,sizeof(UDPSrvAddr));
    char ServerUDPBuf[1024];
    while(TRUE)
    {
        memset(ServerUDPBuf,'\0',sizeof(ServerUDPBuf));
        int iSockAddrLen=sizeof(sockaddr);
        char UDPClientAddr[sizeof(hostent)];
        recvfrom(UDPSrvSocket,ServerUDPBuf,sizeof(ServerUDPBuf),0,
                 (sockaddr*)&UDPClientAddr,&iSockAddrLen);
        printf("UDP: %s \n",ServerUDPBuf);
        iSockAddrLen=sizeof(sockaddr);
        sendto(UDPSrvSocket,ServerUDPBuf,strlen(ServerUDPBuf),0,(sockaddr*)&UDPClientAddr,iSockAddrLen);
    }
}


int main()
{
    //1、初始化套接字
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);

    SOCKET ListenSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    char hostname[256];
    gethostname(hostname,sizeof(hostname));
    hostent * pHostent =gethostbyname(hostname);
    // 枚举本地ip
    for(int i =0; i<3; i++)
    {
        in_addr ip = *(in_addr*)pHostent->h_addr_list[i];
        printf("%d.%d.%d.%d\n",(ip.s_addr<<24)>>24,(ip.s_addr<<16)>>24,(ip.s_addr<<8)>>24,ip.s_addr>>24);

    }

    SOCKADDR_IN ListenAddr;
    ListenAddr.sin_family=AF_INET;
    ListenAddr.sin_port=htons((u_short)atoi("8847"));
    ListenAddr.sin_addr=*(in_addr*)pHostent->h_addr_list[2];
    bind(ListenSocket,(sockaddr*)&ListenAddr,sizeof(ListenAddr));

    printf("listening...\n");
    listen(ListenSocket,SOMAXCONN);
    SOCKET TcpSocket;
    SOCKADDR_IN TcpClientAddr;
    DWORD dwUDPThreadId;
    CreateThread(NULL,0,UdpServer,NULL,0,&dwUDPThreadId);
    while(TRUE)
    {
        int iSockAddrLen=sizeof(sockaddr);
        TcpSocket = accept(ListenSocket,(sockaddr*)&TcpClientAddr,&iSockAddrLen);
        if(TcpClientCount>=MAX_CLIENT)
        {
            closesocket(TcpSocket);
            continue;
        }
        TcpThreadParam Param;
        Param.socket=TcpSocket;
        Param.addr=TcpClientAddr;




        DWORD dwThreadId;
        CreateThread(NULL,0,TcpServeThread,&Param,0,&dwThreadId);

        InterlockedIncrement(&TcpClientCount);
    }
    closesocket(ListenSocket);
    WSACleanup();
}












