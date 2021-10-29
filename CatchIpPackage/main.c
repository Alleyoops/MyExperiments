#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iphlpapi.h>
#define IO_RCVALL _WSAIOW(IOC_VENDOR,1)
#define BUFFER_SIZE 65535
typedef struct sockaddr_in sockaddr_in;
typedef struct in_addr in_addr;
typedef struct sockaddr sockaddr;
typedef struct
{
    unsigned char Version_HeaderLength; //�汾(4λ)+�ײ�����(4λ)
    unsigned char TypeOfService;//��������
    unsigned short TotalLength;//�ܳ���
    unsigned short Identification;//��ʶ
    unsigned short Flags_Fragmentoffset;//��־(3λ)+��Ƭƫ��(13λ)
    unsigned char TimeToLive;//����ʱ��
    unsigned char Protocal ;//Э��
    unsigned short HeaderChecksum;//�ײ�У���
    unsigned long SourceAddress;//ԴIP��ַ
    unsigned long DestAddress;//Ŀ��IP��ַ
} IPHEADER;
typedef struct
{
    USHORT usSourcePort;//16λԴ�˿�
    USHORT usDestPort;//16λĿ�Ķ˿�
    ULONG dwSeq;//���к�
    ULONG dwAck;//ȷ�Ϻ�
    UCHAR ucLength;//4λ�ײ�����+4λ������һ��8λ
    UCHAR ucFlag;//6λ��־λ
    USHORT usWindow;//16λ���ڴ�С
    USHORT usCheckSum;//16λУ���
    USHORT usUrgent;//16λ��������ƫ����
    UINT unMssOpt;
    USHORT usNopOpt;
    USHORT usSackOpt;
} TCPHEADER;
void Print(IPHEADER *piphdr,TCPHEADER *tcp);
int main()
{
    //��ʼ��Socket
    WSADATA wsData;
    WSAStartup(MAKEWORD(2,2),&wsData);
    //����ԭʼ�׽��� raw socket
    SOCKET sock;
    sock = WSASocket(AF_INET,SOCK_RAW,IPPROTO_IP,NULL,0,WSA_FLAG_OVERLAPPED);
    //���׽���
    char localName[256];//���ػ�����
    gethostname(localName,256);//��ȡ������
    HOSTENT *pHost;//ָ��������Ϣ��ָ��
    pHost=gethostbyname(localName);//��ȡ����IP��ַ�б���Ҫö�ٳ���
    for(int i=0; i<sizeof(pHost->h_addr_list)-1; i++)
    {
        printf("IP addr %d: %s\n", i, inet_ntoa( *(in_addr*)pHost->h_addr_list[i] ) );
    }
    int choose = 0;
    //scanf("%d",&choose);
    sockaddr_in addr_in;
    addr_in.sin_family=AF_INET;
    addr_in.sin_port=htons(8000);
    addr_in.sin_addr=*(in_addr *)pHost->h_addr_list[2];
    bind(sock,(sockaddr *)&addr_in,sizeof(addr_in));//��ԭʼ�׽��ֵ�������ַ
    //��������Ϊ����ģʽ
    DWORD dwBufferLen[10];
    DWORD dwBufferInLen=1;
    DWORD dwBytesReturned=0;
    WSAIoctl(sock,IO_RCVALL,&dwBufferInLen,sizeof(dwBufferInLen),dwBufferLen,sizeof(dwBufferLen),&dwBytesReturned,NULL,NULL);
    //����IP���ݰ�
    char buffer[BUFFER_SIZE];//���ܻ�����
    while(1)
    {
        int nPacketSize=recv(sock,buffer,BUFFER_SIZE,0);
        //int err = WSAGetLastError();
        //printf("%d\n", err);
        //printf("%d ", nPacketSize);
        if (nPacketSize>0)
        {
            IPHEADER *pIpHdr;
            pIpHdr=(IPHEADER *)buffer;//ָ��ǿ��ת��ΪIPHEADER���ݽṹ
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
        printf("�汾��IPV%d\n",(piphdr->Version_HeaderLength)>>4);
        break;
    case 6:
        printf("�汾��IPV%d\n",(piphdr->Version_HeaderLength)>>4);
        break;
    default:
        printf("�汾��IPV%d\n",(piphdr->Version_HeaderLength)>>4);
        break;
    }
    printf("��ͷ���ȣ�%d���ֽ�\n",((piphdr->Version_HeaderLength)&0x0f)*4);
    printf("�ܳ��ȣ�%d���ֽ�\n",piphdr->TotalLength);
    switch((piphdr->Protocal))
    {
    case 1:
        printf("Э�����ͣ�ICMP\n");
        break;
    case 2:
        printf("Э�����ͣ�IGMP\n");
        break;
    case 6:
        printf("Э�����ͣ�TCP\n");
        printf("Դ�˿ڣ�%x\n",tcp->usSourcePort);
        printf("Դ�˿ڣ�%x\n",htons(tcp->usSourcePort));//��������ͬ��������ʵ��Ӧ����һ����
        printf("Դ�˿ڣ�%d\n",ntohs(tcp->usSourcePort));
        //printf("Ŀ�Ķ˿ڣ�%d\n",*((USHORT *)tcp+1));
        printf("Ŀ�Ķ˿ڣ�%d\n",ntohs(tcp->usDestPort));
        printf("Ŀ�Ķ˿ڣ�%d\n",htons(tcp->usDestPort));
        if(ntohs(tcp->usDestPort)==80)
        {
        char *temptcp=(char *)tcp;
        char *pchar=temptcp+(tcp->ucLength>>4)*4;
        printf("TCP���ݲ��֣�%s\n",pchar);
        }
        break;
    case 8:
        printf("Э�����ͣ�EGP\n");
        break;
    case 17:
        printf("Э�����ͣ�UDP\n");
        break;
    case 41:
        printf("Э�����ͣ�IPv6\n");
        break;
    case 89:
        printf("Э�����ͣ�OSPF\n");
        break;
    }
    ULONG temp=piphdr->SourceAddress;
    printf("ԴIP��ַ��%d.%d.%d.%d\n",temp&0x000000ff,(temp>>8)&0x0000ff,(temp>>16)&0x00ff,(temp>>24)&0xff);
    ULONG temp2=piphdr->DestAddress;
    printf("Ŀ��IP��ַ��%d.%d.%d.%d\n",temp2&0x000000ff,(temp2>>8)&0x0000ff,(temp2>>16)&0x00ff,(temp2>>24)&0xff);
    printf("\n");
}

