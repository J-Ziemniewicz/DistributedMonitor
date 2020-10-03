#include "Monitor.h"
#include "message_type_converter.h"
#include <zmq.h>
#include <iostream>
#include <cassert>
#include <algorithm>


void Monitor::_insertMessageToReqQueque(Message message, std::string sharedObjectId)
{
	_requestQueue[sharedObjectId].push_back(message);
	//if (_requestQueue[sharedObjectId].size() > 1) {
		//std::cout << "Sorting Request queue" << std::endl;
		//std::sort(_requestQueue[sharedObjectId].begin(), _requestQueue[sharedObjectId].end(), _compareMessages);
	//}
}

bool Monitor::_compareMessages(Message msg1, Message msg2) {
	int msg1TS = msg1.getMessageTimeStamp();
	int msg2TS = msg2.getMessageTimeStamp();
	if (msg1TS < msg2TS) {
		return true;
	}
	else if (msg1TS == msg2TS) {
		if (msg1.getSenderPort() < msg2.getSenderPort()) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}

void Monitor::_updateLamportClock( bool ifMax, long receivedClock)
{
	if (ifMax) {
		_lamportClock = std::max(_lamportClock, receivedClock);
	}
	_lamportClock++;
}

long Monitor::_getLamportClock()
{
	return _lamportClock;
}

void Monitor::_broadcastMessage(MessageType messageType, std::string sharedObjectId,std::string sharedObject,std::vector<int> skipPeers)
{
	
	_updateLamportClock(false, 0);
	Message newMessage = Message(0, _port, sharedObjectId, "", messageType, _lamportClock);

	std::vector<int> updatedPeerSet;
	std::set_difference(_peersPorts.begin(), _peersPorts.end(), skipPeers.begin(), skipPeers.end(), std::inserter(updatedPeerSet, updatedPeerSet.begin()));

	if (message_type_converter::message_type_to_string(messageType) == "REQ") {
		_myRequests.push_back(sharedObjectId);
		_insertMessageToReqQueque(newMessage,sharedObjectId);
		std::cout << "Send REQ" << std::endl;
	}
	
	
	for (std::vector<int>::iterator it = updatedPeerSet.begin(); it != updatedPeerSet.end(); ++it) { // send the messages to all other peers
		char* recvBuffer = new char[255];
		memset(recvBuffer, 0, sizeof(recvBuffer));
		zmq_send(_otherPeers[*it], static_cast<void*>(&newMessage.serialize()),1000,0);
		std::cout << "Sending message to.. " << std::to_string(*it) << std::endl;
		zmq_recv(_otherPeers[*it], recvBuffer, sizeof(recvBuffer), 0); //Dummy recv forced by REQ-REP Model
		std::cout << "Received dummy msg..." << std::endl;
		//Message message = Message(recvBuffer);
		//_parseMessage(message);
	}
}

void Monitor::_receiveMessageThread()
{
	std::cout << "Second thread for incomming messages running..." << std::endl;
	char* recvBuffer = new char[1000];
	while (1) {
		memset(recvBuffer, 0, sizeof(recvBuffer));
		if (zmq_recv(_receiveSocket, recvBuffer, sizeof(recvBuffer), 0) != -1) {
			//Message dummyResponse = Message(0, _port, "", "", MessageType::DUMMY, -1);
			//zmq_send(_receiveSocket, dummyResponse.serialize().c_str(), sizeof(dummyResponse.serialize().c_str()), 0);
			
			_mtx.lock();
			Message receivedMessage = Message(recvBuffer);
			Message dummyResponse = Message(0, _port, "", "", MessageType::DUMMY, -1);
			zmq_send(_receiveSocket, dummyResponse.serialize().c_str(), sizeof(dummyResponse.serialize().c_str()), 0);
			std::cout << "Received msg from" << message_type_converter::message_type_to_string(receivedMessage.getMessageType()) << std::endl;
			_parseMessage(receivedMessage);
			_mtx.unlock();
		}
	}
}

void Monitor::_parseMessage(Message message)
{	
	MessageType recvMessageType = message.getMessageType();
	_updateLamportClock(true, message.getMessageTimeStamp());
	switch (recvMessageType) {
	case MessageType::REQ: {
		_insertMessageToReqQueque(message, message.getSharedObjectId());
		
		break; }
	case MessageType::RES: {
		std::string sharedObject = message.getSharedObjectId();
		if (std::find(_myRequests.begin(), _myRequests.end(), sharedObject) != _myRequests.end()) {
			if (std::find(_replyMessagess[sharedObject].begin(), _replyMessagess[sharedObject].end(), message.getSenderPort()) == _replyMessagess[sharedObject].end()) {
				_replyMessagess[sharedObject].push_back(message.getSenderPort());
			}
		}
		break; }
	case MessageType::REMOVE: {
		std::string sharedObject = message.getSharedObjectId();
		int recvPort = message.getSenderPort();
		if (_requestQueue.count(sharedObject) > 0) {
			for (std::vector<int>::size_type i = 0; i != _requestQueue.size(); i++) {
				if (_requestQueue[sharedObject][i].getSenderPort() == recvPort) {
					_requestQueue[sharedObject].erase(_requestQueue[sharedObject].begin() + i);
				}
			}
		}
		break; }
	case MessageType::UPDATE:
	{
		std::string sharedObjectId = message.getSharedObjectId();
		_sharedObjectMap[sharedObjectId] = message.getSharedObject();
		break;
	}
	case MessageType::NOTIFY:
		_sharedObjectWaitingMap[message.getSharedObjectId()] = false;
		std::string sharedObjectId = message.getSharedObjectId();
		_sharedObjectMap[sharedObjectId] = message.getSharedObject();
		break;
	}
}

bool Monitor::_canAcquireCS(std::string sharedObject)
{
	_mtx.lock();
	if (_replyMessagess[sharedObject].size() == _otherPeers.size()) {
		std::cout << "Received all needed replies" << std::endl;
		_mtx.unlock();
		return true;
	}
	std::cout << "Received " + std::to_string(_replyMessagess[sharedObject].size()) + " replies from " + std::to_string(_otherPeers.size())<<std::endl;
	_mtx.unlock();
	return false;
}



Monitor::Monitor(int port)
{
	_receiveCtx = zmq_ctx_new();
	_sendCtx = zmq_ctx_new();
	_receiveSocket = zmq_socket(_receiveCtx, ZMQ_REP);
	_port = port;
	_lamportClock = 0;
	std::string socketURL = "tcp://127.0.0.1:" + std::to_string(_port);
	int rc = zmq_bind(_receiveSocket, socketURL.c_str());
	assert(rc == 0);
	std::thread receiveThread(&Monitor::_receiveMessageThread, this);
	receiveThread.detach();
}

Monitor::~Monitor()
{
	zmq_close(_receiveSocket);
	for (std::map<int, void*>::iterator it = _otherPeers.begin(); it != _otherPeers.end(); ++it) {
		zmq_close(it->second);
	}
	zmq_ctx_destroy(_receiveCtx);
}

void Monitor::addPeer(int port)
{
	void* sendSocket = zmq_socket(_sendCtx, ZMQ_REQ);
	std::string socketURL = "tcp://127.0.0.1:" + std::to_string(port);
	int rc = zmq_connect(sendSocket, socketURL.c_str());
	if (rc == 0) {
		_otherPeers[port] = sendSocket;
		_peersPorts.push_back(port);
		std::cout << "Successfully connected to port " << std::to_string(port) << '\n';
	}
	else {
		std::cout << "Connection to port " << std::to_string(port) << " FAILED\n";
	}
}

void Monitor::addSharedObject(std::string sharedObjectId,std::string serializedObject) //object is serialized to string
{
	if (_sharedObjectMap.count(sharedObjectId) == 0) {
		_sharedObjectMap[sharedObjectId] = serializedObject;
		_requestQueue[sharedObjectId] = std::vector<Message>();
		_replyMessagess[sharedObjectId] = std::vector<int>();
		_sharedObjectWaitingMap[sharedObjectId] = false;

	}
	else {
		std::cout << "Shared Object already exist in shared map" << std::endl;
	}
}

void Monitor::acquire(std::string sharedObjectId)
{
	std::cout << "Try aquire object " << sharedObjectId << std::endl;
	_mtx.lock();
	_broadcastMessage(MessageType::REQ, sharedObjectId, _sharedObjectMap[sharedObjectId],std::vector<int>() );
	_mtx.unlock();
	while (!_canAcquireCS(sharedObjectId)) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << "Object " << sharedObjectId << " is aquired..." << std::endl;
}

void Monitor::update(std::string sharedObjectId, std::string sharedObject) {
	if (_sharedObjectMap.count(sharedObjectId) > 0) {
		_sharedObjectMap[sharedObjectId] = sharedObject;
		std::cout << "Object " << sharedObjectId << " is updated..." << std::endl;
	}
	else {
		std::cout << "Object " << sharedObjectId << " is not shared object. Please add it to shared objects..." << std::endl;
	}
}

void Monitor::release(std::string sharedObjectId)
{
	_mtx.lock();
	auto it = std::find(_myRequests.begin(), _myRequests.end(), sharedObjectId);
	if (it != _myRequests.end())
		_myRequests.erase(it);
	
	_requestQueue[sharedObjectId].erase(_requestQueue[sharedObjectId].begin()); //usuwanie pierwszego elementu z kolejki REQ do wspoldzielonego obiektu
	std::vector<int> skipPeers = std::vector<int>();
	_updateLamportClock(false, 0);
	if (_requestQueue[sharedObjectId].size()>0) {
		int nextPeerInQueue = _requestQueue[sharedObjectId].front().getSenderPort();
		skipPeers.push_back(nextPeerInQueue);
		Message replyMessage = Message(0, _port, sharedObjectId, _sharedObjectMap[sharedObjectId], MessageType::RES, _lamportClock);
		zmq_send(_otherPeers[nextPeerInQueue], replyMessage.serialize().c_str(), sizeof(replyMessage.serialize().c_str()), 0);
	}
	// broadcast shared object updated value
	_broadcastMessage(MessageType::UPDATE, sharedObjectId, _sharedObjectMap[sharedObjectId], skipPeers);
	_requestQueue[sharedObjectId].clear();
	_replyMessagess[sharedObjectId].clear();
	std::cout << "Object " << sharedObjectId << " is released..." << std::endl;
	_mtx.unlock();

}

void Monitor::wait(std::string sharedObjectId)
{
	_sharedObjectWaitingMap[sharedObjectId] = true;
	release(sharedObjectId);
	std::cout << "Waiting for satisfying the condition... ";
	while (_sharedObjectWaitingMap[sharedObjectId]);
	acquire(sharedObjectId);
}

void Monitor::notifyAll(std::string sharedObjectId)
{
	_broadcastMessage(MessageType::NOTIFY, sharedObjectId, _sharedObjectMap[sharedObjectId], std::vector<int>());
}






