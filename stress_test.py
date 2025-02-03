import socket
import threading
import time
import random
import logging

# Server configuration
SERVER_IP = "127.0.0.1"  # Change if needed
PORT = 12345
MESSAGES_PER_CLIENT = 20
MAX_THREADS_PER_USER = 5  # Gradually increase up to this value

# Logging setup
logging.basicConfig(
    filename="stress_test.log",
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
)

# Metrics
auth_success = 0
auth_failures = 0
total_messages_sent = 0
total_response_time = 0
errors = 0
lock = threading.Lock()

# Load users from users.txt
def load_users(filename="users.txt"):
    users = {}
    try:
        with open(filename, "r") as file:
            for line in file:
                line = line.strip()
                if not line or ":" not in line:
                    continue
                username, password = line.split(":", 1)
                users[username.strip()] = password.strip()
    except FileNotFoundError:
        logging.error("[ERROR] users.txt file not found!")
        exit(1)
    return users

users = load_users()
NUM_CLIENTS = len(users)  # Ensure we only use valid users

def log_metrics():
    """Log final metrics at the end of the test."""
    avg_response_time = total_response_time / total_messages_sent if total_messages_sent else 0
    throughput = total_messages_sent / (NUM_CLIENTS * MESSAGES_PER_CLIENT)

    logging.info("\n--- Stress Test Summary ---")
    logging.info(f"Total Clients: {NUM_CLIENTS}")
    logging.info(f"Total Messages Sent: {total_messages_sent}")
    logging.info(f"Authentication Success: {auth_success}")
    logging.info(f"Authentication Failures: {auth_failures}")
    logging.info(f"Errors Encountered: {errors}")
    logging.info(f"Average Response Time: {avg_response_time:.4f} sec")
    logging.info(f"Message Throughput: {throughput:.2f} messages/sec")


def client_behavior(username, password, thread_id):
    global auth_success, auth_failures, total_messages_sent, total_response_time, errors

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((SERVER_IP, PORT))

        # Receive initial prompt
        sock.recv(1024)
        sock.send(username.encode())

        sock.recv(1024)
        sock.send(password.encode())

        auth_response = sock.recv(1024).decode()
        if "Authentication failed" in auth_response:
            logging.warning(f"[FAILED LOGIN] {username} (Thread-{thread_id})")
            with lock:
                auth_failures += 1
            sock.close()
            return

        logging.info(f"[LOGGED IN] {username} (Thread-{thread_id})")
        with lock:
            auth_success += 1

        group_name = f"group_{random.randint(1, 10)}"

        for _ in range(MESSAGES_PER_CLIENT):
            action = random.choice(["private", "group", "broadcast", "group_action"])
            target_user = random.choice(list(users.keys()))
            message = f"Hello from {username} (Thread-{thread_id})"

            start_time = time.time()

            if action == "private":
                sock.send(f"/msg {target_user} {message}".encode())
            elif action == "group":
                sock.send(f"/group_msg {group_name} {message}".encode())
            elif action == "broadcast":
                sock.send(f"/broadcast {message}".encode())
            elif action == "group_action":
                if random.choice([True, False]):
                    sock.send(f"/create_group {group_name}".encode())
                else:
                    sock.send(f"/join_group {group_name}".encode())

            response = sock.recv(1024).decode()
            end_time = time.time()

            response_time = end_time - start_time
            with lock:
                total_messages_sent += 1
                total_response_time += response_time

            logging.info(
                f"[MESSAGE SENT] {username} (Thread-{thread_id}) -> {action} ({response_time:.4f}s) | Response: {response.strip()}"
            )

            time.sleep(random.uniform(0.1, 1.0))

        # Simulate logout
        sock.send("/logout".encode())
        sock.close()
        logging.info(f"[LOGGED OUT] {username} (Thread-{thread_id})")

    except Exception as e:
        logging.error(f"Error with {username} (Thread-{thread_id}): {e}")
        with lock:
            errors += 1


# Launch stress test
threads = []
for username, password in users.items():
    for thread_id in range(1, MAX_THREADS_PER_USER + 1):
        t = threading.Thread(target=client_behavior, args=(username, password, thread_id))
        threads.append(t)
        t.start()
        time.sleep(0.5)  # Slow ramp-up to prevent overloading

# Wait for all clients to finish
for t in threads:
    t.join()

# Log final metrics
log_metrics()

print("\nStress test completed. Check 'stress_test.log' for details.")