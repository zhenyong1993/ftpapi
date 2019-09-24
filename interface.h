#ifndef _INTERFACE_H
#define _INTERFACE_H

#include <string>
#include <string.h>
#include <iostream>
#include <algorithm> 
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <map>

using namespace std;

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#define DLLImport __declspec(dllimport)
#define DLLExport __declspec(dllexport)
#else
#define DLLImport extern
#define DLLExport __attribute__ ((visibility ("default")))
#endif
DLLExport int libInit();
//ftp_addr: Ϊftp�������ϴ����ص��ļ�����·����
DLLExport int ftpDownloadFile(char* ftp_addr, char* path, const char *username, const char *password);
//ftp_addr: Ϊftp�������ϴ��ϴ����ļ�����·����
DLLExport int ftpUploadFile(char* ftp_addr, char* filePath, const char *username, const char *password);
//ftp_addr: Ϊftp��������ָ��Ŀ¼��
//dirInfo : ����ָ�����ļ�·���������ָ��Ŀ¼�£������ļ�������Ŀ¼�����ƣ���С���޸�ʱ��ȵ���Ϣ
DLLExport int ftpGetDirInfo(char* ftp_addr, char* dirpath, const char *username, const char *password);
//ftp_addr: ftp_addrΪftp��������ָ��Ŀ¼��
//filePath: ��ɾ����ftp�������ϵ��ļ���(Ϊ�ոյ�ftp_addr�����·����)
DLLExport int ftpDeleteFile(char* ftp_addr, char* filePath, const char *username, const char *password);

DLLExport int uploadFile(char* cnc_addr, char* machineNo, char* filePath);
DLLExport int downloadFile(char* cnc_addr, char* machineNo, char* filePath);
DLLExport int modifyPara(char* cnc_addr, char* machineNo, char* paraName, char* paraValue);
DLLExport void libCleanUp();
DLLExport int checkFile(char* cnc_addr, char* machineNo, char* filePath);
DLLExport int deleteFile(char* cnc_addr, char* machineNo, char* filePath);
DLLExport int getPara(char* cnc_addr, char* machineNo, char* paraName);

DLLExport int getFile(char* url, char* destFile);
DLLExport string GetFileName(const char *fileFullPath,char* flag);
DLLExport int sendFileByPost(char* file, char *url,char* utfName);
DLLExport int getDownloadFileLenth(const char *url);
DLLExport string& trim(string &s);
#if defined(__cplusplus)
}
#endif

#endif
