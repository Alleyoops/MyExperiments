#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
#include <iphlpapi.h>
int main()
{
    //MIB_IPADDRTABLE*是结构体类型的指针
    //malloc申请堆内存，需要free()，返回任意类型指针，再强制转化为MIB_IPADDRTABLE*指针
    MIB_IPADDRTABLE* pIPAddrTable = (MIB_IPADDRTABLE*)malloc(sizeof(MIB_IPADDRTABLE));
    //关于ULONG：typedef unsigned long ULONG,*PULONG;和DWORD等效，因为WORD是USHORT
    ULONG dwSize=0,dwRetVal=0;//RetVal：返回值
    if(GetIpAddrTable(pIPAddrTable, &dwSize, 0)==ERROR_INSUFFICIENT_BUFFER)
    {
        //ERROR_INSUFFICIENT_BUFFER内存不够，要释放后重新申请分配内存
        free(pIPAddrTable);
        //pIPAddrTable得到的是所分配空间的首地址（即低地址）
        pIPAddrTable=(MIB_IPADDRTABLE*)malloc(dwSize);
    }
    if((dwRetVal=GetIpAddrTable(pIPAddrTable, &dwSize, 0))==NO_ERROR)//得到IP地址表，没有错误。
    {
        //自主选择主机IP地址和子网掩码
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
            printf("输入%d选择：IP地址 %d.%d.%d.%d\n",i,tempIpAddr[0],tempIpAddr[1],tempIpAddr[2],tempIpAddr[3]);
        }
        int choose = 0;
        scanf("%d",&choose);
        //ntohl：把4个字节的输入内容从网络字节序转为主机字节序（高低地址转换）
        ULONG ulHostIp=ntohl(pIPAddrTable->table[choose].dwAddr);
        //获取子网掩码
        ULONG ulHostMask=ntohl(pIPAddrTable->table[choose].dwMask);

        //从1到掩码最后一位-1，分别与IP地址相与运算，得到网络号
        for (ULONG I=1; I<(~ulHostMask); I++) //~是位运算符，表示按位取反。即0，1互换
        {
            printf("第%d次循环\n",I);
            static ULONG uNo=0;//不知道有什么用
            HRESULT hr;//Long，接受探测结果
            IPAddr ipAddr;//ULONG
            ULONG pulMac[2];//两个元素八个字节，才能装下6个字节的mac地址
            ULONG ulLen;//长度
            //遍历所有IP地址
            ipAddr=htonl(I+(ulHostIp&ulHostMask));
            //把pulmac所有字节设成0xff（广播地址）。menset()：作用是将某一块内存中的内容全部设置为指定的值
            memset(pulMac,0xff,sizeof(pulMac));
            ulLen=6;//mac地址长度为6字节
            hr=SendARP(ipAddr,0,pulMac,&ulLen);//探测主机地址，赋值给hr
            if(ulLen==6)//探测成功
            {
                uNo++;//
                /*PBYTE：byte* ,改变读取pulmac的方式，使之方便输出每个字节。
                原来是两个长度每个占四个字节，现在是六个长度(不晓得是六个还是八个)，每个元素占一个字节
                */
                PBYTE pbHexMac=(PBYTE)pulMac;
                //下面同理，ipAddr是ulong类型，四个字节，刚好等于IP地址长度，char是一个字节，强制转换成四个长度的数组，方便输出ip地址每一段
                unsigned char* strIpAddr = (unsigned char*)(&ipAddr);
                //输出打印，按16进制输出mac地址，10进制输出IP地址
                printf("%d: MAC地址 %02x:%02x:%02x:%02x:%02x:%02x  IP地址 %d.%d.%d.%d\n",
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
    free(pIPAddrTable);//释放IP地址表，堆内存
    getchar();//阻塞弹窗消失，方便观察打印结果
    return 0;
}
