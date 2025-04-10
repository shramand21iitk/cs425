# TCP Three-Way Handshake Using Raw Sockets

## Overview
This project is part of the assignment submission for CS425: Computer Networks. It implements the client-side of the TCP three-way handshake using raw sockets. The server-side code is provided and can be downloaded from the [GitHub repository](https://github.com/privacy-iitk/cs425-2025.git).

The client constructs and sends custom TCP packets (SYN, ACK) using raw sockets, bypassing the operating system's transport layer. This allows manual control over packet fields such as sequence numbers, flags, and checksums.

## Requirements
- Linux operating system
- C++ compiler (e.g., g++)
- Root privileges to run raw sockets

## Files
- `client.cpp`: Client-side implementation of the TCP three-way handshake.
- `server.cpp`: Server-side implementation (provided).
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
  - Constructs a SYN packet with the correct sequence number (200).
  - Sends the SYN packet to the server.

- **SYN-ACK Reception:**
  - Waits for the SYN-ACK response from the server.
  - Parses the SYN-ACK packet to extract the sequence and acknowledgment numbers.

- **ACK Packet Construction:**
  - Constructs and sends the final ACK packet to complete the handshake.

- **Timeout Handling:**
  - Implements a timeout feature. If no response is received within 5 seconds, the client resends the SYN packet.

- **Checksum Calculation:**
  - Calculates and sets the checksum for both IP and TCP headers to ensure packet integrity.

### Server-Side Functionality
- Listens for incoming SYN packets from the client.
- Responds with a SYN-ACK packet.
- Waits for the final ACK packet to complete the handshake.

## Design Decisions

### Raw Sockets
- **Reason:** Raw sockets allow direct access to network protocols, enabling the construction of custom packets.
- **Implementation:** Used raw sockets to manually construct and send TCP packets.

### Timeout Handling
- **Reason:** Ensures reliability in case of packet loss or server unavailability.
- **Implementation:** Configured a 5-second timeout for receiving packets. Resends the SYN packet if no response is received.

### Checksum Calculation
- **Reason:** Ensures packet integrity and prevents silent handshake failures.
- **Implementation:** Calculated checksums for both IP and TCP headers, including a pseudo-header for TCP checksum calculation.

## Code Flow

1. **Client Initialization:**
   - Creates a raw socket.
   - Configures the socket to include IP headers and sets a timeout.

2. **SYN Packet:**
   - Constructs and sends a SYN packet with sequence number 200.

3. **SYN-ACK Reception:**
   - Waits for the SYN-ACK response from the server.
   - Parses the SYN-ACK packet to extract the server's sequence number.

4. **ACK Packet:**
   - Constructs and sends the final ACK packet with sequence number 600.

5. **Timeout Handling:**
   - If no response is received within 5 seconds, resends the SYN packet.

6. **Handshake Completion:**
   - Prints a success message upon completing the handshake.

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

### Checksum Calculation
- **Challenge:** Implementing checksum calculation for IP and TCP headers.
- **Solution:** Used a pseudo-header for TCP checksum calculation and verified the implementation with test cases.

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

## Usage

### Running the Server
1. **Start the server**:
   - Navigate to the `Assignment_3` directory.
   - Run the following command:
     ```sh
     sudo ./server
     ```
   - The server will start listening for incoming SYN packets on port `12345`.
   - Expected output:
     ```
     [+] Server listening on port 12345...
     ```

### Running the Client
1. **Start the client**:
   - Navigate to the `Assignment_3` directory.
   - Run the following command:
     ```sh
     sudo ./client
     ```
   - The client will initiate the TCP three-way handshake by sending a SYN packet to the server.
   - Expected output:
     ```
     [+] Client started
     [+] Sending SYN to server...
     [+] TCP Flags:  SYN: 1 ACK: 1 FIN: 0 RST: 0 PSH: 0 SEQ: 400
     [+] Received SYN-ACK from server
     [+] Sending ACK to server...
     [+] Handshake complete, ACK sent
     [+] Client exiting
     ```

### Expected Server Output
- When the server receives a SYN packet:
  ```
  [+] TCP Flags:  SYN: 1 ACK: 0 FIN: 0 RST: 0 PSH: 0 SEQ: 200
  [+] Received SYN from 127.0.0.1
  [+] Sent SYN-ACK
  ```
- When the server receives the final ACK packet:
  ```
  [+] TCP Flags:  SYN: 0 ACK: 1 FIN: 0 RST: 0 PSH: 0 SEQ: 600
  [+] Received ACK, handshake complete.
  ```

### Timeout Handling
- If the client does not receive a SYN-ACK response within 5 seconds, it will resend the SYN packet.
- Example output when a timeout occurs:
  ```
  [+] Sending SYN to server...
  [!] Timeout occurred, resending SYN...
  [+] Sending SYN to server...
  ```

### Error Handling
- If the client or server encounters an error (e.g., socket creation failure), an appropriate error message will be displayed.
- Example:
  ```
  Socket creation failed: Permission denied
  ```