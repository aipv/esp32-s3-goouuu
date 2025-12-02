import socket
import os

HOST = "0.0.0.0"   # Listen on all local network interfaces
PORT = 8888
MAX_BYTES = 130000
BUFFER_SIZE = 4096

def get_next_filename():
    n = 1
    while os.path.exists(f"{n}.wav"):
        n += 1
    return f"{n}.wav"

def main():
    print(f"Starting TCP server on port {PORT}...")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST, PORT))
        server.listen(5)

        while True:
            print("Waiting for connection...")
            conn, addr = server.accept()
            print(f"Connected from {addr}")

            data = bytearray()

            with conn:
                while True:
                    chunk = conn.recv(BUFFER_SIZE)
                    if not chunk:
                        break

                    remaining = MAX_BYTES - len(data)
                    if remaining <= 0:
                        break

                    data.extend(chunk[:remaining])

            if data:
                filename = get_next_filename()
                with open(filename, "wb") as f:
                    f.write(data)
                print(f"Saved {len(data)} bytes to {filename}")
            else:
                print("No data received.")

            print("Connection closed.\n")

if __name__ == "__main__":
    main()
