#include "Distribute.h"


struct StationSocketParam
{
	Distribute* point;
	SOCKET socket;
	/* data */
};
struct StationInfo {
	int port;
	std::string ip;
	int fileNum;
};
struct AnalyzingFileInfo {
	int port;
	std::string ip;
	int fileNum;
};

Distribute::Distribute()
{
	Init();
}

void Distribute::Run()
{
	// create socket wait station connect
	InitDistributeSocket();

	//
	InitAnalyzingSocket();
	//防止被关掉
	while (true)
	{

	}
}

void Distribute::Init()
{
	nowAnalysis=0;
	GetIps();
	GetConfig();
	InitSocket();


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
	auto path = QCoreApplication::applicationDirPath() + "/config.config";
	QFile file(path);
	if (file.exists()) {
		file.open(QFile::ReadOnly | QFile::Text);
		auto portstr = file.readLine();
		port = atoi(portstr);
		file.close();
	}

	//get analysis config
	auto analysisPath= QCoreApplication::applicationDirPath() + "/analysis.config";
	QFile als(analysisPath);
	if (als.exists()) {
		als.open(QFile::ReadOnly);
		QTextStream in(&als);
		while (!in.atEnd()) {
			QStringList analysisInfo=in.readLine().split(',');
			AnalysisData info;
			info.ip = analysisInfo[0].toStdString();
			info.port = analysisInfo[1].toInt();
			this->AnalysisList.push_back(info);
			this->AnalyzingFile.push_back(0);
		}
		als.close();
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
			//ips.push_back(ip.toStdString());

			//测试
			ips.push_back("127.0.0.1");
		}
	}
}

int Distribute::InitDistributeSocket()
{
	CreateDistributeSocket();
	HANDLE hThread = CreateThread(NULL, 0, Distribute::AcceptStationThread, this, 0, NULL);
	return 0;
}

int Distribute::CreateDistributeSocket()
{
	int err;
	//创建套接字
	//SOCKET m_socket=socket(AF_INET,SOCK_STREAM,0);
	distributeSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (distributeSocket == INVALID_SOCKET)
	{
		std::cout << "distributeSocket Create Failed！" << endl;
		return FALSE;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(this->port);

	err = ::bind(distributeSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));    //绑定本地端口
	if (err == SOCKET_ERROR)
	{
		closesocket(distributeSocket);
		return FALSE;
	}
	listen(distributeSocket, 5);//开启监听

	return 1;
}

int Distribute::AcceptStationSocket()
{
	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (true)
	{
		SOCKET socket = accept(this->distributeSocket, (SOCKADDR*)&addrClient, &len);

		if (socket == INVALID_SOCKET) {
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(socket);
			return -1;
		}


		StationSocketParam param;
		param.socket=socket;
		param.point=this;
		//多客户端的方法就每接收到一个accept就在开启一个线程
		//同时有可能有多个station过来
		HANDLE hThread = CreateThread(NULL, 0, Distribute::GetStationInfoThread, &param, 0, NULL);
	}
}

void Distribute::GetWorkStationRequest(SOCKET socket)
{
	char* ack="ack";
	char recvBuffer[1024];
	StationInfo* info;
	recv(socket, recvBuffer, 1024, NULL);
	//char[] 转结构体
	info = (StationInfo *)recvBuffer;
	std::string ip=info->ip;
	int port=info->port;
	//确认
	send(socket,ack,sizeof(ack),0);

	//通知analysis server
	NoticeAnalysis(ip,port);
}

DWORD Distribute::AcceptStationThread(LPVOID lpParam)
{
	Distribute* p = (Distribute*)lpParam;
	p->AcceptStationSocket();
	return 0;
}
DWORD Distribute::GetStationInfoThread(LPVOID lpParam)
{
	StationSocketParam* param = (StationSocketParam*)lpParam;
	param->point->GetWorkStationRequest(param->socket);
	return 0;
}

