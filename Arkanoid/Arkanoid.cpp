// Arkanoid.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Arkanoid.h"


struct // структура для окна
{
    HBITMAP hBitmap;
    HWND hWnd;
    int width, height;
} window;


void InitWindow()
{
    RECT r;
    GetClientRect(window.hWnd, &r);
    window.width = r.right - r.left;
    window.height = r.bottom - r.top;
}

void DrawBitmap(HDC hdcDest, int x, int y, int w, int h, HBITMAP hBmp, bool transparent)
{
    if (!hBmp) return;
    HDC hMemDC = CreateCompatibleDC(hdcDest);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);
    BITMAP bmp;
    GetObject(hBmp, sizeof(BITMAP), &bmp);

    StretchBlt(hdcDest, x, y, w, h, hMemDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    SelectObject(hMemDC, hOldBmp);
    DeleteDC(hMemDC);
};

void ShowObjects(HDC hMemDC)
{
    //Код отрисовки объектов
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

    case WM_KEYDOWN: // нажатие клавиш 
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY: // уничтожение
    {
        PostQuitMessage(0);
        return 0;
    }

    case WM_CREATE: // создание окна 
    {

        window.hBitmap = (HBITMAP)LoadImageW(NULL, L"background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); // загрузка изображения в HBITMAP 

        if (!window.hBitmap) MessageBoxW(hwnd, L"Не удалось загрузить изображение!", L"Ошибка", MB_ICONERROR);

        break;
    }

    case WM_PAINT: // вывод на экран 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 1. Создаём буфер в памяти
        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, window.width, window.height);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);

        // 2. Рисуем всё в буфер

        if (window.hBitmap) // фон
        {
            HDC hBackDC = CreateCompatibleDC(hMemDC);
            HBITMAP hOldBackBmp = (HBITMAP)SelectObject(hBackDC, window.hBitmap);
            BITMAP bmp;
            GetObject(window.hBitmap, sizeof(BITMAP), &bmp);
            StretchBlt(hMemDC, 0, 0, window.width, window.height, hBackDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
            SelectObject(hBackDC, hOldBackBmp);
            DeleteDC(hBackDC);
        }

        ShowObjects(hMemDC); // объекты


        // 3. Копирование готового буфера на экран
        BitBlt(hdc, 0, 0, window.width, window.height, hMemDC, 0, 0, SRCCOPY);

        // 4. Очистка
        SelectObject(hMemDC, hOldBmp);
        DeleteObject(hMemBmp);
        DeleteDC(hMemDC);
        EndPaint(hwnd, &ps);
    }

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hI, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"Arkanoid";

    WNDCLASS wc = { };

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hI;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    window.hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Arkanoid",
        WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), // функция узнает размер окна 
        NULL,
        NULL,
        hI,
        NULL
    );

    if (window.hWnd == NULL) return 0;

    InitWindow();

    ShowWindow(window.hWnd, nCmdShow);

    SetTimer(window.hWnd, 1, 16, NULL);

    MSG msg = { };

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
