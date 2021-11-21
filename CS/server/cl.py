import socket
import time
import threading

class sock_get_send():
    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr
        self.online = True
        self.msg = ''
        self.t = threading.Thread(target=self.get_msg, args=())
        self.t1 = threading.Thread(target=self.send_msg, args=())
        self.t2 = threading.Thread(target=self.sock_online, args =())
        self.to_process()

    def sock_online(self):
        while self.online:
            time.sleep(0.2)
        self.online = False
        self.sock.close()

    def to_process(self):
        print("{}连接成功".format(self.addr))
        self.t.start()
        self.t1.start()
        self.t2.start()
        self.t.join()
        self.t1.join()
        self.t2.join()
        print("{}已断开连接".format(self.addr))

    def send_msg(self):
        while self.online:
            try:
                msg = input().encode('utf-8')
                if not self.online:
                    return
                if msg == '1':
                    return
                self.sock.send(msg)
                time.sleep(0.2)
            except:
                self.online = False

    def get_msg(self):
        while self.online:
            try:
                self.msg = self.sock.recv(1024).decode('utf-8')
                if self.msg[0] in ['#', '@', '!']:
                    self.judge()
                else:
                    print(self.msg)
                time.sleep(0.2)
            except:
                self.online = False

    def judge(self):
        if self.msg[0] in ['#']:
            temp = self.msg[1:3]
            self.msg = "\033[" + temp +"m"+ self.msg[4:] + "\033[0m"
            print(str(self.addr[1]) + ': ', end='')
            print(self.msg)
        elif (self.msg[0] in ['@']):
            temp = self.msg[1:3]
            self.msg = "\033[47;" + temp + "m"+ self.msg[4:] + "\033[0m"
            print(str(self.addr[1]) + ': ', end='')
            print(self.msg)
        else:
            print("bye-bye")
            self.sock.send("bye-bye".encode())
            self.online = False

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    info = ('172.20.10.4', 8847 )
    con = 1
    while con:
        try:
            s.connect(info)
            break
        except:
            pass
            print("第 %d 次连接中......"%con)
            con +=1
            time.sleep(1)
    sock_get_send(s,info)

if __name__ == '__main__':
    main()
