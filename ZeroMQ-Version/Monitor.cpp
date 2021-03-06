#include "Monitor.h"
#include "message_type_converter.h"
#include <zmq.h>
#include <iostream>
#include <cassert>
#include <algorithm>


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

}

Monitor::~Monitor()
{
	zmq_close(_receiveSocket);
	for (std::map<int, void*>::iterator it = _otherPeers.begin(); it != _otherPeers.end(); ++it) {
		zmq_close(it->second);
	}
	zmq_ctx_destroy(_receiveCtx);
}

void Monitor::startReceivingThread() {
	std::thread receiveThread(&Monitor::_receiveMessageThread, this);
	receiveThread.detach();
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

void Monitor::addSharedObject(std::string sharedObjectId, std::string serializedObject) 
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
	_broadcastMessage(MessageType::REQ, sharedObjectId, _sharedObjectMap[sharedObjectId], std::vector<int>());
	_mtx.unlock();
	while (!_canAcquireCS(sharedObjectId)) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	std::cout << "\nObject " << sharedObjectId << " is aquired..." << std::endl;
}

void Monitor::update(std::string sharedObjectId, std::string sharedObject) {
	if (_sharedObjectMap.count(sharedObjectId) > 0) {
		std::cout << "	" << sharedObjectId << " value before update is " << _sharedObjectMap[sharedObjectId] << std::endl;
		_sharedObjectMap[sharedObjectId] = sharedObject;
		std::cout << "	Object " << sharedObjectId << " is updated..." << std::endl;
		std::cout << "	" << sharedObjectId << " value after update is " << _sharedObjectMap[sharedObjectId] << std::endl;
	}
	else {
		std::cout << "	Object " << sharedObjectId << " is not shared object. Please add it to shared objects..." << std::endl;
	}
}

