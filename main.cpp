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
    COLLECTIBLE = 2,// collectible: adds one life
    HAZARD = 3,     // harmful hurdle: subtracts life and time
    OBSTACLE = 4,   // blocks movement
    MINIDOT = 5     // safe passage with a mini-dot (score available)
};

// Maze cell structure for generating the maze.
struct MazeCell {
    bool visited = false;
    // Walls: true means wall exists; false means passage.
    bool top = true, bottom = true, left = true, right = true;
};

// Global variables
HINSTANCE hInst;
HWND hWndMain;

// Player state: starting at (0,0) and ending at (9,9)
POINT playerPosition = { 0, 0 };
POINT endPosition = { GRID_COLS - 1, GRID_ROWS - 1 };

// Game state
int currentLevel = 0;
int lives = 2;      // Player starts with 2 lives.
int timeLeft = 15;  // 15-second timer per level.
int score = 0;      // Score increases by 1 for every mini-dot collected.

// Global flags for pausing and ensuring one-time messages.
bool g_paused = false;          // When true, timer events and movement are ignored.
bool g_timeOverShown = false;   // Set to true when the "Time Over" message has been shown.
bool g_hazardShown = false;     // Set to true when the hazardous collision message has been shown.
bool isPlayingBackgroundMusic = true; // Flag to control background music

// Container for maze levels; each level is a 2D vector (grid) of integers.
std::vector<std::vector<std::vector<int>>> levels;

// For undo functionality (if desired)
std::stack<POINT> playerMoveHistory;

// Forward declarations for functions defined later.
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
std::wstring ConvertToWString(int number);
void PlayGameSound(const std::wstring& soundFile);     // Sound function GF1
void PlayBackgroundMusic(const std::wstring& soundFile); // Sound function GF2
void StopBackgroundMusic();                              // Sound function GF3

// Initialize a 2D vector of MazeCell objects.
std::vector<std::vector<MazeCell>> InitializeMazeCells(int rows, int cols) {
    return std::vector<std::vector<MazeCell>>(rows, std::vector<MazeCell>(cols));
}

// Remove wall between two adjacent cells.
void RemoveWall(MazeCell& current, MazeCell& next, int dx, int dy) {
    if (dx == 1) {          // next is to the right
        current.right = false;
        next.left = false;
    }
    else if (dx == -1) {    // next is to the left
        current.left = false;
        next.right = false;
    }
    else if (dy == 1) {     // next is below
        current.bottom = false;
        next.top = false;
    }
    else if (dy == -1) {    // next is above
        current.top = false;
        next.bottom = false;
    }
}

