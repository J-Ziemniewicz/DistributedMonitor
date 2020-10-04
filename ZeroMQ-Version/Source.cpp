#include <zmq.h>
#include <iostream>
#include "Monitor.h"



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

	//Creating Monitor
	Monitor monitor = Monitor(port);
	
	std::this_thread::sleep_for(std::chrono::seconds(3));

	//Adding peers and shared object
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

	//Starting thread which receives messages from other peers
	monitor.startReceivingThread();
	
	std::this_thread::sleep_for(std::chrono::seconds(3));
	
	//Try acquire A's Critical Section
	monitor.acquire("A");
	
	std::this_thread::sleep_for(std::chrono::seconds(3));
	
	//Update A's value
	monitor.update("A", std::to_string(port));
	
	//Example with wait and notify
	// first start on port 5555, than on 5556
	
	
	if (port == 5555) {
		//Peer on port 5555 call wait function and release CS
		monitor.wait("A");
	}
	else if (port == 5556) {
		//Peer on port 5556 wait 3s and send NotifyAll to otherPeers notifying peer 5555 that his condition is meet
		std::this_thread::sleep_for(std::chrono::seconds(3));
		monitor.notifyAll("A");
	}
	
	//Release A's Critical Section
	monitor.release("A");
	

	while (1) {
		
	};
	
	return 0;

	

}