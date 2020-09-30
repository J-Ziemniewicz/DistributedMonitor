#pragma once
#include "Message.h"
#include <map>
#include <mutex>

class Monitor
{
public:

	
	

	Monitor(int port, std::string sharedObject);
	~Monitor();


	void addPeer(int port);
	void acquire();
	void release();
	void wait();
	void notifyAll();

private:

	int _port;
	void* _receiveCtx;
	void* _sendCtx;
	void* _receiveSocket;
	long _lamportClock;
	bool _waiting;
	std::mutex _mtx;
	std::string _sharedObject;
	std::map<int, void*> _otherPeers; //int is port number, void* is zmq_socket connected to this port
	

	void insertMessageToQueque(Message message);
	void updateLamportClock( bool ifMax, long receivedClock);
	long getLamportClock();
	void bradcastMessage(MessageType messageType);
	void sendMessage(void* socket, Message message);
	void receiveMessageThread();
	void parseMessage(Message message);
	bool canAcquireCS(std::string sharedObject);



};

