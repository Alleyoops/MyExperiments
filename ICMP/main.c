#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
const BYTE ICMP_ECHO_REQUEST = 8;//请求回显
const BYTE ICMP_ECHO_REPLY = 0;//回显应答
const BYTE ICMP_TIMEOUT = 11;//传输超时
const int DEF_ICMP_DATA_SIZE = 32;//ICMP报文默认数据字段长度
const int MAX_ICMP_PACKET_SIZE = 1024;//ICMP报文最大长度（包括报头）
const DWORD DEF_ICMP_TIMEOUT = 3000;//回显应答超时时间
const int DEF_MAX_HOP = 30;//最大跳站数
typedef struct sockaddr_in sockaddr_in;
typedef struct in_addr in_addr;
typedef struct sockaddr sockaddr;
typedef struct hostent hostent;
typedef struct
{
    unsigned char hdr_len:4;//4位头部长度
    unsigned char version:4;//4位版本号
    unsigned char tos;
    unsigned short total_len;
    unsigned short identifier;
    unsigned short frag_and_flags;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short checksum;
    unsigned long sourceIP;
    unsigned long destIP;
}IP_HEADER;
typedef struct
{
    BYTE type;
    BYTE code;
    USHORT cksum;
    USHORT id;
    USHORT seq;
}ICMP_HEADER;
typedef struct
{
    USHORT usSeqNo;//序列号
    DWORD dwRoundTripTime;//往返时间
    in_addr dwIPaddr;//返回报文得到IP地址
}DECODE_RESULT;
USHORT checksum(USHORT *pBuf,int iSize)
{
        unsigned long cksum = 0;
        while(iSize>1)
        {
            cksum+=*pBuf++;
            iSize-=sizeof(USHORT);//相当于iSize-=2;
        }
        if(iSize)//iSize可能为奇数（iSize==1）
        {
            cksum+=*(UCHAR *)pBuf;
        }
        cksum=(cksum>>16)+(cksum&0xffff);
        cksum+=(cksum>>16);
        return (USHORT)(~cksum);
}
BOOL DecodeIcmpResponse(char *pBuf,int iPacketSize,DECODE_RESULT* DecodeResult)
{
        IP_HEADER* pIpHdr = (IP_HEADER *)pBuf;
        int iIpHdrLen = (pIpHdr->hdr_len)*4;//计算IP头部长度
        //根据ICMP报文类型提取ID字段和序列号字段
        ICMP_HEADER *pIcmpHdr = (ICMP_HEADER *)(pBuf+iIpHdrLen);
        USHORT usID,usSquNo;
        if(pIcmpHdr->type==ICMP_ECHO_REPLY)
        {
            //对于回显应答报文，检查ICMP报头的ID字段和序列号
            usID = pIcmpHdr->id;
            usSquNo = pIcmpHdr->seq;
        }
        else if(pIcmpHdr->type==ICMP_TIMEOUT)
        {
            char *pInnerIpHdr = pBuf+iIpHdrLen+sizeof(ICMP_HEADER);//载荷中的IP头
            int iInnerIPHdrLen = (((IP_HEADER*)pInnerIpHdr)->hdr_len)*4;
            ICMP_HEADER *pinnerIcmpHdr = (ICMP_HEADER *)(pInnerIpHdr+iInnerIPHdrLen);//载荷中的ICMP头
            //对于超时差错报文，，应该检查数据部分所含ICMP报头的ID字段和序列号
            usID = pinnerIcmpHdr->id;
            usSquNo = pinnerIcmpHdr->seq;
        }
        else return FALSE;
        //检查ID和序列号以确定收到期待数据包
        if (usID!=(USHORT)GetCurrentProcessId()||usSquNo!=DecodeResult->usSeqNo) return FALSE;
        //记录IP地址并计算往返时间
        DecodeResult->dwIPaddr.s_addr=pIpHdr->sourceIP;
        DecodeResult->dwRoundTripTime=GetTickCount()-DecodeResult->dwRoundTripTime;
        //打印往返时间信息
        if(DecodeResult->dwRoundTripTime) printf("%d ms\t",(int)DecodeResult->dwRoundTripTime);
        else printf("<1 ms\t");
        return TRUE;
}
int main()
{
    //1、初始化套接字
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
    char str[1024];//输入IP地址或域名
    scanf("%s",str);
    printf("tracert %s\n",str);
    //2、解析命令行参数，构造目的端Socket地址
    u_long ulDestIP=inet_addr(str);//将命令行参数转化为ip地址
    if(ulDestIP==INADDR_NONE)
    {
        hostent *pHostent=gethostbyname(str);//转换不成功时按域名解析
        if(pHostent)
        {
            ulDestIP=(*(in_addr *)pHostent->h_addr).s_addr;
        }
        else
        {
            WSACleanup();
            return -1;
        }
    }
    //填充目的端Socket地址
    sockaddr_in destSockAddr;
    ZeroMemory(&destSockAddr,sizeof(sockaddr_in));//用0填充一块内存区域
    destSockAddr.sin_family=AF_INET;
    destSockAddr.sin_addr.s_addr=ulDestIP;
    //3、创建原始套接字
    SOCKET sockRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_ICMP,NULL,0,WSA_FLAG_OVERLAPPED);
    int iTimeout = DEF_ICMP_TIMEOUT;//设置Socket超时机制
    setsockopt(sockRaw,SOL_SOCKET,SO_RCVTIMEO,(char *)&iTimeout,sizeof(iTimeout));
    setsockopt(sockRaw,SOL_SOCKET,SO_SNDTIMEO,(char *)&iTimeout,sizeof(iTimeout));
    //4、定义IP和ICMP头部数据结构
    //5、构造ICMP回显请求消息，并以TTL（初始值为1）递增发送报文
    char IcmpSendBuf[sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE];//初始化大小为头部大小加默认数据字段长度（即40个字节）
    ICMP_HEADER *pIcmpHeader = (ICMP_HEADER *)IcmpSendBuf;
    pIcmpHeader->type = ICMP_ECHO_REQUEST;//类型为回显请求
    pIcmpHeader->code = 0;//代码字段为0
    pIcmpHeader->id = (USHORT)GetCurrentProcessId();//当前进程号作为ID
    memset(IcmpSendBuf+sizeof(ICMP_HEADER),'E',DEF_ICMP_DATA_SIZE);//数据字段设置内存为‘E’
    USHORT usSeqNo = 0;//报文序列号
    int iTTL = 1;//TTL初始值为1
    BOOL bReachDestHost = FALSE;//循环退出标志
    int iMaxHop = DEF_MAX_HOP;//循环最大次数
    DECODE_RESULT DecodeResult;//报文解析函数的参数
    while(!bReachDestHost&&iMaxHop--)//未达目标主机且未达最大跳数
    {
        setsockopt(sockRaw,IPPROTO_IP,4,(char *)&iTTL,sizeof(iTTL));//设置IP报头的TTL字段
        printf("%d\t",iTTL);
        //填充ICMP报文每次发送时需要变化的字段
        ((ICMP_HEADER *)IcmpSendBuf)->cksum = 0;//校验和先设为0
        ((ICMP_HEADER *)IcmpSendBuf)->seq = htons(usSeqNo++);//填充序列号
        ((ICMP_HEADER *)IcmpSendBuf)->cksum = checksum((USHORT *)IcmpSendBuf,sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE);//计算校验和
        //记录序列号和当前时间
        DecodeResult.usSeqNo = ((ICMP_HEADER *)IcmpSendBuf)->seq;
        DecodeResult.dwRoundTripTime = GetTickCount();
        //发送ICMP回显请求报文
        sendto(sockRaw,IcmpSendBuf,sizeof(IcmpSendBuf),0,(sockaddr *)&destSockAddr,sizeof(destSockAddr));
        //6、接收ICMP差错报文并解析处理
        sockaddr_in from;//对端Socket地址
        int iFromLen = sizeof(from);//地址结构大小
        int iReadDataLen;//接收数据长度
        char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
        while(1)//循环接收直到收到所需数据或超时
        {
            iReadDataLen = recvfrom(sockRaw,IcmpRecvBuf,MAX_ICMP_PACKET_SIZE,0,(sockaddr *)&from,&iFromLen);
            //int i = WSAGetLastError();
            //printf("%d",i);
            if(iReadDataLen!=SOCKET_ERROR)//有数据到达
            {
                //对数据包进行解码
                if(DecodeIcmpResponse(IcmpRecvBuf,iReadDataLen,&DecodeResult))
                {
                    //到达目的主机，退出循环
                    if(DecodeResult.dwIPaddr.s_addr==destSockAddr.sin_addr.s_addr) bReachDestHost = TRUE;
                    printf("%s\n",inet_ntoa(DecodeResult.dwIPaddr));
                    break;
                }
            }
            else if(WSAGetLastError()==WSAETIMEDOUT)//接收超时
            {
                printf(" * \t Request timed out\n");
                break;
            }else
            {
                //既不是接收超时，那么就是函数调用出错
                printf("recvfrom（）调用出错\n");
            }
        }
        iTTL++;//递增TTL值
    }
}


