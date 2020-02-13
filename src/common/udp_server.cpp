#include <common/udp_server.h>
#include <common/common.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>

#define DATA_THREADS 4
#define BUFFER_SIZE 2048

UdpServer::UdpServer(int port) {
	m_receiveThread = NULL;
	m_callback = NULL;
	m_started = false;
	m_port = port;
	m_dataThreads = NULL;
	m_startedDataThreads = 0;
	
	m_fd = -1;
}

UdpServer::~UdpServer() {
	stop();
}

void UdpServer::fillAddress(const char * ip, int port, sockaddr_in * addr, size_t addrLen) {
	struct hostent * server = gethostbyname(ip);
	
	memset(addr, 0, addrLen);
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	memcpy(&addr->sin_addr.s_addr, server->h_addr, server->h_length);
}

void UdpServer::receiveThread() {
	struct sockaddr_in client;
	socklen_t clientLength = sizeof(client);
	ssize_t n;
	
	char * buffer = new char[BUFFER_SIZE];
	char * ip;
	ReceiveData receiveData;
	while (m_started) {
		n = recvfrom(m_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client, &clientLength);
		if (n <= 0) {
			continue;
		}
		ip = new char[INET_ADDRSTRLEN];
		receiveData.port = client.sin_port;
		receiveData.dataLength = (size_t) n;
		receiveData.ip = inet_ntop(AF_INET, &client.sin_addr, ip, INET_ADDRSTRLEN);
		receiveData.data = new char[(int) n];
		memcpy((char *) receiveData.data, buffer, n);
		
		m_dataQueueMutex.lock();
		m_dataQueue.push(receiveData);
		m_dataQueueConditionVariable.notify_one();
		m_dataQueueMutex.unlock();
	}
	delete [] buffer;
}

void UdpServer::dataThread() {
	{
		std::lock_guard<std::mutex> lock(m_dataQueueMutex);
		m_startedDataThreads++;
	}
	
	void (*callback)(const char *, int, const char *, size_t);
	ReceiveData data;
	while (m_started) {
		{
			// Wait for data or termination
			std::unique_lock<std::mutex> lock(m_dataQueueMutex);
			while (m_dataQueue.empty()) {
				if (!m_started)
					goto end;
				m_dataQueueConditionVariable.wait(lock);
			}
			
			// Grab data
			data = m_dataQueue.front();
			m_dataQueue.pop();
		}
		
		callback = m_callback;
		if (callback != NULL)
			callback(data.ip, data.port, data.data, data.dataLength);
		
		delete [] data.ip;
		delete [] data.data;
	}
	end:
	{
		std::lock_guard<std::mutex> lock(m_dataQueueMutex);
		m_startedDataThreads--;
	}
}

void UdpServer::start() {
	if (m_started)
		return;
	m_started = true;
	
	m_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	int optval = 1;
	setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
	
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short) m_port);
	
	if (bind(m_fd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) 
		perror("bind");
	
	m_dataThreads = new std::thread*[DATA_THREADS];
	for (int i = 0; i < DATA_THREADS; i++) {
		m_dataThreads[i] = new std::thread(&UdpServer::dataThread, this);
	}
	m_receiveThread = new std::thread(&UdpServer::receiveThread, this);
	while (m_startedDataThreads < DATA_THREADS) {
		usleep(100);
	}
}

void UdpServer::stop() {
	if (!m_started)
		return;
	m_started = false;
	
	shutdown(m_fd, SHUT_RDWR);
	
	m_receiveThread->join();
	delete m_receiveThread;
	
	m_dataQueueConditionVariable.notify_all();
	for (int i = 0; i < DATA_THREADS; i++) {
		m_dataThreads[i]->join();
		delete m_dataThreads[i];
	}
	delete [] m_dataThreads;
	m_fd = -1;
}

bool UdpServer::send(const char * ip, int port, const char * data, size_t length) {
	struct sockaddr_in server;
	fillAddress(ip, port, &server, sizeof(server));
	
	ssize_t n = sendto(m_fd, data, length, 0, (struct sockaddr*) &server, sizeof(server));
	if (n < 0) {
		perror("sendto");
		return false;
	}
	return true;
}

void UdpServer::setCallback(void (*callback)(const char *, int, const char *, size_t)) {
	m_callback = callback;
}

