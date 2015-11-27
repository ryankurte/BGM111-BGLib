
#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "uart.hpp"

using namespace std;

volatile int running = 1;

void sig_handler(int signo)
{
    running = 0;
}

int main(int argc, char** argv) {
	
	Serial s;
	int res;

	std::cout << "bgm111 utility" << std::endl;

	if(argc != 3) exit(1);

	signal(SIGINT, sig_handler);

	std::cout << "Connecting to: " << argv[1] << " at " << atoi(argv[2]) << " baud" << std::endl;

	res = s.Connect(argv[1], atoi(argv[2]));
	if(res < 0) exit(2);

	std::cout << "Connected" << std::endl;

	while(running) {	
		if(s.Available() > 0) {
			cout << s.Get();
		}
	}

	std::cout << "Exiting" << std::endl;

	s.Disconnect();

	return 0;
}
