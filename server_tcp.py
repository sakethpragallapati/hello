import socket

server_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
host = "0.0.0.0"
port = 5000

server_socket.bind((host,port))
server_socket.listen(1)

print(f"Server connected to host: {host}, port: {port}")

conn,addr = server_socket.accept()

while True:
    data = conn.recv(1024).decode()
    print(f"Client: {data}")
    if "bye" in data.lower().split(" "):
        print("Client has ended the connection")
        break

    message = input("Server: ")
    conn.send(message.encode())
    if "bye" in message.lower().split(" "):
        print("Server has ended the connection")
        break

conn.close()
server_socket.close()