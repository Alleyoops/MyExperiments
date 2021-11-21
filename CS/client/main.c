#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#define MAX_BUF_SIZE 65535//接受和发送缓存区大小
int main(int argc,char *argv[])
{
    //初始化winsock2环境
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);

    //建立TCP套接字
    SOCKET TCPSocket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    //创建TCP Socket地址结构
    SOCKADDR_IN TCPServer;
    TCPServer.sin_family = AF_INET;
    TCPServer.sin_addr.S_un.S_addr = inet_addr(argv[1]);//输入的服务器IP地址
    TCPServer.sin_port = htons((u_short)atoi(argv[2]));//输入的服务器TCP端口号

    connect(TCPSocket,(SOCKADDR *)&TCPServer,sizeof(TCPServer));//与服务器主机请求建立tcp连接

    //tip：UDP端口号在连接TCP成功后再从服务器获取
    //与服务器建立TCP连接获取UDP端口，启动命令字“START“
    int ByteReceived = 0;//接受字节数
    const char Clientbuf[MAX_BUF_SIZE];//接受和发送缓存区指针
    char portnum[5];//接收的UDP端口号字符串，还需要用atoi转化成数字
    u_short ServerUDPPort;//定义UDP端口号
    ByteReceived = recv(TCPSocket,Clientbuf,sizeof(Clientbuf),0);
    memcpy(portnum,Clientbuf,5);//复制UDP端口号字符串到（端口号字符串固定占用5个字节）
    ServerUDPPort = (u_short)atoi(portnum);//UDP端口号

    //建立UDP套接字
    SOCKET UDPSocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    //创建UDP Socket地址结构
    SOCKADDR_IN UDPServer;
    UDPServer.sin_family = AF_INET;
    UDPServer.sin_addr.S_un.S_addr = inet_addr(argv[1]);//输入的服务器IP地址
    UDPServer.sin_port = htons(ServerUDPPort);//前面的recv得到的服务器UDP端口号
    if(strcmp("START",Clientbuf+5)!=0)
    {
        int e = WSAGetLastError();
        return -1;//验证服务端发来的“START”相同，为0说明服务端启动成功
    }

    while(TRUE)
    {
        //提示用户输入，选择相应服务（TCP还是UDP）
        int UserChoice = 0;
        printf("1：TCP\t（得到Server的系统时间）\n2：UDP\t（Server回显功能）\n");
        scanf("%d",&UserChoice);
        switch(UserChoice)
        {
        case 1:
            strcpy(Clientbuf,"GET CUR TIME");
            send(TCPSocket,Clientbuf,strlen(Clientbuf),0);//有多少发多少
            recv(TCPSocket,Clientbuf,sizeof(Clientbuf),0);//最大缓存接收s
            break;
        case 2:
            printf("请输入消息内容：\n");
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
            printf("输入错误\n");
            break;
        }

        printf("recv：%s\n",Clientbuf);
    }
    return 0;
}
