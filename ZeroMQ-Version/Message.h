#pragma once
#include <string>
#include <vector>
#include <sstream>

enum class MessageType {REQ, RES, REMOVE, NOTIFY, TEST};

class Message
{
private:
	int _messageId;
	long _timeStamp;
	int _senderPort;
	std::string _sharedObject;
	MessageType _messageType;
	void _tokenize(std::string const& str, const char delim, std::vector<std::string>& out);

public:
	

	Message(char* recievedMessage);
	Message(int messageId, int senderPort, std::string sharedObject, MessageType messageType, long timeStamp);


	bool isReq();
	bool isRes();
	bool isRemove();
	bool isNotify();
	bool isTest();

	std::string serialize();
	std::string print_message();




};

