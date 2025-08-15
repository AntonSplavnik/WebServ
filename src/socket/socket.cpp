
#include "socket.hpp"

Socket::Socket():_fd(-1){ }

Socket::~Socket(){}

void Socket::createCustom(int domain, int type, int protocol){

	_fd = socket(domain, type, protocol);
	if(_fd < 0)
		std::cout << "socket creation error" << std::endl;

	// _is_created = true;
}

void Socket::createDefault(){

	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(_fd < 0)
		std::cout << "socket creation error" << std::endl;

	// _is_created = true;
}

void Socket::setReuseAddr(bool enable){

	setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
	std::cout << "Set SO_REUSEADDR option on FD " << _fd << std::endl;
}


void Socket::binding(int port){

	std::memset(&_address, 0, sizeof(_address));
	_address.sin_family = AF_INET;
	_address.sin_port = htons(port);
 	_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(_fd, (sockaddr*)&_address, sizeof(_address)) < 0) {
		std::cerr << "Bind failed.\n";
	close(_fd);
	}
	std::cout << "Bound socket FD " << _fd << " to port " << PORT << std::endl;
}

void Socket::listening(){

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
