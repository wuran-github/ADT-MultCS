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
#include <time.h>
#include <queue>
#include <QtCore/QCoreApplication>

#pragma comment(lib,"ws2_32.lib")

class FileAnalysis
{
public:
	FileAnalysis();
	//entry
	void Run();
	//通用方法
	void Init();
	int InitSocket();
	//在这里配置IP等信息
	void GetConfig();

	//distribute
	int InitAnalysisSocket();
	int CreateAnalysisSocket();
	int AcceptDistributeSocket();
	void GetWorkStationInfo();
	static DWORD WINAPI AcceptDistributeThread(LPVOID lpParam);

	//workstation
	int InitWorkStationSocket();
	int RecevieFile();
	static DWORD WINAPI connectStationThread(LPVOID lpParam);

	//通知管理器
	int InitManagerSocket();
	int CreateManagerSocket();
	void SendIPAndStatus();
	static DWORD WINAPI connectManagerThread(LPVOID lpParam);

	//分析文件
	void CreateAnalyzeThread();
	void AnalyzeFile();
	static DWORD WINAPI AnalyzeFileThread(LPVOID lpParam);

	//通知分配器当前分析文件数量
	int InitDistributeSocket();
	void SendAnalyzingFileNum();
	static DWORD WINAPI connectDistributeThread(LPVOID lpParam);

	~FileAnalysis();
private:
	void GetIps();
private:
	int port;
	std::vector<std::string> ips;

	std::string workStationIP;
	int workStationPort;

	std::string managerIP;
	int managerPort;
	char* ack = "ack";

	std::string distributeIP;
	int distributePort;

	//status
	bool isReceiving;
	bool isAnalysing;

	//总接受文件数
	int totalNum = 0;
	//analysis list
	std::queue<QString> analysisList;
	std::queue<int> sizeList;

	//要开启分析的线程数
	int threadNum = 2;
	//文件列表互斥锁
	HANDLE hMutex;
	//文件互斥锁
	HANDLE fileMutex;
	//等待时间
	int millisec = 1000*2;
	//分析后是否删除
	bool deleteAfter = true;
	//收了多少文件后开始分析
	int beginAnalyzeFileNums = 10;

	//sendAnalysis Status sec
	double toDistributeSec = 0.2;



	SOCKET stationSocket;
	SOCKET distributeSocket;
	SOCKET managerSocket;
	SOCKET analysisSocket;
};