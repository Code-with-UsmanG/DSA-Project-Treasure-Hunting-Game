#include "framework.h"
#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <vector>
#include <stack>
#include <queue>
#include <sstream>
#include <thread>
#include <ctime>
#include <cstdlib>
#include <string>
#include <fstream>     // For file I/O

// Link with winmm.lib for multimedia functions
#pragma comment(lib, "winmm.lib")

// Constants for maze and window dimensions
#define CELL_SIZE 50
#define GRID_ROWS 10
#define GRID_COLS 10
#define TIMER_ID 1
#define TIMER_INTERVAL 1000

// Total levels
#define TOTAL_LEVELS 5

// --- Cell values ---
// For better readability, define symbolic names for cell values.
enum CellType {
    PASSAGE = 0,    // empty (used for start/end)
    WALL = 1,
    COLLECTIBLE = 2, // collectible: adds one life
    HAZARD = 3,      // harmful hurdle: subtracts life and time
    OBSTACLE = 4,    // blocks movement
    MINIDOT = 5      // safe passage with a mini-dot (score available)
};

// --- Game States ---
enum GameState {
    MENU,
    PLAYING
};
GameState currentState = MENU;

// Maze cell structure for generating the maze.
struct MazeCell {
    bool visited = false;
    bool top = true, bottom = true, left = true, right = true;
};

// Global variables
HINSTANCE hInst;
HWND hWndMain;

// Player state: starting at (0,0) and ending at (9,9)
POINT playerPosition = { 0, 0 };
POINT endPosition = { GRID_COLS - 1, GRID_ROWS - 1 };

// Game state variables
int currentLevel = 0;
int lives = 2;      // Player starts with 2 lives.
int timeLeft = 15;  // 15-second timer per level.
int score = 0;      // Score increases by 1 for every mini-dot collected.

// Global flags for pausing and one-time messages.
bool g_paused = false;
bool g_timeOverShown = false;
bool g_hazardShown = false;
bool isPlayingBackgroundMusic = true;

// Container for maze levels; each level is a 2D vector (grid) of integers.
std::vector<std::vector<std::vector<int>>> levels;

// For undo functionality (if desired)
std::stack<POINT> playerMoveHistory;

// Forward declarations for functions defined later.
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
std::wstring ConvertToWString(int number);
void PlayGameSound(const std::wstring& soundFile);     // GF1
void PlayBackgroundMusic(const std::wstring& soundFile); // GF2
void StopBackgroundMusic();                              // GF3

// Menu drawing and mouse click handling.
void DrawMenu(HDC hdc);
void HandleMenuClick(int x, int y);

// File handling for saving and loading game state.
void SaveGameState();
bool LoadGameState();

//-----------------------------------------------------
// Maze Generation Functions
//-----------------------------------------------------

// Initialize a 2D vector of MazeCell objects.
std::vector<std::vector<MazeCell>> InitializeMazeCells(int rows, int cols) {
    return std::vector<std::vector<MazeCell>>(rows, std::vector<MazeCell>(cols));
}

// Remove wall between two adjacent cells.
void RemoveWall(MazeCell& current, MazeCell& next, int dx, int dy) {
    if (dx == 1) {
        current.right = false;
        next.left = false;
    }
    else if (dx == -1) {
        current.left = false;
        next.right = false;
    }
    else if (dy == 1) {
        current.bottom = false;
        next.top = false;
    }
    else if (dy == -1) {
        current.top = false;
        next.bottom = false;
    }
}

