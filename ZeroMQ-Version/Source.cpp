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

	char  dumMsg[] = "0;5555;A;[tak];REQ;6";
	Message msg = Message(dumMsg);
	
	std::this_thread::sleep_for(std::chrono::seconds(3));
	switch (port)
	{
	case 5555:
		monitor.addPeer(5556);
		monitor.addPeer(5557);

		monitor.addSharedObject("A", "test");
		monitor.acquire("A");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		monitor.update("A", std::to_string(port));
		monitor.release("A");
		break;
	case 5556:
		monitor.addPeer(5555);
		monitor.addPeer(5557);
		monitor.addSharedObject("A", "test");
		while (1) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		break;
	case 5557:
		monitor.addPeer(5555);
		monitor.addPeer(5556);
		monitor.addSharedObject("A", "test");
		while (1);
		break;
	}
	
	
	
	return 0;

	

}