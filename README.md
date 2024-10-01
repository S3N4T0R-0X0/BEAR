
[![Project Status](https://img.shields.io/badge/status-BETA-yellow?style=flat-square)]()

# BEAR-C2
Bear C2 is a compilation of C2 scripts, payloads, and stagers used in simulated attacks by Russian APT groups,
Bear features a variety of encryption methods, including `AES, XOR, DES, TLS, RC4, RSA and ChaCha` to secure communication between the payload and the operator machine.
This C2 is for simulation only and is still under development.

![imageedit_4_5482104706](https://github.com/S3N4T0R-0X0/Bear/assets/121706460/a43fdb26-c4d6-4b3e-b494-baed4c4b137d)

> [!CAUTION]
> It's essential to note that this project is for educational and research purposes only, and any unauthorized use of it could lead to legal consequences.

## Installation

This project requires some scripts that utilize `ngrok`, so ngrok must be downloaded before completing the installation process.

Install ngrok: https://ngrok.com/download

```
git clone https://github.com/S3N4T0R-0X0/BEAR.git

cd BEAR

chmod +x requirements.sh

sudo ./requirements.sh

cd Profiles

chmod +x compile.sh

./compile.sh

cd ..

python3 Bear.py
```
When the ngrok window appears, press `Control + C` to exit and return to the C2 console.

![photo_2024-07-06_18-31-17](https://github.com/S3N4T0R-0X0/BEAR/assets/121706460/c552fcb6-3440-44e5-81d2-f49dafbcbc42)


## POC For Customization TTPs

The types of connection encryption for the C2 profiles in Bear were chosen based on the encryption methods used by the attackers in the actual attacks.

![imageedit_3_7181989344](https://github.com/user-attachments/assets/39bbbe75-1d04-4c22-80c1-beed436e6f7f)

________________________________________________________________________________________________________

Шахимат / Checkmate: This payload execution technique uses a Fake Windows SmartScreen, requiring Administrator privileges and disabling the real SmartScreen Filter. The application includes functionalities to run shellcode and execute specific actions based on user input. This execution technique has been redeveloped and rewritten in C++ to easily integrate evasion techniques.

Buttons Activity:

     - "Don't Run": Launches the payload from any directory, such as C:\\Windows\\System32\\payload.exe

     - "Run Anyway": Executes a predefined shellcode

     - "❌️": Closes the warning window and opens any URL, like https://t.me/BearC2

Hidden Functionality:

- SmartScreen Bypass: The code attempts to disable SmartScreen by modifying registry settings.

- Administrator Check: If the application is not running with administrative privileges, it attempts to relaunch itself with elevated rights.




https://github.com/user-attachments/assets/bbdd8354-b7c0-4955-88dd-294d4bbabf15


________________________________________________________________________________________________________


Кинжал / Kinzhal: This payload performs several actions. It first checks if it has administrator privileges, and if not, it requests them. It then attempts to bypass User Account Control (UAC) and disables security features like SmartScreen and Windows Defender by modifying registry settings and disabling scheduled tasks. The payload clears system and security event logs to cover its tracks. It sets up a network connection to a remote server to receive commands and send data. The payload retrieves the machine's unique identifier, calculates its CRC32 checksum, and sends this information to the server. It can execute system and PowerShell commands received from the server, with results either sent back over the network or uploaded to `OneDrive, Google Drive, Dropbox, or AWS` via an API token. Additionally, it uses process hollowing to inject its payload into a legitimate system process (svchost.exe), ensuring it runs hidden in the background. Finally, the payload hides its console window to avoid detection by the user. This combination of techniques makes the payload capable of evading detection, maintaining persistence, performing remote command execution, and exfiltrating data to multiple cloud platforms.


![photo_2024-07-26_16-47-49](https://github.com/user-attachments/assets/68e1b851-bc6e-4faa-a1c7-d712106de9d4)


