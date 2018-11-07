#include "WorkStation.h"


struct FileParam
{
	std::string fileNames[100] = { 0 };
	int fileSize[100] = { 0 };
	int num = 0;
};
struct SendToDistribute {
	int port;
	std::string ip;
	int fileNum;
};

struct Status {
	std::string ip;
	int port;
	bool sending;
};
WorkStation::WorkStation()
{

}

void WorkStation::Run()
{
}

void WorkStation::Init()
{
	GetConfig();
	InitSocket();
}

int WorkStation::InitSocket()
{
	//加载套接字库
	WORD wVersionRequested;
	//这个结构被用来存储被WSAStartup函数调用后返回的Windows Sockets数据。它包含Winsock.dll执行的数据。
	WSADATA wsaData;
	int err;
	//声明调用1.1版本的winsock。MAKEWORD(2,2)是2.2版本
	wVersionRequested = MAKEWORD(1, 1);
	//启动WSA，之前client没执行这个就一直报错
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

}

void WorkStation::GetConfig()
{
	GetIps();
	port = 8989;
	isSending = false;
	auto path=QDir::currentPath()+"/config.config";
	QFile file(path);
	if (file.exists()) {
		auto portstr=file.readAll().data();
		port=atoi(portstr);
	}
}

void WorkStation::SendFile()
{
	//已读的总大小
	long totalBytes = 0;
	int actualSendBytes = 0;
	char infochar[1024];
	char recvBuffer[1024];
	
	//获取列表
	auto fileList = GetFileList();
	//

	//读取文件信息
	FileParam fileParam;
	int index = 0;
	for each (QString filePath in fileList)
	{
		QFileInfo info(filePath);
		auto qname = info.fileName().toStdString();
		fileParam.fileNames[index]=qname;
		fileParam.fileSize[index]=info.size();
		index++;

	}
	fileParam.num = index;

	//结构体转成char[]
	memcpy(infochar, &fileParam, sizeof(fileParam));

	//改标志位
	this->isSending = true;

	//第一次先发送文件信息
	send(analysisSocket, infochar, 1024, NULL);

	//可以确保对方接受完了文件信息
	recv(analysisSocket, recvBuffer, 1024, NULL);

	//循环发送文件
	for each(QString filePath in fileList)
	{
		QFile file(filePath);
		file.open(QFile::ReadOnly);

		QDataStream stream(&file);

		char Buffer[1024];
		DWORD dwNumberOfBytesRead;



		do
		{
			//::ReadFile(hFile, Buffer, sizeof(Buffer), &dwNumberOfBytesRead, NULL);
			dwNumberOfBytesRead = stream.readRawData(Buffer, sizeof(Buffer));
			actualSendBytes = ::send(analysisSocket, Buffer, dwNumberOfBytesRead, 0);

			//防止缓冲区溢出丢失数据
			totalBytes += actualSendBytes;
			stream.device()->seek(totalBytes);
		} while (dwNumberOfBytesRead && actualSendBytes>0);

		file.close();

		//等待对方确认,确认完毕再传输下一个文件
		recv(analysisSocket, recvBuffer, 1024, NULL);

		//检验确认信号 fix me
	}

	
}

int WorkStation::InitStationSocket()
{
	CreateStationSocket();
	HANDLE hThread = CreateThread(NULL, 0, WorkStation::AcceptAnalysisThread, this, 0, NULL);
}

int WorkStation::CreateStationSocket()
{
	
	int err;
	//创建套接字
	//SOCKET m_socket=socket(AF_INET,SOCK_STREAM,0);
	stationSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (stationSocket == INVALID_SOCKET)
	{
		std::cout << "socket Create Failed！" << endl;
		return FALSE;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(this->port);

	err = ::bind(stationSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));    //绑定本地端口
	if (err == SOCKET_ERROR)
	{
		closesocket(stationSocket);
		return FALSE;
	}
	listen(stationSocket, 5);//开启监听

	return 1;
}

