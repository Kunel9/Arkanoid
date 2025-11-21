// Arkanoid.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Arkanoid.h"
#include <vector>
#include <cmath>
#include <random>
#include <ctime>
#include <mmsystem.h>
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")
using namespace std;

HBITMAP LoadBmp(LPCWSTR bmp_name)
{
    return (HBITMAP)LoadImageW(NULL, bmp_name, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
}

struct Window
{
    HBITMAP hBitmap;
    HWND hWnd;
    float width, height;

    Window()
    {
        hBitmap = LoadBmp(L"background.bmp");
    }

} window;

struct Point
{
    float x;
    float y;

    Point(): x(0.0f), y(0.0f) {}
    Point(float x_pos, float y_pos) : x(x_pos), y(y_pos) {}
};

void InitWindow() 
{
    RECT r;
    GetClientRect(window.hWnd, &r);
    window.width = r.right - r.left;
    window.height = r.bottom - r.top;
}

enum class BonusTypes
{
    add_ball,
    size_up,
    size_down
};

enum class GameStatuses
{
    wait,
    process,
    defeat,
    win
};

void ProcessSound(LPCWSTR wav_name)
{
    PlaySound(wav_name, NULL, SND_FILENAME | SND_ASYNC);
}

struct Game
{
    struct Ball
    {
        HBITMAP hBitmap = nullptr;
        float x, y, height, width, speed, dx, dy;
        Point trace_points[32];

        Ball()
        {
            hBitmap = LoadBmp(L"ball.bmp");
        }
    };

    struct Block
    {
        HBITMAP hBitmaps[3] = { nullptr };
        float x, y, height, width;
        int endurance;

        Block()
        {
            hBitmaps[0] = LoadBmp(L"block_yellow.bmp");
            hBitmaps[1] = LoadBmp(L"block_orange.bmp");
            hBitmaps[2] = LoadBmp(L"block_red.bmp");
        }
       
    };

    struct Platform
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

    struct Bonus
    {
        HBITMAP hBitmap = nullptr;
        BonusTypes type;
        float x, y, height, width, speed;
        bool active, positive;

        Bonus()
        {
            active = true;

            if (rand() % 100 < 50) // Random determination of bonus type
            {
                BonusTypes positive_bonuses[] = { BonusTypes::add_ball , BonusTypes::size_up }; 
                positive = true;
                type = positive_bonuses[rand() % size(positive_bonuses)];
            }
            else
            {
                BonusTypes negative_bonuses[] = { BonusTypes::size_down }; 
                positive = false;
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

        void activate(Game& game_link) // Activate bonus effect
        {
            if (active)
            {
                if (positive) ProcessSound(L"positive_bonus.wav");
                else ProcessSound(L"negative_bonus.wav");

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
    vector<Ball> balls; // Vector for balls
    vector<vector<Block>> blocks; // Two-dimensional array for blocks
    vector<Bonus> bonuses; // Vector for bonuses
    GameStatuses status; // Current game status
    float gameplay_speed = 20;

    Game()
    {
        hBitmap_defeat = LoadBmp(L"defeat.bmp");
        hBitmap_win = LoadBmp(L"win.bmp");
    }

    void SetGameStatus(GameStatuses new_status)
    {
        if (game.status != new_status)
        {
            game.status = new_status;

            switch (new_status)
            {
            case GameStatuses::defeat:
            {
                ProcessSound(L"defeat.wav");
                break;
            }
            case GameStatuses::win:
            {
                ProcessSound(L"win.wav");
                break;
            }
            case GameStatuses::process:
            {
                break;
            }
            case GameStatuses::wait:
            {
                ProcessSound(L"click.wav");
                break;
            }
            default:
                return;
            }
        }
    }

    void InitGame() 
    {
        gameplay_speed = 20;

        // Platform initialization
        platform.size = 5;
        platform.min_size = 1;
        platform.max_size = 32;
        platform.section_width = 32;
        platform.height = 32;
        platform.width = platform.section_width * platform.size;
        platform.x = window.width / 2 - platform.width / 2;
        platform.y = window.height - platform.height;
        platform.speed = gameplay_speed;

        // Start ball initialization
        AddBall();

        // Create blocks
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
        
        SetGameStatus(GameStatuses::wait);
    }

    // Checking collision between two objects
    bool CheckCollision(pair <Point, Point>& points_a, pair <Point, Point>& points_b) 
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

    void CreateBonus(float x, float y)
    {
        Bonus new_bonus;
        new_bonus.width = 32;
        new_bonus.height = 32;
        new_bonus.x = x - new_bonus.width / 2;
        new_bonus.y = y - new_bonus.height / 2;
        new_bonus.speed = gameplay_speed / 3;
        bonuses.push_back(new_bonus);
    }

    void AddBall() // Create new ball
    {
        Ball new_ball;
        new_ball.height = 32;
        new_ball.width = 32;
        new_ball.x = platform.x + platform.width / 2 - new_ball.width / 2;
        new_ball.y = platform.y - new_ball.height;
        new_ball.speed = gameplay_speed;
        new_ball.dy = (20 + rand() % 60) / 100.;
        new_ball.dx = ((rand() % 2 == 1) ? 1 : -1) * (1 - new_ball.dy);

        for (int i = 0; i < size(new_ball.trace_points); i++)
        {
            new_ball.trace_points[i].x = new_ball.x + new_ball.width / 2;
            new_ball.trace_points[i].y = new_ball.y + new_ball.height / 2;
        }

        balls.push_back(new_ball);
    }

    void RestartGame()
    {
        for (const auto& ball : balls)
        {
            if (ball.hBitmap) DeleteObject(ball.hBitmap);
        }

        balls.clear();

        for (int row = 0; row < blocks.size(); row++)
        {
            for (int col = 0; col < blocks[row].size(); col++)
            {
                for (int i = 0; i < size(blocks[row][col].hBitmaps); i++)
                {
                    if (blocks[row][col].hBitmaps[i]) DeleteObject(blocks[row][col].hBitmaps[i]);
                }
            }
        }

        blocks.clear();

        for (const auto& bonus : bonuses)
        {
            if (bonus.hBitmap) DeleteObject(bonus.hBitmap);
        }

        bonuses.clear();

        InitGame();

        ProcessSound(L"click.wav");
    }

    void ProcessInput() 
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

        if (GetAsyncKeyState(VK_SPACE) && status == GameStatuses::wait)
        {
            SetGameStatus(GameStatuses::process);
        }

        if (GetAsyncKeyState('R') && (status == GameStatuses::defeat || status == GameStatuses::win))
        {
            RestartGame();
        }
    }

    // Check collision between ball and platform
    void CheckPlatform(Ball& ball, pair<Point, Point> ball_points, pair<Point, Point> platform_points, int& i_contact_point, int current_i)
    {
        if (CheckCollision(platform_points, ball_points) && ball.dy > 0)
        {
            i_contact_point = current_i;
            for (int j = i_contact_point + 1; j < size(ball.trace_points); j++)
            {
                ball.trace_points[j].y = ball.trace_points[j].y - (copysign(abs(ball.trace_points[j].y - ball.trace_points[i_contact_point].y) * 2, ball.dy));
            }

            ball.dy *= -1;

            ProcessSound(L"knock.wav");
        }
    }

    // Check collision between ball and side walls
    void CheckWalls(Ball& ball, pair<Point, Point> ball_points, int& i_contact_point, int current_i)
    {
        if ((ball_points.first.x <= 0 && ball.dx < 0) || (ball_points.second.x >= window.width && ball.dx > 0))
        {
            i_contact_point = current_i;
            for (int j = i_contact_point + 1; j < size(ball.trace_points); j++)
            {
                ball.trace_points[j].x = ball.trace_points[j].x - (copysign(abs(ball.trace_points[j].x - ball.trace_points[i_contact_point].x) * 2, ball.dx));
            }

            ball.dx *= -1;

            ProcessSound(L"knock.wav");
        };
    }

    // Check collision between ball and roof
    void CheckRoof(Ball& ball, pair<Point, Point> ball_points, int& i_contact_point, int current_i)
    {
        if (ball_points.first.y <= 0 && ball.dy < 0)
        {
            i_contact_point = current_i;
            for (int j = i_contact_point + 1; j < size(ball.trace_points); j++)
            {
                ball.trace_points[j].y = ball.trace_points[j].y - (copysign(abs(ball.trace_points[j].y - ball.trace_points[i_contact_point].y) * 2, ball.dy));
            }

            ball.dy *= -1;

            ProcessSound(L"knock.wav");
        };
    }

    // Check collision between ball and blocks
    void CheckBlocks(Ball& ball, pair<Point, Point> ball_points, int& i_contact_point, int current_i)
    {
        bool collision_processed = false;
        for (int row = 0; row < blocks.size(); row++)
        {
            for (int col = 0; col < blocks[row].size(); col++)
            {
                if (blocks[row][col].endurance <= 0) continue; // Skip destroyed blocks

                pair<Point, Point> block_points = { Point(blocks[row][col].x, blocks[row][col].y), Point(blocks[row][col].x + blocks[row][col].width, blocks[row][col].y + blocks[row][col].height) }; 
                if (CheckCollision(block_points, ball_points))
                {

                    Point ball_center{ ball_points.first.x + ball.width / 2, ball_points.first.y + ball.height / 2 };
                    Point block_center{ blocks[row][col].x + blocks[row][col].width / 2, blocks[row][col].y + blocks[row][col].height / 2 };

                    if (abs(ball_center.x - block_center.x) > abs(ball_center.y - block_center.y))
                    {
                        i_contact_point = current_i;
                        for (int j = i_contact_point + 1; j < size(ball.trace_points); j++)
                        {
                            ball.trace_points[j].x = ball.trace_points[j].x - (copysign(abs(ball.trace_points[j].x - ball.trace_points[i_contact_point].x) * 2, ball.dx));
                        }

                        ball.dx *= -1;
                    }
                    else if (abs(ball_center.x - block_center.x) < abs(ball_center.y - block_center.y))
                    {
                        i_contact_point = current_i;
                        for (int j = i_contact_point + 1; j < size(ball.trace_points); j++)
                        {
                            ball.trace_points[j].y = ball.trace_points[j].y - (copysign(abs(ball.trace_points[j].y - ball.trace_points[i_contact_point].y) * 2, ball.dy));
                        }

                        ball.dy *= -1;
                    }
                    else
                    {
                        i_contact_point = current_i;
                        for (int j = i_contact_point + 1; j < size(ball.trace_points); j++)
                        {
                            ball.trace_points[j].x = ball.trace_points[j].x - (copysign(abs(ball.trace_points[j].x - ball.trace_points[i_contact_point].x) * 2, ball.dx));
                            ball.trace_points[j].y = ball.trace_points[j].y - (copysign(abs(ball.trace_points[j].y - ball.trace_points[i_contact_point].y) * 2, ball.dy));
                        }

                        ball.dx *= -1;
                        ball.dy *= -1;
                    }

                    blocks[row][col].endurance--;

                    ProcessSound(L"knock.wav");

                    if (blocks[row][col].endurance <= 0) // Create bonus after destruction block
                    {
                        if (rand() % 100 < 50) CreateBonus(block_center.x, block_center.y);

                        ProcessSound(L"destruction.wav");
                    }

                    collision_processed = true; 

                    break; 
                }
            }

            if (collision_processed) break;
        }
    }


    void ProcessBalls(pair<Point, Point> platform_points)
    {
        for (auto& ball : balls)
        {
            if (status == GameStatuses::wait)
            {
                ball.x = platform.x + platform.width / 2 - ball.width / 2;
            }
            else
            {
                ball.x = ball.trace_points[size(ball.trace_points)-1].x - ball.width / 2;
                ball.y = ball.trace_points[size(ball.trace_points)-1].y - ball.height / 2;

                for (int i = 0; i < size(ball.trace_points); i++)
                {
                    Point coordinates{ (ball.x + ball.dx * (float(i) / size(ball.trace_points) * ball.speed)) + ball.width / 2, (ball.y + ball.dy * (float(i) / size(ball.trace_points) * ball.speed)) + ball.height / 2 };
                    ball.trace_points[i] = Point{ coordinates };
                };

                int i_contact_point;

                for (int i = 0; i < size(ball.trace_points); i++)
                {
                    pair<Point, Point> ball_points = { Point(ball.trace_points[i].x - ball.width / 2, ball.trace_points[i].y - ball.height / 2), Point(ball.trace_points[i].x + ball.width / 2, ball.trace_points[i].y + ball.height / 2) };
                    
                    CheckPlatform(ball, ball_points, platform_points, i_contact_point, i); // Check collision between ball and platform
                
                    CheckWalls(ball, ball_points, i_contact_point, i); // Check collision between ball and side walls
                
                    CheckRoof(ball, ball_points, i_contact_point, i); // Check collision between ball and roof
                    
                    CheckBlocks(ball, ball_points, i_contact_point, i); // Check collision between ball and blocks
                };
            }
        }
    }

    void ProcessBonuses(pair<Point, Point> platform_points)
    {
        for (auto& bonus : bonuses)
        {
            bonus.y += bonus.speed;

            pair<Point, Point> bonus_points = { Point(bonus.x, bonus.y), Point(bonus.x + bonus.width, bonus.y + bonus.height) };

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

    void CheckDefeatCondition()
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

                if (!bonus_found) SetGameStatus(GameStatuses::defeat);
            }
            else
            {
                SetGameStatus(GameStatuses::defeat);
            }
        }
    }

    void CheckWinCondition()
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

        if (!block_found) SetGameStatus(GameStatuses::win);
    }

    void UpdateGameStatus()
    {
        CheckDefeatCondition();
        CheckWinCondition();    
    }

    void ProcessGame() 
    {
        pair<Point, Point> platform_points = { Point(platform.x, platform.y), Point(platform.x + platform.width, platform.y + platform.height) }; 

        ProcessInput();

        if (game.status != GameStatuses::win && game.status != GameStatuses::defeat)
        {
            ProcessBalls(platform_points);
            ProcessBonuses(platform_points);

            Cleaning();

            UpdateGameStatus();
        }
    }
} game;

void DrawBitmap(HDC hdcDest, int x, int y, int w, int h, HBITMAP hBmp, bool transparent) 
{
    if (!hBmp) return;
    HDC hMemDC = CreateCompatibleDC(hdcDest);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);
    BITMAP bmp;
    GetObject(hBmp, sizeof(BITMAP), &bmp);

    if (transparent)
    {
        TransparentBlt(hdcDest, x, y, w, h, hMemDC, 0, 0, w, h, RGB(0, 0, 0)); 
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
      
        //for (int i = 0; i < size(ball.trace_points); i++) // show trace points
        //{
        //    SetPixel(hMemDC, ball.trace_points[i].x, ball.trace_points[i].y, RGB(0, 255, 0));
        //};
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

void ShowObjects(HDC hMemDC) 
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

    case WM_TIMER:
    {
        if (wParam == 1)
        {
            InvalidateRect(hwnd, NULL, FALSE);
            game.ProcessGame();
        }
        break;
    }

    case WM_PAINT: 
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Create memory buffer for double buffering

        HDC hMemDC = CreateCompatibleDC(hdc);
        HBITMAP hMemBmp = CreateCompatibleBitmap(hdc, window.width, window.height);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hMemBmp);

        // Draw everything to the memory buffer

        if (window.hBitmap) // Draw background
        {
            HDC hBackDC = CreateCompatibleDC(hMemDC);
            HBITMAP hOldBackBmp = (HBITMAP)SelectObject(hBackDC, window.hBitmap);
            BITMAP bmp;
            GetObject(window.hBitmap, sizeof(BITMAP), &bmp);
            StretchBlt(hMemDC, 0, 0, window.width, window.height, hBackDC, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
            SelectObject(hBackDC, hOldBackBmp);
            DeleteDC(hBackDC);
        }

        ShowObjects(hMemDC); // Draw all game objects

        // Copy finished buffer to screen
        BitBlt(hdc, 0, 0, window.width, window.height, hMemDC, 0, 0, SRCCOPY);

        // Resource cleaning
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

    // Get screen dimensions for centering
    int screenWidth = GetSystemMetrics(SM_CXSCREEN); 
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Set window size (including borders (16 and 38 pixels))
    int windowWidth = 1024 + 16; 
    int windowHeight = 768 + 38; 

    // Calculate center position
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    window.hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"Arkanoid",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        windowX, windowY,           // Window position (center)
        windowWidth, windowHeight,  // Window size
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
