#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define IO_RCVALL _WSAIOW(IOC_VENDOR,1)
#define BUFFER_SIZE 65535
typedef struct sockaddr_in sockaddr_in;
typedef struct in_addr in_addr;
typedef struct sockaddr sockaddr;
typedef struct
{
    unsigned char Version_HeaderLength; //版本(4位)+首部长度(4位)
    unsigned char TypeOfService;//服务类型
    unsigned short TotalLength;//总长度
    unsigned short Identification;//标识
    unsigned short Flags_Fragmentoffset;//标志(3位)+分片偏移(13位)
    unsigned char TimeToLive;//生存时间
    unsigned char Protocal ;//协议
    unsigned short HeaderChecksum;//首部校验和
    unsigned long SourceAddress;//源IP地址
    unsigned long DestAddress;//目的IP地址
} IPHEADER;
typedef struct
{
    USHORT usSourcePort;//16位源端口
    USHORT usDestPort;//16位目的端口
    ULONG dwSeq;//序列号
    ULONG dwAck;//确认号
    UCHAR ucLength;//4位首部长度+4位保留字一共8位
    UCHAR ucFlag;//6位标志位
    USHORT usWindow;//16位窗口大小
    USHORT usCheckSum;//16位校验和
    USHORT usUrgent;//16位紧急数据偏移量
    UINT unMssOpt;
    USHORT usNopOpt;
    USHORT usSackOpt;
} TCPHEADER;
void Print(IPHEADER *piphdr,TCPHEADER *tcp);
int main()
{
    //初始化Socket
    WSADATA wsData;
    WSAStartup(MAKEWORD(2,2),&wsData);
    //创建原始套接字 raw socket
    SOCKET sock;
    sock = WSASocket(AF_INET,SOCK_RAW,IPPROTO_IP,NULL,0,WSA_FLAG_OVERLAPPED);
    //绑定套接字
    char localName[256];//本地机器名
    gethostname(localName,256);//获取主机名
    HOSTENT *pHost;//指向主机信息的指针
    pHost=gethostbyname(localName);//获取本地IP地址列表，需要枚举出来
    for(int i=0; i<pHost->h_length-1; i++)
    {
        //打印IP地址
        printf("IP addr %d: %s\n", i, inet_ntoa( *(in_addr*)pHost->h_addr_list[i] ) );
    }
    int choose = 0;
    scanf("%d",&choose);
    sockaddr_in addr_in;
    addr_in.sin_family=AF_INET;
    addr_in.sin_port=htons(8000);//转换为网络字节序
    addr_in.sin_addr=*(in_addr *)pHost->h_addr_list[choose];//选择主机IP地址
    bind(sock,(sockaddr *)&addr_in,sizeof(addr_in));//绑定原始套接字到本机地址
    //设置网卡为混杂模式
    DWORD dwBufferLen[10];
    DWORD dwBufferInLen=1;
    DWORD dwBytesReturned=0;
    WSAIoctl(sock,IO_RCVALL,&dwBufferInLen,sizeof(dwBufferInLen),dwBufferLen,sizeof(dwBufferLen),&dwBytesReturned,NULL,NULL);
    //接受IP数据包
    char buffer[BUFFER_SIZE];//接受缓存区
    while(1)
    {
        int nPacketSize=recv(sock,buffer,BUFFER_SIZE,0);
        //找寻Socket出错代码
        //int err = WSAGetLastError();
        //printf("%d\n", err);
        //printf("%d ", nPacketSize);
        if (nPacketSize>0)
        {
            IPHEADER *pIpHdr;
            pIpHdr=(IPHEADER *)buffer;//指针强制转换为IPHEADER数据结构
            TCPHEADER *tcp=(TCPHEADER *)(buffer+((pIpHdr->Version_HeaderLength)&0x0f)*4);
            Print(pIpHdr,tcp);
        }
    }
    printf("over\n");
    return 0;
}
void Print(IPHEADER *piphdr,TCPHEADER *tcp)
{
    switch((piphdr->Version_HeaderLength)>>4)
    {
    case 4:
        printf("版本：IPV%d\n",(piphdr->Version_HeaderLength)>>4);
        break;
    case 6:
        printf("版本：IPV%d\n",(piphdr->Version_HeaderLength)>>4);
        break;
    default:
        printf("版本：IPV%d\n",(piphdr->Version_HeaderLength)>>4);
        break;
    }
    printf("报头长度：%d个字节\n",((piphdr->Version_HeaderLength)&0x0f)*4);
    printf("总长度：%d个字节\n",piphdr->TotalLength);
    switch((piphdr->Protocal))
    {
    case 1:
        printf("协议类型：ICMP\n");
        break;
    case 2:
        printf("协议类型：IGMP\n");
        break;
    case 6:
        printf("协议类型：TCP\n");
        printf("源端口：%d\n",ntohs(tcp->usSourcePort));
        //printf("目的端口：%d\n",*((USHORT *)tcp+1));
        printf("目的端口：%d\n",ntohs(tcp->usDestPort));
        if(ntohs(tcp->usDestPort)==80)
        {
        char *temptcp=(char *)tcp;
        char *pchar=temptcp+(tcp->ucLength>>4)*4;
        printf("TCP数据部分：%s\n",pchar);//打印Http请求消息
        }
        break;
    case 8:
        printf("协议类型：EGP\n");
        break;
    case 17:
        printf("协议类型：UDP\n");
        break;
    case 41:
        printf("协议类型：IPv6\n");
        break;
    case 89:
        printf("协议类型：OSPF\n");
        break;
    }
    //移位法输出IP地址
    ULONG temp=piphdr->SourceAddress;
    printf("源IP地址：%d.%d.%d.%d\n",temp&0x000000ff,(temp>>8)&0x0000ff,(temp>>16)&0x00ff,(temp>>24)&0xff);
    ULONG temp2=piphdr->DestAddress;
    printf("目的IP地址：%d.%d.%d.%d\n",temp2&0x000000ff,(temp2>>8)&0x0000ff,(temp2>>16)&0x00ff,(temp2>>24)&0xff);
    printf("\n");
}

