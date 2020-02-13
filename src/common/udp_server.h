#pragma once
#include <netinet/in.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

class UdpServer {
	private:
	struct ReceiveData {
		const char * ip;
		const char * data;
		int port;
		size_t dataLength;
	};
	
	std::thread * m_receiveThread;
	std::thread ** m_dataThreads;
	std::queue<ReceiveData> m_dataQueue;
	std::mutex m_dataQueueMutex;
	std::condition_variable m_dataQueueConditionVariable;
	void (*m_callback)(const char *, int, const char *, size_t);
	int m_startedDataThreads;
	bool m_started;
	int m_port;
	int m_fd;
	
	void fillAddress(const char * ip, int port, sockaddr_in * addr, size_t addrLen);
	void receiveThread();
	void dataThread();
	void onDataReceived();
	
	public:
	UdpServer(int port);
	~UdpServer();
	
	void start();
	void stop();
	
	bool send(const char * ip, int port, const char * data, size_t length);
	/** callback(ip, port, data, length) */
	void setCallback(void (*callback)(const char *, int, const char *, size_t));
	
};