// Maze generation using DFS (iterative with a stack).
void GenerateMazeDFS(std::vector<std::vector<MazeCell>>& maze, int startRow, int startCol) {
    int rows = maze.size();
    int cols = maze[0].size();
    std::stack<POINT> cellStack;
    maze[startRow][startCol].visited = true;
    cellStack.push({ startCol, startRow });  // POINT.x = col, POINT.y = row

    // Directions: up, right, down, left (dx, dy)
    std::vector<POINT> directions = { {0, -1}, {1, 0}, {0, 1}, {-1, 0} };

    while (!cellStack.empty()) {
        POINT current = cellStack.top();
        int curRow = current.y;
        int curCol = current.x;
        std::vector<POINT> neighbors;
        for (auto dir : directions) {
            int newRow = curRow + dir.y;
            int newCol = curCol + dir.x;
            if (newRow >= 0 && newRow < rows && newCol >= 0 && newCol < cols &&
                !maze[newRow][newCol].visited) {
                neighbors.push_back(dir);
            }
        }
        if (!neighbors.empty()) {
            int index = rand() % neighbors.size();
            POINT chosen = neighbors[index];
            int newRow = curRow + chosen.y;
            int newCol = curCol + chosen.x;
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
// Initially, we mark passages as 0 (empty). Later, we decorate.
std::vector<std::vector<int>> ConvertMazeToGrid(const std::vector<std::vector<MazeCell>>& maze) {
    int rows = maze.size();
    int cols = maze[0].size();
    std::vector<std::vector<int>> grid(rows, std::vector<int>(cols, WALL));  // initialize all as walls
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            // If any wall is missing or the cell was visited, mark as passage.
            if (!maze[r][c].top || !maze[r][c].bottom || !maze[r][c].left || !maze[r][c].right || maze[r][c].visited)
                grid[r][c] = PASSAGE;
        }
    }
    // Mark starting and ending positions explicitly.
    grid[0][0] = PASSAGE;
    grid[rows - 1][cols - 1] = PASSAGE;
    return grid;
}

// Check if there is a valid path from start to end using BFS.
bool IsPathValid(const std::vector<std::vector<int>>& grid) {
    int rows = grid.size(), cols = grid[0].size();
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::queue<POINT> q;
    POINT start = { 0, 0 };
    POINT end = { cols - 1, rows - 1 };
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

// Use BFS with predecessor tracking to retrieve one valid path.
std::vector<POINT> GetValidPath(const std::vector<std::vector<int>>& grid) {
    int rows = grid.size(), cols = grid[0].size();
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::vector<std::vector<POINT>> parent(rows, std::vector<POINT>(cols, { -1, -1 }));
    std::queue<POINT> q;
    POINT start = { 0, 0 };
    POINT end = { cols - 1, rows - 1 };
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

// Decorate the maze:
// For every passage cell (value PASSAGE) that is NOT on the valid path,
// randomly change it to COLLECTIBLE, HAZARD, or OBSTACLE. Then, for all remaining
// passage cells, set them to MINIDOT (for scoring).
void DecorateMaze(std::vector<std::vector<int>>& grid) {
    int rows = grid.size(), cols = grid[0].size();
    std::vector<POINT> validPath = GetValidPath(grid);
    std::vector<std::vector<bool>> isValidPath(rows, std::vector<bool>(cols, false));
    for (auto p : validPath)
        isValidPath[p.y][p.x] = true;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            // Only decorate passage cells that are not on the valid path.
            if (grid[r][c] == PASSAGE && !isValidPath[r][c]) {
                // Do not change the start and end cells.
                if ((r == 0 && c == 0) || (r == rows - 1 && c == cols - 1))
                    continue;
                int randVal = rand() % 100; // 0 to 99.
                if (randVal < 5) {             // 5% chance: collectible.
                    grid[r][c] = COLLECTIBLE;
                }
                else if (randVal < 15) {       // Next 10%: harmful hurdle.
                    grid[r][c] = HAZARD;
                }
                else if (randVal < 35) {       // Next 20%: obstacle.
                    grid[r][c] = OBSTACLE;
                }
                // Otherwise, leave as PASSAGE.
            }
        }
    }
    // Finally, convert all remaining PASSAGE cells to MINIDOT.
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            if (grid[r][c] == PASSAGE)
                grid[r][c] = MINIDOT;
        }
    }
}