void Monitor::release(std::string sharedObjectId)
{
	_mtx.lock();

	// Delete sharedObjectId from myRequest vector
	auto it = std::find(_myRequests.begin(), _myRequests.end(), sharedObjectId);
	if (it != _myRequests.end())
		_myRequests.erase(it);

	// Delete request message from requestQueue 
	_requestQueue[sharedObjectId].erase(_requestQueue[sharedObjectId].begin()); 
	std::vector<int> skipPeers = std::vector<int>();
	_updateLamportClock(false, 0);

	// Send reply message to all peers which tries to enter critical section
	if (_requestQueue[sharedObjectId].size() > 0) {
		for (std::vector<Message>::iterator it = _requestQueue[sharedObjectId].begin(); it != _requestQueue[sharedObjectId].end(); ++it) {
			int nextPeerInQueue = it->getSenderPort();
			skipPeers.push_back(nextPeerInQueue);
			char* recvBuffer = new char[255];
			memset(recvBuffer, 0, sizeof(recvBuffer));
			Message replyMessage = Message(_port, sharedObjectId, _sharedObjectMap[sharedObjectId], MessageType::RES, _lamportClock);

			zmq_send(_otherPeers[nextPeerInQueue], replyMessage.serialize().c_str(), 1000, 0);
			zmq_recv(_otherPeers[nextPeerInQueue], recvBuffer, 255, 0);
		}
	}

	// Broadcast updated value of shared object to rest of peer
	_broadcastMessage(MessageType::UPDATE, sharedObjectId, _sharedObjectMap[sharedObjectId], skipPeers);

	// Clear vector with reply messagess
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


//Insert message in request queue and sort request queue by priority
void Monitor::_insertMessageToReqQueque(Message message, std::string sharedObjectId)
{
	_requestQueue[sharedObjectId].push_back(message);
	if (_requestQueue[sharedObjectId].size() > 1) {	
		std::sort(_requestQueue[sharedObjectId].begin(), _requestQueue[sharedObjectId].end(), _compareMessages);
	}
}

//Compare messagess priority
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

//Broadcast messagess
void Monitor::_broadcastMessage(MessageType messageType, std::string sharedObjectId,std::string sharedObject,std::vector<int> skipPeers)
{
	
	_updateLamportClock(false, 0);
	Message newMessage = Message(_port, sharedObjectId, sharedObject, messageType, _lamportClock);

	std::vector<int> updatedPeerSet;
	
	//Check if updatedPeerSet size will be greater than 0 
	if (skipPeers.size() < _peersPorts.size()) {
		std::set_difference(_peersPorts.begin(), _peersPorts.end(), skipPeers.begin(), skipPeers.end(), std::inserter(updatedPeerSet, updatedPeerSet.begin()));
	}
	//Check if broadcasted message is Request to CS
	if (message_type_converter::message_type_to_string(messageType) == "REQ") {
		_myRequests.push_back(sharedObjectId);
		_insertMessageToReqQueque(newMessage,sharedObjectId);
		std::cout << "Send REQ" << std::endl;
	}
	//Broadcast message
	if (updatedPeerSet.size() > 0) {
		for (std::vector<int>::iterator it = updatedPeerSet.begin(); it != updatedPeerSet.end(); ++it) { // send the messages to all other peers
			char* recvBuffer = new char[255];
			memset(recvBuffer, 0, sizeof(recvBuffer));
			zmq_send(_otherPeers[*it], newMessage.serialize().c_str(), 1000, 0);
			std::cout << "Sending " << message_type_converter::message_type_to_string(messageType) << " message to.. " << std::to_string(*it) << std::endl;
			zmq_recv(_otherPeers[*it], recvBuffer, 255, 0); //Dummy recv forced by REQ-REP Model
			std::cout << "Received ack msg..." << std::endl;
		}
	}
	
}

//Handle receiving messages on second thread
void Monitor::_receiveMessageThread()
{
	std::cout << "Second thread for incomming messages running..." << std::endl;
	char* recvBuffer = new char[1000];
	while (1) {
		memset(recvBuffer, 0, 1000);
		if (zmq_recv(_receiveSocket, recvBuffer, 1000, 0) > -1) {
			
			//Dummy response forced by REQ-REP ZeroMQ Model
			Message dummyResponse = Message(_port, "", "", MessageType::DUMMY, -1);
			zmq_send(_receiveSocket, dummyResponse.serialize().c_str(), 255, 0);
			
			_mtx.lock();
			Message receivedMessage = Message(recvBuffer);
			_parseMessage(receivedMessage);
			_mtx.unlock();
		}
	}
}

// Check if incomming Request has higher priority than home peer Request
bool Monitor::_handleReqRep(Message message, std::string sharedObject) {
	if (_requestQueue[sharedObject].size() > 0) {
		for (std::vector<Message>::iterator it = _requestQueue[sharedObject].begin(); it != _requestQueue[sharedObject].end(); ++it) {
			if (it->getSenderPort() == _port) {
				return _compareMessages(message, *it);
			}
		}
		return false;
	}
	return true;
}

// Parse message 
void Monitor::_parseMessage(Message message)
{	
	MessageType recvMessageType = message.getMessageType();
	_updateLamportClock(true, message.getMessageTimeStamp());
	std::string sharedObjectId= message.getSharedObjectId();
	switch (recvMessageType) {
	case MessageType::REQ: {
		if (_handleReqRep(message, sharedObjectId)) {
			char* recvBuffer = new char[255];
			memset(recvBuffer, 0, sizeof(recvBuffer));
			_updateLamportClock(false, 0);
			Message resMsg = Message(_port, sharedObjectId, "", MessageType::RES, _lamportClock);
			zmq_send(_otherPeers[message.getSenderPort()], resMsg.serialize().c_str(), 1000, 0);
			zmq_recv(_otherPeers[message.getSenderPort()], recvBuffer, 255, 0);
		}
		_insertMessageToReqQueque(message, sharedObjectId);
		
		break; }
	case MessageType::RES: {
		
		if (std::find(_myRequests.begin(), _myRequests.end(), sharedObjectId) != _myRequests.end()) {
			if (std::find(_replyMessagess[sharedObjectId].begin(), _replyMessagess[sharedObjectId].end(), message.getSenderPort()) == _replyMessagess[sharedObjectId].end()) {
				_replyMessagess[sharedObjectId].push_back(message.getSenderPort());
				if (message.getSharedObject() != "") {
					_sharedObjectMap[sharedObjectId] = message.getSharedObject();
				}
			}
		}
		int recvPort = message.getSenderPort();
		if (_requestQueue.count(sharedObjectId) > 0) {
			for (std::vector<int>::size_type i = 0; i != _requestQueue.size(); i++) {
				if (_requestQueue[sharedObjectId][i].getSenderPort() == recvPort) {
					_requestQueue[sharedObjectId].erase(_requestQueue[sharedObjectId].begin() + i);
				}
			}
		}

		break; }
	case MessageType::REMOVE: {
		
		int recvPort = message.getSenderPort();
		if (_requestQueue.count(sharedObjectId) > 0) {
			for (std::vector<int>::size_type i = 0; i != _requestQueue.size(); i++) {
				if (_requestQueue[sharedObjectId][i].getSenderPort() == recvPort) {
					_requestQueue[sharedObjectId].erase(_requestQueue[sharedObjectId].begin() + i);
				}
			}
		}
		break; }
	case MessageType::UPDATE:
	{
		_sharedObjectMap[sharedObjectId] = message.getSharedObject();
		int recvPort = message.getSenderPort();
		if (_requestQueue.count(sharedObjectId) > 0) {
			for (std::vector<int>::size_type i = 0; i != _requestQueue.size(); i++) {
				if (_requestQueue[sharedObjectId][i].getSenderPort() == recvPort) {
					_requestQueue[sharedObjectId].erase(_requestQueue[sharedObjectId].begin() + i);
				}
			}
		}
		break;
	}
	case MessageType::NOTIFY:
		_sharedObjectWaitingMap[sharedObjectId] = false;
		std::string sharedObjectId = sharedObjectId;
		_sharedObjectMap[sharedObjectId] = message.getSharedObject();
		break;
	}
}

// Check if Critical Section can by acquired
bool Monitor::_canAcquireCS(std::string sharedObject)
{
	_mtx.lock();
	if (_replyMessagess[sharedObject].size() == _otherPeers.size()) {
		std::cout << "Received all needed replies" << std::endl;
		_mtx.unlock();
		return true;
	}
	std::cout << "Received " + std::to_string(_replyMessagess[sharedObject].size()) + " replies from " + std::to_string(_otherPeers.size())<<std::endl;
	for (int i = 0; i < _replyMessagess[sharedObject].size(); i++) {
		std::cout << std::to_string(_replyMessagess[sharedObject][i]) << " ";
		
	}
	std::cout << std::endl;
	_mtx.unlock();
	return false;
}
















