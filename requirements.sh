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


echo "Compilation script executed."
