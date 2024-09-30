#!/bin/bash

# Update package list
sudo apt update

# Install required packages
sudo apt-get install -y \
    libcurl4-openssl-dev \
    libcrypto++-dev \
    libopencv-dev \
    libssl-dev \
    nlohmann-json3-dev \
    libjsoncpp-dev

# Optional: Install g++ if not already installed
sudo apt-get install -y g++

echo "All required packages have been installed."

# Navigate to Profiles directory
cd ~/BEAR-C2/Profiles

# Make compile.sh executable
chmod +x compile.sh

# Run compile.sh
./compile.sh

echo "Compilation script executed."
