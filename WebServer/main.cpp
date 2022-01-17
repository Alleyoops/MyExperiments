#include <stdio.h>
#include <iphlpapi.h>
#include <WinSock2.h>
char WEB_ROOT[MAX_PATH]= {0};
volatile unsigned int TcpClientCount = 0;
const int MAX_CLIENT=100;
const int MAX_BUF_SIZE=1024;
#define HTTPPORT 9000
#define METHOD_GET 0
#define METHOD_HEAD 1
#define HTTP_STATUS_OK "200 OK"
#define HTTP_STATUS_CREATED "201 Created"
#define HTTP_STATUS_ACCEPTED "202 Accepted"
#define HTTP_STATUS_NOCONTENT "204 No Content"
#define HTTP_STATUS_MOVEDPERM "301 Moved Permanently"
#define HTTP_STATUS_MOVEDTEMP "302 Moved Temporarily"
#define HTTP_STATUS_NOTMODIFIED "304 Not Modified"
#define HTTP_STATUS_BADREQUEST "400 Bad Request"
#define HTTP_STATUS_UNAUTHORIZED "401 Unauthorized"
#define HTTP_STATUS_FORBIDDEN "403 Forbidden"
#define HTTP_STATUS_NOTFOUND "404 File can not found"
#define HTTP_STATUS_SERVERERROR "500 Internal Server Error"
#define HTTP_STATUS_NOTIMPLEMENTED "501 Not Implemented"
#define HTTP_STATUS_BADGATEWAY "502 Bad Gateway"
#define HTTP_STATUS_UNAVAILABLE "503 Service Unavailable"
// ��������ʱ�������ת��
char *week[] =
{
    "Sun,",
    "Mon,",
    "Tue,",
    "Wed,",
    "Thu,",
    "Fri,",
    "Sat,",
};
// ��������ʱ����·�ת��
char *month[] =
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec",
};
char* typeMap[70][2]=
{
    {".doc","application/msword"},
    {".bin","application/octet-stream"},
    {".dll","application/octet-stream"},
    {".exe","application/octet-stream"},
    {".pdf","application/pdf"},
    {".ai","application/postscript"},
    {".eps","application/postscript"},
    {".ps","application/postscript"},
    {".rtf","application/rtf"},
    {".fdf","application/vnd.fdf"},
    {".arj", "application/x-arj"},
    {".gz","application/x-gzip"},
    {".class","application/x-java-class"},
    {".js","application/x-javascript"},
    {".lzh","application/x-lzh"},
    {".lnk","application/x-ms-shortcut"},
    {".tar","application/x-tar"},
    {".hlp","application/x-winhelp"},
    {".cert","application/x-x509-ca-cert"},
    {".zip","application/zip"},
    {".cab","application/x-compressed"},
    {".arj","application/x-compressed"},
    {".aif","audio/aiff"},
    {".aifc","audio/aiff"},
    {".aiff","audio/aiff"},
    {".au","audio/basic"},
    {".snd","audio/basic"},
    {".mid","audio/midi"},
    {".rmi","audio/midi"},
    {".mp3","audio/mpeg"},
    {".vox","audio/voxware"},
    {".wav","audio/wav"},
    {".ra","audio/x-pn-realaudio"},
    {".ram","audio/x-pn-realaudio"},
    {".bmp","image/bmp"},
    {".gif","image/gif"},
    {".jpeg","image/jpeg"},
    {".jpg","image/jpeg"},
    {".tif","image/tiff"},
    {".tiff","image/tiff"},
    {".xbm","image/xbm"},
    {".wrl","model/vrml"},
    {".htm","text/html"},
    {".html","text/html"},
    {".c","text/plain"},
    {".cpp","text/plain"},
    {".def","text/plain"},
    {".h","text/plain"},
    {".txt","text/plain"},
    {".rtx","text/richtext"},
    {".rtf","text/richtext"},
    {".java","text/x-java-source"},
    {".css","text/css"},
    {".mpeg","video/mpeg"},
    {".mpg","video/mpeg"},
    {".mpe","video/mpeg"},
    {".avi","video/msvideo"},
    {".mov","video/quicktime"},
    {".qt","video/quicktime"},
    {".shtml","wwwserver/html-ssi"},
    {".asa","wwwserver/isapi"},
    {".asp","wwwserver/isapi"},
    {".cfm","wwwserver/isapi"},
    {".dbm","wwwserver/isapi"},
    {".isa","wwwserver/isapi"},
    {".plx","wwwserver/isapi"},
    {".url","wwwserver/isapi"},
    {".cgi","wwwserver/isapi"},
    {".php","wwwserver/isapi"},
    {".wcgi","wwwserver/isapi"}
};
//���ݸ�TCP�̵߳Ľṹ������
struct TcpThreadParam
{
    SOCKET socket;
    sockaddr_in addr;
};
DWORD WINAPI TcpServeThread(LPVOID lpParam); //TCP�̵߳��̺߳���
typedef struct REQUEST
{
    int nMethod; // �����ʹ�÷�����GET��HEAD
    HANDLE hFile; // �������ӵ��ļ�
    char szFileName[MAX_PATH]; // �ļ������·��
    char postfix[10]; // �洢��չ��
    int fileSize;
    char StatuCodeReason[100]; // ͷ����status cod�Լ�reason-phrase
} REQUEST, *PREQUEST;
typedef struct HTTPSTATS
{
    DWORD dwRecv; // �յ��ֽ���
    DWORD dwSend; // �����ֽ���
} HTTPSTATS, *PHTTPSTATS;
//����WEB��������͵�http����
int Analyze(REQUEST* pReq, const char* pBuf);
int MakeResHeader(REQUEST *pReq,char* resHeader);
void SendFile(REQUEST *pReq,SOCKET skt);
int FileExist(REQUEST *pReq);
//����http������URL��Դ�ĺ�׺������ȡhttp��Ӧ��Ϣͷ��Content-Type�ֶ�
void GetContenType(REQUEST *pReq, char *type);
int main(int argc, char* argv[])
{
    //��ʼ��winsock2����
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("��ʼ��winsock 2ʧ�ܣ�������=%d\r\n",WSAGetLastError());
        return -1;
    }
    //��������������TCP Server Socket
    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(ListenSocket==INVALID_SOCKET )
    {
        printf("���������׽���ʧ�ܣ�������=%d\r\n",WSAGetLastError());
    }
    //��ȡTCP�����˿ں�
    u_short ListenPort=HTTPPORT;
    if(argc>=2)
    {
        ListenPort = (u_short)atoi(argv[1]);
    }
    if(argc>=3)
    {
        strcpy(WEB_ROOT,argv[2]);
    }
    else
    {
        printf("������Դ��Ŀ¼��\r\n");
        scanf("%[^\r\n]",WEB_ROOT);
    }
    //��ȡ������
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    //��ȡ����IP��ַ
    hostent *pHostent = gethostbyname(hostname);
    //��䱾��TCP Socket��ַ�ṹ
    SOCKADDR_IN ListenAddr;
    memset(&ListenAddr, 0, sizeof(SOCKADDR_IN));
    ListenAddr.sin_family = AF_INET;
    ListenAddr.sin_port = htons(ListenPort);
    ListenAddr.sin_addr.S_un.S_addr = INADDR_ANY;//*(in_addr*)pHostent->h_addr_list[0];
    //��TCP�˿�
    if (bind(ListenSocket, (sockaddr*)&ListenAddr, sizeof(ListenAddr)) == SOCKET_ERROR)
    {
        printf("�󶨣��������׽���ʧ�ܣ�������=%d\r\n",WSAGetLastError());
        return -1;
    }
    //����
    if ((listen(ListenSocket, SOMAXCONN)) == SOCKET_ERROR)
    {
        printf("����ʧ�ܣ�������=%d\r\n", WSAGetLastError());
        return -1;
    }
    //printf("����WEB�������������˿�Ϊ��%d\r\n",ListenPort);
    printf("�����������ɹ���\r\n");
    SOCKET TcpSocket;
    SOCKADDR_IN TcpClientAddr;
    while (TRUE)
    {
        //����WEB�������������
        int iSockAddrLen = sizeof(sockaddr);
        if ((TcpSocket = accept(ListenSocket, (sockaddr*)&TcpClientAddr, &iSockAddrLen))
                == SOCKET_ERROR)
        {
            printf("����WEB���������ʧ�ܣ�������=%d\r\n",WSAGetLastError());
            return -1;
        }
        //TCP�߳����ﵽ���ޣ�ֹͣ�����µ�Client
        if (TcpClientCount >= MAX_CLIENT)
        {
            closesocket(TcpSocket);
            printf("���ڷ������������ﵽ����(d),����WEB�����[%s:%d]���������󱻾ܾ���\r\n",MAX_CLIENT,inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port));
            continue;
        }
        //printf("���յ�����WEB�����[%s:%d]����������\r\n",inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port));
        printf("���������[%s:%d]�ɹ���\r\n",inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port));
        TcpThreadParam Param;
        Param.socket = TcpSocket;
        Param.addr = TcpClientAddr;
        //����TCP�����߳�
        DWORD dwThreadId;
        CreateThread(NULL, 0, TcpServeThread, &Param, 0, &dwThreadId);
        InterlockedIncrement(&TcpClientCount);
        //printf("��������ǰ������Ϊ��%d\r\n",TcpClientCount);
    }
    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}