// Maze generation using DFS (iterative with a stack).
void GenerateMazeDFS(std::vector<std::vector<MazeCell>>& maze, int startRow, int startCol) {
    int rows = maze.size(), cols = maze[0].size();
    std::stack<POINT> cellStack;
    maze[startRow][startCol].visited = true;
    cellStack.push({ startCol, startRow });
    std::vector<POINT> directions = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };
    while (!cellStack.empty()) {
        POINT current = cellStack.top();
        int curRow = current.y, curCol = current.x;
        std::vector<POINT> neighbors;
        for (auto dir : directions) {
            int newRow = curRow + dir.y, newCol = curCol + dir.x;
            if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols &&
                !maze[newRow][newCol].visited) {
                neighbors.push_back(dir);
            }
        }
        if (!neighbors.empty()) {
            int index = rand() % neighbors.size();
            POINT chosen = neighbors[index];
            int newRow = curRow + chosen.y, newCol = curCol + chosen.x;
            RemoveWall(maze[curRow][curCol], maze[newRow][newCol], chosen.x, chosen.y);
            maze[newRow][newCol].visited = true;
            cellStack.push({ newCol, newRow });
        }
        else {
            cellStack.pop();
        }
    }
}

// Convert the MazeCell grid to a grid of integers.
// Initially mark passages as PASSAGE.
std::vector<std::vector<int>> ConvertMazeToGrid(const std::vector<std::vector<MazeCell>>& maze) {
    int rows = maze.size(), cols = maze[0].size();
    std::vector<std::vector<int>> grid(rows, std::vector<int>(cols, WALL));
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            if (!maze[r][c].top || !maze[r][c].bottom || !maze[r][c].left || !maze[r][c].right || maze[r][c].visited)
                grid[r][c] = PASSAGE;
    grid[0][0] = PASSAGE;
    grid[rows - 1][cols - 1] = PASSAGE;
    return grid;
}

// Check if there is a valid path from start to end using BFS.
bool IsPathValid(const std::vector<std::vector<int>>& grid) {
    int rows = grid.size(), cols = grid[0].size();
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::queue<POINT> q;
    POINT start = { 0, 0 }, end = { cols - 1, rows - 1 };
    q.push(start);
    visited[start.y][start.x] = true;
    std::vector<POINT> directions = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };
    while (!q.empty()) {
        POINT cur = q.front();
        q.pop();
        if (cur.x == end.x && cur.y == end.y)
            return true;
        for (auto d : directions) {
            int nx = cur.x + d.x, ny = cur.y + d.y;
            if (nx >= 0 && nx < cols && ny >= 0 && ny < rows &&
                !visited[ny][nx] && grid[ny][nx] == PASSAGE) {
                visited[ny][nx] = true;
                q.push({ nx, ny });
            }
        }
    }
    return false;
}

// Get one valid path from start to end using BFS with predecessor tracking.
std::vector<POINT> GetValidPath(const std::vector<std::vector<int>>& grid) {
    int rows = grid.size(), cols = grid[0].size();
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<POINT>> parent(rows, std::vector<POINT>(cols, { -1, -1 }));
    std::queue<POINT> q;
    POINT start = { 0, 0 }, end = { cols - 1, rows - 1 };
    q.push(start);
    visited[start.y][start.x] = true;
    std::vector<POINT> directions = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };
    bool found = false;
    while (!q.empty() && !found) {
        POINT cur = q.front();
        q.pop();
        if (cur.x == end.x && cur.y == end.y) {
            found = true;
            break;
        }
        for (auto d : directions) {
            int nx = cur.x + d.x, ny = cur.y + d.y;
            if (nx >= 0 && nx < cols && ny >= 0 && ny < rows &&
                !visited[ny][nx] && grid[ny][nx] == PASSAGE) {
                visited[ny][nx] = true;
                parent[ny][nx] = cur;
                q.push({ nx, ny });
            }
        }
    }
    std::vector<POINT> path;
    if (found) {
        POINT cur = end;
        while (!(cur.x == start.x && cur.y == start.y)) {
            path.push_back(cur);
            cur = parent[cur.y][cur.x];
        }
        path.push_back(start);
    }
    return path;
}

