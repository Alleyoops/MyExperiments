#include <stdio.h>
#include <WinSock2.h>
#include <ctime>

#define MAX_CLIENT 10
#define MAX_BUF_SIZE 65535
#define UDP_SRV_PORT 8848//UDP端口号

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
    char ServerTCPBuf[MAX_BUF_SIZE];
    sprintf(ServerTCPBuf,"%5d%s",UDP_SRV_PORT,"START");
    send(TcpSocket,ServerTCPBuf,strlen(ServerTCPBuf),0);
    int TCPBytesReceived;
    time_t CurSysTime;
    while(TRUE)
    {
        memset(ServerTCPBuf,'\0',sizeof(ServerTCPBuf));
        TCPBytesReceived=recv(TcpSocket,ServerTCPBuf,sizeof(ServerTCPBuf),0);
        if(TCPBytesReceived==0||TCPBytesReceived==SOCKET_ERROR)
        {
            break;
        }
        if(strcmp(ServerTCPBuf,"GET CUR TIME")!=0)
        {
            continue;
        }
        time(&CurSysTime);
        memset(ServerTCPBuf,'\0',sizeof(ServerTCPBuf));
        strftime(ServerTCPBuf,sizeof(ServerTCPBuf),"%Y-%m-%d %H:%M:%S",localtime(&CurSysTime));
        send(TcpSocket,ServerTCPBuf,strlen(ServerTCPBuf),0);

    }

    closesocket(TcpSocket);
    //线程数减1
    InterlockedDecrement(&TcpClientCount);
}


DWORD WINAPI UdpServer(LPVOID  lpParam)
{
    SOCKET UDPSrvSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    char hostname[256];
    gethostname(hostname,sizeof(hostname));
    hostent * pHostent =gethostbyname(hostname);
    SOCKADDR_IN UDPSrvAddr;
    memset(&UDPSrvAddr,0,sizeof(SOCKADDR_IN));
    UDPSrvAddr.sin_family=AF_INET;
    UDPSrvAddr.sin_port=htons(UDP_SRV_PORT);
    UDPSrvAddr.sin_addr=*(in_addr*)pHostent->h_addr_list[2];
    bind(UDPSrvSocket,(sockaddr*)&UDPSrvAddr,sizeof(UDPSrvAddr));
    char ServerUDPBuf[MAX_BUF_SIZE];
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
    for(int i =0; i<pHostent->h_length-1; i++)
    {
        in_addr ip = *(in_addr*)pHostent->h_addr_list[i];
        printf("%d.%d.%d.%d\n",(ip.s_addr<<24)>>24,(ip.s_addr<<16)>>24,(ip.s_addr<<8)>>24,ip.s_addr>>24);

    }

    SOCKADDR_IN ListenAddr;
    ListenAddr.sin_family=AF_INET;
    ListenAddr.sin_port=htons((u_short)atoi("8847"));//tcp端口号
    ListenAddr.sin_addr=*(in_addr*)pHostent->h_addr_list[2];
    //绑定TCP监听端口
    bind(ListenSocket,(sockaddr*)&ListenAddr,sizeof(ListenAddr));

    printf("listening...\n");
    listen(ListenSocket,SOMAXCONN);

    DWORD dwUDPThreadId;
    CreateThread(NULL,0,UdpServer,NULL,0,&dwUDPThreadId);

    SOCKET TcpSocket;//accept返回得到的新的socket
    SOCKADDR_IN TcpClientAddr;//accept返回得到的新的Socket地址
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
        //线程数加1
        InterlockedIncrement(&TcpClientCount);
    }
    closesocket(ListenSocket);
    WSACleanup();
}












