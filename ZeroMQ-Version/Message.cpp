#include "Message.h"
#include "message_type_converter.h"
#include <iostream>

void Message::_tokenize(std::string const& str, const char delim, std::vector<std::string>& out)
{
	std::stringstream ss(str);

	std::string s;
	while (std::getline(ss, s, delim)) {
		out.push_back(s);
	}
}

Message::Message(char* recievedMessage)
{
	std::string message = std::string(recievedMessage);
	const char delim = ';';
	std::vector<std::string> out;
	_tokenize(message, delim, out);
	_messageId = std::stoi(out[0]);
	_senderPort = std::stoi(out[1]);
	_sharedObjectId = out[2];
	_sharedObject = out[3];
	_messageType = message_type_converter::string_to_message_type(out[4]);
	_timeStamp = std::stoi(out[5]);
}

Message::Message(int messageId, int senderPort,std::string sharedObjectId, std::string sharedObject, MessageType messageType, long timeStamp)
{
	_messageId = messageId;
	_senderPort = senderPort;
	_sharedObject = sharedObject;
	_messageType = messageType;
	_timeStamp = timeStamp;
	_sharedObjectId = sharedObjectId;
}

std::string Message::serialize()
{
	return std::to_string(_messageId) + ";" + std::to_string(_senderPort) + ";" + _sharedObjectId + ";" +_sharedObject + ";" + message_type_converter::message_type_to_string(_messageType)+";"+std::to_string(_timeStamp)+";";
}

std::string Message::print_message()
{
	return "Message ID: " + std::to_string(_messageId) + "\nSenders port: " + std::to_string(_senderPort) + "\nShared object Id: " + _sharedObjectId + "\nShared object: " + _sharedObject + "\nMessage type: " + message_type_converter::message_type_to_string(_messageType) + "\n";
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

int Message::getMessageId()
{
	return _messageId;
}

bool Message::isReq()
{
	return Message::_messageType == MessageType::REQ;

}

bool Message::isRes()
{
	return Message::_messageType == MessageType::RES;
}

bool Message::isRemove()
{
	return Message::_messageType == MessageType::REMOVE;
}

bool Message::isNotify()
{
	return Message::_messageType == MessageType::NOTIFY;
}

bool Message::isTest()
{
	return Message::_messageType == MessageType::TEST;
}
