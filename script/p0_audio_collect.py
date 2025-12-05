import socket
import os
import sys

HOST = "0.0.0.0"   # Listen on all local network interfaces
PORT = 8888
MAX_BYTES = 130000
BUFFER_SIZE = 4096

def get_next_filename(data_path):
    n = 1
    datafile = data_path + str(n).zfill(4) + ".pcm"
    while os.path.exists(datafile):
        n += 1
        datafile = data_path + str(n).zfill(4) + ".pcm"
    return datafile

def data_collection(data_path):
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
                filename = get_next_filename(data_path)
                with open(filename, "wb") as f:
                    f.write(data)
                print(f"Saved {len(data)} bytes to {filename}")
            else:
                print("No data received.")

            print("Connection closed.\n")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python p0_audio_collect.py <data_path>")
        sys.exit(1)

    data_collection(sys.argv[1])
