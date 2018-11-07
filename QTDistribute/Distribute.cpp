#include "Distribute.h"



Distribute::Distribute()
{
}

void Distribute::Run()
{
}

void Distribute::Init()
{
}

int Distribute::InitSocket()
{
	//加载套接字库
	WORD wVersionRequested;
	//这个结构被用来存储被WSAStartup函数调用后返回的Windows Sockets数据。它包含Winsock.dll执行的数据。
	WSADATA wsaData;
	int err;
	//声明调用1.1版本的winsock。MAKEWORD(2,2)是2.2版本
	wVersionRequested = MAKEWORD(1, 1);
	//启动WSA
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return FALSE;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return FALSE;
	}
	return 0;
}

void Distribute::GetConfig()
{
	GetIps();
	port = 8989;
	isReceiving = false;
	isAnalysing = false;
	auto path = QDir::currentPath() + "/config.config";
	QFile file(path);
	if (file.exists()) {
		auto portstr = file.readAll().data();
		port = atoi(portstr);
	}
}


Distribute::~Distribute()
{
}

void Distribute::GetIps()
{
	for each (QHostAddress address in QNetworkInterface().allAddresses())
	{
		if (address.protocol() == QAbstractSocket::IPv4Protocol)
		{
			QString ip = address.toString();
			ips.push_back(ip.toStdString());
		}
	}
}

int Distribute::InitStationSocket()
{
	return 0;
}

int Distribute::CreateStationSocket()
{
	return 0;
}

int Distribute::AcceptStationSocket()
{
	return 0;
}

void Distribute::GetWorkStationRequest()
{
}

DWORD Distribute::AcceptStationThread(LPVOID lpParam)
{
	return 0;
}
