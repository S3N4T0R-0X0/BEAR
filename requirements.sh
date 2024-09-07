#!/bin/bash

if [ "$(id -u)" != "0" ]; then
  echo '[Error]: You must run this setup script with root privileges.'
  echo
  exit 1
fi

apt-get install -y libldap2-dev libsasl2-dev

cpan IO::Socket::INET MIME::Base64 Crypt::DES File::Slurp Time::HiRes HTTP::Tiny

if ! command -v pip &> /dev/null
then
    echo "pip not found, installing pip..."
    apt-get install -y python3-pip
fi

pip install --user pycryptodome pyautogui pyvirtualdisplay requests secrets cryptography discord.py

echo -e "


\033[0;31m[*]\033[0m" "\033[0;32mDon't forget to install ngrok\033[0m"
