#pragma once;
#include "Message.h"

namespace message_type_converter
{
	std::string message_type_to_string(MessageType messageType);
	MessageType string_to_message_type(std::string stringMessageType);
};

