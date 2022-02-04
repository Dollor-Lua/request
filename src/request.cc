#include "../include/request.hpp"

#ifdef _WIN32
#undef UNICODE
#undef _WIN32_WINNT
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <process.h>

#else

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>

#endif

using namespace std;

function<vector<string>(string, string)> requestCallback;
bool already_served = false;
int clients = 0;

vector<string> split(string s, string delimiter)
{
    size_t pos = 0;
    string token;

    vector<string> parts;
    while ((pos = s.find(delimiter)) != string::npos)
    {
        token = s.substr(0, pos);
        s.erase(0, pos + delimiter.length());
        parts.push_back(token);
    }

    parts.push_back(s);
    return parts;
}

#ifdef _WIN32

DWORD WINAPI ProcessClient(LPVOID lpParam)
{
    SOCKET ClientSocket = (SOCKET)lpParam;

    int iResult;
    int iSendResult;

    int recvbuflen = 10000;
    char recvbuf[10000];

    do
    {
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
        {
            string recieved;

            int bytes = iResult;

            vector<string> ends = split(recvbuf, "\r\n\r\n");

            // Parsing header data
            int content_length = 0;
            string request = "GET";
            string directory = "/";
            string HTTP = "";
            if (ends.size() > 1)
            {
                string headers = ends[0];
                vector<string> recievedVector = split(headers, "\r\n");

                for (int i = 0; i < recievedVector.size(); i++)
                {
                    vector<string> header_value = split(recievedVector.at(i), ": ");
                    if (header_value.size() < 2)
                    {
                        header_value = split(recievedVector.at(i), " ");
                        if (header_value.size() < 3)
                            std::cout << "\tGot invalid header: " << recievedVector.at(i) << endl;
                        else
                        {
                            string method = header_value[0];
                            if (method == "GET" || method == "POST" || method == "PUT" || method == "PATCH" || method == "DELETE" || method == "HEAD" || method == "CONNECT" || method == "OPTIONS" || method == "TRACE")
                            {
                                request = method;
                                directory = header_value[1];
                                HTTP = header_value[2];

                                if (HTTP != "HTTP/1.1" && HTTP != "HTTP/1.0" && HTTP != "HTTP/2.0")
                                    std::cout << "Invalid HTTP Method: " << HTTP << endl;
                            }
                        }
                    }
                    else
                    {
                        if (header_value[0] == "Content-Length")
                            content_length = stoi(header_value[1]);
                    }
                }
            }

            // Getting actual data from request
            if (ends.size() < 2)
                recieved = recvbuf;
            else if (ends.size() >= 2)
                recieved = ends[1].substr(0, content_length);

            vector<string> data = requestCallback(request, directory);
            string responseType = data.size() >= 2 ? data[1] : "text/plain";

            // generate http request
            string resend = "HTTP/1.1 200 OK\r\nHost: 127.0.0.1:3000\r\nUser-Agent: cc_request/1.0\r\nContent-Type: " + responseType + "\r\nContent-Length: ";
            resend += to_string(data[0].length());
            resend += "\r\n\r\n";
            resend += data[0];

            // Send bytes of data
            iSendResult = send(ClientSocket, resend.c_str(), iResult, 0);
            if (iSendResult == SOCKET_ERROR)
            {
                std::cout << "Failed to send data, got WSAERROR of: " << WSAGetLastError() << endl;
                closesocket(ClientSocket);
                break;
            }
        }
        else if (iResult == 0)
        { // std::cout << "Closing client #" << ClientSocket << "; New client count: " << clients - 1 << endl;
        }
        else
        {
            std::cout << "recv failed with error: " << WSAGetLastError() << endl;
            closesocket(ClientSocket);
            break;
        }
    } while (iResult > 0);

    clients--;
    return 0;
}

namespace request
{
    bool serve(string ip, int port, function<vector<string>(string, string)> _requestCallback)
    {
        if (already_served)
        {
            cout << "You cannot re-serve the server." << endl;
            return false;
        }

        already_served = true;
        requestCallback = _requestCallback;

        if (port < 1025)
        {
            std::cout << "Invalid port, got: " << port << endl;
            return false;
        }
        else if (port > 65535)
        {
            std::cout << "Invalid port, got: " << port << endl;
            return false;
        }

        WSADATA wsaData;
        int iResult;

        SOCKET ListenSocket = INVALID_SOCKET;

        struct addrinfo *result = NULL;
        struct addrinfo hints;

        iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0)
        {
            std::cout << "Failed to start server (WSAStartup), got iResult of: " << iResult << endl;
            return false;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;
        hints.ai_flags = AI_PASSIVE;

        PCSTR true_port = to_string(port).c_str();
        iResult = getaddrinfo(NULL, true_port, &hints, &result);
        if (iResult != 0)
        {
            std::cout << "Failed to start server (getaddrinfo), got iResult of: " << iResult << endl;
            WSACleanup();
            return false;
        }

        ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (ListenSocket == INVALID_SOCKET)
        {
            std::cout << "Failed to start server (ListenSocket = INVALID_SOCKET, NO SOCKET), got WSAERROR of: " << WSAGetLastError() << endl;
            freeaddrinfo(result);
            WSACleanup();
            return false;
        }

        iResult = ::bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
        if (iResult == SOCKET_ERROR)
        {
            std::cout << "Failed to start server (BIND FAILURE), got WSAERROR of: " << WSAGetLastError() << endl;
            freeaddrinfo(result);
            closesocket(ListenSocket);
            WSACleanup();
            return false;
        }

        freeaddrinfo(result);

        iResult = listen(ListenSocket, SOMAXCONN);
        if (iResult == SOCKET_ERROR)
        {
            std::cout << "Failed to start server (LISTEN FAILURE), got WSAERROR of: " << WSAGetLastError() << endl;
            closesocket(ListenSocket);
            WSACleanup();
            return false;
        }

        SOCKET ClientSocket;
        while ((ClientSocket = accept(ListenSocket, NULL, NULL)))
        {
            if (ClientSocket == INVALID_SOCKET)
            {
                // std::cout << "Failed to accept client, got WSAERROR of: " << WSAGetLastError() << endl;
                continue;
            }

            DWORD dwThreadId;
            CreateThread(NULL, 0, ProcessClient, (LPVOID)ClientSocket, 0, &dwThreadId);
            clients++;
        }

        closesocket(ListenSocket);

        iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR)
        {
            std::cout << "ClientSocket shutdown failed with error: " << WSAGetLastError() << endl;
            closesocket(ClientSocket);
            WSACleanup();
            return false;
        }

