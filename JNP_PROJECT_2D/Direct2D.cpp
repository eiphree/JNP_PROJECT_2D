#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <windowsx.h>
#include "scene.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    if (RegisterClass(&wc) == 0) {
        return 1;
    }

    HWND hwnd = CreateWindowEx(
        0,                              
        CLASS_NAME,                     
        TEXT("Spotkanie Herbaciane"),    
        WS_OVERLAPPEDWINDOW | WS_MAXIMIZE,            

        0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),

        NULL,          
        NULL,      
        hInstance,  
        NULL        
    );

    if (hwnd == NULL)
    {
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { };
    BOOL ret;

    while ((ret = GetMessage(&msg, NULL, 0, 0)) != 0)
    {
        if (ret == -1) {
            return 1;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UINT_PTR IDT_TIMER1 = 1;
    switch (uMsg)
    {
    case WM_CREATE:
    {
        InitDirect2D(hwnd);
        SetTimer(hwnd, IDT_TIMER1, 10, (TIMERPROC)NULL);
        return 0;
    }
    case WM_DESTROY:
    {
        DestroyDirect2D();
        PostQuitMessage(0);
        KillTimer(hwnd, IDT_TIMER1);
        return 0;
    }
    case WM_PAINT:
    {
        OnPaint(hwnd, 0, -1., -1.);
        ValidateRect(hwnd, nullptr);
        return 0;
    }
    case WM_TIMER:
    {
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_LBUTTONDOWN:
    {
        OnPaint(hwnd, 1, -1., -1.);
        ValidateRect(hwnd, nullptr);
        return 0;
    }
    case WM_LBUTTONUP:
    {
        OnPaint(hwnd, -1, -1., -1.);
        ValidateRect(hwnd, nullptr);
        return 0;
    }
    case WM_MOUSEMOVE: {
        FLOAT xPos = GET_X_LPARAM(lParam);
        FLOAT yPos = GET_Y_LPARAM(lParam);
        OnPaint(hwnd, 0, xPos, yPos);
        ValidateRect(hwnd, nullptr);
        return 0;
    }
    return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
