#include "Message.h"
#include "message_type_converter.h"
#include <iostream>



Message::Message(char* recievedMessage)
{
	std::string message = std::string(recievedMessage);
	const char delim = ';';
	std::vector<std::string> out;
	_tokenize(message, delim, out);
	_senderPort = std::stoi(out[0]);
	_sharedObjectId = out[1];
	_sharedObject = out[2];
	_messageType = message_type_converter::string_to_message_type(out[3]);
	_timeStamp = std::stoi(out[4]);
}

Message::Message(int senderPort,std::string sharedObjectId, std::string sharedObject, MessageType messageType, long timeStamp)
{

	_senderPort = senderPort;
	_sharedObject = sharedObject;
	_messageType = messageType;
	_timeStamp = timeStamp;
	_sharedObjectId = sharedObjectId;
}

void Message::_tokenize(std::string const& str, const char delim, std::vector<std::string>& out)
{
	std::stringstream ss(str);

	std::string s;
	while (std::getline(ss, s, delim)) {
		out.push_back(s);
	}
}

//Serialize message to string
std::string Message::serialize()
{
	return  std::to_string(_senderPort) + ";" + _sharedObjectId + ";" +_sharedObject + ";" + message_type_converter::message_type_to_string(_messageType)+";"+std::to_string(_timeStamp)+";";
}

MessageType Message::getMessageType()
{
	return _messageType;
}

long Message::getMessageTimeStamp()
{
	return _timeStamp;
}

int Message::getSenderPort()
{
	return _senderPort;
}

std::string Message::getSharedObjectId() {
	return _sharedObjectId;
}

std::string Message::getSharedObject()
{
	return _sharedObject;
}


