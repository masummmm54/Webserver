#include "HttpServer.hpp"

bool HttpServer::validateServer(int client_socket)
{
	auto serverIt = servers.find(clientInfoMap[client_socket].server_fd);
	if (serverIt == servers.end())
	{
		sendErrorResponse(client_socket, 500, "Internal Server Error");
		return (false);
	}
	return (true);
}

bool HttpServer::validateRouteAndMethod(int client_socket, const HttpRequest &request)
{
	auto serverIt = servers.find(clientInfoMap[client_socket].server_fd);
	const serverInfo &srv = serverIt->second->getServerInfo();
	auto routeIt = srv.routes.find(request.uri);

	if (routeIt != srv.routes.end())
	{
		const routeConfig &route = routeIt->second;
		if (std::find(route.allowedMethods.begin(), route.allowedMethods.end(), request.method) == route.allowedMethods.end())
		{
			sendErrorResponse(client_socket, 405, "Method Not Allowed");
			return (false);
		}
		if (!route.redirect.empty())
		{
			sendRedirectResponse(client_socket, route.redirect);
			return (false);
		}
		if (route.directoryListing)
		{
			std::string fullPath = "." + route.rootDirectory + request.uri;
			if (isDirectory(fullPath))
			{
				handleDirectoryListing(client_socket, fullPath);
				return (false);
			}
			else
			{
				sendErrorResponse(client_socket, 403, "Forbidden");
				return (false);
			}
		}
	}
	return (true);
}

void HttpServer::decideConnectionPersistence(int client_socket, const HttpRequest &request)
{
	std::string connectionValue = "close"; // Default to close

	auto header = request.headers.find("connection");
	if (header != request.headers.end())
	{
		connectionValue = header->second;
		trim(connectionValue);
		std::transform(connectionValue.begin(), connectionValue.end(), connectionValue.begin(), ::tolower);
	}

	bool keepAlive = (connectionValue == "keep-alive");
	clientInfoMap[client_socket].shouldclose = !keepAlive;

}


void HttpServer::processRequestMethod(int client_socket)
{
	HttpRequest &request = clientInfoMap[client_socket].request;
	if (request.method == "GET")
		handleGetRequest(client_socket);
	else if (request.method == "POST")
		handlePostRequest(client_socket);
	else
		sendErrorResponse(client_socket, 501, "Not Implemented");
}

void HttpServer::handleRequest(int client_socket)
{
	if (!validateServer(client_socket))
		return;
	HttpRequest &request = clientInfoMap[client_socket].request;
	if (!validateRouteAndMethod(client_socket, request))
		return;
	decideConnectionPersistence(client_socket, request);
	processRequestMethod(client_socket);
}

void HttpServer::sendRedirectResponse(int client_socket, const std::string &redirectUrl)
{
	// Ensure the redirect URL is a full URL including the protocol
	std::string fullRedirectUrl = redirectUrl;
	if (fullRedirectUrl.find("http://") != 0 && fullRedirectUrl.find("https://") != 0)
	{
		fullRedirectUrl = "http://" + fullRedirectUrl; // Use https:// if appropriate
	}

	std::string httpVersion = clientInfoMap[client_socket].request.version;
	if (httpVersion.empty())
	{
		httpVersion = "HTTP/1.1";
	}

	std::string connectionHeader = clientInfoMap[client_socket].shouldclose ? "Connection: close\r\n" : "Connection: keep-alive\r\n";

	std::string htmlContent =
		httpVersion + " 302 Found\r\n"
					  "Location: " +
		fullRedirectUrl + "\r\n" + connectionHeader +
		"\r\n";
	//clientInfoMap[client_socket].response = htmlContent;
	clientResponse[client_socket] = htmlContent;

	deregisterReadEvent(client_socket);
	registerWriteEvent(client_socket);
}
