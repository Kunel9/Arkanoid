// Arkanoid.cpp : Определяет точку входа для приложения.
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
    HBITMAP hBitmap = (HBITMAP)LoadImageW(NULL, L"background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); // загрузка изображения в HBITMAP;
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

    struct Ball // структура мяча
    {
        HBITMAP hBitmap = (HBITMAP)LoadImageW(NULL, L"ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
        float x, y, height, width, speed, dx, dy;
    };

    struct Block // структура блока
    {
        HBITMAP hBitmaps[3] = {
        (HBITMAP)LoadImageW(NULL, L"block_yellow.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE),
        (HBITMAP)LoadImageW(NULL, L"block_orange.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE),
        (HBITMAP)LoadImageW(NULL, L"block_red.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE) };

        float x, y, height, width;
        int endurance;
    };

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
                width = section_width * size;
                x = x - section_width / 2;
            }
        }

        void SizeDown()
        {
            if (size > min_size)
            {
                size--;
                width = section_width * size;
                x = x + section_width / 2;
            }
        }
    } platform;

    enum BonusTypes // перечисление типов бонусов
    {
        bonus_add_ball,
        bonus_size_up,
        bonus_size_down
    };

    struct Bonus // структура бонуса
    {
        HBITMAP hBitmap;
        BonusTypes type;
        float x, y, height, width, speed;
        bool active;

        Bonus()
        {
            active = true;

            if (rand() % 100 < 50) // случайное определение типа создаваемого бонуса
            {   
                BonusTypes positive_bonuses[] = { bonus_add_ball , bonus_size_up }; // позитивные бонусы
                type = positive_bonuses[rand() % size(positive_bonuses)];
            }
            else 
            {              
                BonusTypes negative_bonuses[] = { bonus_size_down }; // негативные бонусы
                type = negative_bonuses[rand() % size(negative_bonuses)];
            }
            
            // загрузка изображения бонуса
            if (type == bonus_add_ball)
            {
                hBitmap = (HBITMAP)LoadImageW(NULL, L"bonus_add_ball.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            }
            else if (type == bonus_size_up)
            {
                hBitmap = (HBITMAP)LoadImageW(NULL, L"bonus_size_up.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            }   
            else if (type == bonus_size_down)
            {
                hBitmap = (HBITMAP)LoadImageW(NULL, L"bonus_size_down.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            }
        }

        void activate(Game& game_link) // процедура активации бонуса
        {   
            if (active)
            {
                active = false;

                if (type == bonus_add_ball)
                {     
                    game_link.AddBall();
                }
                else if (type == bonus_size_up)
                {
                    game_link.platform.SizeUp();
                }
                else if (type == bonus_size_down)
                {
                    game_link.platform.SizeDown();
                }
            }
        }
    };

    vector<Ball> balls; // вектор для отслеживания мячей 
    vector<vector<Block>> blocks; // двумерный вектор для отслеживания блоков
    vector<Bonus> bonuses; // вектор для отслеживания бонусов
    
    void InitGame() // процедура инициализации игры
    {
        // инициализация платформы
        platform.size = 5;
        platform.min_size = 1;
        platform.max_size = 32;  
        platform.section_width = 32;
        platform.height = 32;
        platform.width = platform.section_width * platform.size;
        platform.x = window.width / 2 - platform.width / 2;
        platform.y = window.height - platform.height;
        platform.speed = 16;

        // инициализация стартового мяча
        AddBall();    

        // инициализация блоков
        for (int row = 1; row < 7; row++) 
        {
            vector<Block> blocks_row;
            for (int col = 1; col < 15; col++) 
            {
                Block new_block;
                new_block.width = 64;
                new_block.height = 64;
                new_block.endurance = 1 + rand() % 3;
                new_block.x = col * new_block.width;
                new_block.y = row * new_block.height;
                blocks_row.push_back(new_block);
            }
            blocks.push_back(blocks_row);
        }
    }

    struct Point // структура точки
    {
        float x;
        float y;

        Point(float x_pos, float y_pos) : x(x_pos), y(y_pos){}
    };

    bool CheckCollision(pair <Point, Point>& points_a, pair <Point, Point>& points_b) // функция проверки столкновения двух коллизий
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
    
    void CreateBonus(float x, float y) // процедура создания бонуса
    {
        Bonus new_bonus;
        new_bonus.width = 32;
        new_bonus.height = 32;
        new_bonus.x = x - new_bonus.width / 2;
        new_bonus.y = y - new_bonus.height / 2;
        new_bonus.speed = 6;
        bonuses.push_back(new_bonus);
    }

    void AddBall() // процедура добавления мяча
    {
        Ball new_ball;
        new_ball.height = 32;
        new_ball.width = 32;
        new_ball.x = platform.x + platform.width / 2 - new_ball.width / 2;
        new_ball.y = platform.y - new_ball.height;
        new_ball.speed = 16;
        new_ball.dy = (20 + rand() % 60) / 100.;
        new_ball.dx = ((rand() % 2 == 1) ? 1 : -1) * (1 - new_ball.dy);
        balls.push_back(new_ball);
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

        pair<Point, Point> platform_points = { Point(platform.x, platform.y), Point(platform.x + platform.width, platform.y + platform.height) }; // точки коллизии платформы

        for (auto& ball : balls) // обработка мячей
        {
            ball.x -= ball.dx * ball.speed;
            ball.y -= ball.dy * ball.speed;

            pair<Point, Point> ball_points = { Point(ball.x , ball.y), Point(ball.x + ball.width, ball.y + ball.height) }; // точки коллизии мяча

            if (CheckCollision(platform_points, ball_points) && ball.dy < 0) // отскок мяча от платформы
            {
                ball.dy *= -1;
            }

            if (ball.x <= 0 || ball.x + ball.width >= window.width) // отскок мяча от краев экрана
            {
                ball.dx *= -1;
            };

            if (ball.y <= 0) // отскок мяча от потолка
            {
                ball.dy *= -1;
            };

            // обработка столкновения мяча с блоками
            bool collision_processed = false;
            for (int row = 0; row < blocks.size(); row++)
            {
                for (int col = 0; col < blocks[row].size(); col++)
                {
                    if (blocks[row][col].endurance <= 0) continue;

                    pair<Point, Point> block_points = { Point(blocks[row][col].x, blocks[row][col].y), Point(blocks[row][col].x + blocks[row][col].width, blocks[row][col].y + blocks[row][col].height) }; // точки коллизии блока
                    if (CheckCollision(block_points, ball_points))
                    {
                        Point ball_center{ ball.x + ball.width / 2, ball.y + ball.height / 2 }; // центр мяча
                        Point block_center{ blocks[row][col].x + blocks[row][col].width / 2, blocks[row][col].y + blocks[row][col].height / 2 }; // центр блока

                        if (abs(ball_center.x - block_center.x) > abs(ball_center.y - block_center.y))
                        {
                            ball.dx *= -1;
                        }
                        else if (abs(ball_center.x - block_center.x) < abs(ball_center.y - block_center.y))
                        {
                            ball.dy *= -1;
                        }
                        else
                        {
                            ball.dx *= -1;
                            ball.dy *= -1;
                        }

                        blocks[row][col].endurance--;

                        if (blocks[row][col].endurance <= 0) // создание бонуса при разрушении блока
                        {
                            if (rand() % 100 < 50) CreateBonus(block_center.x, block_center.y);
                        }

                        collision_processed = true; // фиксация столкновения в текущем кадре

                        break; // выход из цикла обработки столкновения (по колонкам)
                    }
                }

                if (collision_processed) break; // если было зафиксировано столкновения в текущем кадре - выход из цикла обработки столкновения (по строкам);
            }
        }

        for (auto& bonus : bonuses) // обработка бонусов
        {
            bonus.y += bonus.speed;

            pair<Point, Point> bonus_points = { Point(bonus.x, bonus.y), Point(bonus.x + bonus.width, bonus.y + bonus.height) }; // точки коллизии бонуса

            if (CheckCollision(platform_points, bonus_points))
            {
                bonus.activate(game);
            }
        }

        // удаление лишних бонусов
        for (int i = 0; i < bonuses.size(); )
        {
            if (bonuses[i].y > window.height)
            {
                bonuses.erase(bonuses.begin() + i);
            }
            else
            {
                ++i;
            }
        }
        
        // удаление лишних мячей
        for (int i = 0; i < balls.size(); )
        {
            if (balls[i].y > window.height)
            {
                balls.erase(balls.begin() + i);
            }
            else
            {
                ++i;
            }
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
    
    //if (transparent) // по какой-то причине не работает прозрачность
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
    // блоки
    for (int row = 0; row < game.blocks.size(); row++) 
    {
        for (int col = 0; col < game.blocks[row].size(); col++)
        {
            if (game.blocks[row][col].endurance > 0)
            {
                DrawBitmap(hMemDC, game.blocks[row][col].x, game.blocks[row][col].y, game.blocks[row][col].width, game.blocks[row][col].height, game.blocks[row][col].hBitmaps[game.blocks[row][col].endurance - 1], false);
            }
        }
    }

    // бонусы
    for (auto& bonus : game.bonuses)
    {
        if (bonus.active) DrawBitmap(hMemDC, bonus.x, bonus.y, bonus.width, bonus.height, bonus.hBitmap, false);
    }

    // мячи
    for (auto& ball : game.balls)
    {
        DrawBitmap(hMemDC, ball.x, ball.y, ball.width, ball.height, ball.hBitmap, true);
    }

    // платформа
    DrawBitmap(hMemDC, game.platform.x, game.platform.y, game.platform.width, game.platform.height, game.platform.hBitmap_middle, false);
    DrawBitmap(hMemDC, game.platform.x, game.platform.y, game.platform.section_width / 4, game.platform.height, game.platform.hBitmap_begin, false);
    DrawBitmap(hMemDC, game.platform.x + game.platform.width - game.platform.section_width / 4, game.platform.y, game.platform.section_width / 4, game.platform.height, game.platform.hBitmap_end, false);
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
        if (!window.hBitmap) MessageBoxW(hwnd, L"Не удалось загрузить изображение!", L"Ошибка", MB_ICONERROR);

        break;
    }

    case WM_TIMER: // В функции wWinMain есть строка "SetTimer(window.hWnd, 1, 16, NULL);" - таймер с nIDEvent = 1 (второй аргумент)
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

        // 1. Создаём буфер в памяти

        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, window.width, window.height);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);

        // 2. Отрисовываем всё в буфер

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

    int screenWidth = GetSystemMetrics(SM_CXSCREEN); // получение размеров экрана
    int screenHeight = GetSystemMetrics(SM_CYSCREEN); // получение размеров экрана
    
    // Задаем размеры окна
    int windowWidth = 1024 + 17; // размер окна
    int windowHeight = 832 + 6; // размер окна
    
    // вычисление координат для центрирования окна
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;
    
    window.hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Arkanoid",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        windowX, windowY,           // Позиция окна (центр экрана)
        windowWidth, windowHeight,  // Размеры окна
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
