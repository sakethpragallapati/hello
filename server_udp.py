import socket

server_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
host = "0.0.0.0"
port = 6000

server_socket.bind((host,port))
print(f"Server connected to host: {host}, port: {port}")

while True:
    encoded_data,addr = server_socket.recvfrom(1024)
    data = encoded_data.decode()
    print(f"Client: {data}")
    if "bye" in data.lower().split(" "):
        print("Client has ended the connection")
        break

    message = input("Server: ")
    server_socket.sendto(message.encode(),addr)
    if "bye" in message.lower().split(" "):
        print("Server has ended the connection")
        break