//TCP�����߳�
DWORD WINAPI TcpServeThread(LPVOID lpParam)
{
    char buf[MAX_BUF_SIZE]= {0};
    char resHeader[MAX_BUF_SIZE]= {0};
    REQUEST httpReq= {0};
    //��ȡ�̲߳���
    SOCKET TcpSocket = ((TcpThreadParam*)lpParam)->socket;
    SOCKADDR_IN TcpClientAddr = ((TcpThreadParam*)lpParam)->addr;
    //�����ʾ��Ϣ
    //printf("����WEB�����[%s:%d]���߳�ID�ǣ�%d\r\n",inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port),GetCurrentThreadId());
    int TCPBytesReceived;
    TCPBytesReceived = recv(TcpSocket, buf, sizeof(buf), 0);
    //TCPBytesReceivedֵΪ0��ʾclient���������ر�����
    //TCPBytesRecievedֵΪSOCKET_ERROR���ʾsocket��״̬������,�޷���������ͨѶ
    //��������¶��������̵߳������ѽ�������Ҫ�˳�
    if (TCPBytesReceived == 0 || TCPBytesReceived == SOCKET_ERROR)
    {
        //printf("WEB�����[%s:%d]�Ѿ��رգ��߳�IDΪ%d���߳��˳���\r\n",inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port),GetCurrentThreadId());
    }
    else
    {
        int nRet=Analyze(&httpReq,buf);
        if (nRet)
        {
            printf("����WEB�����http�������\r\n");
        }
        else
        {
            //����http��Ӧ��Ϣ��״̬�С�ͷ���кͿ���
            MakeResHeader(&httpReq,resHeader);
            //����http��Ӧ��Ϣ��״̬�С�ͷ���кͿ���
            send(TcpSocket, resHeader, strlen(resHeader), 0);
            if(httpReq.nMethod == METHOD_GET && httpReq.hFile!=INVALID_HANDLE_VALUE)
            {
                SendFile(&httpReq,TcpSocket);
            }
        }
    }
    InterlockedDecrement(&TcpClientCount);
    closesocket(TcpSocket);
    return 0;
}
// ����request��Ϣ
int Analyze(PREQUEST pReq, const char* pBuf)
{
    // �������յ�����Ϣ
    char szSeps[] = " \r\n";
    char *cpToken;
    // ��ֹ�Ƿ�����
    if (strstr((const char *)pBuf, "..") != NULL)
    {
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_BADREQUEST);
        return 1;
    }
    // �ж�ruquest��mothed
    cpToken = strtok((char *)pBuf, szSeps); // �������ַ����ֽ�Ϊһ���Ǵ���
    if (!_stricmp(cpToken, "GET")) // GET����
    {
        pReq->nMethod = METHOD_GET;
    }
    else if (!_stricmp(cpToken, "HEAD")) // HEAD����
    {
        pReq->nMethod = METHOD_HEAD;
    }
    else
    {
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_NOTIMPLEMENTED);
        return 1;
    }

    // ��ȡRequest-URI
    cpToken = strtok(NULL, szSeps);
    if (cpToken == NULL)
    {
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_BADREQUEST);
        return 1;
    }
    strcpy(pReq->szFileName, WEB_ROOT);
    if (strlen(cpToken) > 1)
    {
        strcat(pReq->szFileName, cpToken); // �Ѹ��ļ�����ӵ���β���γ�·��
    }
    else
    {
        strcat(pReq->szFileName, "/index.html");
    }
    return 0;
}
//����http��Ӧ��Ϣ��״̬�С�ͷ���кͿ���
int MakeResHeader(REQUEST *pReq,char* resHeader)
{
    int ret=FileExist(pReq);
    char curTime[50] = {0};
    // �����ʱ��
    SYSTEMTIME st;
    GetLocalTime(&st);
    // ʱ���ʽ��
    sprintf(curTime, "%s %02d %s %d %02d:%02d:%02d GMT",week[st.wDayOfWeek],
            st.wDay,month[st.wMonth-1],st.wYear, st.wHour, st.wMinute, st.wSecond);
    // ȡ���ļ���last-modifiedʱ��
    char last_modified[60] = {0};
    if(ret==0)
    {
        // ����ļ���last-modified ʱ��
        FILETIME ftCreate, ftAccess, ftWrite;
        SYSTEMTIME stCreate;
        FILETIME ftime;
        // ����ļ���last-modified��UTCʱ��
        GetFileTime(pReq->hFile, &ftCreate, &ftAccess, &ftWrite);
        FileTimeToLocalFileTime(&ftWrite,&ftime);
        // UTCʱ��ת���ɱ���ʱ��
        FileTimeToSystemTime(&ftime, &stCreate);
        // ʱ���ʽ��
        sprintf(last_modified, "%s %02d %s %d %02d:%02d:%02d GMT",
                week[stCreate.wDayOfWeek],
                stCreate.wDay, month[stCreate.wMonth-1], stCreate.wYear, stCreate.wHour,
                stCreate.wMinute, stCreate.wSecond);
    }
    // ȡ���ļ�������
    char ContenType[50] = {0};
    GetContenType(pReq, ContenType);
    sprintf(resHeader,
            "HTTP/1.1 %s\r\nDate: %s\r\nServer: %s\r\nContent-Type: %s\r\nContent-Length: %d\r\nLast-Modified: %s\r\n\r\n",
            pReq->StatuCodeReason,
            curTime, // Date
            "My Http Server", // Server
            ContenType, // Content-Type
            pReq->fileSize, // Content-length
            ret==0?last_modified:curTime); // Last-Modified
    return 0;
}
int FileExist(REQUEST *pReq)
{
    pReq->hFile = CreateFile(pReq->szFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    // ����ļ������ڣ��򷵻س�����Ϣ
    if (pReq->hFile == INVALID_HANDLE_VALUE)
    {
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_NOTFOUND);
        pReq->fileSize=0;
        return 1;
    }
    else
    {
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_OK);
        pReq->fileSize=GetFileSize(pReq->hFile,NULL);
        return 0;
    }
}
// ����ЧӦ��Ϣ�����ݲ��֣�Ŀǰ��ʱֻ֧�֡�html�ļ�����
void SendFile(REQUEST *pReq,SOCKET skt)
{
    int n = FileExist(pReq);
    if(n) // �ļ������ڣ��򷵻�
    {
        return;
    }

    char buf[2048]= {0};
    DWORD dwRead=0;
    BOOL fRet=FALSE;
    char szMsg[512]= {0};
    int flag = 1;
    // ��д����ֱ�����
    while(1)
    {
        // ��file�ж��뵽buffer��
        fRet = ReadFile(pReq->hFile, buf, sizeof(buf), &dwRead, NULL);
        if (!fRet)
        {
            sprintf(szMsg, "%s", HTTP_STATUS_SERVERERROR);
            // ��ͻ��˷��ͳ�����Ϣ
            send(skt, szMsg, strlen(szMsg), 0);
            break;
        }
        // ���
        if (dwRead == 0)
        {
            break;
        }
        // ��buffer���ݴ��͸�client
        send(skt, buf, dwRead,0);
    }
    // �ر��ļ�
    CloseHandle(pReq->hFile);
    pReq->hFile = INVALID_HANDLE_VALUE;
}
void GetContenType(REQUEST *pReq, char *type)
{
    // ȡ���ļ�������
    char* cpToken = strrchr(pReq->szFileName,'.');
    strcpy(pReq->postfix, cpToken);
    // �����������ļ����Ͷ�Ӧ��content-type
    if(cpToken==NULL)
    {
        sprintf(type,"%s","text/html");
        return;
    }
    int i=0;
    for(i=0; i<70; i++)
    {
        if(stricmp(typeMap[i][0],pReq->postfix)==0)
        {
            strcpy(type,typeMap[i][1]);
            break;
        }
    }
    if(i==70) //δ�ҵ�
    {
        sprintf(type,"%s","text/html");
    }
}
