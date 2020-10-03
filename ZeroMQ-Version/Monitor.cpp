#include "Monitor.h"
#include "message_type_converter.h"
#include <zmq.h>
#include <iostream>
#include <cassert>
#include <algorithm>


void Monitor::_insertMessageToReqQueque(Message message, std::string sharedObjectId)
{
	_requestQueue[sharedObjectId].push_back(message);
	if (_requestQueue[sharedObjectId].size() != 0) {
		std::sort(_requestQueue[sharedObjectId].begin(), _requestQueue[sharedObjectId].end(), _compareMessages);
	}
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
	_mtx.lock();
	_updateLamportClock(false, 0);
	Message newMessage = Message(0, _port, sharedObjectId, "", messageType, _lamportClock);

	std::vector<int> updatedPeerSet;
	std::set_difference(_peersPorts.begin(), _peersPorts.end(), skipPeers.begin(), skipPeers.end(), std::inserter(updatedPeerSet, updatedPeerSet.begin()));

	if (message_type_converter::message_type_to_string(messageType) == "REQ") {
		_myRequests.push_back(sharedObjectId);
		_insertMessageToReqQueque(newMessage,sharedObjectId);
	}
	char* recvBuffer = new char[255];
	for (std::vector<int>::iterator it = updatedPeerSet.begin(); it != updatedPeerSet.end(); ++it) { // send the messages to all other peers
		
		memset(recvBuffer, 0, sizeof(recvBuffer));
		zmq_send(_otherPeers[*it], newMessage.serialize().c_str(),1000,0);
		zmq_recv(_otherPeers[*it], recvBuffer, sizeof(recvBuffer), 0); //Dummy recv forced by REQ-REP Model
		//Message message = Message(recvBuffer);
		//_parseMessage(message);
	}
	_mtx.unlock();
}

void Monitor::_receiveMessageThread()
{
	std::cout << "Second thread for incomming messages running..." << std::endl;
	char* recvBuffer = new char[1000];
	while (1) {
		memset(recvBuffer, 0, sizeof(recvBuffer));
		if (zmq_recv(_receiveSocket, recvBuffer, sizeof(recvBuffer), 0) != -1) {
			Message dummyResponse = Message(0, _port, "", "", MessageType::DUMMY, -1);
			zmq_send(_receiveSocket, dummyResponse.serialize().c_str(), sizeof(dummyResponse.serialize().c_str()), 0);
			
			_mtx.lock();
			Message receivedMessage = Message(recvBuffer);
			_parseMessage(receivedMessage);
			_mtx.unlock();
		}
		else {
			std::cout << "Error in message receiving..." << std::endl;
		}
	}
	
}

void Monitor::_parseMessage(Message message)
{	
	
	MessageType recvMessageType = message.getMessageType();
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
	}
	case MessageType::NOTIFY:
		_sharedObjectWaitingMap[message.getSharedObjectId()] = false;
	}
}

bool Monitor::_canAcquireCS(std::string sharedObject)
{
	_mtx.lock();
	if (_replyMessagess[sharedObject].size() == _otherPeers.size()) {
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
	_broadcastMessage(MessageType::REQ, sharedObjectId, _sharedObjectMap[sharedObjectId],std::vector<int>() );
	while (_canAcquireCS(sharedObjectId)) {
		std::cout << "Object " << sharedObjectId << " is aquired..." << std::endl;
	}
}

void Monitor::update(std::string sharedObjectId, std::string sharedObject) {
	if (_sharedObjectMap.count(sharedObjectId) > 0) {
		_sharedObjectMap[sharedObjectId] = sharedObject;
		std::cout << "Object " << sharedObjectId << "is updated..." << std::endl;
	}
	else {
		std::cout << "Object " << sharedObjectId << "is not shared object. Please add it to shared objects..." << std::endl;
	}
}

void Monitor::release(std::string sharedObjectId)
{
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






