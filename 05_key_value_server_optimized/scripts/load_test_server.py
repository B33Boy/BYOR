import socket
import threading
import time

NUM_THREADS = 1
NUM_CONNECTIONS = 100

HOST = "127.0.0.1"
PORT = 1234


def run_thread(thread_id: int) -> None:
    for i in range(NUM_CONNECTIONS):
        send_message(thread_id, i)


def send_message(thread_id: int, client_id: int) -> None:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
            sock.connect((HOST, PORT))
            message = f"Hello from client {client_id}"
            print(f"sending message for client {client_id}")
            sock.sendall(message.encode())
            resp = sock.recv(1024)
            print(f"Client {client_id} received: {resp.decode()}")
            time.sleep(10)

    except Exception as e:
        print(f"Client {client_id} error: {e}")


if __name__ == "__main__":
    threads = []

    for i in range(NUM_THREADS):
        thread = threading.Thread(target=run_thread, args=(i,))
        threads.append(thread)
        thread.start()

    for thread in threads:
        thread.join()
