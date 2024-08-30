// Client.cpp: определяет точку входа для приложения.
//

#include "Client2.h"
#include <iostream>
#include <string>
#include <winsock2.h>
#include <thread>
#include <mutex>
#include <set>
#include <locale>
#pragma comment(lib, "Ws2_32.lib")
#define PORT 8080


std::mutex mtx;
std::set<std::string> usersNotifications;
std::string userToChat;

void receiveMessages(SOCKET sock, std::string username) {
	char buffer[4096] = "\0";
	while (true) {
		ZeroMemory(buffer,4096);
		int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
		if (bytesReceived > 0) {
			std::string ChatingUser;
			for (auto i : buffer) {
				if (i == ':') {
					break;
				}
				ChatingUser += i;
			}
			if (buffer[0] == 'S' || buffer[0] == 'U') {
				continue;
			}
			if (ChatingUser == userToChat) {
				std::cout << "\r" << buffer << "\n";
				std::cout << username << ": "<< std::flush;
			}
			else if (username == ChatingUser) {
				std::cout << "\r" << buffer << "\n" << std::flush;;
			}
			else {
				usersNotifications.insert(ChatingUser);
				std::cout << "\rNotification: " << usersNotifications.size();
			}
		}
	}
}

void sendMessages(SOCKET sock, const std::string& username) {
	while (true) {
		char buffer[1024];
		ZeroMemory(buffer, 1024);
		std::string userInput = "";
		std::getline(std::cin, userInput);
		if (userInput == "exit from " + userToChat) {
			break;
		}
		std::string sendingMessage = "";
		sendingMessage = "S|" + username + ":" + userInput + "|" + userToChat;
		std::strcpy(buffer, sendingMessage.c_str());
		send(sock, buffer, strlen(buffer), 0);
		std::cout << "\r" << username << ": ";
	}
}
int main()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	std::locale::global(std::locale("ru_RU.UTF-8"));
	WSADATA wsaData;
	SOCKET sock = INVALID_SOCKET;
	struct sockaddr_in serverAddr;
	char buffer[1024] = { 0 };
	char message[1024] = { 0 };
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
	serverAddr.sin_addr.s_addr = inet_addr("26.103.198.212");
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
	std::thread receiveThread(receiveMessages, sock, username);
	receiveThread.detach();
	while (true) {
		system("cls");
		std::cout << "\rChoose option: \n";
		std::cout << "\rCheck notifications - N\n";
		std::cout << "\rChose user to chat - C\n";
		std::cout << "\rExit - E\n";
		std::cout << "\rNotification: " << usersNotifications.size();
		std::cin >> option;
		if (option == 'E') {
			break;
		}
		switch (option)
		{
		case 'C':
			while (true) {
				userToChat = "";
				std::cout << "Exit - E\n";
				std::cout << "Search user to chat: ";
				std::cin >> userToChat;
				if (userToChat == "E") {
					break;
				}
				response = "C|" + userToChat + "|";
				std::strcpy(message, response.c_str());
				send(sock, message, strlen(message), 0);
				if (buffer[0] == 'S') {
					std::cout << "\rIf you want to exit write - exit from " << userToChat << "\n";
					if (find(usersNotifications.begin(), usersNotifications.end(), userToChat) != usersNotifications.end()) {
						usersNotifications.erase(userToChat);
					}
					std::thread senderThread(sendMessages, sock, username);
					senderThread.join();

				}
				else {
					std::cout << "ERROR: User not found\n";
					continue;
				}
			}
			break;
		case 'N':
			while (true) {
				std::cout << "\rExit - E \n";
				std::cout << "Notifications from users:\n";
				for (auto i : usersNotifications) {
					std::cout << i << "\n";
				}
				std::cin >> option;
				if (option == 'E') {
					break;
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
