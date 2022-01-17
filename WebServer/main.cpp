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
// 格林威治时间的星期转换
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
// 格林威治时间的月份转换
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
//传递给TCP线程的结构化参数
struct TcpThreadParam
{
    SOCKET socket;
    sockaddr_in addr;
};
DWORD WINAPI TcpServeThread(LPVOID lpParam); //TCP线程的线程函数
typedef struct REQUEST
{
    int nMethod; // 请求的使用方法：GET或HEAD
    HANDLE hFile; // 请求连接的文件
    char szFileName[MAX_PATH]; // 文件的相对路径
    char postfix[10]; // 存储扩展名
    int fileSize;
    char StatuCodeReason[100]; // 头部的status cod以及reason-phrase
} REQUEST, *PREQUEST;
typedef struct HTTPSTATS
{
    DWORD dwRecv; // 收到字节数
    DWORD dwSend; // 发送字节数
} HTTPSTATS, *PHTTPSTATS;
//分析WEB浏览器发送的http请求
int Analyze(REQUEST* pReq, const char* pBuf);
int MakeResHeader(REQUEST *pReq,char* resHeader);
void SendFile(REQUEST *pReq,SOCKET skt);
int FileExist(REQUEST *pReq);
//根据http请求中URL资源的后缀名，获取http响应消息头中Content-Type字段
void GetContenType(REQUEST *pReq, char *type);
int main(int argc, char* argv[])
{
    //初始化winsock2环境
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("初始化winsock 2失败！错误码=%d\r\n",WSAGetLastError());
        return -1;
    }
    //创建用于侦听的TCP Server Socket
    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(ListenSocket==INVALID_SOCKET )
    {
        printf("创建监听套接字失败！错误码=%d\r\n",WSAGetLastError());
    }
    //获取TCP监听端口号
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
        printf("输入资源根目录：\r\n");
        scanf("%[^\r\n]",WEB_ROOT);
    }
    //获取本机名
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    //获取本地IP地址
    hostent *pHostent = gethostbyname(hostname);
    //填充本地TCP Socket地址结构
    SOCKADDR_IN ListenAddr;
    memset(&ListenAddr, 0, sizeof(SOCKADDR_IN));
    ListenAddr.sin_family = AF_INET;
    ListenAddr.sin_port = htons(ListenPort);
    ListenAddr.sin_addr.S_un.S_addr = INADDR_ANY;//*(in_addr*)pHostent->h_addr_list[0];
    //绑定TCP端口
    if (bind(ListenSocket, (sockaddr*)&ListenAddr, sizeof(ListenAddr)) == SOCKET_ERROR)
    {
        printf("绑定（监听）套接字失败！错误码=%d\r\n",WSAGetLastError());
        return -1;
    }
    //监听
    if ((listen(ListenSocket, SOMAXCONN)) == SOCKET_ERROR)
    {
        printf("监听失败！错误码=%d\r\n", WSAGetLastError());
        return -1;
    }
    //printf("启动WEB服务器，监听端口为：%d\r\n",ListenPort);
    printf("服务器启动成功！\r\n");
    SOCKET TcpSocket;
    SOCKADDR_IN TcpClientAddr;
    while (TRUE)
    {
        //接受WEB浏览器连接请求
        int iSockAddrLen = sizeof(sockaddr);
        if ((TcpSocket = accept(ListenSocket, (sockaddr*)&TcpClientAddr, &iSockAddrLen))
                == SOCKET_ERROR)
        {
            printf("接受WEB浏览器连接失败！错误码=%d\r\n",WSAGetLastError());
            return -1;
        }
        //TCP线程数达到上限，停止接受新的Client
        if (TcpClientCount >= MAX_CLIENT)
        {
            closesocket(TcpSocket);
            printf("由于服务器连接数达到上限(d),来自WEB浏览器[%s:%d]的连接请求被拒绝！\r\n",MAX_CLIENT,inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port));
            continue;
        }
        //printf("接收到来自WEB浏览器[%s:%d]的连接请求！\r\n",inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port));
        printf("浏览器访问[%s:%d]成功！\r\n",inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port));
        TcpThreadParam Param;
        Param.socket = TcpSocket;
        Param.addr = TcpClientAddr;
        //创建TCP服务线程
        DWORD dwThreadId;
        CreateThread(NULL, 0, TcpServeThread, &Param, 0, &dwThreadId);
        InterlockedIncrement(&TcpClientCount);
        //printf("服务器当前连接数为：%d\r\n",TcpClientCount);
    }
    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}
