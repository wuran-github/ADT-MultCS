#include "FileAnalysis.h"


struct WorkStationInfo {
	int port;
	std::string ip;
	int fileNum;
};
struct AnalysisStatus {
	std::string ip;
	int port;
	bool analyzing;
	bool receiving;
};
struct FileParam
{
	std::string fileNames[100];
	int fileSize[100] ;
	int num = 0;
};
FileAnalysis::FileAnalysis()
{
	Init();
}

void FileAnalysis::Run()
{
	//创建socket 等待distribute connect 发配任务
	InitAnalysisSocket();
	//connect manager send ip
	InitManagerSocket();

	//connect distribute send analysing status
	InitDistributeSocket();
	//防止被关掉
	while (true)
	{
		
	}

}

void FileAnalysis::Init()
{
	hMutex = CreateMutexA(NULL, FALSE, "hMutex");
	fileMutex= CreateMutexA(NULL, FALSE, "File Mutex");
	GetIps();
	GetConfig();
	InitSocket();
	for (int i = 0; i < threadNum; i++) {
		CreateAnalyzeThread();
	}
}

int FileAnalysis::InitSocket()
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
	return 1;
}

void FileAnalysis::GetConfig()
{
	GetIps();
	isReceiving = false;
	isAnalysing = false;
	auto temp=QCoreApplication::applicationDirPath();
	auto path = temp + "/config.config";
	QFile file(path);
	if (file.exists()) {
		file.open(QFile::ReadOnly | QFile::Text);
		auto portstr = file.readLine();
		this->port = atoi(portstr);
		file.close();
	}

	//获取管理器的IP和端口
	auto managerPath= temp + "/man.config";
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
}

int FileAnalysis::InitAnalysisSocket()
{
	CreateAnalysisSocket();
	HANDLE hThread = CreateThread(NULL, 0, FileAnalysis::AcceptDistributeThread, this, 0, NULL);
	return 0;
}

int FileAnalysis::CreateAnalysisSocket()
{
	int err;
	//创建套接字
	//SOCKET m_socket=socket(AF_INET,SOCK_STREAM,0);
	analysisSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (analysisSocket == INVALID_SOCKET)
	{
		std::cout << "analysisSocket Create Failed！" << endl;
		return FALSE;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(this->port);

	err = ::bind(analysisSocket, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));    //绑定本地端口
	if (err == SOCKET_ERROR)
	{
		closesocket(analysisSocket);
		return FALSE;
	}
	listen(analysisSocket, 5);//开启监听

	return 1;
}

int FileAnalysis::AcceptDistributeSocket()
{
	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (true)
	{
		this->distributeSocket = accept(this->analysisSocket, (SOCKADDR*)&addrClient, &len);

		if (distributeSocket == INVALID_SOCKET) {
			wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
			closesocket(distributeSocket);
			WSACleanup();
			return -1;
		}
		//多客户端的方法就每接收到一个accept就在开启一个线程
		GetWorkStationInfo();
	}
}

void FileAnalysis::GetWorkStationInfo()
{
	
	char recvBuffer[1024];
	WorkStationInfo* info;
	recv(distributeSocket, recvBuffer, 1024, NULL);
	//char[] 转结构体
	info = (WorkStationInfo *)recvBuffer;

	this->workStationIP = info->ip;
	this->workStationPort = info->port;

	//notice ack
	send(distributeSocket, ack, sizeof(ack), 0);

	//开始接收文件
	InitWorkStationSocket();
}

DWORD FileAnalysis::AcceptDistributeThread(LPVOID lpParam)
{
	FileAnalysis* p = (FileAnalysis*)lpParam;
	p->AcceptDistributeSocket();
	return 0;
}

void FileAnalysis::GetIps()
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

int FileAnalysis::InitWorkStationSocket()
{
	HANDLE hThread = CreateThread(NULL, 0, FileAnalysis::connectStationThread, this, 0, NULL);
	return 0;
}

