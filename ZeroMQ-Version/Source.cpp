#include <zmq.h>
#include <iostream>
#include "Monitor.h"
#include "Message.h"



int main(int argc, char* argv[])
{

	if (argc < 2) {
		std::cout << "Missing required argument..." << std::endl;
	}

	int major = 0;
	int minor = 0;
	int patch = 0;
	zmq_version(&major, &minor, &patch);
	std::wcout << "Current 0MQ version is " << major << '.' << minor << '.' << patch << '\n';
	std::this_thread::sleep_for(std::chrono::seconds(1));
	int port = atoi(argv[1]);
	if (port == 0) {
		std::cout << "Invalid port number..." << std::endl;
		return 0;
	}
	Monitor monitor = Monitor(port);
	
	std::this_thread::sleep_for(std::chrono::seconds(3));
	switch (port)
	{
	case 5555:
		monitor.addPeer(5556);
		monitor.addSharedObject("A", "test");
		
		break;
	case 5556:
		monitor.addPeer(5555);
		monitor.addSharedObject("A", "test");
		break;
	}
	monitor.startReceivingThread();
	std::this_thread::sleep_for(std::chrono::seconds(3));
	monitor.acquire("A");
	std::this_thread::sleep_for(std::chrono::seconds(3));
	monitor.update("A", std::to_string(port));
	monitor.release("A");
	
	while (1) {
		
	};
	
	return 0;

	

}