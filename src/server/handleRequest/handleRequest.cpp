#include "HttpServer.hpp"

void	HttpServer::handleRequest(int client_socket)
{
	HttpRequest& request = clientInfoMap[client_socket].request;

	// decide to keep connection open based on HTTP response
	if (request.headers.find("connection") != request.headers.end())
	{
		std::string connectionValue = request.headers["connection"];
		trim(connectionValue);
		std::transform(connectionValue.begin(), connectionValue.end(), connectionValue.begin(), ::tolower);
		if (connectionValue == "keep-alive")
			clientInfoMap[client_socket].shouldclose = false;
		else
			clientInfoMap[client_socket].shouldclose = true;
	}
	else
		clientInfoMap[client_socket].shouldclose = true;
	if (request.method == "GET")
		handleGetRequest(client_socket);
	else if (request.method == "POST")
		handlePostRequest(client_socket);
	else
		sendErrorResponse(client_socket, 501, "Not Implemented");

	// set up write event for client response
	struct kevent change;
	EV_SET(&change, static_cast<uintptr_t>(client_socket), EVFILT_WRITE, EV_ADD | EV_ENABLE, 0, 0, NULL);

	// validate fd before using
	if (fcntl(client_socket, F_GETFL) != -1)
	{
		if (kevent(kq, &change, 1, NULL, 0, NULL) == -1)
			logger.logMethod("ERROR", "Kevent registration failure for writing: " + std::string(strerror(errno)));
		else
			logger.logMethod("INFO", "Succesfully registered kevent for socket: " + std::to_string(client_socket));
	}
	else
	{
		logger.logMethod ("ERROR", "Attempted to register kevent for invalid FD: " + std::to_string(client_socket));
		closeSocket(client_socket);
	}
}

// first iterator points to beginning of the file
// second used as end marker, correct syntax
