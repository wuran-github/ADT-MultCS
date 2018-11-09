#pragma once
#include <vector>
#include <string>
#include <QNetworkInterface>
#include <Windows.h>
#include <iostream>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qdatastream.h>
#include <QtCore/QCoreApplication>

#pragma comment(lib,"ws2_32.lib")

class WorkStation
{
public:
	WorkStation();

	//entry
	void Run();
	//通用方法
	void Init();
	int InitSocket();
	//在这里配置IP等信息
	void GetConfig();

	//发送文件给分析器
	void SendFile();
	//创建socket等待analysis connect
	int InitStationSocket();
	int CreateStationSocket();
	void AcceptAnalysis();
	static DWORD WINAPI AcceptAnalysisThread(LPVOID lpParam);

	//通知分配器有文件要发送
	int InitDistributeSocket();
	int CreateDistributeSocket();
	void NoticeDistributeServer();
	static DWORD WINAPI connectDistributeThread(LPVOID lpParam);

	//通知管理器
	int InitManagerSocket();
	int CreateManagerSocket();
	void SendIPAndStatus();
	static DWORD WINAPI connectManagerThread(LPVOID lpParam);

	~WorkStation();
private:
	void GetIps();
	QStringList GetFileList();
private:
	int port;
	std::vector<std::string> ips;

	std::string distributeIP;
	int distributePort;

	std::string managerIP;
	int managerPort;

	//status
	bool isSending;
	int sendBufferSize = 1024 * 100;

	SOCKET stationSocket;
	SOCKET distributeSocket;
	SOCKET managerSocket;
	SOCKET analysisSocket;
};

