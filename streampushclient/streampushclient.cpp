// streampushclient.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
SOCKET g_socket = INVALID_SOCKET;

#include "h264frame.h"

int _tmain(int argc, _TCHAR* argv[])
{
    WORD wVersionRequested;  
    WSADATA wsaData;  
    int ret;  		
    BOOL fSuccess = TRUE;  

    //WinSock��ʼ��  
    wVersionRequested = MAKEWORD(2, 2); //ϣ��ʹ�õ�WinSock DLL�İ汾  
    ret = WSAStartup(wVersionRequested, &wsaData);  //�����׽��ֿ�  
    if(ret!=0)  
    {  
        printf("WSAStartup() failed!\n");  
        //return 0;  
    }  
    //ȷ��WinSock DLL֧�ְ汾2.2  
    if(LOBYTE(wsaData.wVersion)!=2 || HIBYTE(wsaData.wVersion)!=2)  
    {  
        WSACleanup();   //�ͷ�Ϊ�ó���������Դ����ֹ��winsock��̬���ʹ��  
        printf("Invalid WinSock version!\n");  
        //return 0;  
    }
    {		
        SOCKET sClient; //�����׽���  
        struct sockaddr_in saServer; //��������ַ��Ϣ  		
        //����Socket,ʹ��TCPЭ��  
        g_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  
        if (g_socket == INVALID_SOCKET)  
        {  
            WSACleanup();  
            printf("socket() failed!\n");  
            //return 0;  
        }  

        //������������ַ��Ϣ  
        saServer.sin_family = AF_INET; //��ַ����  
        saServer.sin_port = htons(1985); //ע��ת��Ϊ�������  
        saServer.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  
        //saServer.sin_addr.S_un.S_addr = inet_addr("114.215.175.1");
        //���ӷ�����  
        int ret = connect(g_socket, (struct sockaddr *)&saServer, sizeof(saServer));  
        if (ret == SOCKET_ERROR)  
        {  
            printf("connect() failed!\n");  
            closesocket(g_socket); //�ر��׽���  
            WSACleanup();  
            //return 0;  
        }  

        /*FILE* pFiletest = fopen("d:/dest.flv", "rb");
        if (!pFiletest)
        {
        AfxMessageBox("read file error");
        return;
        }

        int dwFlvLen = 30*1024;
        char* pBuffer = new char[dwFlvLen];
        while (1)
        {		
        while (!feof(pFiletest)) 
        {		
        int nRet = fread(pBuffer, 1, dwFlvLen, pFiletest);
        if (nRet != dwFlvLen)
        {
        break;
        }			
        ::send(g_socket, (const char*)(&nRet), 4, 0);
        ::send(g_socket, (const char*)pBuffer, nRet, 0);	
        Sleep(30);
        }
        fseek(pFiletest, 0, SEEK_SET);
        continue;
        }
        fclose(pFiletest);
        return;*/
        if (g_socket != INVALID_SOCKET)
        {
            char* strdeviceid = "123abcdef32153421";
            int nLen = strlen(strdeviceid );

            ::send(g_socket, (const char*)&nLen, 4, 0);
            ::send(g_socket, strdeviceid, nLen, 0);	
        }


        //FILE* pFile = fopen("D:\\dashmp4-master\\dashmp4demo\\Debug\\test.h264", "rb");
        FILE* pFile = fopen("d:\\test.h264", "rb");
        if (!pFile)
        {
            printf("read file error");
            return -1;
        }




        u8* buffer = new u8[1*1024*1024];
        u8* dstflvbuffer = new u8[1*1024*1024];
        u8* flvheader = new u8[1*1024*1024];
        u32 dwTime = 0;
        u32 dwLen = 0;	

        CFlv flv;

        while (1)
        {
            printf("started to send\r\n");
            while (1)
            {
                //Sleep(30);
                int nRet = fread(&dwLen, 1, 4, pFile);
                if (4 != nRet) 
                {
                    break;
                }

                nRet = fread(buffer, 1, dwLen, pFile);
                if (nRet != dwLen)
                {
                    break;	
                }
                Sleep(30);

                Buffer bufH264;
                bufH264.pBuffer = buffer;
                bufH264.dwBufLen = dwLen;
                Buffer bufFlvheader;
                bufFlvheader.pBuffer = flvheader;
                bufFlvheader.dwBufLen = 1*1024*1024;

                Buffer bufFlvData;
                bufFlvData.pBuffer = dstflvbuffer;
                bufFlvData.dwBufLen = 1*1024*1024;
                u32 dwRet = flv.ConvertH264ToFlv(&bufH264, 0, &bufFlvData, &bufFlvheader);
                if ((dwRet & HAS_FLVHEADER) == HAS_FLVHEADER)
                {
                    ::send(g_socket, (const char*)(&bufFlvheader.dwBufLen), 4, 0);
                    ::send(g_socket, (const char*)bufFlvheader.pBuffer, bufFlvheader.dwBufLen, 0);	
                }
                if ((dwRet & HAS_FLVFRAMEDATA) == HAS_FLVFRAMEDATA)
                {
                    int nRet = ::send(g_socket, (const char*)(&bufFlvData.dwBufLen), 4, 0);
                    nRet = ::send(g_socket, (const char*)bufFlvData.pBuffer, bufFlvData.dwBufLen, 0);	
                    if (nRet == SOCKET_ERROR)
                    {
                        printf("send socket error\r\n ");
                        return -1;
                    }
                }

            }
            fseek(pFile, 0, SEEK_SET);
        }

        fclose(pFile);		
    }
    return 0;
}