int FileAnalysis::RecevieFile()
{
	unsigned long long file_size = 0;
	std::string analysisContent;
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(this->workStationIP.c_str());
	addrSrv.sin_port = htons(this->workStationPort);
	addrSrv.sin_family = AF_INET;
	//::connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	//recv(sockClient,(char*)&file_size,sizeof(unsigned long long)+1,NULL);

	if (!::connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)))
	{
		isReceiving = true;
		QString path = QCoreApplication::applicationDirPath() + "/";
		char sendBuffer[1024];
		char recvBuffer[1024];

		char infoBuffer[1024 * 100];
		//chulilog

		//一直发送状态信息

			//第一次先接收文件信息
		FileParam* param;

		int res=recv(sockClient, infoBuffer, 1024*100, 0);

		param = (FileParam*)infoBuffer;

		if (param != NULL) {
			//确认收到了文件信息
			send(sockClient, ack, sizeof(ack), 0);

			totalNum++;

			int receivedNum = 0;
			for (int i = 0; i < param->num; i++) {

				auto filename = QString::number(totalNum) + QString::fromStdString(param->fileNames[i]);
				auto tempPath = path + filename;
				file_size = param->fileSize[i];
				QFile file(tempPath);
				file.open(QIODevice::ReadWrite);

				unsigned short maxvalue = file_size;

				if (file_size > 0)
				{

					DWORD dwNumberOfBytesRecv = 0;
					DWORD dwCountOfBytesRecv = 0;

					char Buffer[1024];

					do
					{
						//一直接收文件
						dwNumberOfBytesRecv = ::recv(sockClient, Buffer, sizeof(Buffer), 0);
						file.write(Buffer, dwNumberOfBytesRecv);
						dwCountOfBytesRecv += dwNumberOfBytesRecv;
						//每接受完一次告诉对方接收完毕
						//send(sockClient, ack, sizeof(ack), 0);

					} while ((file_size - dwCountOfBytesRecv) && dwNumberOfBytesRecv > 0);


				}
				else
				{
					std::cout << "get file size failed" << std::endl;
				}
				this->sizeList.push(file_size);
				this->analysisList.push(filename);
				//接收完毕后通知接收完毕,开始下一个文件
				send(sockClient, ack, sizeof(ack), 0);
				file.close();
				
			}

		}
	}
	isReceiving = false;
	closesocket(sockClient);//关闭套接字


	return 0;
}

DWORD FileAnalysis::connectStationThread(LPVOID lpParam)
{
	FileAnalysis* p = (FileAnalysis*)lpParam;
	p->RecevieFile();
	return 0;
}

int FileAnalysis::InitManagerSocket()
{
	//CreateManagerSocket();
	HANDLE hThread = CreateThread(NULL, 0, FileAnalysis::connectManagerThread, this, 0, NULL);
	return 0;
}

int FileAnalysis::CreateManagerSocket()
{

	return 1;
}

void FileAnalysis::SendIPAndStatus()
{
	unsigned long long file_size = 0;

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
			AnalysisStatus status;
			status.ip = this->ips[0];
			status.port = this->port;
			status.analyzing = this->isAnalysing;
			status.receiving = this->isReceiving;

			//th
			//结构体转成char[]
			memcpy(sendBuffer, &status, sizeof(status));

			//第一次先发送文件信息
			send(sockClient, sendBuffer, 1024, NULL);

			//然后等待确认指令
			recv(sockClient, recvBuffer, 1024, 0);

			//1 sec
			Sleep(1000);
		}
		//判断信号，fix me
	}

	closesocket(sockClient);//关闭套接字
}

DWORD FileAnalysis::connectManagerThread(LPVOID lpParam)
{
	FileAnalysis* p = (FileAnalysis*)lpParam;
	p->SendIPAndStatus();
	return 0;
}

void FileAnalysis::CreateAnalyzeThread()
{
	HANDLE hThread = CreateThread(NULL, 0, FileAnalysis::AnalyzeFileThread, this, 0, NULL);

}

