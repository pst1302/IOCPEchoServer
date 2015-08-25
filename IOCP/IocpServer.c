#include<stdio.h>
#include<stdlib.h>
#include<process.h>
#include<WinSock2.h>
#include<Windows.h>

#define BUF_SIZE 1024
#define READ 3
#define WRITE 5

// socket info
typedef struct
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
}PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// buffer info
typedef struct
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode;		
}PER_IO_DATA, *LPPER_IO_DATA;

// ������ ���� �Լ� -> �Ϸ�� ��û�� �м��Ͽ� �ش��ϴ� �۾��� �����մϴ�.
DWORD WINAPI EchoThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(char* message);

int main(int argc, char* argv[]) {
	
	// wsaData
	WSADATA wsaData;
	// Completion Port
	HANDLE hComPort;
	// �ý��� ���� ������ �����մϴ�.
	SYSTEM_INFO sysInfo;
	// ���� ���õ� ������ �����մϴ�.
	LPPER_IO_DATA ioInfo;
	// Ŭ���̾�Ʈ ���ϰ� ������ �����ϴ� ����ü �Դϴ�.
	LPPER_HANDLE_DATA handleInfo;

	// ���� �����Դϴ�.
	SOCKET hServSock;
	// ���� �ּҿ� ���õ� ������ �����մϴ�.
	SOCKADDR_IN servAdr;
	int recvBytes, i, flags = 0;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling("WSAStartup error");
	}

	// CompletionPort ����
	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	
	// �ý��� ������ �ҷ��ɴϴ�. ���ÿ� ���μ��� ����ŭ �����带 �����մϴ�.
	GetSystemInfo(&sysInfo);
	for (i = 0; i < sysInfo.dwNumberOfProcessors; i++)
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);

	// ���� ���� ����
	hServSock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(argv[1]);

	// bind
	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("bind error!");

	// listen
	if (listen(hServSock, 5) == SOCKET_ERROR)
		ErrorHandling("Listen error");

	while (1) {
		
		// Ŭ���̾�Ʈ ����
		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);

		// Client Accept�ϰ� handleInfo ����ü�� ����
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);
		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		handleInfo->clntAdr = clntAdr;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		// CompletionPort�� client socket �����ؼ� �Ϸ������� �ѱ⵵�� ��
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		// io���õ� ����ü ���� �� recv ���
		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ;
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf.buf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
	}

	getchar();
	return 0;
}

void ErrorHandling(char* message) {
	fputs(message, stderr);
	fputc('\n', stderr);
	getchar();
	exit(1);
}

// �Ϸ�� ���ϵ��� �˻��ؼ� �ش� �۾��� �´� ���� ����
DWORD WINAPI EchoThreadMain(LPVOID pComPort) {
	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	// ��� ���鼭 IO ����
	while (1) {
		
		// �Ϸ�� IO�� ������
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;

		if (ioInfo->rwMode == READ){
			puts("message received");
			if (bytesTrans == 0)
			{
				closesocket(sock);
				free(handleInfo); free(ioInfo);
				continue;
			}

			// echo
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
		}
		else
		{
			puts("message sent");
			free(ioInfo);
		}
	}
}