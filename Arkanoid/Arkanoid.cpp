// Arkanoid.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Arkanoid.h"
#include <vector>
#include <cmath>
#include <random>
#include <ctime>
#pragma comment(lib, "msimg32.lib")
using namespace std;

struct Window // структура окна
{
    HBITMAP hBitmap = (HBITMAP)LoadImageW(NULL, L"background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); // загрузка изображения в HBITMAP;
    HWND hWnd;
    float width, height;
} window;

struct Point // структура точки
{
    float x;
    float y;

    Point(float x_pos, float y_pos) : x(x_pos), y(y_pos) {}
};

void InitWindow() // процедура инициализации окна
{
    RECT r;
    GetClientRect(window.hWnd, &r);
    window.width = r.right - r.left;
    window.height = r.bottom - r.top;
}

enum class BonusTypes // перечисление типов бонусов
{
    add_ball,
    size_up,
    size_down
};

enum class GameStatuses // перечисление типов бонусов
{
    wait,
    process,
    defeat,
    win
};

HBITMAP LoadBmp(LPCWSTR bmp_name)
{
    return (HBITMAP)LoadImageW(NULL, bmp_name, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
}

struct Game // структура игры
{
    struct Ball // структура мяча
    {
        HBITMAP hBitmap = nullptr;
        float x, y, height, width, speed, dx, dy;

        Ball()
        {
            hBitmap = LoadBmp(L"ball.bmp");
        }
    };

    struct Block // структура блока
    {
        HBITMAP hBitmaps[3] = { nullptr };

        Block()
        {
            hBitmaps[0] = LoadBmp(L"block_yellow.bmp");
            hBitmaps[1] = LoadBmp(L"block_orange.bmp");
            hBitmaps[2] = LoadBmp(L"block_red.bmp");
        }

        float x, y, height, width;
        int endurance;
    };

    struct Platform // структура платформы
    {
        HBITMAP hBitmap_begin = nullptr;
        HBITMAP hBitmap_middle = nullptr;
        HBITMAP hBitmap_end = nullptr;
        float x, y, section_width, height, width, speed;
        int size, max_size, min_size;

        Platform()
        {
            hBitmap_begin = LoadBmp(L"platform_begin.bmp");
            hBitmap_middle = LoadBmp(L"platform_middle.bmp");
            hBitmap_end = LoadBmp(L"platform_end.bmp");
        }

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

    struct Bonus // структура бонуса
    {
        HBITMAP hBitmap = nullptr;
        BonusTypes type;
        float x, y, height, width, speed;
        bool active;

        Bonus()
        {
            active = true;

            if (rand() % 100 < 50) // случайное определение типа создаваемого бонуса
            {
                BonusTypes positive_bonuses[] = { BonusTypes::add_ball , BonusTypes::size_up }; // позитивные бонусы
                type = positive_bonuses[rand() % size(positive_bonuses)];
            }
            else
            {
                BonusTypes negative_bonuses[] = { BonusTypes::size_down }; // негативные бонусы
                type = negative_bonuses[rand() % size(negative_bonuses)];
            }

            load_HBitmap();
        }

        void load_HBitmap()
        {
            LPCWSTR name_bmp = nullptr;

            switch (type)
            {
            case BonusTypes::add_ball:
                name_bmp = L"bonus_add_ball.bmp";
                break;
            case BonusTypes::size_up:
                name_bmp = L"bonus_size_up.bmp";
                break;
            case BonusTypes::size_down:
                name_bmp = L"bonus_size_down.bmp";
                break;
            default:
                return;
            }

            hBitmap = LoadBmp(name_bmp);
        }

        void activate(Game& game_link) // процедура активации бонуса
        {
            if (active)
            {
                active = false;

                switch (type)
                {
                case BonusTypes::add_ball:
                {
                    game_link.AddBall();
                    break;
                }
                case BonusTypes::size_up:
                {
                    game_link.platform.SizeUp();
                    break;
                }
                case BonusTypes::size_down:
                {
                    game_link.platform.SizeDown();
                    break;
                }
                default:
                    return;
                }
            }
        }
    };

    HBITMAP hBitmap_defeat = nullptr;
    HBITMAP hBitmap_win = nullptr;
    vector<Ball> balls; // вектор для отслеживания мячей 
    vector<vector<Block>> blocks; // двумерный вектор для отслеживания блоков
    vector<Bonus> bonuses; // вектор для отслеживания бонусов
    GameStatuses status;

    Game()
    {
        hBitmap_defeat = LoadBmp(L"defeat.bmp");
        hBitmap_win = LoadBmp(L"win.bmp");
        status = GameStatuses::wait;
    }

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

    void ProcessBlocks(Ball& ball, pair <Point, Point> ball_points)
    {
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

    void ProcessBalls(pair<Point, Point> platform_points)
    {
        for (auto& ball : balls)
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

            ProcessBlocks(ball, ball_points);
        }
    }

    void ProcessBonuses(pair<Point, Point> platform_points)
    {
        for (auto& bonus : bonuses)
        {
            bonus.y += bonus.speed;

            pair<Point, Point> bonus_points = { Point(bonus.x, bonus.y), Point(bonus.x + bonus.width, bonus.y + bonus.height) }; // точки коллизии бонуса

            if (CheckCollision(platform_points, bonus_points))
            {
                bonus.activate(game);
            }
        }
    }

    void CleanBonuses()
    {
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
    }

    void CleanBalls()
    {
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

    void Cleaning()
    {
        CleanBonuses();
        CleanBalls();
    }

    void ChekDefeatCondition()
    {
        if (game.balls.size() == 0)
        {
            if (game.bonuses.size() > 0)
            {
                bool bonus_found = false;

                for (auto& bonus : bonuses)
                {
                    if (bonus.type == BonusTypes::add_ball)
                    {
                        bonus_found = true;
                        break;
                    }
                }

                if (!bonus_found) game.status = GameStatuses::defeat;
            }
            else
            {
                game.status = GameStatuses::defeat;
            }
        }
    }

    void ChekWinCondition()
    {
        bool block_found = false;

        for (int row = 0; row < blocks.size(); row++)
        {
            for (int col = 0; col < blocks[row].size(); col++)
            {
                if (blocks[row][col].endurance > 0)
                {
                    block_found = true;
                    break;
                }
            }

            if (block_found) break;            
        }

        if (!block_found) game.status = GameStatuses::win;
    }

    void UpdateGameStatus()
    {
        ChekDefeatCondition();
        ChekWinCondition();
    }

    void ProcessGame() // процедура игрового процесса
    {
        pair<Point, Point> platform_points = { Point(platform.x, platform.y), Point(platform.x + platform.width, platform.y + platform.height) }; // точки коллизии платформы

        ProcessInput();

        ProcessBalls(platform_points);
        ProcessBonuses(platform_points);

        Cleaning();

        UpdateGameStatus();
    }
} game;

void DrawBitmap(HDC hdcDest, int x, int y, int w, int h, HBITMAP hBmp, bool transparent) // процедура отрисовки объекта
{
    if (!hBmp) return;
    HDC hMemDC = CreateCompatibleDC(hdcDest);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);
    BITMAP bmp;
    GetObject(hBmp, sizeof(BITMAP), &bmp);

    if (transparent)
    {
        TransparentBlt(hdcDest, x, y, w, h, hMemDC, 0, 0, w, h, RGB(0, 0, 0)); // черные пиксели в прозрачные
    }
    else
    {
        StretchBlt(hdcDest, x, y, w, h, hMemDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
    }

    SelectObject(hMemDC, hOldBmp);
    DeleteDC(hMemDC);
};

void ShowBlocks(HDC hMemDC)
{
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
}

void ShowBonuses(HDC hMemDC)
{
    for (auto& bonus : game.bonuses)
    {
        if (bonus.active) DrawBitmap(hMemDC, bonus.x, bonus.y, bonus.width, bonus.height, bonus.hBitmap, false);
    }
}

void ShowBalls(HDC hMemDC)
{
    for (auto& ball : game.balls)
    {
        DrawBitmap(hMemDC, ball.x, ball.y, ball.width, ball.height, ball.hBitmap, true);
    }
}

void ShowPlatform(HDC hMemDC)
{
    DrawBitmap(hMemDC, game.platform.x, game.platform.y, game.platform.width, game.platform.height, game.platform.hBitmap_middle, false);
    DrawBitmap(hMemDC, game.platform.x, game.platform.y, game.platform.section_width / 4, game.platform.height, game.platform.hBitmap_begin, false);
    DrawBitmap(hMemDC, game.platform.x + game.platform.width - game.platform.section_width / 4, game.platform.y, game.platform.section_width / 4, game.platform.height, game.platform.hBitmap_end, false);
}

void ShowDefeat(HDC hMemDC)
{    
    DrawBitmap(hMemDC, 0, 0, window.width, window.height, game.hBitmap_defeat, true);
}

void ShowWin(HDC hMemDC)
{
    DrawBitmap(hMemDC, 0, 0, window.width, window.height, game.hBitmap_win, true);
}

void ShowObjects(HDC hMemDC) // процедура отрисовки всех объектов игры
{
    switch (game.status)
    {
    case GameStatuses::wait:
    case GameStatuses::process:
    {
        ShowBlocks(hMemDC);
        ShowBonuses(hMemDC);
        ShowBalls(hMemDC);
        ShowPlatform(hMemDC);
        break;
    }
    case GameStatuses::defeat:
    {
        ShowDefeat(hMemDC);
        break;
    }
    case GameStatuses::win:
    {
        ShowWin(hMemDC);
        break;
    }
    default:
        return;
    }
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
    int windowWidth = 1024 + 16; // размер окна
    int windowHeight = 768 + 38; // размер окна

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
