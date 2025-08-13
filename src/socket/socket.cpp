
#include "socket.hpp"

Socket::Socket(){ create(AF_INET, SOCK_STREAM, 0); }

Socket::Socket(int domain, int type, int protocol){ create(domain, type, protocol); }

Socket::~Socket(){}

void Socket::create(int domain, int type, int protocol){

	int _fd = socket(domain, type, protocol);
	if(socket < 0)
		std::cout << "socket creation error" << std::endl;

	// _is_created = true;
}

void Socket::setReuseAddr(bool enable){

	setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	std::cout << "Set SO_REUSEADDR option on FD " << _fd << std::endl;
}


void Socket::binding(const std::string& ip, int port){

	std::memset(&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
	_address.sin_port = htons(port);
	_address.sin_addr.s_addr = INADDR_ANY; // or parse ip parameter

	if (bind(_fd, (sockaddr*)&_address, sizeof(_address)) < 0) {
		std::cerr << "Bind failed.\n";
	close(_fd);
	}
	std::cout << "Bound socket FD " << _fd << " to port " << PORT << std::endl;
}

void Socket::listening(int backlog){

	 if (listen(_fd, MAX_CON_BACKLOG) < 0) {
		std::cerr << "Listen failed.\n";
		close(_fd);
	}
	std::cout << "Socket FD " << _fd << " is now listening (backlog: 10)" << std::endl;

}

int Socket::accepting(){

	sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	int client_fd = accept(_fd, (sockaddr*)&client_addr, &client_len);
	if (client_fd < 0)
		std::cout << "Client accept Error" << std::endl;
	return client_fd;
}


void Socket::setNonBlocking(void){

	std::cout << "Making socket FD " << _fd << " non-blocking" << std::endl;
	int flags = fcntl(_fd, F_GETFL, 0);
	fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
}

void Socket::closing(int _fd){
	if (_fd >= 0) {
		::close(_fd);  // Call standard close()
		_fd = -1;
		// _is_bound = false;
		// _is_listening = false;
		// _is_created = false;
	}
}


int Socket::getFd() const {

	return _fd;
}
