#include "FileAnalysis.h"


struct WorkStationInfo {
	std::string ip;
	int port;
};
struct AnalysisStatus {
	std::string ip;
	int port;
	bool analyzing;
	bool receiving;
};
struct FileParam
{
	std::string fileNames[100] = { 0 };
	int fileSize[100] = { 0 };
	int num = 0;
};
FileAnalysis::FileAnalysis()
{
}

void FileAnalysis::Run()
{
}

void FileAnalysis::Init()
{
	GetConfig();
	InitSocket();
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

}

void FileAnalysis::GetConfig()
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
			return;
		}
		//多客户端的方法就每接收到一个accept就在开启一个线程
		GetWorkStationInfo();
	}
}

void FileAnalysis::GetWorkStationInfo()
{
	char recvBuffer[1024];
	WorkStationInfo* info;
	recv(analysisSocket, recvBuffer, 1024, NULL);
	//char[] 转结构体
	info = (WorkStationInfo *)recvBuffer;

	this->workStationIP = info->ip;
	this->workStationPort = info->port;


}

DWORD FileAnalysis::AcceptDistributeThread(LPVOID lpParam)
{
	FileAnalysis* p = (FileAnalysis*)lpParam;
	p->GetWorkStationInfo();
	return 0;
}

void FileAnalysis::GetIps()
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
		QString path = QDir::currentPath() + "/";
		QString logName = path + "FileResult.log";
		QFile log(logName);
		char sendBuffer[1024];
		char recvBuffer[1024];
		char* ack = "ack";

		//chulilog
		log.open(QFile::Append|QFile::WriteOnly);

		//一直发送状态信息

			//第一次先接收文件信息
		FileParam* param;

		recv(sockClient, recvBuffer, 1024, 0);

		param = (FileParam*)recvBuffer;

		if (param != NULL) {
			//确认收到了文件信息
			send(sockClient, ack, sizeof(ack), 0);


			for (int i = 0; i < param->num; i++) {

				auto filename = QString::fromStdString(param->fileNames[i]);
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

					} while ((file_size - dwCountOfBytesRecv) && dwNumberOfBytesRecv > 0);


				}
				else
				{
					std::cout << "get file size failed" << std::endl;
				}
				//接收完毕后通知接收完毕,开始下一个文件
				send(sockClient, ack, sizeof(ack), 0);

				isAnalysing = true;
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
				temp += QString("%1").arg(file_size) + "\n";

				log.write(temp.toLatin1());
				file.close();
			}

		}
		log.close();
	}
	isAnalysing = false;
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

FileAnalysis::~FileAnalysis()
{
}
