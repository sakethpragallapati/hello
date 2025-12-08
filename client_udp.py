import socket

client_socket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
ip = "192.168.56.1"
port = 6000

print(f"Client connected to ip: {ip}, port: {port}")

while True:
    message = input("Client: ")
    client_socket.sendto(message.encode(),(ip,port))
    if "bye" in message.lower().split(" "):
        print("Client has ended the connection")
        break

    encoded_data,addr = client_socket.recvfrom(1024)
    data = encoded_data.decode()
    print(f"Server: {data}")
    if "bye" in data.lower().split(" "):
        print("Server has ended the connection")
        break