// Decorate the maze: for every PASSAGE cell not on the valid path,
// randomly change it to COLLECTIBLE, HAZARD, or OBSTACLE. Then convert remaining PASSAGE cells to MINIDOT.
void DecorateMaze(std::vector<std::vector<int>>& grid) {
    int rows = grid.size(), cols = grid[0].size();
    std::vector<POINT> validPath = GetValidPath(grid);
    std::vector<std::vector<bool>> isValidPath(rows, std::vector<bool>(cols, false));
    for (auto p : validPath)
        isValidPath[p.y][p.x] = true;
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (grid[r][c] == PASSAGE && !isValidPath[r][c]) {
                if ((r == 0 && c == 0) || (r == rows - 1 && c == cols - 1))
                    continue;
                int randVal = rand() % 100;
                if (randVal < 5)
                    grid[r][c] = COLLECTIBLE;
                else if (randVal < 15)
                    grid[r][c] = HAZARD;
                else if (randVal < 35)
                    grid[r][c] = OBSTACLE;
            }
        }
    }
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            if (grid[r][c] == PASSAGE)
                grid[r][c] = MINIDOT;
}

// Generate one valid maze level and decorate it.
std::vector<std::vector<int>> GenerateRandomMazeLevel() {
    while (true) {
        auto mazeCells = InitializeMazeCells(GRID_ROWS, GRID_COLS);
        GenerateMazeDFS(mazeCells, 0, 0);
        auto grid = ConvertMazeToGrid(mazeCells);
        if (IsPathValid(grid)) {
            DecorateMaze(grid);
            grid[0][0] = PASSAGE;
            grid[GRID_ROWS - 1][GRID_COLS - 1] = PASSAGE;
            return grid;
        }
    }
}

// Generate all levels.
void GenerateRandomLevels() {
    levels.clear();
    for (int i = 0; i < TOTAL_LEVELS; i++) {
        levels.push_back(GenerateRandomMazeLevel());
    }
    currentLevel = 0;
}

//-----------------------------------------------------
// File Handling Functions
//-----------------------------------------------------
void SaveGameState() {
    std::ofstream ofs("savegame.dat");
    if (!ofs)
        return;
    ofs << currentLevel << "\n"
        << lives << "\n"
        << timeLeft << "\n"
        << score << "\n"
        << playerPosition.x << " " << playerPosition.y << "\n";
    const auto& grid = levels[currentLevel];
    int rows = grid.size(), cols = grid[0].size();
    ofs << rows << " " << cols << "\n";
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            ofs << grid[r][c] << " ";
        }
        ofs << "\n";
    }
    ofs.close();
}

bool LoadGameState() {
    std::ifstream ifs("savegame.dat");
    if (!ifs)
        return false;
    ifs >> currentLevel >> lives >> timeLeft >> score;
    ifs >> playerPosition.x >> playerPosition.y;
    int rows, cols;
    ifs >> rows >> cols;
    std::vector<std::vector<int>> grid(rows, std::vector<int>(cols));
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            ifs >> grid[r][c];
    ifs.close();
    if (currentLevel < levels.size())
        levels[currentLevel] = grid;
    else
        levels.push_back(grid);
    return true;
}

//-----------------------------------------------------
// Sound and Utility Functions
//-----------------------------------------------------
std::wstring ConvertToWString(int number) {
    std::wstringstream ss;
    ss << number;
    return ss.str();
}

