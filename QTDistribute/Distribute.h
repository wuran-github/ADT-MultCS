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

struct AnalysisData
{
	std::string ip;
	int port;
	/* data */
};

class Distribute
{
public:
	Distribute();
	//entry
	void Run();
	//通用方法
	void Init();
	int InitSocket();
	//在这里配置IP等信息
	void GetConfig();

	//distribute
	int InitDistributeSocket();
	int CreateDistributeSocket();
	int AcceptStationSocket();
	void GetWorkStationRequest(SOCKET socket);
	static DWORD WINAPI AcceptStationThread(LPVOID lpParam);
	//用多线程来获取要发送文件的station
	static DWORD WINAPI GetStationInfoThread(LPVOID lpParam);

	//analysis
	int InitDistributeSocket();
	int CreateDistributeSocket();
	int AcceptStationSocket();
	int NoticeAnalysis(std::string stationIP,int stationPort);
	static DWORD WINAPI AcceptStationThread(LPVOID lpParam);

	~Distribute();
private:
	void GetIps();
private:
	int port;
	std::vector<std::string> ips;

	std::string workStationIP;
	int workStationPort;

	std::string managerIP;
	int managerPort;
	//File Analysis
	std::vector<AnalysisData> AnalysisList;
	int nowAnalysis;
	//status
	bool isReceiving;
	bool isAnalysing;

	SOCKET stationSocket;
	SOCKET distributeSocket;
	SOCKET managerSocket;
	SOCKET analysisSocket;
};

