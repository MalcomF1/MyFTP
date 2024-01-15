import socket
 
size = 8192
 
try:
  msg = input()
  msg = str(msg).encode()
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  for i in range(0, 51):
      sock.sendto(msg, ('localhost', 9876))
      print(sock.recv(size).decode())
  sock.close()
 
except:
  print("cannot reach the server")
