#pragma once
#include <string>
#include <vector>
#include <sstream>

enum class MessageType {REQ, RES, REMOVE, NOTIFY, UPDATE, DUMMY};

class Message
{
private:
	long _timeStamp;
	int _senderPort;
	std::string _sharedObjectId;
	std::string _sharedObject;
	MessageType _messageType;
	void _tokenize(std::string const& str, const char delim, std::vector<std::string>& out);

public:
	

	Message(char* recievedMessage);
	Message(int senderPort, std::string sharedObjectId, std::string sharedObject, MessageType messageType, long timeStamp);

	MessageType getMessageType();
	long getMessageTimeStamp();
	int getSenderPort();
	std::string getSharedObjectId();
	std::string getSharedObject();
	std::string serialize();




};

