#include "WorkStation.h"


struct FileParam
{
	std::string fileNames[100]  ;
	int fileSize[100] ;
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
	Init();
}

void WorkStation::Run()
{
	//开启socket，等待 connect
	InitStationSocket();
	//connect manager send ip
	InitManagerSocket();
	while (true) {
		//通知发送文件
		if (!isSending) {
			InitDistributeSocket();
		}
		Sleep(1000 * 20);
	}
}

void WorkStation::Init()
{
	//测试
	ips.push_back("127.0.0.1");
	//GetIps();
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
	return 1;
}

void WorkStation::GetConfig()
{
	GetIps();
	port = 8989;
	isSending = false;
	auto path= QCoreApplication::applicationDirPath() +"/config.config";
	QFile file(path);
	if (file.exists()) {
		file.open(QFile::ReadOnly | QFile::Text);
		auto portstr=file.readLine();
		port=atoi(portstr);
		file.close();
	}

	//获取管理器的IP和端口
	auto managerPath = QCoreApplication::applicationDirPath() + "/man.config";
	QFile mfile(managerPath);
	if (mfile.exists()) {
		mfile.open(QFile::ReadOnly);
		QTextStream in(&mfile);
		while (!in.atEnd()) {
			QStringList infoList = in.readLine().split(',');
			this->managerIP = infoList[0].toStdString();
			this->managerPort = infoList[1].toInt();
		}
		mfile.close();
	}

	//获取分配器的IP和端口
	auto distributePath = QCoreApplication::applicationDirPath() + "/dis.config";
	QFile dfile(distributePath);
	if (dfile.exists()) {
		dfile.open(QFile::ReadOnly);
		QTextStream in(&dfile);
		while (!in.atEnd()) {
			QStringList infoList = in.readLine().split(',');
			this->distributeIP = infoList[0].toStdString();
			this->distributePort = infoList[1].toInt();
		}
		dfile.close();
	}


}

void WorkStation::SendFile()
{

	char infochar[1024*100];
	char recvBuffer[1024];
	
	//获取列表
	auto fileList = GetFileList();
	//
	auto path = QCoreApplication::applicationDirPath();
	path += "/file/";
	//读取文件信息
	FileParam fileParam;
	int index = 0;
	for each (QString filePath in fileList)
	{
		
		QFileInfo info(path+filePath);
		auto qname = info.fileName().toStdString();
		fileParam.fileNames[index]=qname;
		fileParam.fileSize[index]=info.size();
		index++;

	}
	fileParam.num = index;

	//结构体转成char[]
	memcpy(infochar, &fileParam, sizeof(fileParam));

	//改标志位
	isSending = true;

	//第一次先发送文件信息
	send(analysisSocket, infochar, sizeof(infochar), NULL);

	//可以确保对方接受完了文件信息
	recv(analysisSocket, recvBuffer, 1024, NULL);

	//循环发送文件
	for each(QString filePath in fileList)
	{
		QFile file(path+filePath);
		auto res=file.open(QFile::ReadOnly);

		QDataStream stream(&file);

		char Buffer[1024];
		DWORD dwNumberOfBytesRead;

		//已读的总大小
		long totalBytes = 0;
		int actualSendBytes = 0;
		do
		{
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

		//模拟比较久的发送
		Sleep(100);
	}
	isSending = false;
	
}

int WorkStation::InitStationSocket()
{
	CreateStationSocket();
	HANDLE hThread = CreateThread(NULL, 0, WorkStation::AcceptAnalysisThread, this, 0, NULL);
	return 0;
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

			//发送状态信息
			send(sockClient, sendBuffer, 1024, NULL);

			//然后等待确认指令
			recv(sockClient, recvBuffer, 1024, 0);

			//每1秒发一次
			Sleep(1000);
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
	auto path= QCoreApplication::applicationDirPath();
	path += "/file/";
	QDir dir(path);
	auto fileList=dir.entryList(QDir::Files);
	return fileList;
}


WorkStation::~WorkStation()
{
}