int WorkStation::InitDistributeSocket()
{
	//connect的话不需要创建
	//CreateDistributeSocket();
	HANDLE hThread = CreateThread(NULL, 0, WorkStation::connectDistributeThread, this, 0, NULL);
	return 0;
}
int WorkStation::CreateDistributeSocket()
{
	//加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	wVersionRequested = MAKEWORD(1, 1);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return -1;
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return -1;
	}

	return 1;
}

void WorkStation::NoticeDistributeServer()
{
	unsigned long long file_size = 0;

	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(this->distributeIP.c_str());
	addrSrv.sin_port = htons(this->distributePort);
	addrSrv.sin_family = AF_INET;
	//::connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	//recv(sockClient,(char*)&file_size,sizeof(unsigned long long)+1,NULL);

	if (!::connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)))
	{


		char sendBuffer[1024];
		char recvBuffer[1024];
		//获取文件信息
		SendToDistribute param;
		param.ip = this->ips[0];
		param.fileNum = GetFileList().size();
		param.port = this->port;

		//结构体转成char[]
		memcpy(sendBuffer, &param, sizeof(param));

		//第一次先发送文件信息
		send(sockClient, sendBuffer, 1024, NULL);

		//然后等待确认指令
		recv(sockClient, recvBuffer, 1024, 0);

		//判断信号，fix me
	}

	closesocket(sockClient);//关闭套接字
}


DWORD WorkStation::connectDistributeThread(LPVOID lpParam)
{
	WorkStation* p = (WorkStation*)lpParam;
	p->NoticeDistributeServer();
	return 0;
}

int WorkStation::InitManagerSocket()
{
	//CreateManagerSocket();
	HANDLE hThread = CreateThread(NULL, 0, WorkStation::connectManagerThread, this, 0, NULL);
	return 0;
}

int WorkStation::CreateManagerSocket()
{
	
	return 1;
}

void WorkStation::SendIPAndStatus()
{
	unsigned long long file_size = 0;
	char* ack = "ack";
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(this->managerIP.c_str());
	addrSrv.sin_port = htons(this->managerPort);
	addrSrv.sin_family = AF_INET;
	//::connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	//recv(sockClient,(char*)&file_size,sizeof(unsigned long long)+1,NULL);

	if (!::connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)))
	{


		char sendBuffer[1024];
		char recvBuffer[1024];
		//一直发送状态信息
		while (true)
		{
			//获取状态信息
			Status status;
			status.ip = this->ips[0];
			status.port = this->port;
			status.sending = this->isSending;

			//th
			//结构体转成char[]
			memcpy(sendBuffer, &status, sizeof(status));

			//第一次先发送文件信息
			send(sockClient, sendBuffer, 1024, NULL);

			//然后等待确认指令
			recv(sockClient, recvBuffer, 1024, 0);
		}
		//判断信号，fix me
	}

	closesocket(sockClient);//关闭套接字
}

DWORD WorkStation::connectManagerThread(LPVOID lpParam)
{
	WorkStation* p = (WorkStation*)lpParam;
	p->SendIPAndStatus();
	return 0;
}

void WorkStation::AcceptAnalysis()
{
	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (true)
	{
		this->analysisSocket = accept(stationSocket, (SOCKADDR*)&addrClient, &len);

		if (analysisSocket == INVALID_SOCKET) {
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(analysisSocket);
			WSACleanup();
			return;
		}
		//多客户端的方法就每接收到一个accept就在开启一个线程
		//参数赋值
		SendFile();
	}
}

DWORD WorkStation::AcceptAnalysisThread(LPVOID lpParam)
{
	WorkStation* p = (WorkStation*)lpParam;
	p->AcceptAnalysis();
	return 0;
}

void WorkStation::GetIps()
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

QStringList WorkStation::GetFileList()
{

	//获取文件列表
	auto path=QDir::currentPath();
	path += "/file/";
	QDir dir(path);
	auto fileList=dir.entryList();
	return fileList;
}


WorkStation::~WorkStation()
{
}
