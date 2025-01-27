# Wazzapp: Simple Chat Server


## Overview

This project is part of the asignment submission for CS425 : Computer Networks. It implements a multi-threaded chat server that supports private messages, broadcast messages, group communication, and user authentication along with some extra utility features. The server is built using C++ and utilizes socket programming, multithreading, and data synchronization to handle multiple concurrent client connections.

## Features

1. **Client Connection:**
   - Authentication with username and password.
   - Prevents multiple connections with the same username.
   - Logout functionality.

2. **Messaging:**
   - Broadcast messages to all connected clients.
   - Send private messages to specific users.
   - Send messages to group members.

3. **Group Management:**
   - Create new groups.
   - Join existing groups.
   - Leave groups.

4. **Statistics:**
   - List all active users with socket numbers
   - List all groups and their members with socket numbers

## Requirements

- C++20 compiler
- POSIX-compliant operating system (e.g., Linux)

## Files

- `server_grp.cpp`: Server-side implementation.
- `client_grp.cpp`: Client-side implementation.
- `users.txt`: File containing test usernames and passwords.
- `Makefile`: Build script for compiling the server and client.
- `README.md`: This file.

## Setup


1. **Builld the server and client:**
     ```sh 
     make
     ```
    
2. **Run the server**
    ```sh
    ./server_grp
    ```

3. **Run the client**
   ```sh
   ./client_grp
   ```

## Usage

### Server
The server is designed to handle multiple clients concurrently using threads. It supports various commands for private messaging, group communication, and user management. The use of mutexes ensures thread-safe access to shared data structures, preventing race conditions and ensuring data consistency. All commands and server/client status updates are printed on the server. **There are no server-side specific commands.**

### Client

The client connects to the server and supports the following commands:

```/msg <username> <message>```: Send a private message to a user.

```/broadcast <message>```: Send a message to all users.

```/create_group <group_name>```: Create a new group.

```/join_group <group_name>```: Join an existing group.

```/leave_group <group_name>```: Leave a group.

```/group_msg <group_name> <message>```: Send a message to a group.

```/grps```: List all groups and their members.

```/active```: List all active users.

```/logout```: Logout from the server.

```/exit```: Close the client session.

## Walk Through

### Running the server:
```sh
./server_grp
```
### Running a client:
```sh
./client_grp
```
### Logging in:
The client upon connecting to the server is prompted by an authemtication process, asking for the username and password. The users.txt file contains test usernames and passwords in the format username:password. The server uses this file to authenticate clients.


Example of `users.txt`:

```sh
alice:password123
bob:qwerty456
charlie:secure789
david:helloWorld!
eve:trustno1
frank:letmein
grace:passw0rd
```

Successful entry into the server will look like:

```sh
Welcome to the server

Enter username: alice
Enter password: password123
Welcome to the chat server!

To broadcast the message to all online users type /broadcast <message>
To send message to a specific online client type /msg <username> <message>
To send message to a specific group type /group_msg <group_name> <message>
To create a new group type /create group <group name>
To join an existing group type /join group <group name>
To leave a group type /leave group <group name>
To get a list of all active users type /active
To get a list of all groups type /grps

To log out type /logout

Type /exit for closing the session

Enjoy your time here!
```

If the username and password do not exist in `users.txt`, the client will be re-promted with another attempt to log in

```sh
Connected to the server.
Welcome to Wazzapp

Enter the username: bob
Enter the password: bob

Authentication failed!

Welcome to Wazzapp

Enter the username: 

```

If the user is already logged in via a client session, it is not possible for another session to start under the same username. 

```sh
Connected to the server.
Welcome to Wazzapp

Enter the username: alice
Client already connected! Log out from previous session to connect.
Welcome to Wazzapp

Enter the username: 

```
### Statistics

Users can check number of active users and active groups using `/active` and `/grps` respectively. 

```sh
/active
- alice (Socket: 6)
- frank (Socket: 5)
- bob (Socket: 4)

/grps
- cs425
  * bob (Socket: 4)
  * frank (Socket: 5)
``` 

### Private Messaging
![alt text](image-1.png)

### Broadcast Messaging
![alt text](image-2.png)

#### Groups
Uses can easily see available groups with list of all active users using `/grps`. Using the `/create_group` command users can create new groups with themselves as the first member.
![alt text](image.png)

`/join_group` is used by other users to join groups. All members in that group are notified about the new user and the list of active users in that group is updated, visible to all users.
![alt text](image-3.png)

`/msg <group_name> <message>` is the the syntax used to message in groups. Non-group members can't send or recieve messages.
![alt text](image-4.png)

`\leave_group` is used to leave group. If all members leave the group the group is deleted.
![alt text](image-5.png)

### Logging out:

After successful client login, the suer can log out of the session and use the same socket to log in with a different username.

```sh
/logout
Welcome to Wazzapp

Enter the username: 

```
## Contributing
Contributions are welcome! The repository will be public soon. Please fork the repository and submit a pull request. 
