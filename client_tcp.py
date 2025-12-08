import socket

client_socket = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
ip = "192.168.56.1"
port = 5000

client_socket.connect((ip,port))
print(f"Client connected to ip: {ip}, port: {port}")

while True:
    message = input("Client: ")
    client_socket.send(message.encode())
    if "bye" in message.lower().split(" "):
        print("Client has ended the connection")
        break

    data = client_socket.recv(1024).decode()
    print(f"Server: {data}")
    if "bye" in data.lower().split(" "):
        print("Server has ended the connection")
        break

client_socket.close()