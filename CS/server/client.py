import socket
import time

info = ('172.20.10.4', 8848)


def main():
    global info
    client_s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    while 1:
        data=input()
        client_s.sendto(data.encode("gb2312"),info)
        TempData = client_s.recvfrom(1024)
        print(TempData)
        client_s.close()


if __name__ == '__main__':
    main()
