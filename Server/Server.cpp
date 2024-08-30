
#include "Server.h"
#include <iostream>
#include <winsock2.h>
#include <cstring>
#include <thread>
#include <fstream>
#include <sqlite3.h>
#include <locale>
#include <mutex>
#include <unordered_map>
#include <json.hpp>
#include <locale>

struct Message {
	std::string sender;
	std::string text;
	std::string receiver;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Message, sender, text, receiver);
};



#pragma comment(lib, "Ws2_32.lib")
#define PORT 8080
sqlite3* db;
sqlite3_stmt* stmt;
std::unordered_map<std::string, SOCKET> onlineClients;
std::mutex onlineCli_mutex;
std::vector<Message> chatHistory;

void initializeDataBase(sqlite3* &db) {
	int rc = sqlite3_open("users.db", &db);
	char* errMsg = nullptr;
	if (rc) {
		std::cerr << "Can't open database: " << sqlite3_errmsg(db);
		return;
	}
	else {
		std::cout << "Success open database\n";
	}
	const char* createTableSql = "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY, password TEXT);";
	rc = sqlite3_exec(db, createTableSql, nullptr,0,&errMsg);
	if (rc != SQLITE_OK) {
		std::cerr << "SQL error:" << errMsg << "\n";
		sqlite3_free(errMsg);
	}
}

bool userExists(sqlite3*& db, std::string& username, SOCKET clientSock, sqlite3_stmt*& stmt) {
	const char* sql = "SELECT 1 FROM users WHERE username = ?;";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
		return true;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		return true;
	}
	sqlite3_finalize(stmt);
	return false;
}

void addUser(sqlite3* &db, std::string& username, std::string& password, SOCKET& clientSock) {
	const char* response = "S";
	if (userExists(db, username, clientSock,  stmt)) {
		response = "U";
		send(clientSock, response, 1, 0);
		return;
	}
	const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		std::cerr << "Error preparing statement: " << sqlite3_errmsg(db) << std::endl;
		return;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) != SQLITE_DONE) {
		std::cerr << "Error executing statement: " << sqlite3_errmsg(db) << std::endl;
		sqlite3_finalize(stmt);
		return;
	}
	send(clientSock, response, strlen(response), 0);
	sqlite3_finalize(stmt);
}


void convertToString(std::string& option, char buffer[1024], int& itter) {
	for (; itter < strlen(buffer) ; itter++) {
		if (buffer[itter] == '|') {
			itter++;
			break;
		}
		option += buffer[itter];
	}
}

void checkLoginPassword(std::string& username, std::string& password, SOCKET& clientSock) {
	sqlite3_stmt* stmt;

	const char* sql = "SELECT 1 FROM users WHERE username = ? AND password = ?;";
	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
		std::cerr << "Error preparing: " << sqlite3_errmsg(db) << "\n";
		return;
	}
	sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
	if (sqlite3_step(stmt) == SQLITE_ROW) {
		onlineClients[username] = clientSock;
		send(clientSock, "S", 1, 0);
	}
	else {
		send(clientSock, "U", 1, 0);
	}
	sqlite3_finalize(stmt);
	return;
}

void sendMessage(const std::string& message,const std::string& receiver,const std::string& sender) {
	for (const auto& i : onlineClients) {
		if (i.first == receiver) {
			send(i.second, message.c_str(), message.size(), 0);
		}
		if (i.first == sender) {
			//send(i.second, message.c_str(), message.size(), 0);
		}
	}
}

void loadChatHistory(SOCKET clientSock,const std::string receive) {
	for (auto i : onlineClients) {
		if (i.second == clientSock) {
			for (int j = 0; j < chatHistory.size(); j++) {
				std::string historyMessages = chatHistory[j].text + "\n";
				if ((chatHistory[j].receiver == receive && chatHistory[j].sender == i.first) || (chatHistory[j].receiver == i.first && chatHistory[j].sender == receive)) {
					send(clientSock, historyMessages.c_str(), historyMessages.size(), 0);
				}
			}
			break;
		}
	}
	
}