// Generate one random maze level that is valid, then decorate it.
std::vector<std::vector<int>> GenerateRandomMazeLevel() {
    while (true) {
        auto mazeCells = InitializeMazeCells(GRID_ROWS, GRID_COLS);
        GenerateMazeDFS(mazeCells, 0, 0);
        auto grid = ConvertMazeToGrid(mazeCells);
        if (IsPathValid(grid)) {
            DecorateMaze(grid);
            // Guarantee the start and end remain safe (empty).
            grid[0][0] = PASSAGE;
            grid[GRID_ROWS - 1][GRID_COLS - 1] = PASSAGE;
            return grid;
        }
        // Otherwise, try again.
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

// Utility: Convert an integer to a wstring.
std::wstring ConvertToWString(int number) {
    std::wstringstream ss;
    ss << number;
    return ss.str();
}

// Sound Functions
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

// Helper: Pause the timer, show a modal message box, then restart the timer.
void ShowPausedMessage(LPCWSTR message, LPCWSTR title) {
    KillTimer(hWndMain, TIMER_ID);
    MessageBox(hWndMain, message, title, MB_OK);
    SetTimer(hWndMain, TIMER_ID, TIMER_INTERVAL, NULL);
}

void DrawMaze(HDC hdc) {
    const std::vector<std::vector<int>>& maze = levels[currentLevel];
    // Create and select a larger Unicode-capable font.
    // Here we use a height of 42 so that the symbols fill the 50-pixel cell nicely.
    HFONT hFont = CreateFont(
        48,              // Increased height for larger symbols.
        0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        VARIABLE_PITCH,
        L"Segoe UI Emoji"  // Use a Unicode-capable font.
    );
    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    // Iterate through each cell.
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            RECT cell = { col * CELL_SIZE, row * CELL_SIZE, (col + 1) * CELL_SIZE, (row + 1) * CELL_SIZE };
            // For walls, fill with black.
            if (maze[row][col] == WALL) {
                HBRUSH wallBrush = CreateSolidBrush(RGB(0, 0, 0));
                FillRect(hdc, &cell, wallBrush);
                DeleteObject(wallBrush);
            }
            else {
                // For all non-wall cells, fill with plain white.
                HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
                FillRect(hdc, &cell, whiteBrush);
                DeleteObject(whiteBrush);
            }
            // Draw cell border.
            FrameRect(hdc, &cell, (HBRUSH)GetStockObject(BLACK_BRUSH));

            // Determine if we need to draw a Unicode symbol.
            std::wstring symbol;
            COLORREF symbolColor = RGB(0, 0, 0); // default fallback color

            switch (maze[row][col]) {
            case COLLECTIBLE:
                symbol = L"💎";
                // Use a diamond-like blue for collectibles.
                symbolColor = RGB(0, 191, 255);
                break;
            case HAZARD:
                symbol = L"☠️";
                // Use red for hazards.
                symbolColor = RGB(255, 0, 0);
                break;
            case OBSTACLE:
                symbol = L"🚧";
                // Use purple for obstacles.
                symbolColor = RGB(128, 0, 128);
                break;
            case MINIDOT:
                symbol = L"•";
                // Use gold for mini-dots (score items).
                symbolColor = RGB(255, 215, 0);
                break;
            default:
                // For PASSAGE or any cell without decoration, no symbol is drawn.
                break;
            }
            if (!symbol.empty()) {
                // Measure text size.
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
    // Do NOT override the starting cell; leave it as a normal passage.

    // For the ending cell, fill with dark orange instead of red.
    RECT endRect = { (GRID_COLS - 1) * CELL_SIZE, (GRID_ROWS - 1) * CELL_SIZE,
                     GRID_COLS * CELL_SIZE, GRID_ROWS * CELL_SIZE };
    HBRUSH endBrush = CreateSolidBrush(RGB(255, 140, 0)); // Dark orange.
    FillRect(hdc, &endRect, endBrush);
    DeleteObject(endBrush);

    // Overlay a door Unicode symbol ("⛩️") at the ending cell.
    std::wstring doorSymbol = L"⛩️";
    SIZE doorSize;
    GetTextExtentPoint32(hdc, doorSymbol.c_str(), doorSymbol.length(), &doorSize);
    int doorX = (GRID_COLS - 1) * CELL_SIZE + (CELL_SIZE - doorSize.cx) / 2;
    int doorY = (GRID_ROWS - 1) * CELL_SIZE + (CELL_SIZE - doorSize.cy) / 2;
    SetBkMode(hdc, TRANSPARENT);
    // Use white for the door symbol.
    SetTextColor(hdc, RGB(255, 255, 255));
    TextOut(hdc, doorX, doorY, doorSymbol.c_str(), doorSymbol.length());

    // Draw the player as a Unicode character ("🤺") with blue color.
    int playerX = playerPosition.x * CELL_SIZE;
    int playerY = playerPosition.y * CELL_SIZE;
    std::wstring playerSymbol = L"🤺";
    SIZE pSize;
    GetTextExtentPoint32(hdc, playerSymbol.c_str(), playerSymbol.length(), &pSize);
    int pTextX = playerX + (CELL_SIZE - pSize.cx) / 2;
    int pTextY = playerY + (CELL_SIZE - pSize.cy) / 2;
    SetTextColor(hdc, RGB(0, 0, 255));
    TextOut(hdc, pTextX, pTextY, playerSymbol.c_str(), playerSymbol.length());

    // Draw the HUD (lives, time, score) to the right of the maze.
    // Increase the width of the HUD rectangle to display larger numbers.
    std::wstring hud = L"Lives: " + ConvertToWString(lives) +
        L"\nTime: " + ConvertToWString(timeLeft) +
        L"\nScore: " + ConvertToWString(score);
    RECT hudRect = { GRID_COLS * CELL_SIZE + 10, 10, GRID_COLS * CELL_SIZE + 250, 150 };
    DrawText(hdc, hud.c_str(), -1, &hudRect, DT_LEFT | DT_TOP);

    // Restore the original font.
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

// Update player movement to handle obstacles, collectibles, mini-dots (for score), and hazards.
void MovePlayer(int dx, int dy) {
    int newX = playerPosition.x + dx;
    int newY = playerPosition.y + dy;
    if (newX >= 0 && newX < GRID_COLS && newY >= 0 && newY < GRID_ROWS) {
        int cellValue = levels[currentLevel][newY][newX];
        if (cellValue == WALL || cellValue == OBSTACLE) {
            // Cannot move into a wall or an obstacle.
            return;
        }
        else if (cellValue == HAZARD) {
            // Hazard: deduct a life and 5 seconds, then reset player.
            lives--;
            //timeLeft = (timeLeft > 10) ? timeLeft - 5 : 0;
            playerPosition = { 0, 0 };
            PlayGameSound(L"hazard01.wav"); // Play hazard sound
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
            // Collectible: gain one life.
            lives++;
            timeLeft = timeLeft + 5;
            PlayGameSound(L"powerup.wav"); // Play power-up sound
            levels[currentLevel][newY][newX] = PASSAGE; // Remove collectible.
        }
        else if (cellValue == MINIDOT) {
            // Mini-dot: collect for score.
            score++;
            levels[currentLevel][newY][newX] = PASSAGE; // Remove dot.
        }
        // For a passage cell (PASSAGE), allow movement.
        playerMoveHistory.push(playerPosition);
        playerPosition.x = newX;
        playerPosition.y = newY;
    }
    // Check if the player has reached the end.
    if (playerPosition.x == endPosition.x && playerPosition.y == endPosition.y) {
        if (currentLevel < TOTAL_LEVELS - 1) {
            currentLevel++;
            PlayGameSound(L"lvl.wav"); // Level transfer sound
            playerPosition = { 0, 0 };
            timeLeft = 25;  // Reset timer for next level.
        }
        else {
            KillTimer(hWndMain, TIMER_ID);
            PlayGameSound(L"win01.wav"); // Winning sound
            MessageBox(hWndMain, L"Congratulations! You've completed all levels!", L"Victory", MB_OK);
            PostQuitMessage(0);
        }
    }
    InvalidateRect(hWndMain, NULL, TRUE);
}

// Window procedure.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // Double buffering to reduce flicker.
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
        DrawMaze(hdcMem);
        BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);
        EndPaint(hWnd, &ps);
    }
                 break;

    case WM_KEYDOWN:
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
        }
        break;

    case WM_TIMER:
        if (wParam == TIMER_ID) {
            timeLeft--;
            if (timeLeft <= 0) {
                lives--;  // Deduct one life when time runs out.
                if (lives <= 0) {
                    KillTimer(hWndMain, TIMER_ID);
                    MessageBox(hWndMain, L"Time's up and no lives remaining. Game Over.", L"Game Over", MB_OK);
                    PostQuitMessage(0);
                    break;
                }
                else {
                    ShowPausedMessage(L"Time's up! Restarting level.", L"Timer");
                    timeLeft = 25;      // Reset timer.
                    playerPosition = { 0, 0 };  // Reset player position.
                }
            }
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_ERASEBKGND:
        // Prevent background erasing to reduce flicker.
        return 1;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    srand((unsigned int)time(NULL));  // Seed the random number generator.
    hInst = hInstance;

    // Register the window class.
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MazeGameClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    // Create the main window.
    hWndMain = CreateWindow(L"MazeGameClass", L"Maze Game",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CELL_SIZE * GRID_COLS + 150, CELL_SIZE * GRID_ROWS + 40,
        nullptr, nullptr, hInstance, nullptr);
    if (!hWndMain)
        return FALSE;
    ShowWindow(hWndMain, nCmdShow);

    // Generate the decorated maze levels.
    GenerateRandomLevels();

    // Set a timer (1000 ms interval).
    SetTimer(hWndMain, TIMER_ID, TIMER_INTERVAL, NULL);

    // Play background music in a separate thread.
    std::thread bgMusicThread(PlayBackgroundMusic, L"background 01.mp3");
    bgMusicThread.detach();

    // Main message loop.
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Stop background music when the game ends.
    StopBackgroundMusic();

    return (int)msg.wParam;
}
