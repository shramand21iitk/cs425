# TCP Three-Way Handshake Using Raw Sockets

## Overview
This project is part of the assignment submission for CS425: Computer Networks. It implements the client-side and server-side of the TCP three-way handshake using raw sockets. The client and server manually construct and process TCP packets, bypassing the operating system's transport layer.

## Requirements
- Linux operating system
- C++ compiler (e.g., g++)
- Root privileges to run raw sockets

## Files
- `client.cpp`: Client-side implementation of the TCP three-way handshake.
- `server.cpp`: Server-side implementation of the handshake.
- `Makefile`: Build script for compiling the client and server.
- `README.md`: This file.

## Setup

1. **Build the client and server:**
   ```sh
   make
   ```

2. **Run the server:**
   ```sh
   sudo ./server
   ```

3. **Run the client:**
   ```sh
   sudo ./client
   ```

## Features

### Client-Side Functionality
- **SYN Packet Construction:**
  - Constructs a SYN packet with the sequence number `200`.
  - Sends the SYN packet to the server.

- **SYN-ACK Reception:**
  - Waits for the SYN-ACK response from the server.
  - Parses the SYN-ACK packet to extract the sequence and acknowledgment numbers.

- **ACK Packet Construction:**
  - Constructs and sends the final ACK packet to complete the handshake.

- **Timeout Handling:**
  - Implements a timeout feature. If no response is received within 5 seconds, the client resends the SYN packet up to 5 times.

### Server-Side Functionality
- Listens for incoming SYN packets from the client.
- Responds with a SYN-ACK packet.
- Waits for the final ACK packet to complete the handshake.

## Code Flow

### Client
1. **Initialization:**
   - Creates a raw socket.
   - Configures the socket to include IP headers and sets a timeout.

2. **SYN Packet:**
   - Constructs and sends a SYN packet with sequence number `200`.

3. **SYN-ACK Reception:**
   - Waits for the SYN-ACK response from the server.
   - Parses the SYN-ACK packet to extract the server's sequence number.

4. **ACK Packet:**
   - Constructs and sends the final ACK packet with sequence number `600`.

5. **Timeout Handling:**
   - If no response is received within 5 seconds, resends the SYN packet.

6. **Completion:**
   - Prints a success message upon completing the handshake.

### Server
1. **Initialization:**
   - Creates a raw socket.
   - Configures the socket to include IP headers.

2. **Packet Reception:**
   - Waits for incoming SYN packets.
   - Parses the received packets to extract TCP flags and sequence numbers.

3. **SYN-ACK Packet:**
   - Constructs and sends a SYN-ACK packet in response to a SYN packet.

4. **ACK Reception:**
   - Waits for the final ACK packet to complete the handshake.

5. **Completion:**
   - Prints a success message upon completing the handshake.

## Usage

### Running the Server
1. **Start the server**:
   - Navigate to the `Assignment_3` directory.
   - Run the following command:
     ```sh
     sudo ./server
     ```
   - The server will start listening for incoming SYN packets on port `12345`.
   - Example output:
     ```
     [+] Server listening on port 12345...
     ```

2. **When the server receives a SYN packet**:
   ```
   [+] TCP Flags:  SYN: 1 ACK: 0 FIN: 0 RST: 0 PSH: 0 SEQ: 200
   [+] Received SYN from 127.0.0.1
   [+] Sent SYN-ACK
   ```

3. **When the server receives the final ACK packet**:
   ```
   [+] TCP Flags:  SYN: 0 ACK: 1 FIN: 0 RST: 0 PSH: 0 SEQ: 600
   [+] Received ACK, handshake complete.
   ```

### Running the Client
1. **Start the client**:
   - Navigate to the `Assignment_3` directory.
   - Run the following command:
     ```sh
     sudo ./client
     ```
   - The client will initiate the TCP three-way handshake by sending a SYN packet to the server.

2. **Example output when the handshake is successful**:
   ```
   Client started
   Sending SYN to server...
   Sent packet with SYN=1, ACK=0
   TCP Flags: SYN=1, ACK=1, FIN=0, RST=0, PSH=0, SEQ=400
   Received SYN-ACK from server
   Sending ACK to server...
   Sent packet with SYN=0, ACK=1
   ACK sent
   Server seems to be unreachable or shut down.
   Client exiting
   ```

3. **Example output when a timeout occurs**:
   ```
   Sending SYN to server...
   Sent packet with SYN=1, ACK=0
   Timeout. Didn't receiving SYN-ACK. Resending SYN. Attempt:1/5
   ```

4. **Example output when the server is unreachable**:
   ```
   Sending SYN to server...
   Sent packet with SYN=1, ACK=0
   Timeout. Didn't receiving SYN-ACK. Resending SYN. Attempt:5/5
   Server seems to be unreachable or shut down.
   Client exiting
   ```

## Testing

### Correctness Testing
- Verified that the client constructs and sends the correct SYN, SYN-ACK, and ACK packets.
- Ensured that the server responds appropriately to the client's packets.

### Timeout Testing
- Simulated server unavailability to test the client's timeout and retransmission mechanism.

### Stress Testing
- Tested the client and server under high load to ensure reliability and performance.

## Challenges

### Raw Socket Permissions
- **Challenge:** Raw sockets require root privileges.
- **Solution:** Used `sudo` to run the client and server.

### Timeout Handling
- **Challenge:** Ensuring reliable retransmission in case of packet loss.
- **Solution:** Configured a 5-second timeout and implemented retransmission logic.

## Contribution of Each Member

| Contributor [Roll Number] | Features Implemented   | Code Contribution (%) | Documentation Contribution (%) | Testing & Debugging (%) |
|---------------------------|------------------------|-----------------------|-------------------------------|-------------------------|
| Ravija Chandel [210835]   | Timeout Handling, Debugging | 40%                  | 30%                          | 30%                    |
| Shaurya Singh [218070969] | Checksum Calculation, Client Logic | 35%                  | 35%                          | 30%                    |
| Shraman Das [218070996]   | Packet Construction, Testing | 25%                  | 35%                          | 40%                    |

## Sources Referred
1. **Books:**
   - Computer Networking: A Top-Down Approach, 8th Edition
2. **Blogs/Articles/Videos:**
   - [Raw Sockets in C++](https://www.binarytides.com/raw-sockets-c-code-linux/)
   - [TCP Three-Way Handshake](https://en.wikipedia.org/wiki/Transmission_Control_Protocol#Connection_establishment)