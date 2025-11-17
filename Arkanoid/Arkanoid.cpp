// Arkanoid.cpp : ќпредел€ет точку входа дл€ приложени€.
//

#include "framework.h"
#include "Arkanoid.h"
#include <vector>
#include <cmath>
#include <random>
#include <ctime>
using namespace std;


struct Window // структура окна
{
    HBITMAP hBitmap = (HBITMAP)LoadImageW(NULL, L"background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); // загрузка изображени€ в HBITMAP;
    HWND hWnd;
    float width, height;
} window;

void InitWindow() // процедура инициализации окна
{
    RECT r;
    GetClientRect(window.hWnd, &r);
    window.width = r.right - r.left;
    window.height = r.bottom - r.top;
}

struct Game // структура игры
{
    struct Platform // структура платформы
    {
        HBITMAP hBitmap_begin = (HBITMAP)LoadImageW(NULL, L"platform_begin.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        HBITMAP hBitmap_middle = (HBITMAP)LoadImageW(NULL, L"platform_middle.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        HBITMAP hBitmap_end = (HBITMAP)LoadImageW(NULL, L"platform_end.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        float x, y, section_width, height, width, speed;
        int size, max_size, min_size;

        void SizeUp()
        {
            if (size < max_size)
            {
                size++;
            }
        }

        void SizeDown()
        {
            if (size > min_size)
            {
                size--;
            }
        }
    } platform;

    struct Ball // структура м€ча
    {
        HBITMAP hBitmap = (HBITMAP)LoadImageW(NULL, L"ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        float x, y, height, width, speed, dx, dy;
    };

    vector<Ball> balls; // вектор, с помошью которого будут отслеживатьс€ все м€чи в игре

    void InitGame() // процедура инициализации игры
    {
        platform.size = 5;
        platform.min_size = 1;
        platform.max_size = 9;  
        platform.section_width = 64;
        platform.height = 64;
        platform.width = platform.section_width * platform.size;
        platform.x = window.width / 2 - platform.width / 2;
        platform.y = window.height - platform.height;
        platform.speed = 30;

        Ball new_ball;
        new_ball.height = 64;
        new_ball.width = 64;
        new_ball.x = platform.x + platform.width / 2 - new_ball.width / 2;
        new_ball.y = platform.y - new_ball.height;
        new_ball.speed = 30;
        new_ball.dy = (rand() % 65 + 35) / 100.; // формирование вектора полета м€ча
        new_ball.dx = -(1 - new_ball.dy); // формирование вектора полета м€ча
        balls.push_back(new_ball);      
    }

    struct collision_point // структура точки коллизии
    {
        int x;
        int y;

        collision_point(int x_pos, int y_pos) : x(x_pos), y(y_pos){}
    };

    bool check_collision(pair <collision_point, collision_point>& points_a, pair <collision_point, collision_point>& points_b) // функци€ проверки столкновени€ двух коллизий
    {
        bool intersection_x = false;
        bool intersection_y = false;
        if (points_a.first.x <= points_b.second.x && points_a.second.x >= points_b.first.x)
        {
            intersection_x = true;
        }
        if (points_a.first.y <= points_b.second.y && points_a.second.y >= points_b.first.y)
        {
            intersection_y = true;
        }
        if (intersection_x && intersection_y)
        {
            return true;
        }
        else
        {
            return false;
        }

    }

    void ProcessInput() // процедура обработки ввода
    {
        if (GetAsyncKeyState(VK_LEFT))
        {
            platform.x -= platform.speed;

            if (platform.x < 0) platform.x = 0;
        }

        if (GetAsyncKeyState(VK_RIGHT))
        {
            platform.x += platform.speed;

            if (platform.x > window.width - platform.width) platform.x = window.width - platform.width;
        }
    }

    void ProcessGame() // процедура игрового процесса
    {
        ProcessInput();

        pair<collision_point, collision_point> platform_points = { collision_point(platform.x, platform.y), collision_point(platform.x + platform.width, platform.y + platform.height) }; // точки коллизии платформы

        for (auto& ball : game.balls)
        {
            ball.x -= ball.dx * ball.speed;
            ball.y -= ball.dy * ball.speed;

            pair<collision_point, collision_point> ball_points = { collision_point(ball.x, ball.y), collision_point(ball.x + ball.width, ball.y + ball.height) };

            if (check_collision(platform_points, ball_points) && ball.dy < 0) // направл€ем м€ч вверх при столкновении с платформой
            {   
                ball.dy *= -1; 
            }

            if (ball.x <= 0 || ball.x + ball.width >= window.width) // отскок м€ча от краев экрана
            {
                ball.dx *= -1;
            };

            if (ball.y <= 0) // отскок м€ча от потолка
            {
                ball.dy *= -1;
            };
        }
    }

} game;

void DrawBitmap(HDC hdcDest, int x, int y, int w, int h, HBITMAP hBmp, bool transparent) // процедура отрисовки объекта
{
    if (!hBmp) return;
    HDC hMemDC = CreateCompatibleDC(hdcDest);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);
    BITMAP bmp;
    GetObject(hBmp, sizeof(BITMAP), &bmp);
    
    //if (transparent) //ѕо какой-то причине не работает прозрачность
    //{
    //    TransparentBlt(hdcDest, x, y, w, h, hMemDC, 0, 0, w, h, RGB(0, 0, 0)); // черные пиксели в прозрачные
    //}
    //else
    //{
        StretchBlt(hdcDest, x, y, w, h, hMemDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    //}
    
    SelectObject(hMemDC, hOldBmp);
    DeleteDC(hMemDC);
};

void ShowObjects(HDC hMemDC) // процедура отрисовки всех объектов игры
{
    // м€чи
    for (auto& ball : game.balls)
    {
        DrawBitmap(hMemDC, ball.x, ball.y, ball.width, ball.height, ball.hBitmap,true);
    }

    // платформа
    DrawBitmap(hMemDC, game.platform.x, game.platform.y, game.platform.width, game.platform.height, game.platform.hBitmap_middle, true);
    DrawBitmap(hMemDC, game.platform.x, game.platform.y, game.platform.section_width / 4, game.platform.height, game.platform.hBitmap_begin, true);
    DrawBitmap(hMemDC, game.platform.x + game.platform.width - game.platform.section_width / 4, game.platform.y, game.platform.section_width / 4, game.platform.height, game.platform.hBitmap_end, true);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

    switch (uMsg) {

    case WM_KEYDOWN:
    {
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
        }
        break;
    }

    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }

    case WM_CREATE: 
    {
        if (!window.hBitmap) MessageBoxW(hwnd, L"Ќе удалось загрузить изображение!", L"ќшибка", MB_ICONERROR);

        break;
    }

    case WM_TIMER: // ¬ функции wWinMain есть строка "SetTimer(window.hWnd, 1, 16, NULL);" - таймер с nIDEvent = 1 (второй аргумент)
    {
        if (wParam == 1) 
        {
            InvalidateRect(hwnd, NULL, FALSE);
            game.ProcessGame();
        }
        break;
    }

    case WM_PAINT: // вывод на экран 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 1. —оздаЄм буфер в пам€ти

        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, window.width, window.height);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);

        // 2. ќтрисовываем всЄ в буфер

        if (window.hBitmap) // задний фон
        {
            HDC hBackDC = CreateCompatibleDC(hMemDC);
            HBITMAP hOldBackBmp = (HBITMAP)SelectObject(hBackDC, window.hBitmap);
            BITMAP bmp;
            GetObject(window.hBitmap, sizeof(BITMAP), &bmp);
            StretchBlt(hMemDC, 0, 0, window.width, window.height, hBackDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
            SelectObject(hBackDC, hOldBackBmp);
            DeleteDC(hBackDC);
        }

        ShowObjects(hMemDC); // объекты игры

        // 3.  опирование готового буфера на экран
        BitBlt(hdc, 0, 0, window.width, window.height, hMemDC, 0, 0, SRCCOPY);

        // 4. ќчистка
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
        CW_USEDEFAULT, CW_USEDEFAULT, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), // функци€ узнает размер окна 
        NULL,
        NULL,
        hI,
        NULL
    );

    if (window.hWnd == NULL) return 0;

    srand(time(0));

    InitWindow();

    game.InitGame();

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
