#!/bin/bash

# Compile each C++ file with the specified options

# Compile Backdoor-C2.cpp
g++ -o Backdoor-C2.o Backdoor-C2.cpp -lssl -lcrypto
if [ $? -ne 0 ]; then
    echo "Failed to compile Backdoor-C2.cpp"
fi

# Compile DES.cpp
g++ -o DES.o DES.cpp -lssl -lcrypto
if [ $? -ne 0 ]; then
    echo "Failed to compile DES.cpp"
fi

# Compile Discord.cpp
g++ -o Discord.o Discord.cpp -lpthread
if [ $? -ne 0 ]; then
    echo "Failed to compile Discord.cpp"
fi

# Compile GoogleDrive.cpp
g++ -o GoogleDrive.o GoogleDrive.cpp -lcurl -lssl -lcrypto -lpthread
if [ $? -ne 0 ]; then
    echo "Failed to compile GoogleDrive.cpp"
fi

# Compile OneDrive.cpp
g++ -o OneDrive.o OneDrive.cpp -lcurl -lssl -lcrypto -lpthread
if [ $? -ne 0 ]; then
    echo "Failed to compile OneDrive.cpp"
fi

# Compile RSA.cpp
g++ -o RSA.o RSA.cpp -lssl -lcrypto
if [ $? -ne 0 ]; then
    echo "Failed to compile RSA.cpp"
fi

# Compile XOR.cpp
g++ -o XOR.o XOR.cpp -ljsoncpp -I/usr/include/jsoncpp
if [ $? -ne 0 ]; then
    echo "Failed to compile XOR.cpp"
fi

# Compile Dropbox.cpp
g++ Dropbox.cpp -o Dropbox.o `pkg-config --cflags --libs opencv4` -lcurl -lssl -lcrypto -lpthread
if [ $? -ne 0 ]; then
    echo "Failed to compile XOR.cpp"
fi

chmod +x OneDrive.o GoogleDrive.o Dropbox.o Discord.o Backdoor-C2.o XOR.o RSA.o DES.o

echo "Compilation complete."

