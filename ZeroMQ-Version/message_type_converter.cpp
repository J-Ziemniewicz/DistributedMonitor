#include "message_type_converter.h"

std::string message_type_converter::message_type_to_string(MessageType messageType)
{
	std::string stringMessageType;
	if (messageType == MessageType::REQ) {
		stringMessageType = "REQ";
	}
	else if (messageType == MessageType::RES) {
		stringMessageType = "RES";
	}
	else if (messageType == MessageType::REMOVE) {
		stringMessageType = "REMOVE";
	}
	else if (messageType == MessageType::NOTIFY) {
		stringMessageType = "NOTIFY";
	}
	else if (messageType == MessageType::UPDATE) {
		stringMessageType = "UPDATE";
	}	
	return stringMessageType;
}

MessageType message_type_converter::string_to_message_type(std::string stringMessageType)
{
	if (stringMessageType == "REQ") {
		return MessageType::REQ;
	}
	else if (stringMessageType == "RES") {
		return MessageType::RES;
	}
	else if (stringMessageType == "REMOVE") {
		return MessageType::REMOVE;
	}
	else if (stringMessageType == "NOTIFY") {
		return MessageType::NOTIFY;
	}
	else if (stringMessageType == "UPDATE") {
		return MessageType::UPDATE;
	}
	return MessageType();
}
