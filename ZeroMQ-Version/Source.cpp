#include <zmq.h>
#include <iostream>

void updateLamportClock(long& lamportClock, bool ifMax, long receivedClock)
{
	if (ifMax) {
		lamportClock = std::max(lamportClock, receivedClock);
	}
	lamportClock++;
}


int main()
{
	int major = 0;
	int minor = 0;
	int patch = 0;
	zmq_version(&major, &minor, &patch);
	std::wcout << "Current 0MQ version is " << major << '.' << minor << '.' << patch << '\n';
	
	long lamport;
	lamport = 0;

	updateLamportClock(lamport, false, 3);
	std::wcout << lamport << '\n';

}