int Distribute::NoticeAnalysis(std::string stationIP,int stationPort)
{
	unsigned long long file_size = 0;
	//sequence distribute
	//this->nowAnalysis= (this->nowAnalysis+1)%this->AnalysisList.size();

	//
	int min = 99999;
	int minIndex = 0;
	for (int i = 0; i < this->AnalyzingFile.size();i++) {
		int v = AnalyzingFile[i];
		if (min > v) {
			min = v;
			minIndex = i;
		}
	}

	this->nowAnalysis = minIndex;

	
	
	//get analysis server data
	AnalysisData analysis = this->AnalysisList[this->nowAnalysis];

	//set satation info
	

	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(analysis.ip.c_str());
	addrSrv.sin_port = htons(analysis.port);
	addrSrv.sin_family = AF_INET;
	//::connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	//recv(sockClient,(char*)&file_size,sizeof(unsigned long long)+1,NULL);

	if (!::connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)))
	{


		char sendBuffer[1024];
		char recvBuffer[1024];
		StationInfo info;
		info.ip = stationIP;
		info.port = stationPort;
			//th
			//结构体转成char[]
			memcpy(sendBuffer, &info, sizeof(info));

			//第一次先发送文件信息
			send(sockClient, sendBuffer, 1024, NULL);

			//然后等待确认指令
			recv(sockClient, recvBuffer, 1024, 0);
		
		//判断信号，fix me
	}

	closesocket(sockClient);//关闭套接字
	return 0;
}

int Distribute::InitAnalyzingSocket()
{
	CreateAnalyzingSocket();
	HANDLE hThread = CreateThread(NULL, 0, Distribute::AcceptAnalyzingFileThread, this, 0, NULL);
	return 0;
}

int Distribute::CreateAnalyzingSocket()
{
	int err;
	//创建套接字
	//SOCKET m_socket=socket(AF_INET,SOCK_STREAM,0);
	acceptAnalyzingFileSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (acceptAnalyzingFileSocket == INVALID_SOCKET)
	{
		std::cout << "acceptAnalyzingFileSocket Create Failed！" << endl;
		return FALSE;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(8001);

	err = ::bind(acceptAnalyzingFileSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));    //绑定本地端口
	if (err == SOCKET_ERROR)
	{
		closesocket(acceptAnalyzingFileSocket);
		return FALSE;
	}
	listen(acceptAnalyzingFileSocket, 5);//开启监听

	return 1;
}

int Distribute::AcceptAnalyzingFileSocket()
{
	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (true)
	{
		SOCKET socket = accept(this->acceptAnalyzingFileSocket, (SOCKADDR*)&addrClient, &len);

		if (socket == INVALID_SOCKET) {
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(socket);
			return -1;
		}


		StationSocketParam param;
		param.socket = socket;
		param.point = this;
		//多客户端的方法就每接收到一个accept就在开启一个线程
		//同时有可能有多个station过来
		HANDLE hThread = CreateThread(NULL, 0, Distribute::GetAnalyzingFileThread, &param, 0, NULL);
	}
}

void Distribute::GetAnalyzingFileNum(SOCKET socket)
{
	char* ack = "ack";
	char recvBuffer[1024];
	AnalyzingFileInfo* info;
	while (true)
	{
		recv(socket, recvBuffer, 1024, NULL);
		//char[] 转结构体
		info = (AnalyzingFileInfo *)recvBuffer;
		std::string ip = info->ip;
		int port = info->port;
		int num = info->fileNum;
		int i = 0;
		for each (auto analysis in this->AnalysisList)
		{
			if (analysis.ip == ip && port == analysis.port) {
				AnalyzingFile[i] = num;
			}
			i++;
		}

		//确认
		send(socket, ack, sizeof(ack), 0);

		//防止太频繁
		//Sleep(1000*0.5);
	}

}

DWORD Distribute::GetAnalyzingFileThread(LPVOID lpParam)
{
	StationSocketParam* param = (StationSocketParam*)lpParam;
	param->point->GetAnalyzingFileNum(param->socket);
	return 0;
}

DWORD Distribute::AcceptAnalyzingFileThread(LPVOID lpParam)
{
	Distribute* p = (Distribute*)lpParam;
	p->AcceptAnalyzingFileSocket();
	return 0;
}