void PlayGameSound(const std::wstring& soundFile) {
    PlaySound(soundFile.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

void PlayBackgroundMusic(const std::wstring& soundFile) {
    std::wstring command = L"open \"" + soundFile + L"\" type mpegvideo alias bgm";
    mciSendString(command.c_str(), nullptr, 0, nullptr);
    mciSendString(L"play bgm repeat", nullptr, 0, nullptr);
}

void StopBackgroundMusic() {
    mciSendString(L"stop bgm", nullptr, 0, nullptr);
    mciSendString(L"close bgm", nullptr, 0, nullptr);
}

void ShowPausedMessage(LPCWSTR message, LPCWSTR title) {
    KillTimer(hWndMain, TIMER_ID);
    MessageBox(hWndMain, message, title, MB_OK);
    SetTimer(hWndMain, TIMER_ID, TIMER_INTERVAL, NULL);
}

//-----------------------------------------------------
// Drawing Functions: Game Screen and Menu Screen
//-----------------------------------------------------

// DrawMaze: Draws the maze, the door at the end, the player, and the HUD.
void DrawMaze(HDC hdc) {
    const std::vector<std::vector<int>>& maze = levels[currentLevel];
    HFONT hFont = CreateFont(
        48, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        VARIABLE_PITCH,
        L"Segoe UI Emoji"
    );
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            RECT cell = { col * CELL_SIZE, row * CELL_SIZE, (col + 1) * CELL_SIZE, (row + 1) * CELL_SIZE };
            if (maze[row][col] == WALL) {
                HBRUSH wallBrush = CreateSolidBrush(RGB(0, 0, 0));
                FillRect(hdc, &cell, wallBrush);
                DeleteObject(wallBrush);
            }
            else {
                // Fill non-wall cells with plain white.
                HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
                FillRect(hdc, &cell, whiteBrush);
                DeleteObject(whiteBrush);
            }
            FrameRect(hdc, &cell, (HBRUSH)GetStockObject(BLACK_BRUSH));
            std::wstring symbol;
            COLORREF symbolColor = RGB(0, 0, 0);
            switch (maze[row][col]) {
            case COLLECTIBLE:
                symbol = L"💎";
                // Diamond-like blue for collectibles.
                symbolColor = RGB(0, 191, 255);
                break;
            case HAZARD:
                symbol = L"☠️";
                symbolColor = RGB(255, 0, 0);
                break;
            case OBSTACLE:
                symbol = L"🚧";
                symbolColor = RGB(128, 0, 128);
                break;
            case MINIDOT:
                symbol = L"•";
                symbolColor = RGB(255, 215, 0);
                break;
            default:
                break;
            }
            if (!symbol.empty()) {
                SIZE textSize;
                GetTextExtentPoint32(hdc, symbol.c_str(), symbol.length(), &textSize);
                int textX = col * CELL_SIZE + (CELL_SIZE - textSize.cx) / 2;
                int textY = row * CELL_SIZE + (CELL_SIZE - textSize.cy) / 2;
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, symbolColor);
                TextOut(hdc, textX, textY, symbol.c_str(), symbol.length());
            }
        }
    }
    // Leave the starting cell as is (plain white).
    // Draw the ending cell: fill with dark orange.
    RECT endRect = { (GRID_COLS - 1) * CELL_SIZE, (GRID_ROWS - 1) * CELL_SIZE,
                     GRID_COLS * CELL_SIZE, GRID_ROWS * CELL_SIZE };
    HBRUSH endBrush = CreateSolidBrush(RGB(255, 140, 0));
    FillRect(hdc, &endRect, endBrush);
    DeleteObject(endBrush);
    // Overlay a door symbol ("⛩️") on the ending cell.
    std::wstring doorSymbol = L"⛩️";
    SIZE doorSize;
    GetTextExtentPoint32(hdc, doorSymbol.c_str(), doorSymbol.length(), &doorSize);
    int doorX = (GRID_COLS - 1) * CELL_SIZE + (CELL_SIZE - doorSize.cx) / 2;
    int doorY = (GRID_ROWS - 1) * CELL_SIZE + (CELL_SIZE - doorSize.cy) / 2;
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));
    TextOut(hdc, doorX, doorY, doorSymbol.c_str(), doorSymbol.length());
    // Draw the player.
    int playerX = playerPosition.x * CELL_SIZE;
    int playerY = playerPosition.y * CELL_SIZE;
    std::wstring playerSymbol = L"🤺";
    SIZE pSize;
    GetTextExtentPoint32(hdc, playerSymbol.c_str(), playerSymbol.length(), &pSize);
    int pTextX = playerX + (CELL_SIZE - pSize.cx) / 2;
    int pTextY = playerY + (CELL_SIZE - pSize.cy) / 2;
    SetTextColor(hdc, RGB(0, 0, 255));
    TextOut(hdc, pTextX, pTextY, playerSymbol.c_str(), playerSymbol.length());
    // Draw the HUD.
    std::wstring hud = L"Lives: " + ConvertToWString(lives) +
        L"\nTime: " + ConvertToWString(timeLeft) +
        L"\nScore: " + ConvertToWString(score);
    RECT hudRect = { GRID_COLS * CELL_SIZE + 10, 10, GRID_COLS * CELL_SIZE + 250, 150 };
    DrawText(hdc, hud.c_str(), -1, &hudRect, DT_LEFT | DT_TOP);
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

