#include "Monitor.h"
#include <zmq.h>
#include <iostream>
#include <cassert>

void Monitor::updateLamportClock( bool ifMax, long receivedClock)
{
	if (ifMax) {
		_lamportClock = std::max(_lamportClock, receivedClock);
	}
	_lamportClock++;
}

long Monitor::getLamportClock()
{
	return _lamportClock;
}

Monitor::Monitor(int port, std::string sharedObject)
{
	_receiveCtx = zmq_ctx_new();
	_sendCtx = zmq_ctx_new();
	_receiveSocket = zmq_socket(_receiveCtx, ZMQ_REP);
	_port = port;
	_sharedObject = sharedObject;
	_lamportClock = 0;
	_waiting = false;
	std::string socketURL = "tcp://127.0.0.1:" + std::to_string(_port);
	int rc = zmq_bind(_receiveSocket, socketURL.c_str());
	assert(rc == 0);
	std::thread receiveThread(&Monitor::receiveMessageThread, this);
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





