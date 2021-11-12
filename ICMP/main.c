#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h>
const BYTE ICMP_ECHO_REQUEST = 8;
const BYTE ICMP_ECHO_REPLY = 0;
const BYTE ICMP_TIMEOUT = 11;
const int DEF_ICMP_DATA_SIZE = 32;
const int MAX_ICMP_PACKET_SIZE = 1024;
const DWORD DEF_ICMP_TIMEOUT = 3000;
const int DEF_MAX_HOP = 30;
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
    USHORT usSeqNo;
    DWORD dwRoundTripTime;
    in_addr dwIPaddr;
}DECODE_RESULT;
USHORT checksum(USHORT *pBuf,int iSize)
{
        unsigned long cksum = 0;
        while(iSize>1)
        {
            cksum+=*pBuf++;
            iSize-=sizeof(USHORT);
        }
        if(iSize)
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
        int iIpHdrLen = (pIpHdr->hdr_len)*4;
        ICMP_HEADER *pIcmpHdr = (ICMP_HEADER *)(pBuf+iIpHdrLen);
        USHORT usID,usSquNo;
        if(pIcmpHdr->type==ICMP_ECHO_REPLY)
        {
            usID = pIcmpHdr->id;
            usSquNo = pIcmpHdr->seq;
        }
        else if(pIcmpHdr->type==ICMP_TIMEOUT)
        {
            char *pInnerIpHdr = pBuf+iIpHdrLen+sizeof(ICMP_HEADER);
            int iInnerIPHdrLen = (((IP_HEADER*)pInnerIpHdr)->hdr_len)*4;
            ICMP_HEADER *pinnerIcmpHdr = (ICMP_HEADER *)(pInnerIpHdr+iInnerIPHdrLen);
            usID = pinnerIcmpHdr->id;
            usSquNo = pinnerIcmpHdr->seq;
        }
        else return FALSE;
        if (usID!=(USHORT)GetCurrentProcessId()||usSquNo!=DecodeResult->usSeqNo) return FALSE;
        DecodeResult->dwIPaddr.s_addr=pIpHdr->sourceIP;
        DecodeResult->dwRoundTripTime=GetTickCount()-DecodeResult->dwRoundTripTime;
        if(DecodeResult->dwRoundTripTime) printf("%d ms\t",(int)DecodeResult->dwRoundTripTime);
        else printf("<1 ms\t");
        return TRUE;
}
int main()
{
    //1、初始化套接字
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
    char str[1024];
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
    ZeroMemory(&destSockAddr,sizeof(sockaddr_in));
    destSockAddr.sin_family=AF_INET;
    destSockAddr.sin_addr.s_addr=ulDestIP;
    //3、创建原始套接字
    SOCKET sockRaw = WSASocket(AF_INET,SOCK_RAW,IPPROTO_ICMP,NULL,0,WSA_FLAG_OVERLAPPED);
    int iTimeout = 3000;//设置Socket超时机制
    setsockopt(sockRaw,SOL_SOCKET,SO_RCVTIMEO,(char *)&iTimeout,sizeof(iTimeout));
    setsockopt(sockRaw,SOL_SOCKET,SO_SNDTIMEO,(char *)&iTimeout,sizeof(iTimeout));
    //4、定义IP和ICMP头部数据结构
    //5、构造ICMP回显请求消息，并以TTL（初始值为1）递增发送报文
    char IcmpSendBuf[sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE];
    ICMP_HEADER *pIcmpHeader = (ICMP_HEADER *)IcmpSendBuf;
    pIcmpHeader->type = ICMP_ECHO_REQUEST;
    pIcmpHeader->code = 0;
    pIcmpHeader->id = (USHORT)GetCurrentProcessId();
    memset(IcmpSendBuf+sizeof(ICMP_HEADER),'E',DEF_ICMP_DATA_SIZE);
    USHORT usSeqNo = 0;
    int iTTL = 1;
    BOOL bReachDestHost = FALSE;
    int iMaxHop = DEF_MAX_HOP;
    DECODE_RESULT DecodeResult;
    while(!bReachDestHost&&iMaxHop--)
    {
        setsockopt(sockRaw,IPPROTO_IP,4,(char *)&iTTL,sizeof(iTTL));
        printf("%d\t",iTTL);
        ((ICMP_HEADER *)IcmpSendBuf)->cksum = 0;
        ((ICMP_HEADER *)IcmpSendBuf)->seq = htons(usSeqNo++);
        ((ICMP_HEADER *)IcmpSendBuf)->cksum = checksum((USHORT *)IcmpSendBuf,sizeof(ICMP_HEADER)+DEF_ICMP_DATA_SIZE);
        DecodeResult.usSeqNo = ((ICMP_HEADER *)IcmpSendBuf)->seq;
        DecodeResult.dwRoundTripTime = GetTickCount();
        sendto(sockRaw,IcmpSendBuf,sizeof(IcmpSendBuf),0,(sockaddr *)&destSockAddr,sizeof(destSockAddr));
        //6、接收ICMP差错报文并解析处理
        sockaddr_in from;
        int iFromLen = sizeof(from);
        int iReadDataLen;
        char IcmpRecvBuf[MAX_ICMP_PACKET_SIZE];
        while(1)
        {
            iReadDataLen = recvfrom(sockRaw,IcmpRecvBuf,MAX_ICMP_PACKET_SIZE,0,(sockaddr *)&from,&iFromLen);
            //int i = WSAGetLastError();
            //printf("%d",i);
            if(iReadDataLen!=SOCKET_ERROR)
            {
                if(DecodeIcmpResponse(IcmpRecvBuf,iReadDataLen,&DecodeResult))
                {
                    if(DecodeResult.dwIPaddr.s_addr==destSockAddr.sin_addr.s_addr) bReachDestHost = TRUE;
                    printf("%s\n",inet_ntoa(DecodeResult.dwIPaddr));
                    break;
                }
            }
            else if(WSAGetLastError()==WSAETIMEDOUT)
            {
                printf(" * \t Request timed out\n");
                break;
            }else
            {
                printf("recvfrom（）调用出错\n");
            }
        }
        iTTL++;
    }
}