// DrawMenu: Draws a menu screen with three buttons.
void DrawMenu(HDC hdc) {
    RECT clientRect;
    GetClientRect(hWndMain, &clientRect);
    HBRUSH bgBrush = CreateSolidBrush(RGB(200, 200, 200));
    FillRect(hdc, &clientRect, bgBrush);
    DeleteObject(bgBrush);
    HFONT hFont = CreateFont(
        48, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        VARIABLE_PITCH,
        L"Segoe UI"
    );
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);
    std::wstring title = L"Maze Game";
    RECT titleRect = { 0, 20, clientRect.right, 100 };
    SetTextColor(hdc, RGB(0, 0, 128));
    DrawText(hdc, title.c_str(), -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    // Define button rectangles.
    RECT startBtn = { clientRect.right / 2 - 100, 150, clientRect.right / 2 + 100, 210 };
    RECT loadBtn = { clientRect.right / 2 - 100, 230, clientRect.right / 2 + 100, 290 };
    RECT exitBtn = { clientRect.right / 2 - 100, 310, clientRect.right / 2 + 100, 370 };
    HBRUSH btnBrush = CreateSolidBrush(RGB(100, 149, 237));
    FillRect(hdc, &startBtn, btnBrush);
    FillRect(hdc, &loadBtn, btnBrush);
    FillRect(hdc, &exitBtn, btnBrush);
    DeleteObject(btnBrush);
    FrameRect(hdc, &startBtn, (HBRUSH)GetStockObject(BLACK_BRUSH));
    FrameRect(hdc, &loadBtn, (HBRUSH)GetStockObject(BLACK_BRUSH));
    FrameRect(hdc, &exitBtn, (HBRUSH)GetStockObject(BLACK_BRUSH));
    SetTextColor(hdc, RGB(255, 255, 255));
    DrawText(hdc, L"Start Game", -1, &startBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawText(hdc, L"Load Game", -1, &loadBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawText(hdc, L"Exit", -1, &exitBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

//-----------------------------------------------------
// Input and Timer Handling
//-----------------------------------------------------

// MovePlayer: Moves the player based on arrow key input.
void MovePlayer(int dx, int dy) {
    int newX = playerPosition.x + dx;
    int newY = playerPosition.y + dy;
    if (newX >= 0 && newX < GRID_COLS && newY >= 0 && newY < GRID_ROWS) {
        int cellValue = levels[currentLevel][newY][newX];
        if (cellValue == WALL || cellValue == OBSTACLE) {
            return;
        }
        else if (cellValue == HAZARD) {
            lives--;
            // Optionally adjust timeLeft if desired.
            playerPosition = { 0, 0 };
            PlayGameSound(L"hazard01.wav");
            if (lives <= 0) {
                KillTimer(hWndMain, TIMER_ID);
                MessageBox(hWndMain, L"You hit a harmful hurdle! No lives remaining. Game Over.", L"Game Over", MB_OK);
                PostQuitMessage(0);
                return;
            }
            else {
                ShowPausedMessage(L"You hit a harmful hurdle! Restarting from the beginning.", L"Hazard");
                InvalidateRect(hWndMain, NULL, TRUE);
                return;
            }
        }
        else if (cellValue == COLLECTIBLE) {
            lives++;
            timeLeft += 5;
            PlayGameSound(L"powerup.wav");
            levels[currentLevel][newY][newX] = PASSAGE;
        }
        else if (cellValue == MINIDOT) {
            score++;
            levels[currentLevel][newY][newX] = PASSAGE;
        }
        playerMoveHistory.push(playerPosition);
        playerPosition.x = newX;
        playerPosition.y = newY;
    }
    if (playerPosition.x == endPosition.x && playerPosition.y == endPosition.y) {
        if (currentLevel < TOTAL_LEVELS - 1) {
            currentLevel++;
            PlayGameSound(L"lvl.wav");
            playerPosition = { 0, 0 };
            timeLeft = 25;
        }
        else {
            KillTimer(hWndMain, TIMER_ID);
            PlayGameSound(L"win01.wav");
            MessageBox(hWndMain, L"Congratulations! You've completed all levels!", L"Victory", MB_OK);
            PostQuitMessage(0);
        }
    }
    InvalidateRect(hWndMain, NULL, TRUE);
}

// Handle menu clicks.
void HandleMenuClick(int x, int y) {
    RECT clientRect;
    GetClientRect(hWndMain, &clientRect);
    RECT startBtn = { clientRect.right / 2 - 100, 150, clientRect.right / 2 + 100, 210 };
    RECT loadBtn = { clientRect.right / 2 - 100, 230, clientRect.right / 2 + 100, 290 };
    RECT exitBtn = { clientRect.right / 2 - 100, 310, clientRect.right / 2 + 100, 370 };
    POINT pt = { x, y };
    if (PtInRect(&startBtn, pt)) {
        currentState = PLAYING;
        lives = 2;
        timeLeft = 15;
        score = 0;
        playerPosition = { 0, 0 };
        GenerateRandomLevels();
    }
    else if (PtInRect(&loadBtn, pt)) {
        if (LoadGameState())
            currentState = PLAYING;
        else
            MessageBox(hWndMain, L"No saved game found.", L"Load Game", MB_OK);
    }
    else if (PtInRect(&exitBtn, pt)) {
        PostQuitMessage(0);
    }
}


// Window procedure.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        HDC hdcMem = CreateCompatibleDC(hdc);
        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, width, height);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
        HBRUSH hbrBkGnd = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(hdcMem, &clientRect, hbrBkGnd);
        DeleteObject(hbrBkGnd);
        if (currentState == MENU)
            DrawMenu(hdcMem);
        else if (currentState == PLAYING)
            DrawMaze(hdcMem);
        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        EndPaint(hWnd, &ps);
    }
                 break;

    case WM_LBUTTONDOWN:
        if (currentState == MENU) {
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            HandleMenuClick(xPos, yPos);
        }
        break;

    case WM_KEYDOWN:
        if (currentState == PLAYING) {
            switch (wParam) {
            case VK_UP:
                MovePlayer(0, -1);
                break;
            case VK_DOWN:
                MovePlayer(0, 1);
                break;
            case VK_LEFT:
                MovePlayer(-1, 0);
                break;
            case VK_RIGHT:
                MovePlayer(1, 0);
                break;
            case 'S':  // Save game on pressing 'S'
                SaveGameState();
                break;
            }
        }
        break;

    case WM_TIMER:
        if (currentState == PLAYING && wParam == TIMER_ID) {
            timeLeft--;
            if (timeLeft <= 0) {
                lives--;
                if (lives <= 0) {
                    KillTimer(hWndMain, TIMER_ID);
                    MessageBox(hWndMain, L"Time's up and no lives remaining. Game Over.", L"Game Over", MB_OK);
                    PostQuitMessage(0);
                    break;
                }
                else {
                    ShowPausedMessage(L"Time's up! Restarting level.", L"Timer");
                    timeLeft = 25;
                    playerPosition = { 0, 0 };
                }
            }
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

//-----------------------------------------------------
// Main Function
//-----------------------------------------------------
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    srand((unsigned int)time(NULL));
    hInst = hInstance;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MazeGameClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    hWndMain = CreateWindow(L"MazeGameClass", L"Maze Game",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CELL_SIZE * GRID_COLS + 300, CELL_SIZE * GRID_ROWS + 40,
        nullptr, nullptr, hInstance, nullptr);
    if (!hWndMain)
        return FALSE;
    ShowWindow(hWndMain, nCmdShow);

    currentState = MENU;
    GenerateRandomLevels();
    SetTimer(hWndMain, TIMER_ID, TIMER_INTERVAL, NULL);

    // Start background music in a separate thread.
    std::thread bgMusicThread(PlayBackgroundMusic, L"background 01.mp3");
    bgMusicThread.detach();

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    StopBackgroundMusic();
    return (int)msg.wParam;
}
