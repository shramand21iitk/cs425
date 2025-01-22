#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFFER_SIZE 1024

std::mutex mtx;                                                  // Mutex for thread safety
std::unordered_map<int, std::string> clients;                    // client socket -> username
std::unordered_map<std::string, std::string> users;              // username -> password
std::unordered_map<std::string, std::unordered_set<int>> groups; // group -> client sockets

void load_users()
{
    std::ifstream file("users.txt");
    std::string line;
    while (std::getline(file, line))
    {
        size_t pos = line.find(':');
        if (pos != std::string::npos)
        {
            std::string username = line.substr(0, pos);
            std::string password = line.substr(pos + 1);
            users[username] = password;
            // std::cout << username << " " << password << std::endl;
        }
    }
}

void handle_client(int client_socket)
{
    char buffer[BUFFER_SIZE];
    std::string username, password;

    // Authentication
    send(client_socket, "Enter username: ", 16, 0);
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0)
    {
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate
    username = std::string(buffer);
    username.erase(username.find_last_not_of(" \n\r\t") + 1); // Trim whitespace

    send(client_socket, "Enter password: ", 16, 0);
    bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0)
    {
        close(client_socket);
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate
    password = std::string(buffer);
    password.erase(password.find_last_not_of(" \n\r\t") + 1); // Trim whitespace

    if (users.find(username) != users.end() && users[username] == password)
    {
        std::lock_guard<std::mutex> lock(mtx);
        clients[client_socket] = username;
        send(client_socket, "Authentication successful!\n", 27, 0);
        // To Implement code for sending joining name for everyone except self.////// ->>>>>>weogwngowon
    }
    else
    {
        send(client_socket, "Authentication failed!\n", 23, 0);
        close(client_socket);
        return;
    }

    // Message handling loop
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0)
            break;

        std::string message(buffer);
        // std::cout << message << std::endl;

        if (message.starts_with("/broadcast "))
        {
            std::string msg = message.substr(11);
            std::lock_guard<std::mutex> lock(mtx);
            for (const auto &client : clients)
            {
                send(client.first, ("[" + clients[client_socket] + "]: " + msg).c_str(), clients[client_socket].length() + msg.length() + 5, 0);
            }
        }
        else if (message.starts_with("/msg "))
        {
            size_t space_pos = message.find(' ', 5);
            if (space_pos != std::string::npos)
            {
                std::string target_user = message.substr(5, space_pos - 5);
                std::string msg = message.substr(space_pos + 1);
                std::lock_guard<std::mutex> lock(mtx);
                for (const auto &client : clients)
                {
                    if (clients[client.first] == target_user)
                    {
                        send(client.first, ("[" + clients[client_socket] + "]: " + msg).c_str(), clients[client_socket].length() + msg.length() + 5, 0);
                        break;
                    }
                }
            }
        }
        else if (message.starts_with("/create_group "))
        {
            std::string group_name = message.substr(14);
            std::lock_guard<std::mutex> lock(mtx);
            groups[group_name].insert(client_socket);
            send(client_socket, ("Group " + group_name + " created.\n").c_str(), group_name.length() + 20, 0);
        }
        else if (message.starts_with("/join_group "))
        {
            std::string group_name = message.substr(12);
            std::lock_guard<std::mutex> lock(mtx);
            // groups[group_name].insert(client_socket);
            // send(client_socket, ("Joined group " + group_name + ".\n").c_str(), group_name.length() + 16, 0);

            if (groups.find(group_name) == groups.end())
            {
                send(client_socket, ("Error: Group " + group_name + " does not exist.\n").c_str(),
                     ("Error: Group " + group_name + " does not exist.\n").length(), 0);
            }
            else
            {
                groups[group_name].insert(client_socket);
                send(client_socket, ("Joined group " + group_name + ".\n").c_str(),
                     ("Joined group " + group_name + ".\n").length(), 0);
            }
        }
        else if (message.starts_with("/leave_group "))
        {
            std::string group_name = message.substr(14);
            std::lock_guard<std::mutex> lock(mtx);
            groups[group_name].erase(client_socket);
            send(client_socket, ("Left group " + group_name + ".\n").c_str(), group_name.length() + 14, 0);
        }
        else if (message.starts_with("/group_msg "))
        {
            size_t space_pos1 = message.find(' ', 10); // Used to find spaces after each word
            if (space_pos1 != std::string::npos)
            {
                size_t space_pos2 = message.find(' ', space_pos1 + 1);

                // std::cout << space_pos1 << " " << space_pos2 << " " << std::string::npos << std::endl;

                std::string group_name = message.substr(space_pos1 + 1, (space_pos2 == std::string::npos ? message.length() : space_pos2) - (space_pos1 + 1));

                std::string msg = (space_pos2 == std::string::npos) ? message.substr(space_pos1 + 1 + group_name.length()) : message.substr(space_pos2 + 1);

                // extra whitespace trim karne ke liye
                msg.erase(0, msg.find_first_not_of(" \t\n"));

                // std::cout << group_name << " " << msg << std::endl;

                std::lock_guard<std::mutex> lock(mtx);
                for (int member : groups[group_name])
                {
                    send(member, ("[" + clients[client_socket] + "]: " + msg).c_str(), clients[client_socket].length() + msg.length() + 5, 0);
                }
            }
        }
    }

    // Cleanup on disconnect
    std::lock_guard<std::mutex> lock(mtx);
    clients.erase(client_socket);
    for (auto &group : groups)
    {
        group.second.erase(client_socket);
    }

    close(client_socket);
}

int main()
{
    load_users();

    int server_socket, client_socket;
    sockaddr_in server_addr{}, client_addr{};
    socklen_t client_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        std::cerr << "Error creating socket." << std::endl;
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Error binding socket." << std::endl;
        return 1;
    }

    if (listen(server_socket, 5) < 0)
    {
        std::cerr << "Error listening on socket." << std::endl;
        return 1;
    }

    std::cout << "Server is running on port " << PORT << "..." << std::endl;

    while (true)
    {
        client_socket = accept(server_socket, (sockaddr *)&client_addr, &client_len);
        if (client_socket < 0)
        {
            std::cerr << "Error accepting connection." << std::endl;
            continue;
        }

        std::thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}