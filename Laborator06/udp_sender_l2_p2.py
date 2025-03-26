import socket
import time

PEER_IP = "192.168.89.10" 
PEER_PORT = 10001         

i = 0
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

while True:
    try:
        if i % 2 == 0:
            TO_SEND = b"GPIO4=1" 
        else:
            TO_SEND = b"GPIO4=0"  

        sock.sendto(TO_SEND, (PEER_IP, PEER_PORT))
        print("Am trimis mesajul: ", TO_SEND)
        i += 1
        time.sleep(1) 

    except KeyboardInterrupt:
        break