void FileAnalysis::AnalyzeFile()
{
	while (true) {

		if (analysisList.empty()) {
			continue;
		}

		QString path = QCoreApplication::applicationDirPath() + "/";
		QString logName = path + "FileResult.log";
		QFile log(logName);
		//加锁
		WaitForSingleObject(this->hMutex, this->millisec);
		if(!analysisList.empty()) {


			isAnalysing = true;
			//read file name
			QString filename = analysisList.front();
			analysisList.pop();
			int size = sizeList.front();
			sizeList.pop();
			ReleaseMutex(this->hMutex);

			QFile file(path + filename);
			file.open(QFile::ReadOnly);
			//保存文件信息
			SYSTEMTIME st = { 0 };
			GetLocalTime(&st);
			std::string date = std::to_string(st.wYear) + "-" + std::to_string(st.wMonth) + "-" + std::to_string(st.wDay);
			std::string time = std::to_string(st.wHour) + ":" + std::to_string(st.wMinute) + ":" + std::to_string(st.wSecond);

			auto temp = filename + ", ";

			temp += QString::fromStdString(this->workStationIP + ", ");

			std::string tempPort = std::to_string(this->workStationPort) + ", ";

			temp += QString::fromStdString(tempPort);
			temp += QString::fromStdString(date + ", ");
			temp += QString::fromStdString(time + ", ");
			temp += file.read(8);
			temp += QString(" ,%1").arg(size) + "\r\n";

			//加锁
			WaitForSingleObject(this->fileMutex, this->millisec);
			log.open(QFile::Append | QFile::WriteOnly);
			log.write(temp.toLatin1());
			log.close();
			ReleaseMutex(this->fileMutex);
			file.close();

			//删除文件
			if (this->deleteAfter) {
				file.remove();
			}
			//模拟长久的分析时间
			Sleep(200);
		}
		else {
			ReleaseMutex(this->hMutex);
		}
		isAnalysing = false;
	}
}

DWORD FileAnalysis::AnalyzeFileThread(LPVOID lpParam)
{
	FileAnalysis* p = (FileAnalysis*)lpParam;
	p->AnalyzeFile();
	return 0;
}

int FileAnalysis::InitDistributeSocket()
{
	HANDLE hThread = CreateThread(NULL, 0, FileAnalysis::connectDistributeThread, this, 0, NULL);
	return 0;
}



void FileAnalysis::SendAnalyzingFileNum()
{

	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = inet_addr(this->distributeIP.c_str());
	addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	//addrSrv.sin_port = htons(this->distributePort);
	addrSrv.sin_port = htons(8001);
	addrSrv.sin_family = AF_INET;
	//::connect(sockClient,(SOCKADDR*)&addrSrv,sizeof(SOCKADDR));
	//recv(sockClient,(char*)&file_size,sizeof(unsigned long long)+1,NULL);
	while (::connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))) {

	}
	/*if (!::connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)))
	{*/


		char sendBuffer[1024];
		char recvBuffer[1024];
		//一直发送状态信息
		while (true)
		{
			//获取状态信息
			WorkStationInfo info;
			info.ip = this->ips[0];
			info.port = this->port;
			info.fileNum = this->analysisList.size();

			//th
			//结构体转成char[]
			memcpy(sendBuffer, &info, sizeof(info));

			//第一次先发送文件信息
			send(sockClient, sendBuffer, 1024, NULL);

			//然后等待确认指令
			recv(sockClient, recvBuffer, 1024, 0);

			//1 sec
			Sleep(1000* toDistributeSec);
		}
		//判断信号，fix me
	//}

	closesocket(sockClient);//关闭套接字
}

DWORD FileAnalysis::connectDistributeThread(LPVOID lpParam)
{
	FileAnalysis* p = (FileAnalysis*)lpParam;
	p->SendAnalyzingFileNum();
	return 0;
}

FileAnalysis::~FileAnalysis()
{
}