void handleClient(SOCKET clientSock) {
	
	while (true) {
		char buffer[1024] = { 0 };
		int itter = 2;
		int result = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
		if (result > 0) {
			std::string username;
			std::string message;
			std::string password;
			
			const char* response = "S";
			// R - Registy // L - LogIn // C - Chose user to chat
			switch (buffer[0]) 
			{
			case 'R':
				convertToString(username, buffer,itter);
				convertToString(password, buffer,itter);
				addUser(db,username,password, clientSock);
				break;
			case 'L':
				convertToString(username, buffer,itter);
				convertToString(password, buffer, itter);
				checkLoginPassword(username, password, clientSock);
				break;
			case 'C':
				convertToString(username, buffer, itter);
				if (!userExists(db, username, clientSock, stmt)) {
					response = "U";
				}
				send(clientSock, response, 1, 0);
				if (response == "S") {
					loadChatHistory(clientSock, username);
				}
				break;
			case 'S':
				std::string receiver;
				message = "";
				for (int i = 0; itter < strlen(buffer); itter++) {
					if (message[i] == '|' && message[i - 1] == ':') {
						message = "";
						break;
					}
					if (buffer[itter] == ':') {
						message += ":";
						itter++;
					}
					else if (buffer[itter] == '|') {
						for (itter++; itter < strlen(buffer); itter++) {
							receiver += buffer[itter];
						}
						break;
					}
					i++;
					message += buffer[itter];
				}
				if (receiver.empty()) {
					break;
				}
				std::string sender;
				for (auto i : onlineClients) {
					if (i.second == clientSock) {
						sender = i.first;
					}
				}
				
				Message currMessage;
				currMessage.receiver = receiver;
				currMessage.sender = sender;
				currMessage.text = message;
				chatHistory.push_back(currMessage);
				nlohmann::json jsonChatHistory = chatHistory;
				std::ofstream inputFile("chatHistory.json");
				inputFile << jsonChatHistory.dump(4);
				inputFile.close();
				std::cout << message << "\n";
				sendMessage(message, receiver, sender);
				break;
			}
		}
		else if (result == 0) {
			std::cout << "User Disconnect\n";
			for (const auto& i : onlineClients) {
				if (i.second == clientSock) {
					std::lock_guard<std::mutex> guard(onlineCli_mutex);
					onlineClients.erase(i.first);
					closesocket(clientSock);
					break;
				}
			}
			break;
		}
		else {
			std::cout << "User Disconnect\n";
			for (const auto& i : onlineClients) {
				if (i.second == clientSock) {
					std::lock_guard<std::mutex> guard(onlineCli_mutex);
					onlineClients.erase(i.first);
					closesocket(clientSock);
					break;
					
				}
			}
			break;
		}
	}
	closesocket(clientSock);
}

int main()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	std::locale::global(std::locale("ru_RU.UTF-8"));
	std::ifstream outputFile("chatHistory.json");
	if (outputFile.is_open() && !(outputFile.peek() == std::ifstream::traits_type::eof())) {
		nlohmann::json jsonChatHistory;
		outputFile >> jsonChatHistory;
		chatHistory = jsonChatHistory.get<std::vector<Message>>();
		outputFile.close();
	}
	WSADATA wsaData;
	SOCKET serverSock = INVALID_SOCKET;
	SOCKET clientSock = INVALID_SOCKET;
	struct sockaddr_in serverAddr, clientAddr;
	int clientAddrSize = sizeof(clientAddr);

	initializeDataBase(db);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "Error WinSock \n";
		return 1;
	}
	serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSock == INVALID_SOCKET) {
		std::cerr << "Error sock creating \n";
		WSACleanup();
		return 1;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(PORT);

	if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Error bind sockets\n";
		closesocket(serverSock);
		WSACleanup();
		return 1;
	}

	if (listen(serverSock, 1) == SOCKET_ERROR) {
		std::cerr << "Error to listen";
		closesocket(serverSock);
		WSACleanup();
		return 1;
	}

	std::cout << "Waiting for connetion\n";
	while (true) {
		clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientAddrSize);
		if (clientSock == INVALID_SOCKET) {
			std::cerr << "Error accept connection\n";
			continue;
		}
		std::thread(handleClient, clientSock).detach();
	}
	outputFile.close();
	closesocket(clientSock);
	closesocket(serverSock);
	WSACleanup();
	return 0;
}
