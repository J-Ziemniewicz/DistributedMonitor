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
	_sharedObject = out[2];
	_messageType = message_type_converter::string_to_message_type(out[3]);
}

Message::Message(int messageId, int senderPort, std::string sharedObject, MessageType messageType, long timeStamp)
{
	_messageId = messageId;
	_senderPort = senderPort;
	_sharedObject = sharedObject;
	_messageType = messageType;
	_timeStamp = timeStamp;
}

std::string Message::serialize()
{
	return std::to_string(_messageId) + ";" + std::to_string(_senderPort) + ";" + _sharedObject + ";" + message_type_converter::message_type_to_string(_messageType);
}

std::string Message::print_message()
{
	return "Message ID: " + std::to_string(_messageId) + "\n" + "Senders port: " + std::to_string(_senderPort) + "\n" + "Shared object: " + _sharedObject + "\n" + "Message body: " + message_type_converter::message_type_to_string(_messageType) + "\n";
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
