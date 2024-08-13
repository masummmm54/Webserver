#include "HttpServer.hpp"

std::string HttpServer::formatHttpResponse(int status_code, const std::string& reasonPhrase,
	const std::string& body)
{
	std::ostringstream response;

	// constructs proper format for HTTP response
	response << "HTTP/1.1 " << status_code << " " << reasonPhrase << "\r\n";
	response << "Content-Length: " << body.size() << "\r\n";
	response << "Content-Type: text/html; charset=UTF-8\r\n";
	response << "Connection: close\r\n";
	response << "\r\n";
	response << body;
	return (response.str());
}

void	HttpServer::closeSocket(int client_socket)
{
	struct kevent change;
	EV_SET(&change, client_socket, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, NULL);
	if (kevent(kq, &change, 1, NULL, 0, NULL) == -1)
		log("ERROR", "Failed to remove read event from kqueue for FD: " + std::to_string(client_socket), NOSTATUS);
	EV_SET(&change, client_socket, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
	if (kevent(kq, &change, 1, NULL, 0, NULL) == -1)
		log("ERROR", "Failed to remove write event from kqueue for FD: " + std::to_string(client_socket), NOSTATUS);
	close(client_socket);
	openSockets.erase(client_socket);
	log("INFO", "Closed client socket FD: " + std::to_string(client_socket), NOSTATUS);
}

void	log(const std::string& level, const std::string& msg, int client_socket)
{
	std::time_t currentTime = std::time(0);
	std::tm* localTime = std::localtime(&currentTime);

	char timestamp[20];
	std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localTime);

	(void)client_socket;
	std::ofstream logFile("log.txt", std::ios_base::app);
	if (logFile.is_open())
	{
		logFile << timestamp << " [" << level << "]" << " - " << msg << std::endl;
		logFile.close();
	}
	else
		std::cerr << "Unable to open log file" << std::endl;
	std::cout << timestamp << " [" << level << "]" << " - " << msg  << std::endl;
	logFile.close();
}

std::string HttpServer::getFilePath(const std::string& uri)
{
	return ("html" + uri);
}