//TCP服务线程
DWORD WINAPI TcpServeThread(LPVOID lpParam)
{
    char buf[MAX_BUF_SIZE]= {0};
    char resHeader[MAX_BUF_SIZE]= {0};
    REQUEST httpReq= {0};
    //获取线程参数
    SOCKET TcpSocket = ((TcpThreadParam*)lpParam)->socket;
    SOCKADDR_IN TcpClientAddr = ((TcpThreadParam*)lpParam)->addr;
    //输出提示信息
    //printf("服务WEB浏览器[%s:%d]的线程ID是：%d\r\n",inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port),GetCurrentThreadId());
    int TCPBytesReceived;
    TCPBytesReceived = recv(TcpSocket, buf, sizeof(buf), 0);
    //TCPBytesReceived值为0表示client端已正常关闭连接
    //TCPBytesRecieved值为SOCKET_ERROR则表示socket的状态不正常,无法继续数据通讯
    //两种情况下都表明本线程的任务已结束，需要退出
    if (TCPBytesReceived == 0 || TCPBytesReceived == SOCKET_ERROR)
    {
        //printf("WEB浏览器[%s:%d]已经关闭，线程ID为%d的线程退出！\r\n",inet_ntoa(TcpClientAddr.sin_addr),ntohs(TcpClientAddr.sin_port),GetCurrentThreadId());
    }
    else
    {
        int nRet=Analyze(&httpReq,buf);
        if (nRet)
        {
            printf("分析WEB浏览器http请求错误！\r\n");
        }
        else
        {
            //构造http响应消息的状态行、头部行和空行
            MakeResHeader(&httpReq,resHeader);
            //发送http响应消息的状态行、头部行和空行
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
// 分析request信息
int Analyze(PREQUEST pReq, const char* pBuf)
{
    // 分析接收到的信息
    char szSeps[] = " \r\n";
    char *cpToken;
    // 防止非法请求
    if (strstr((const char *)pBuf, "..") != NULL)
    {
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_BADREQUEST);
        return 1;
    }
    // 判断ruquest的mothed
    cpToken = strtok((char *)pBuf, szSeps); // 缓存中字符串分解为一组标记串。
    if (!_stricmp(cpToken, "GET")) // GET命令
    {
        pReq->nMethod = METHOD_GET;
    }
    else if (!_stricmp(cpToken, "HEAD")) // HEAD命令
    {
        pReq->nMethod = METHOD_HEAD;
    }
    else
    {
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_NOTIMPLEMENTED);
        return 1;
    }

    // 获取Request-URI
    cpToken = strtok(NULL, szSeps);
    if (cpToken == NULL)
    {
        strcpy(pReq->StatuCodeReason, HTTP_STATUS_BADREQUEST);
        return 1;
    }
    strcpy(pReq->szFileName, WEB_ROOT);
    if (strlen(cpToken) > 1)
    {
        strcat(pReq->szFileName, cpToken); // 把该文件名添加到结尾处形成路径
    }
    else
    {
        strcat(pReq->szFileName, "/index.html");
    }
    return 0;
}
//构造http响应消息的状态行、头部行和空行
int MakeResHeader(REQUEST *pReq,char* resHeader)
{
    int ret=FileExist(pReq);
    char curTime[50] = {0};
    // 活动本地时间
    SYSTEMTIME st;
    GetLocalTime(&st);
    // 时间格式化
    sprintf(curTime, "%s %02d %s %d %02d:%02d:%02d GMT",week[st.wDayOfWeek],
            st.wDay,month[st.wMonth-1],st.wYear, st.wHour, st.wMinute, st.wSecond);
    // 取得文件的last-modified时间
    char last_modified[60] = {0};
    if(ret==0)
    {
        // 获得文件的last-modified 时间
        FILETIME ftCreate, ftAccess, ftWrite;
        SYSTEMTIME stCreate;
        FILETIME ftime;
        // 获得文件的last-modified的UTC时间
        GetFileTime(pReq->hFile, &ftCreate, &ftAccess, &ftWrite);
        FileTimeToLocalFileTime(&ftWrite,&ftime);
        // UTC时间转化成本地时间
        FileTimeToSystemTime(&ftime, &stCreate);
        // 时间格式化
        sprintf(last_modified, "%s %02d %s %d %02d:%02d:%02d GMT",
                week[stCreate.wDayOfWeek],
                stCreate.wDay, month[stCreate.wMonth-1], stCreate.wYear, stCreate.wHour,
                stCreate.wMinute, stCreate.wSecond);
    }
    // 取得文件的类型
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
    // 如果文件不存在，则返回出错信息
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
// 发送效应消息的数据部分（目前暂时只支持“html文件”）
void SendFile(REQUEST *pReq,SOCKET skt)
{
    int n = FileExist(pReq);
    if(n) // 文件不存在，则返回
    {
        return;
    }

    char buf[2048]= {0};
    DWORD dwRead=0;
    BOOL fRet=FALSE;
    char szMsg[512]= {0};
    int flag = 1;
    // 读写数据直到完成
    while(1)
    {
        // 从file中读入到buffer中
        fRet = ReadFile(pReq->hFile, buf, sizeof(buf), &dwRead, NULL);
        if (!fRet)
        {
            sprintf(szMsg, "%s", HTTP_STATUS_SERVERERROR);
            // 向客户端发送出错信息
            send(skt, szMsg, strlen(szMsg), 0);
            break;
        }
        // 完成
        if (dwRead == 0)
        {
            break;
        }
        // 将buffer内容传送给client
        send(skt, buf, dwRead,0);
    }
    // 关闭文件
    CloseHandle(pReq->hFile);
    pReq->hFile = INVALID_HANDLE_VALUE;
}
void GetContenType(REQUEST *pReq, char *type)
{
    // 取得文件的类型
    char* cpToken = strrchr(pReq->szFileName,'.');
    strcpy(pReq->postfix, cpToken);
    // 遍历搜索该文件类型对应的content-type
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
    if(i==70) //未找到
    {
        sprintf(type,"%s","text/html");
    }
}
