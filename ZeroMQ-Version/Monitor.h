#pragma once
#include "Message.h"
#include <map>
#include <mutex>

class Monitor
{
public:

	
	

	Monitor(int port);
	~Monitor();


	void addPeer(int port);
	void addSharedObject(std::string sharedObjectId,std::string serializedObject);
	void acquire(std::string sharedObjectId);  // to acquire critical section on shared object
	void update(std::string sharedObjectId, std::string sharedObject); // to update object after changing it's value in critical section
	void release(std::string sharedObjectId); // exit critical section 
	void wait(std::string sharedObjectId); // wait for specific condition, release CS
	void notifyAll(std::string sharedObjectId); // notify all peers, that critical section is released

private:

	int _port;
	void* _receiveCtx;
	void* _sendCtx;
	void* _receiveSocket;
	long _lamportClock;
	std::mutex _mtx;
	std::vector<int> _peersPorts;
	std::map<int, void*> _otherPeers; //int is port number, void* is zmq_socket connected to this port
	std::map<std::string, std::string> _sharedObjectMap; //shared Object Id : shared object body serialized to string
	std::map<std::string, bool> _sharedObjectWaitingMap; //shared Object Id : bool if object have flag waiting set
	std::map<std::string, std::vector<Message>> _requestQueue;
	std::map<std::string, std::vector<int>> _replyMessagess;
	std::vector<std::string> _myRequests;

	void _insertMessageToReqQueque(Message message, std::string sharedObjectId);
	static bool _compareMessages(Message msg1, Message msg2);
	void _updateLamportClock( bool ifMax, long receivedClock);
	long _getLamportClock();
	void _broadcastMessage(MessageType messageType,std::string sharedObjectId, std::string sharedObject, std::vector<int> skipPeers);
	void _receiveMessageThread();
	bool _handleReqRep(Message message, std::string sharedObject);
	void _parseMessage(Message message);
	bool _canAcquireCS(std::string sharedObject);



};

