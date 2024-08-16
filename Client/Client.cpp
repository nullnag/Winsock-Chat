// Client.cpp: определяет точку входа для приложения.
//

#include "Client.h"
#include <iostream>
#include <string>
#include <winsock2.h>

#pragma comment(lib, "Ws2_32.lib")
#define PORT 8080


int main()
{
	WSADATA wsaData;
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in serverAddr;
	char buffer[1024] = { 0 };
	char message[1024] = {0};
	int result;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Error initialize WinSock\n";
		return 1;
	}

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Error create  socket\n";
		WSACleanup();
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(PORT);

	if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Error connect to server\n";
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	char option = ' ';
	std::string username;
	std::string password;
	std::string response;
	bool pass = false;

	while (true) {
		if (pass == true) {
			break;
		}
		std::cout << "Registy - R\n";
		std::cout << "Log In - L\n";
		std::cout << "Choose option: ";
		std::cin >> option;
		switch (option)
		{
		case 'R':
			system("cls");
			std::cout << "Exit - E\n";
			while (true) {
				std::cout << "Choose your username: ";
				std::cin >> username;
				if (username == "E") {
					break;
				}
				if (username.length() > 6 && username.length() < 16) {
					break;
				}
				std::cout << "ERROR: Invalid Login\n";
			}
			if (username == "E") {
				break;
			}
			while (true) {
				std::cout << "\nChoose your password: ";
				std::cin >> password;
				if (password.length() > 8) {
					break;
				}
				std::cout << "password less than 8 symbols\n";
			}
			response = "R|" + username + "|" + password + "|";
			std::strcpy(message, response.c_str());
			send(sock, message, strlen(message), 0);
			result = recv(sock, buffer, sizeof(buffer), 0);
			if (buffer[0] == 'U') {
				std::cout << "ERROR: User exists\n";
				continue;
			}
			else {
				std::cout << "Success\n";
				continue;
			}
		case 'L':
			system("cls");
			std::cout << "Exit - E\n";
			while (true) {
				std::cout << "Username: ";
				std::cin >> username;
				if (username == "E") {
					break;
				}
				std::cout << "Password: ";
				std::cin >> password;
				response = "L|" + username + "|" + password + "|";
				std::strcpy(message, response.c_str());
				send(sock, message, strlen(message), 0);
				result = recv(sock, buffer, sizeof(buffer), 0);
				if (buffer[0] == 'S') {
					std::cout << "Success\n";
					pass = true;
					break;
				}
				else {
					std::cout << "ERROR: Invalid Login or password\n";
					continue;
				}
				break;
			}
		default:
			std::cout << "Wrong option.\n";
			continue;
		}
	}
	system("cls");
	std::string userToChat;
	std::cout << "Choose option: \n";
	std::cout << "Chose user to chat - C\n";
	std::cout << "Exit - E\n";
	while (true) {
		std::cin >> option;
		if (option == 'E') {
			break;
		}
		switch (option)
		{
		case 'C':
			while (true) {
				std::cout << "Exit - E\n";
				std::cout << "Search user to chat: ";
				std::cin >> userToChat;
				if (userToChat == "E") {
					break;
				}
				response = "C|" + userToChat + "|";
				std::strcpy(message, response.c_str());
				send(sock, message, strlen(message), 0);
				result = recv(sock, buffer, sizeof(buffer), 0);
				if (buffer[0] == 'S') {
					std::cout << "Chat: \n";
					while (true) {
						std::string userInput = "";
						std::getline(std::cin, userInput);
						std::string sendingMessage = "";
						sendingMessage = "S|" + username + ":" + userInput +  "|" + userToChat + "\n";
						std::strcpy(message, sendingMessage.c_str());
						send(sock, message, strlen(message), 0);
					}
				}
				else {
					std::cout << "ERROR: User not found\n";
					continue;
				}
			}
		default:
			break;
		}
	}
	closesocket(sock);
	WSACleanup();
	return 0;
}
