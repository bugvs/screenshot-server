#include "../include/httplib.h"
#include "./screenshot.h"
#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include <comdef.h>
#include <thread>
#include <atomic>
#include <string>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdiplus.lib")

// Глобальные переменные
NOTIFYICONDATA nid = {};
HWND hwnd = NULL;
HMENU hMenu = NULL;
std::atomic<bool> serverRunning{true};
httplib::Server* svrPtr = nullptr;

// Конвертация строк в LPCWSTR
LPCWSTR ToLPCWSTR(const std::string& s) {
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, buf, len);
    return buf;
}

// Оконная процедура
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        
        case WM_USER + 1: // Обработка событий иконки в трее
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            }
            return 0;
        
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // ID пункта "Exit"
                serverRunning = false;
                if (svrPtr) svrPtr->stop();
                Shell_NotifyIcon(NIM_DELETE, &nid);
                PostQuitMessage(0);
            }
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Функция инициализации окна
HWND InitWindow(HINSTANCE hInstance) {
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"ScreenshotServerClass";
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONERROR);
        return NULL;
    }
    
    return CreateWindowEx(0, L"ScreenshotServerClass", L"Screenshot Server", 0, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);
}

// Функция инициализации иконки в трее
bool InitTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_USER + 1;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); // Стандартная иконка
    
    // Преобразование строки в Unicode
    std::wstring tip = L"Screenshot Server";
    wcscpy_s(nid.szTip, tip.c_str());
    
    hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 1, L"Exit");
    
    return Shell_NotifyIcon(NIM_ADD, &nid);
}

// Функция HTTP-сервера
void RunHttpServer() {
    httplib::Server svr;
    svrPtr = &svr;
    
    svr.Get("/screenshot", [](const httplib::Request& req, httplib::Response& res) {
        try {
            HBITMAP hBitmap = TakeScreenshot();
            if (!hBitmap) throw std::runtime_error("Failed to capture screenshot");
            
            IStream* pStream = nullptr;
            if (CreateStreamOnHGlobal(NULL, TRUE, &pStream) != S_OK) {
                DeleteObject(hBitmap);
                throw std::runtime_error("Failed to create stream");
            }
            
            if (!SaveHBITMAPToPNGStream(hBitmap, pStream)) {
                pStream->Release();
                DeleteObject(hBitmap);
                throw std::runtime_error("Failed to save screenshot");
            }
            
            STATSTG stat;
            if (pStream->Stat(&stat, STATFLAG_NONAME) != S_OK) {
                pStream->Release();
                DeleteObject(hBitmap);
                throw std::runtime_error("Failed to get stream size");
            }
            
            LARGE_INTEGER li = {0};
            pStream->Seek(li, STREAM_SEEK_SET, NULL);
            
            std::vector<char> buffer(stat.cbSize.QuadPart);
            ULONG bytesRead = 0;
            if (pStream->Read(buffer.data(), static_cast<ULONG>(buffer.size()), &bytesRead) != S_OK) {
                pStream->Release();
                DeleteObject(hBitmap);
                throw std::runtime_error("Failed to read stream");
            }
            
            res.set_content(buffer.data(), bytesRead, "image/png");
            res.set_header("Content-Disposition", "attachment; filename=screenshot.png");
            
            pStream->Release();
            DeleteObject(hBitmap);
        } catch (const std::exception& e) {
            res.status = 500;
            res.set_content(e.what(), "text/plain");
        }
    });
    
    std::cout << "Server running at http://localhost:5000\n";
    svr.listen("0.0.0.0", 5000);
}

// Точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    hwnd = InitWindow(hInstance);
    if (!hwnd) return 1;
    
    if (!InitTrayIcon(hwnd)) {
        MessageBox(NULL, L"Failed to create tray icon!", L"Error", MB_ICONERROR);
        return 1;
    }
    
    std::thread serverThread(RunHttpServer);
    serverThread.detach();
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) && serverRunning) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    DestroyMenu(hMenu);
    DestroyWindow(hwnd);
    return 0;
}