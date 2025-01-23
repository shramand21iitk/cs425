//Server-side implementation in C++ for a chat server with private messages and group messaging

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <fstream>
#include<cstring>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>

using namespace std;

#define PORT 12343
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100 //what is the need of defining max clients pehle se?

//make an unordered map for storing the online users, groups and their sockets
unordered_map<string, int>clients; // Username --> Client socket
unordered_map<string, string>users; // client username -> password
unordered_map<string, unordered_set<int>>groups; //Group --> client sockets
mutex client_mutex; //Mutex for thread-safe access to clients map


//join a group
void join_group(int sock, const string& group_name) {
    //check if group is there
    lock_guard<mutex> lock(client_mutex);
    auto it = groups.find(group_name);
    if (it == groups.end()) {
        string errorMsg = "Group " + group_name + " does not exist!\n";
        send(sock, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    //add client
    it->second.insert(sock);

    string successMsg = "You have successfully joined the group " + group_name + ".\n";
    send(sock, successMsg.c_str(), successMsg.size(), 0);
}
//create a group
void create_group(int sock, const string& group_name){
    //check if group is there
    lock_guard<mutex> lock(client_mutex);
    if (groups.find(group_name) != groups.end()) {
        string errorMsg = "Group " + group_name + " already exists!\n";
        send(sock, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }
    
    //add user as first member
    groups[group_name].insert(sock);

    string successMsg = "Group " + group_name + " created successfully, and you are added as the first member.\n";
    send(sock, successMsg.c_str(), successMsg.size(), 0);
}

//leave group
void leave_group(int sock, const std::string& group_name) {
    //group exists?
    lock_guard<mutex> lock(client_mutex);
    auto it = groups.find(group_name);
    if (it == groups.end()) {
        string errorMsg = "Group " + group_name + " does not exist!\n";
        send(sock, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    //remove user
    it->second.erase(sock);

    //delete empty grp
    if (it->second.empty()) {
        groups.erase(it);
        string deleteMsg = "Group " + group_name + " is now empty and has been deleted.\n";
        send(sock, deleteMsg.c_str(), deleteMsg.size(), 0);
    } else {
        string successMsg = "You have successfully left the group " + group_name + ".\n";
        send(sock, successMsg.c_str(), successMsg.size(), 0);
    }
}

//print all connected clients
void print_clients(int sock){
    lock_guard<mutex> lock(client_mutex);  

    for (const auto& client : clients) {
        
            string Name = "- " + client.first + "\n";
            send(sock, Name.c_str(), Name.size(), 0);
        }
    }


//print all active groups
void print_groups(int sock) {
    lock_guard<mutex> lock(client_mutex);  

    if (groups.empty()) {
        string noGroupsMsg = "No groups available.\n";
        send(sock, noGroupsMsg.c_str(), noGroupsMsg.size(), 0);
    } else {
        for (const auto& group : groups) {
            
                string GroupName = "- " + group.first + "\n";
                send(sock, GroupName.c_str(), GroupName.size(), 0);
            
        }
    }
}

//send a message to a group
void group_message(int sock, const string& group_name, const string& message) {
        lock_guard<mutex> lock(client_mutex);
        auto it = groups.find(group_name);
        if (it == groups.end()) {
            string errorMsg = "Group " + group_name + " does not exist!\n";
            send(sock, errorMsg.c_str(), errorMsg.size(), 0);
            return;
        }
        string senderName;
        for (const auto& pair : clients) {
            if (pair.second == sock) {
                senderName = pair.first;
                break;
            }
        }


        
        string formattedMsg = "[" + group_name + "] " + senderName + ": " + message + "\n";
        for (int memberSock : it->second) {
            if (memberSock != sock) { 
                send(memberSock, formattedMsg.c_str(), formattedMsg.size(), 0);
            }
        }

        //confirm
        string successMsg = "Message sent to group " + group_name + ".\n";
        send(sock, successMsg.c_str(), successMsg.size(), 0);
    }

//private messaging
void client_message(int sock, const string& name, const string& msg) {
    string senderName; 
    for (const auto& pair : clients) { //find sender name using socket
        if (pair.second == sock) {
            senderName = pair.first;
            break;
        }
    }
    auto it = clients.find(name); //find reciever socket using name
    if (it != clients.end()) { //banda exist karta hai ji
        int destsock = it->second; 
        string formattedMsg = "[" + senderName + "] " + msg; 
        send(destsock, formattedMsg.c_str(), formattedMsg.size(), 0); 
    } else {
        cout << "User " << name << " not found!" << endl;
        string errormsg = "User " +name+ " not found!";
        send(sock, errormsg.c_str(), errormsg.size(), 0);
    }
}

void broadcast_message(int sock, const string& msg) {
    std::string senderName;
    for (const auto& pair : clients) {
        if (pair.second == sock) {
            senderName = pair.first;
            break;
        }
    }


    std::string formattedMsg = "[Broadcast from " + senderName + "] " + msg;

    for (const auto& pair : clients) {
        if (pair.second != sock) { 
            send(pair.second, formattedMsg.c_str(), formattedMsg.size(), 0);
        }
    }

}
//define a function for parsing the messages the client has sent and forwarding to the required sockets
void process_message(const string& message, int client_socket){
    if (message.rfind("/msg", 0) == 0){
        size_t space1 = message.find(' ');
        size_t space2 = message.find(' ', space1 + 1);
        if (space1 != string::npos && space2 != string::npos) {
            std::string client_name = message.substr(space1 + 1, space2 - space1 - 1);
            std::string client_msg = message.substr(space2 + 1);
            // Call function to handle client messaging
            client_message(client_socket, client_name, client_msg);
        }
    }
    else if (message.rfind("/broadcast", 0) == 0){
        // Extract the message after "/broadcast"
    size_t space = message.find(' ');
    if (space != string::npos) {
        string broadcast_msg = message.substr(space + 1);
        // Call function to handle broadcasting
        broadcast_message(client_socket, broadcast_msg);
    }
}
    else if (message.rfind("/create_group", 0) == 0){
    size_t space = message.find(' ');
    if (space != string::npos) {
        string group_name = message.substr(space + 1);
        create_group(client_socket, group_name);
    }

    }
    else if (message.rfind("/join_group", 0) == 0) {
    size_t space = message.find(' ');
    if (space != string::npos) {
        string group_name = message.substr(space + 1);
        join_group(client_socket, group_name);
    }
}   else if (message.rfind("/leave_group", 0) == 0) {
    size_t space = message.find(' ');
    if (space != string::npos) {
        string group_name = message.substr(space + 1);
        leave_group(client_socket, group_name);
    }
}
    else if (message.rfind("/group_msg", 0) == 0){ 
        size_t space1 = message.find(' ');
        size_t space2 = message.find(' ', space1 + 1);
        if (space1 != string::npos && space2 != string::npos) {
            string group_name = message.substr(space1 + 1, space2 - space1 - 1);
            string group_msg = message.substr(space2 + 1);
            
            group_message(client_socket, group_name, group_msg);
        }
    }
    else if (message.rfind("/grps", 0) == 0){ 

        print_groups(client_socket);
        }
    else if (message.rfind("/active", 0) == 0){
        print_clients(client_socket);
    }
    }



//Define a function to handle each client by assigning each of them a thread for communication
void handle_client(int clientSocket){
    //define buffers to store the username and password entered by the user
    char username[BUFFER_SIZE], password[BUFFER_SIZE];
    //Ask the client for username
    const char* message1 = "Enter the username: ";
    send(clientSocket, message1, strlen(message1), 0);
    // Receive the username from the client
    memset(username, 0, BUFFER_SIZE);  // Clear the buffer to avoid old data
    int bytesReceived = recv(clientSocket, username, BUFFER_SIZE, 0);  // Receive username
    if (bytesReceived <= 0)
    {
        close(clientSocket);
        return;
    }
    username[bytesReceived] = '\0'; // Null-terminate
    string receivedUsername(username);  // Store the received username


    //Ask the client for username
    const char* message2 = "Enter the password: ";
    send(clientSocket, message2, strlen(message2), 0);
    // Receive the password from the client
    memset(password, 0, BUFFER_SIZE);  // Clear the buffer to avoid old data
    bytesReceived = recv(clientSocket, password, BUFFER_SIZE, 0);  // Receive password
    
    
    if (bytesReceived <= 0)
    {
        close(clientSocket);
        return;
    }
    username[bytesReceived] = '\0'; // Null-terminate
    string receivedPassword(password);  // Store the received password

    // Check the users file for authentication
    string struser = receivedUsername + ":" + receivedPassword;
    ifstream in("users.txt");
    string strfile;
    bool authenticated = false;
    while (getline(in, strfile)){
        if (strfile == struser) {
            const char* messagever = "Welcome to the chat server!\nTo broadcast the message to all online users type /broadcast <message>\nTo send message to a specific online client type /msg <username> <message>\nTo send message to a specific group type /group_msg <group_name> <message>\nTo create a new group type /create group <group name>\nTo join an existing group type /join group <group name>\nTo leave a group type /leave group <group name>\nTo get a list of all active users type /active\nTo get a list of all groups type /grps\nEnjoy your time here!\n";
            lock_guard<mutex> lock(client_mutex);
            clients[receivedUsername] = clientSocket;
            send(clientSocket, messagever, strlen(messagever), 0);
            authenticated = true;
         break;
        }
    }
    if (!authenticated) {
        const char* messageFail = "Authentication failed!";
        send(clientSocket, messageFail, strlen(messageFail), 0);
    }

    //Continue listening to messages from client without termination
    char buffer[BUFFER_SIZE]; //create a buffer for incoming messages from the client
    
    while(true){
        memset(buffer, 0, BUFFER_SIZE); //clear the buffer before receiving new data
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0); //store the number of bytes(packets) of messages sent from the client
        //Check if the client has disconnected
        if(bytesReceived <= 0){
            cout << "Client " << receivedUsername << " disconnected.\n" << endl;
            break;
        }
        //Display any message sent by the client
        cout << receivedUsername << ":" << buffer << endl;
        //Pass the message into the process_message
        process_message(buffer, clientSocket);
    }
    //Remove the client from the map if client has disconnected and close the client socket
    {
        lock_guard<mutex> lock(client_mutex);
        for (auto it = clients.begin(); it != clients.end(); ++it){
            if (it->second == clientSocket) {
                clients.erase(it->first);
                break;
            }
        } //clients se udaya hai but groups se bhi udane ka rahega
    }
    close(clientSocket);
}

int main()
{   //create server socket to listen to clients
    int server_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1){
        cerr <<"Can not create socket\n" << endl;
        return 1;
    }

    //define server socket address using ip and port
    sockaddr_in serv_sock_addr{};
    serv_sock_addr.sin_family = AF_INET;
    serv_sock_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "0.0.0.0", &serv_sock_addr.sin_addr);

    //bind the socket to ip and port
    if(bind(server_socket, (sockaddr*)&serv_sock_addr, sizeof(serv_sock_addr)) == -1){
        cerr << "Can not bind to IP/Port\n" <<endl;
        return 2;
    }
    //start listening on the socket
    if (listen(server_socket, SOMAXCONN) == -1){
        cerr << "Can't listen via socket\n" <<endl;
        return 3;
    }
    
    cout << "Server is listening for incoming clients on port number" << PORT << "...\n" << endl;

    int client_socket;
    sockaddr_in clt_sock_addr;
    socklen_t clientSize = sizeof(clt_sock_addr);

    while(true){

        //awaits a connection, and upon recieving one creates a new socket to communicate
        client_socket = accept(server_socket, (sockaddr*)&clt_sock_addr, &clientSize);
        if (client_socket == -1) {
            cerr << "Problem with client connecting!\n" <<endl;
            return 4;
        }
        //On successful connection of client to the server, create a new thread for the client and then call the function to handle the client
        thread(handle_client, client_socket).detach();
        // Detach the newly created client thread to allow it to run independently from the main listening thread which can freely continue listening for new clients
        
    }

    close(server_socket);
    return 0;
}   