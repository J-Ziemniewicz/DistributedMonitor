#pragma once
#include <string>
#include <vector>
#include <sstream>

enum class MessageType {REQ, RES, REMOVE, NOTIFY, TEST, UPDATE, DUMMY};

class Message
{
private:
	//mozna by zaimplementowac id programu
	int _messageId;
	long _timeStamp;
	int _senderPort;
	std::string _sharedObjectId;
	std::string _sharedObject;
	MessageType _messageType;
	void _tokenize(std::string const& str, const char delim, std::vector<std::string>& out);

public:
	

	Message(char* recievedMessage);
	Message(int messageId, int senderPort, std::string sharedObjectId, std::string sharedObject, MessageType messageType, long timeStamp);

	MessageType getMessageType();
	long getMessageTimeStamp();
	int getSenderPort();
	std::string getSharedObjectId();
	std::string getSharedObject();
	int getMessageId();
	
	//do usuniecia
	bool isReq();
	bool isRes();
	bool isRemove();
	bool isNotify();
	bool isTest();
	//

	std::string serialize();
	std::string print_message();




};