        closesocket(ClientSocket);
        WSACleanup();

        return true;
    }
}

#else

bool doprocessing(int sock)
{
    int n;
    char buffer[10000];
    bzero(buffer, 10000);
    n = read(sock, buffer, 9999);

    if (n < 0)
    {
        perror("ERROR reading from socket");
        return false;
    }

    string recieved;

    int bytes = n;

    vector<string> ends = split(buffer, "\r\n\r\n");

    // Parsing header data
    int content_length = 0;
    string request = "GET";
    string directory = "/";
    string HTTP = "";
    if (ends.size() > 1)
    {
        string headers = ends[0];
        vector<string> recievedVector = split(headers, "\r\n");

        for (int i = 0; i < recievedVector.size(); i++)
        {
            vector<string> header_value = split(recievedVector.at(i), ": ");
            if (header_value.size() < 2)
            {
                header_value = split(recievedVector.at(i), " ");
                if (header_value.size() < 3)
                    std::cout << "\tGot invalid header: " << recievedVector.at(i) << endl;
                else
                {
                    string method = header_value[0];
                    if (method == "GET" || method == "POST" || method == "PUT" || method == "PATCH" || method == "DELETE" || method == "HEAD" || method == "CONNECT" || method == "OPTIONS" || method == "TRACE")
                    {
                        request = method;
                        directory = header_value[1];
                        HTTP = header_value[2];

                        if (HTTP != "HTTP/1.1" && HTTP != "HTTP/1.0" && HTTP != "HTTP/2.0")
                            std::cout << "Invalid HTTP Method: " << HTTP << endl;
                    }
                }
            }
            else
            {
                if (header_value[0] == "Content-Length")
                    content_length = stoi(header_value[1]);
            }
        }
    }

    // Getting actual data from request
    if (ends.size() < 2)
        recieved = buffer;
    else if (ends.size() >= 2)
        recieved = ends[1].substr(0, content_length);

    vector<string> data = requestCallback(request, directory);
    string responseType = data.size() >= 2 ? data[1] : "text/plain";

    // generate http request
    string resend = "HTTP/1.1 200 OK\r\nHost: 127.0.0.1:3000\r\nUser-Agent: cc_request/1.0\r\nContent-Type: " + responseType + "\r\nContent-Length: ";
    resend += to_string(data[0].length());
    resend += "\r\n\r\n";
    resend += data[0];

    printf("Here is the message: %s\n", buffer);
    n = write(sock, "I got your message", 18);

    if (n < 0)
    {
        perror("ERROR writing to socket");
        return false;
    }
}

namespace request
{
    bool serve(string ip, int port, function<vector<string>(string, string)> _requestCallback)
    {
        if (already_served)
        {
            cout << "You cannot re-serve the server." << endl;
            return false;
        }

        already_served = true;
        requestCallback = _requestCallback;

        if (port < 1025)
        {
            std::cout << "Invalid port, got: " << port << endl;
            return false;
        }
        else if (port > 65535)
        {
            std::cout << "Invalid port, got: " << port << endl;
            return false;
        }

        requestCallback = _requestCallback;

        int sockfd, newsockfd, portno, clilen;
        char buffer[256];
        struct sockaddr_in serv_addr, cli_addr;
        int n, pid;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0)
        {
            perror("ERROR opening socket");
            return false;
        }

        bzero((char *)&serv_addr, sizeof(serv_addr));
        portno = port;

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);

        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            perror("ERROR on binding");
            return false;
        }

        listen(sockfd, 5);
        clilen = sizeof(cli_addr);

        while (1)
        {
            newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

            if (newsockfd < 0)
            {
                perror("ERROR on accept");
                return false;
            }

            pid = fork();

            if (pid < 0)
            {
                perror("ERROR on fork");
                return false;
            }

            if (pid == 0)
            {
                close(sockfd);
                doprocessing(newsockfd);
                return true;
            }
            else
                close(newsockfd);
        }
    }
}

#endif