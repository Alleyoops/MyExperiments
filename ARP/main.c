#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iphlpapi.h>
int main()
{
    //MIB_IPADDRTABLE*�ǽṹ�����͵�ָ��
    //malloc������ڴ棬��Ҫfree()��������������ָ�룬��ǿ��ת��ΪMIB_IPADDRTABLE*ָ��
    MIB_IPADDRTABLE* pIPAddrTable = (MIB_IPADDRTABLE*)malloc(sizeof(MIB_IPADDRTABLE));
    //����ULONG��typedef unsigned long ULONG,*PULONG;��DWORD��Ч����ΪWORD��USHORT
    ULONG dwSize=0,dwRetVal=0;//RetVal������ֵ
    if(GetIpAddrTable(pIPAddrTable, &dwSize, 0)==ERROR_INSUFFICIENT_BUFFER)
    {
        //ERROR_INSUFFICIENT_BUFFER�ڴ治����Ҫ�ͷź�������������ڴ�
        free(pIPAddrTable);
        //pIPAddrTable�õ�����������ռ���׵�ַ�����͵�ַ��
        pIPAddrTable=(MIB_IPADDRTABLE*)malloc(dwSize);
    }
    if((dwRetVal=GetIpAddrTable(pIPAddrTable, &dwSize, 0))==NO_ERROR)//�õ�IP��ַ��û�д���
    {
        //����ѡ������IP��ַ����������
        for (int i=0; i<(pIPAddrTable->dwNumEntries); i++)
            //for (int i=0;(pIPAddrTable->table[i].dwAddr)!=0;i++)
        {
            ULONG tempIA = pIPAddrTable->table[i].dwAddr;
            //int ip1,ip2,ip3,ip4;
            //ip1=tempIA&0x000000ff;
            //ip2=(tempIA>>8)&0x0000ff;
            //ip3=(tempIA>>16)&0x00ff;
            //ip4=(tempIA>>24)&0xff;
            //printf("%d.%d.%d.%d \t",ip1,ip2,ip3,ip4);
            unsigned char* tempIpAddr = (unsigned char*)(&tempIA);
            printf("����%dѡ��IP��ַ %d.%d.%d.%d\n",i,tempIpAddr[0],tempIpAddr[1],tempIpAddr[2],tempIpAddr[3]);
        }
        int choose = 0;
        scanf("%d",&choose);
        //ntohl����4���ֽڵ��������ݴ������ֽ���תΪ�����ֽ��򣨸ߵ͵�ַת����
        ULONG ulHostIp=ntohl(pIPAddrTable->table[choose].dwAddr);
        //��ȡ��������
        ULONG ulHostMask=ntohl(pIPAddrTable->table[choose].dwMask);

        //��1���������һλ-1���ֱ���IP��ַ�������㣬�õ������
        for (ULONG I=1; I<(~ulHostMask); I++) //~��λ���������ʾ��λȡ������0��1����
        {
            printf("��%d��ѭ��\n",I);
            static ULONG uNo=0;//��֪����ʲô��
            HRESULT hr;//Long������̽����
            IPAddr ipAddr;//ULONG
            ULONG pulMac[2];//����Ԫ�ذ˸��ֽڣ�����װ��6���ֽڵ�mac��ַ
            ULONG ulLen;//����
            //��������IP��ַ
            ipAddr=htonl(I+(ulHostIp&ulHostMask));
            //��pulmac�����ֽ����0xff���㲥��ַ����menset()�������ǽ�ĳһ���ڴ��е�����ȫ������Ϊָ����ֵ
            memset(pulMac,0xff,sizeof(pulMac));
            ulLen=6;//mac��ַ����Ϊ6�ֽ�
            hr=SendARP(ipAddr,0,pulMac,&ulLen);//̽��������ַ����ֵ��hr
            if(ulLen==6)//̽��ɹ�
            {
                uNo++;//
                /*PBYTE��byte* ,�ı��ȡpulmac�ķ�ʽ��ʹ֮�������ÿ���ֽڡ�
                ԭ������������ÿ��ռ�ĸ��ֽڣ���������������(���������������ǰ˸�)��ÿ��Ԫ��ռһ���ֽ�
                */
                PBYTE pbHexMac=(PBYTE)pulMac;
                //����ͬ��ipAddr��ulong���ͣ��ĸ��ֽڣ��պõ���IP��ַ���ȣ�char��һ���ֽڣ�ǿ��ת�����ĸ����ȵ����飬�������ip��ַÿһ��
                unsigned char* strIpAddr = (unsigned char*)(&ipAddr);
                //�����ӡ����16�������mac��ַ��10�������IP��ַ
                printf("%d: MAC��ַ %02x:%02x:%02x:%02x:%02x:%02x  IP��ַ %d.%d.%d.%d\n",
                       uNo,pbHexMac[0],pbHexMac[1],pbHexMac[2],pbHexMac[3],pbHexMac[4],pbHexMac[5],
                       strIpAddr[0],strIpAddr[1],strIpAddr[2],strIpAddr[3]);
            }

        }
    }
    else
    {
        printf("Call to GetIpAddrTable failed.\n");
    }
    printf("OVER!\n");
    free(pIPAddrTable);//�ͷ�IP��ַ�����ڴ�
    getchar();//����������ʧ������۲��ӡ���
    return 0;
}
