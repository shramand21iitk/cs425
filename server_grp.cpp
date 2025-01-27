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
//defining port number
#define PORT 12345
//defining buffer size
#define BUFFER_SIZE 1024

//data management
unordered_map<string, int>clients; //unordered map, username > client socket
unordered_map<string, string>users; //unordered map, client username > password
unordered_map<string, unordered_set<int>>groups; //unordered map, group name > client socket
//Mutex for thread-safe access
mutex client_mutex; 

//create a group
void create_group(int sock, const string& group_name){ //takes the socket number and group name to create a group with client as first member
    
    //lock the mutex using std::lock_guard
    lock_guard<mutex> lock(client_mutex);

    //checks if group already exists
    if (groups.find(group_name) != groups.end()) {
        string errorMsg = "Group " + group_name + " already exists!\n";
        send(sock, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }
    //add client as first member
    groups[group_name].insert(sock);

    //inform client
    string successMsg = "Group " + group_name + " created successfully, and you are added as the first member.\n";
    send(sock, successMsg.c_str(), successMsg.size(), 0);
}

//join a group
void join_group(int sock, const string& group_name) { //takes the socket number and group name to add client as member
    
    //lock the mutex using std::lock_guard
    lock_guard<mutex> lock(client_mutex);

    //checks if group exists
    auto it = groups.find(group_name);
    if (it == groups.end()) {
        string errorMsg = "Group " + group_name + " does not exist!\n";
        send(sock, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    //add client as member
    it->second.insert(sock);

    //inform client
    string successMsg = "You have successfully joined the group " + group_name + ".\n";
    send(sock, successMsg.c_str(), successMsg.size(), 0);

    // Find the username associated with the client socket
    string username;
    for (const auto& client : clients) {
        if (client.second == sock) {
            username = client.first;
            break;
        }
    }

    // Send a group message to all members of the group informing them of who has joined
    string joinMsg = username + " has joined the group " + group_name + ".\n";
    for (const auto& memberSock : it->second) {
        if (memberSock != sock) { // Skip sending the message to the joining client
            send(memberSock, joinMsg.c_str(), joinMsg.size(), 0);
        }
    }
}

//leave group
void leave_group(int sock, const std::string& group_name) { //takes the socket number and group name to remove client as member
    
    //lock the mutex using std::lock_guard
    lock_guard<mutex> lock(client_mutex);

    //checks if group exists
    auto it = groups.find(group_name);
    if (it == groups.end()) {
        string errorMsg = "Group " + group_name + " does not exist!\n";
        send(sock, errorMsg.c_str(), errorMsg.size(), 0);
        return;
    }

    //remove client as member
    it->second.erase(sock);

    //delete group if empty otherwise inform client
    if (it->second.empty()) {
        groups.erase(it);
        string deleteMsg = "Group " + group_name + " is now empty and has been deleted.\n";
        send(sock, deleteMsg.c_str(), deleteMsg.size(), 0);
    } 
    
    else {
        string successMsg = "You have successfully left the group " + group_name + ".\n";
        send(sock, successMsg.c_str(), successMsg.size(), 0);

        // Find the username associated with the client socket
    string username;
    for (const auto& client : clients) {
        if (client.second == sock) {
            username = client.first;
            break;
        }
    }

    // Send a group message to all members of the group informing them of who has left
    string joinMsg = username + " has left the group " + group_name + ".\n";
    for (const auto& memberSock : it->second) {
        if (memberSock != sock) { // Skip sending the message to the leaving client
            send(memberSock, joinMsg.c_str(), joinMsg.size(), 0);
        }
    }
    }
}

//print all connected clients
void print_clients(int sock){ //takes the socket number to print all connected clients for the client
    
    //lock the mutex using std::lock_guard
    lock_guard<mutex> lock(client_mutex);  

    //print clients
    for (const auto& client : clients) {
            string Name = "- " + client.first + " (Socket: " + to_string(client.second) + ")\n";
            send(sock, Name.c_str(), Name.size(), 0);
        }
    }

//print all active groups
void print_groups(int sock) { //takes the socket number to print all active groups for the client

    //lock the mutex using std::lock_guard
    lock_guard<mutex> lock(client_mutex);  

    //check if no groups are available
    if (groups.empty()) {
        string noGroupsMsg = "No groups available.\n";
        send(sock, noGroupsMsg.c_str(), noGroupsMsg.size(), 0);
    } 
    
    //print groups
    else {
        for (const auto& group : groups) {
            string groupName = "- " + group.first + "\n";
            send(sock, groupName.c_str(), groupName.size(), 0);

            // Print group members
            for (const auto& clientSocket : group.second) {
                // Find the username associated with the client socket
                string username;
                for (const auto& client : clients) {
                    if (client.second == clientSocket) {
                        username = client.first;
                        break;
                    }
                }
                string memberInfo = "  * " + username + " (Socket: " + to_string(clientSocket) + ")\n";
                send(sock, memberInfo.c_str(), memberInfo.size(), 0);
            }
        }
    }
}

//send a message to a group
void group_message(int sock, const string& group_name, const string& message) { //takes the socket number, group name and message to send message to a group
        
        //lock the mutex using std::lock_guard
        lock_guard<mutex> lock(client_mutex);

        //check if group exists
        auto it = groups.find(group_name);
        if (it == groups.end()) {
            string errorMsg = "Group " + group_name + " does not exist!\n";
            send(sock, errorMsg.c_str(), errorMsg.size(), 0);
            return;
        }

        //get name of sending client
        string senderName;
        for (const auto& pair : clients) {
            if (pair.second == sock) {
                senderName = pair.first;
                break;
            }
        }
        
        //send message to all client sockets in group
        string formattedMsg = "[" + group_name + "] " + senderName + ": " + message + "\n";
        for (int memberSock : it->second) {
            if (memberSock != sock) { 
                send(memberSock, formattedMsg.c_str(), formattedMsg.size(), 0);
            }
        }

        //confirm to sending client
        string successMsg = "Message sent to group " + group_name + ".\n";
        send(sock, successMsg.c_str(), successMsg.size(), 0);
    }

//private messaging
void client_message(int sock, const string& name, const string& msg) {//takes the socket number, client name and message to send message to a specific client

    //lock the mutex using std::lock_guard
    lock_guard<mutex> lock(client_mutex);

    //finds the sender name using the socket number
    string senderName; 
    for (const auto& pair : clients) { 
        if (pair.second == sock) {
            senderName = pair.first;
            break;
        }
    }

    //finds the reciever socket using the name
    auto it = clients.find(name); 
    if (it != clients.end()) { 
        int destsock = it->second; 
        string formattedMsg = "[" + senderName + "] " + msg; 
        send(destsock, formattedMsg.c_str(), formattedMsg.size(), 0); 
    } else {
        cout << "User " << name << " not found!" << endl;
        string errormsg = "User " +name+ " not found!";
        send(sock, errormsg.c_str(), errormsg.size(), 0);
    }
}

//broadcast message
void broadcast_message(int sock, const string& msg) {//takes the socket number and message to broadcast the message to all clients except the sender

    //lock the mutex using std::lock_guard
    lock_guard<mutex> lock(client_mutex);

    //finds the sender name using the socket number
    std::string senderName;
    for (const auto& pair : clients) {
        if (pair.second == sock) {
            senderName = pair.first;
            break;
        }
    }

    //broadcasts the message to all clients except the sender
    std::string formattedMsg = "[Broadcast from " + senderName + "] " + msg;

    for (const auto& pair : clients) {
        if (pair.second != sock) { 
            send(pair.second, formattedMsg.c_str(), formattedMsg.size(), 0);
        }
    }

}

//process the message sent by the client
void process_message(const string& message, int client_socket, bool& logout_flag){ //takes the message and client socket to process the message depending on the command

    if (message.rfind("/msg", 0) == 0){ //check if the message is a private message
        size_t space1 = message.find(' ');
        size_t space2 = message.find(' ', space1 + 1);
        if (space1 != string::npos && space2 != string::npos) {
            std::string client_name = message.substr(space1 + 1, space2 - space1 - 1); // Extract the client name
            std::string client_msg = message.substr(space2 + 1); // Extract the message after the client name
            
            // Call function to handle private messaging
            client_message(client_socket, client_name, client_msg); 
        }
    }
   
    else if (message.rfind("/broadcast", 0) == 0){ //check if the message is a broadcast message
        size_t space = message.find(' ');
        if (space != string::npos) {
        string broadcast_msg = message.substr(space + 1); // Extract the message after the command

        // Call function to handle broadcasting
        broadcast_message(client_socket, broadcast_msg);
        }
    }
    
    else if (message.rfind("/create_group", 0) == 0){//check if the message is to create a group
        size_t space = message.find(' ');
        if (space != string::npos) {
        string group_name = message.substr(space + 1); // Extract the group name

        // Call function to create a group
        create_group(client_socket, group_name); 
        }
    }

    else if (message.rfind("/join_group", 0) == 0) {//check if the message is to join a group
        size_t space = message.find(' ');
        if (space != string::npos) {
        string group_name = message.substr(space + 1); // Extract the group name
        
        // Call function to join a group
        join_group(client_socket, group_name); 
        }
    }   

    else if (message.rfind("/leave_group", 0) == 0) { //check if the message is to leave a group
        size_t space = message.find(' ');
        if (space != string::npos) {
        string group_name = message.substr(space + 1); // Extract the group name    

        // Call function to leave a group
        leave_group(client_socket, group_name);
        }
    }

    else if (message.rfind("/group_msg", 0) == 0){   //check if the message is a group message
        size_t space1 = message.find(' ');
        size_t space2 = message.find(' ', space1 + 1);
        if (space1 != string::npos && space2 != string::npos) {
            string group_name = message.substr(space1 + 1, space2 - space1 - 1);  // Extract the group name      
            string group_msg = message.substr(space2 + 1); // Extract the message after the group name
            
            // Check if the client socket is part of the group
            auto group_it = groups.find(group_name);
            if (group_it != groups.end() && group_it->second.find(client_socket) != group_it->second.end()) {
                // Call function to handle group messaging
                group_message(client_socket, group_name, group_msg);
            } else {
                string errorMsg = "You are not a member of the group " + group_name + ".\n";
                send(client_socket, errorMsg.c_str(), errorMsg.size(), 0);
            }
        }
    }

    else if (message.rfind("/grps", 0) == 0){ //check if the message is to print all groups

        // Call function to print all groups
        print_groups(client_socket);
    }

    else if (message.rfind("/active", 0) == 0){ //check if the message is to print all active clients

        // Call function to print all active clients
        print_clients(client_socket);
    }

    else if (message.rfind("/logout", 0) == 0){ //check if the message is to logout
        logout_flag = true;
    }
}

//Define a function to handle each client by assigning each of them a thread for communication
void clientHandler(int clientSocket){

    //the client handler shall run indefinitely until the client disconnects

    //the handler shall first prompt the client to login and then listen for 
    //messages from the client if the client is authenticated 

    char username[BUFFER_SIZE], password[BUFFER_SIZE]; //define buffers to store the username and password entered by the user
    string receivedUsername, receivedPassword; //define strings to store the username and password entered by the user
    int bytesReceived; //define a variable to store the number of bytes received from the client
    
    while(true){ //break out of this loop if the client is new and authenticated
    
    //Ask the client for username
    const char* message1 = "Welcome to Wazzapp\n\nEnter the username: ";  
    send(clientSocket, message1, strlen(message1), 0); 
    memset(username, 0, BUFFER_SIZE);  // Clear the buffer to avoid old data
    bytesReceived = recv(clientSocket, username, BUFFER_SIZE, 0);   // Receive the username from the client

    if (bytesReceived <= 0) // Check if the client has disconnected
    {
        close(clientSocket);
        return;
    }
    username[bytesReceived] = '\0'; // Null-terminate
    receivedUsername=username;  // Store the received username

    // Check if the username is already in use by another client and send a message to the client
    if (clients.find(receivedUsername) != clients.end()) {
            const char* messageFail = "Client already connected! Log out from previous session to connect.\n";
            send(clientSocket, messageFail, strlen(messageFail), 0);
            continue; // Continue listening to messages from the client without termination
     }

    //Ask the client for password
    const char* message2 = "Enter the password: ";
    send(clientSocket, message2, strlen(message2), 0);
    // Receive the password from the client
    memset(password, 0, BUFFER_SIZE);  // Clear the buffer to avoid old data
    bytesReceived = recv(clientSocket, password, BUFFER_SIZE, 0);  // Receive password from the client
    if (bytesReceived <= 0)// Check if the client has disconnected
    {
        close(clientSocket);
        return;
    }
    username[bytesReceived] = '\0'; // Null-terminate
    receivedPassword=password;  // Store the received password

    // Check the users file for authentication
    string struser = receivedUsername + ":" + receivedPassword;
    ifstream in("users.txt");
    string strfile;
    bool authenticated = false;
    while (getline(in, strfile)){// Check if the username and password match
            // Lock the mutex using std::lock_guard
            lock_guard<mutex> lock(client_mutex);

            if (strfile == struser) {
            // Send the welcome message to the client
            const char* messagever = "Welcome to the chat server!\n\nTo broadcast the message to all online users type /broadcast <message>\nTo send message to a specific online client type /msg <username> <message>\nTo send message to a specific group type /group_msg <group_name> <message>\nTo create a new group type /create group <group name>\nTo join an existing group type /join group <group name>\nTo leave a group type /leave group <group name>\nTo get a list of all active users type /active\nTo get a list of all groups type /grps\n\nTo log out type /logout\n\nType /exit for closing the session\n\nEnjoy your time here!\n ";
            
            
            // Add the client to the map of clients
            clients[receivedUsername] = clientSocket;
            send(clientSocket, messagever, strlen(messagever), 0);
            authenticated = true;
            break;
        }
    }
    if(!authenticated){
        const char* messageFail = "\nAuthentication failed!\n\n"; // Send a message to the client if authentication fails
        send(clientSocket, messageFail, strlen(messageFail), 0);
        continue;
        }
    else{
        break;
        }
    }
    
    bool logout_flag = false; //create a flag to check if the client has logged out
    while(!logout_flag){ //break out of this loop if the client logs out
    //Continue listening to messages from client without termination
    char buffer[BUFFER_SIZE]; //create a buffer for incoming messages from the client
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
    process_message(buffer, clientSocket, logout_flag);

    //Remove the client from the map if client has disconnected and close the client socket
    
    }
    {   //lock the mutex using std::lock_guard
        lock_guard<mutex> lock(client_mutex);

        //remove client from clients
        for (auto it = clients.begin(); it != clients.end(); ++it){
            if (it->second == clientSocket) {
                clients.erase(it->first);
                break;
            }
        } 
        
        //remove client from groups
        for (auto& group : groups) {
        group.second.erase(clientSocket);
        }
    }
    
    if (bytesReceived <= 0) {
            close(clientSocket);//close the client socket if the client has disconnected
            return;
        }
    else{
        clientHandler(clientSocket);
    }
    
  
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
        thread(clientHandler, client_socket).detach();
        // Detach the newly created client thread to allow it to run independently from the main listening thread which can freely continue listening for new clients
        
    }
    close(server_socket);
    return 0;
}