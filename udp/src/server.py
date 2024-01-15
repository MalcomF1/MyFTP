import socket

size = 8192

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('', 9876))

cnt = 0
try:
  while True:
    data, address = sock.recvfrom(size)
    message = str(cnt) + " " + data.decode()
    sock.sendto(message.encode(), address)
    cnt += 1
finally:
  sock.close()
