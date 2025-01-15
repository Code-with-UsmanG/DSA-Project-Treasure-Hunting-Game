#include "framework.h"
#include <windows.h>
#include <vector>
#include <iostream>
#include <mmsystem.h>
#include <sstream> // For stringstream conversion
#include <thread>
#include <stack>
#include <queue>
#include <string>

#define CELL_SIZE 50 // Size of each cell in the maze
#define GRID_ROWS 10 // Number of rows in the maze
#define GRID_COLS 10 // Number of columns in the maze
#define TIMER_ID 1 // Timer identifier
#define TIMER_INTERVAL 1000 // Timer interval in milliseconds (1 second)

// Global Variables
HINSTANCE hInst;
HWND hWndMain;
POINT playerPosition = { 0, 0 }; // Player's starting position
int currentLevel = 0; // The current level the player is on
int lives = 2; // Player lives
int timeLeft = 20; // Time remaining to complete the level
bool timeOutDisplayed = false;  // Flag to track if time's up message was shown
int score = 0; // Initial score
bool isPlayingBackgroundMusic = true; // Flag to control background music

// Define a stack to track player moves
std::stack<POINT> playerMoveHistory;
// Define a queue to manage game events
std::queue<std::wstring> eventQueue;

// Define multiple levels (as 2D arrays)
// Add `-1` for hazardous obstacles (e.g., traps or spikes)
std::vector<std::vector<std::vector<int>>> levels = {
    { // Level 1
        {0, 1, 3, 3, 3, 3, 3, 3, 3, 3},
        {3, 3, 2, 1, 1, 1, 1, 1, 1, 3},
        {3, 3, 3, 3, -1, 3, 3, 3, 1, 3}, // Hazard at (2, 4)
        {3, 1, 3, 1, 3, 1, 1, 3, 1, 3},
        {3, 1, 3, 1, 3, 1, 3, 3, 1, 3},
        {3, 1, 3, 1, 3, 1, 3, 1, 1, 3},
        {3, 1, 3, 3, 3, 1, 3, 3, 3, 3},
        {3, 1, 1, 1, 1, 1, 3, 1, 1, 3},
        {3, 3, 3, 3, 3, 3, 3, 1, 3, 3},
        {3, 3, 3, 3, 3, 3, 1, 1, 3, 0}
    },
    { // Level 2
        {0, 3, 3, 1, 3, 3, 1, 1, 3, 3},
        {3, 1, 3, 1, 1, 3, 3, 3, 3, 3},
        {3, 1, 3, 3, 3, 3, 3, 1, -1, 3}, // Hazard at (2, 8)
        {3, 3, 3, 1, 1, 1, 3, 3, 1, 3},
        {3, 1, 2, 3, 1, 3, 3, 3, 3, 3},
        {3, 3, 1, 3, 3, 1, 3, 3, 1, 3},
        {1, 1, 3, 3, 1, 3, 3, 3, 3, 3},
        {1, 3, 1, 1, 3, 1, 1, 3, 1, 3},
        {3, 3, 3, 3, 3, 3, 1, 3, 1, 3},
        {3, 1, 3, 3, 3, 1, 3, 3, 3, 0}
    },
    { // Level 3
        {0, 3, 3, 3, 3, 3, 1, 3, 1, 3},
        {1, 3, 1, 2, 1, 3, 1, 1, 3, 1},
        {1, 1, 1, 3, 3, 3, 3, -1, 1, 3}, // Hazard at (2, 7)
        {3, 1, 3, 1, 3, 3, 1, 3, 1, 3},
        {1, 3, 3, 3, 3, 1, 1, 3, 3, 1},
        {3, 1, 1, 1, 3, 3, 1, 3, 3, 3},
        {3, 3, 1, 3, 3, 3, 3, 3, 1, 3},
        {1, 1, 3, 1, 3, 1, 1, 1, 1, 1},
        {3, 3, 3, 3, 3, 1, 3, 3, 3, 3},
        {3, 1, 3, 1, 3, 3, 3, 1, 3, 0}
    },
    { // Level 4
    {0, 3, 1, 3, 3, 3, 1, 3, 1, 3},
    {3, 1, 1, 2, 1, 3, 1, 1, 3, 1},
    {3, 3, 3, 3, 1, 3, 3, 3, 1, 3},
    {1, 1, 3, 1, 3, 3, 1, 3, 1, 3},
    {1, 3, 3, 3, 3, 1, 1, -1, 3, 1}, // Hazard at (4, 8)
    {3, 1, 1, 1, 3, 3, 1, 3, 3, 3},
    {3, 3, 1, 3, -1, 3, 3, 3, 1, 3},
    {1, 1, 3, 1, 3, 1, 3, 1, 3, 3},
    {1, 3, 1, 3, 3, 3, 1, 1, 3, 1},
    {3, 1, 3, 3, 3, 3, 3, 3, 3, 0}
    },
    { // Level 5
    {0, 1, 3, 1, 3, 3, 1, 3, 1, 3},
    {3, 3, 3, 2, 1, 3, 1, 1, 3, 1},
    {1, 1, 1, 3, 3, 3, 3, -1, 1, 3},
    {3, 1, 3, 1, 3, 3, 1, 3, 1, 3},
    {1, 3, 3, 3, 3, 1, 1, 3, 3, 1},
    {3, 1, 1, 1, 3, -1, 1, 3, 1, 3},
    {3, 3, 1, 2, 3, 3, 1, 3, 1, 3},
    {1, 1, 3, 1, 3, 1, 3, 3, 3, 3},
    {1, 3, 1, 3, 3, 3, 3, 1, -1, 3}, // Hazard at (8,8)
    {3, 1, 3, 3, 3, 1, 3, 3, 3, 0}
    }
};

POINT endPosition = { 9, 9 }; // Endpoint of the maze

//Forward Declaration
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawMaze(HDC hdc);
void MovePlayer(int dx, int dy);
void NextLevel();
std::wstring ConvertToWString(int number); // Conversion function for integers to wide strings
void RestartLevel(); // Function to restart the level if time runs out or on hazard hit
void PlayGameSound(const std::wstring& soundFile);
void PlayBackgroundMusic(const std::wstring& soundFile);
void StopBackgroundMusic();
void UndoMove();
void ProcessEvents();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;

    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MazeGameClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    hWndMain = CreateWindow(L"MazeGameClass", L"Maze Game", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CELL_SIZE * GRID_COLS + 20, CELL_SIZE * GRID_ROWS + 40,
        nullptr, nullptr, hInstance, nullptr);

    if (!hWndMain) {
        return FALSE;
    }

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

    // Start the timer
    SetTimer(hWndMain, TIMER_ID, TIMER_INTERVAL, nullptr);
    //startup sound
    PlayGameSound(L"thrill01.wav");

    Sleep(2000); //delay of 3 seconds
    // Play background music in a separate thread
    std::thread bgMusicThread(PlayBackgroundMusic, L"background 01.mp3");
    bgMusicThread.detach(); // Detach the thread to run independently

    // Main message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Stop background music when the game ends
    StopBackgroundMusic();

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // Double buffering setup
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hbmMem = CreateCompatibleBitmap(hdc, CELL_SIZE * GRID_COLS + CELL_SIZE * 3, CELL_SIZE * GRID_ROWS);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

        // Clear the background of the buffer
        HBRUSH hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
        RECT bufferRect = { 0, 0, CELL_SIZE * GRID_COLS + CELL_SIZE * 3, CELL_SIZE * GRID_ROWS };
        FillRect(hdcMem, &bufferRect, hbrBackground);
        DeleteObject(hbrBackground);

        // Draw the maze onto the buffer
        DrawMaze(hdcMem);

        // Draw the mini box (2x3)
        int miniBoxStartX = GRID_COLS * CELL_SIZE + 20; // Padding for mini box
        int miniBoxStartY = 20; // Padding from top
        RECT miniBox = {
            miniBoxStartX,
            miniBoxStartY,
            miniBoxStartX + CELL_SIZE * 3, // 3 columns
            miniBoxStartY + CELL_SIZE * 2  // 2 rows
        };
        HBRUSH miniBoxBrush = CreateSolidBrush(RGB(200, 200, 200)); // Gray color for mini box
        FillRect(hdcMem, &miniBox, miniBoxBrush);
        DeleteObject(miniBoxBrush);

        // Draw border around the mini box
        FrameRect(hdcMem, &miniBox, (HBRUSH)GetStockObject(BLACK_BRUSH));

        // Draw the "Lives," "Time," and "Score" text over the mini box
        HFONT hFont = CreateFont(
            20,                  // Height of the font
            0,                   // Width of the font
            0,                   // Escapement angle
            0,                   // Orientation angle
            FW_BOLD,             // Font weight
            FALSE,               // Italic
            FALSE,               // Underline
            FALSE,               // Strikeout
            DEFAULT_CHARSET,     // Character set
            OUT_OUTLINE_PRECIS,  // Output precision
            CLIP_DEFAULT_PRECIS, // Clipping precision
            CLEARTYPE_QUALITY,   // Output quality
            VARIABLE_PITCH,      // Pitch and family
            L"Segoe UI"          // Font name
        );

        HFONT oldFont = (HFONT)SelectObject(hdcMem, hFont);
        SetBkMode(hdcMem, TRANSPARENT); // Transparent background for text
        SetTextColor(hdcMem, RGB(0, 0, 0)); // Black text color

        // Display player lives
        std::wstring livesText = L"Lives: " + ConvertToWString(lives);
        TextOut(hdcMem, miniBoxStartX + 10, miniBoxStartY + 10, livesText.c_str(), livesText.length());

        // Display time remaining
        std::wstring timerText = L"Time: " + ConvertToWString(timeLeft) + L" sec";
        TextOut(hdcMem, miniBoxStartX + 10, miniBoxStartY + 40, timerText.c_str(), timerText.length());

        // Display the score
        std::wstring scoreText = L"Score: " + ConvertToWString(score);
        TextOut(hdcMem, miniBoxStartX + 10, miniBoxStartY + 70, scoreText.c_str(), scoreText.length());

        // Restore font and cleanup
        SelectObject(hdcMem, oldFont);
        DeleteObject(hFont);

        // Copy the buffer to the screen
        BitBlt(hdc, 0, 0, CELL_SIZE * GRID_COLS + CELL_SIZE * 3, CELL_SIZE * GRID_ROWS, hdcMem, 0, 0, SRCCOPY);

        // Cleanup
        SelectObject(hdcMem, hbmOld);
        DeleteObject(hbmMem);
        DeleteDC(hdcMem);

        EndPaint(hWnd, &ps);
    } break;

    case WM_KEYDOWN: {
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
        case 'U':
            UndoMove();
            break; // Undo the last move
        }
        InvalidateRect(hWnd, nullptr, FALSE); // Trigger repaint without erasing background
    } break;

    case WM_TIMER: {
        if (wParam == TIMER_ID) {
            timeLeft--;
            if (timeLeft <= 0 && !timeOutDisplayed) {
                lives--;
                if (lives == 0)
                {
                    MessageBox(hWndMain, L"Game Over! Exiting...", L"Maze Game", MB_OK);
                    Sleep(1000);
                    PostQuitMessage(0); // End the game
                }
                timeOutDisplayed = true; // Set the flag to true to avoid showing again
                if (lives > 0)
                {
                    MessageBox(hWnd, L"Time's up! Restarting level.", L"Timer Alert", MB_OK);
                    RestartLevel();
                    timeOutDisplayed = false; // Reset the flag after restarting the level
                }
            }
            InvalidateRect(hWnd, nullptr, FALSE); // Trigger repaint without erasing background
        }
    } break;

    case WM_DESTROY:
        KillTimer(hWnd, TIMER_ID); // Stop the timer when the window is destroyed
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Function to play sound
void PlayGameSound(const std::wstring& soundFile) {
    PlaySound(soundFile.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}

void PlayBackgroundMusic(const std::wstring& soundFile) {
    // Open the music file
    std::wstring command = L"open \"" + soundFile + L"\" type mpegvideo alias bgm";
    mciSendString(command.c_str(), nullptr, 0, nullptr);

    // Loop the music
    mciSendString(L"play bgm repeat", nullptr, 0, nullptr);
}

void StopBackgroundMusic() {
    mciSendString(L"stop bgm", nullptr, 0, nullptr);
    mciSendString(L"close bgm", nullptr, 0, nullptr);
}

void UndoMove() {
    if (!playerMoveHistory.empty()) {
        // Revert to the previous position
        playerPosition = playerMoveHistory.top();
        playerMoveHistory.pop();

        // Redraw the maze
        InvalidateRect(hWndMain, nullptr, TRUE);
    }
    else {
        eventQueue.push(L"No moves to undo!"); // Add event to queue
        ProcessEvents();
    }
}

void ProcessEvents() {
    while (!eventQueue.empty()) {
        std::wstring event = eventQueue.front();
        eventQueue.pop();

        // Display the event to the player
        MessageBox(hWndMain, event.c_str(), L"Game Event", MB_OK);
    }
}

void DrawMaze(HDC hdc) {
    std::vector<std::vector<int>> currentMaze = levels[currentLevel];

    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int col = 0; col < GRID_COLS; ++col) {
            RECT cell = { col * CELL_SIZE, row * CELL_SIZE, (col + 1) * CELL_SIZE, (row + 1) * CELL_SIZE };
            HBRUSH brush;

            // Determine the cell color based on its type
            if (currentMaze[row][col] == 1) {
                brush = CreateSolidBrush(RGB(0, 0, 0)); // Wall color
            }
            else if (currentMaze[row][col] == 3) {
                brush = CreateSolidBrush(RGB(255, 255, 255)); // Path color
            }
            else if (currentMaze[row][col] == -1) {
                brush = CreateSolidBrush(RGB(255, 255, 255)); // White background for hazard
            }
            else if (currentMaze[row][col] == 2) {
                brush = CreateSolidBrush(RGB(255, 255, 255)); // Power-up color (cyan)
            }
            else if (endPosition.x == col && endPosition.y == row) {
                brush = CreateSolidBrush(RGB(255, 0, 0)); // Red background for the door (end point)
            }
            else {
                brush = CreateSolidBrush(RGB(255, 255, 255)); // Boundary color
            }

            // Fill the cell and draw grid lines
            FillRect(hdc, &cell, brush);
            DeleteObject(brush);
            FrameRect(hdc, &cell, (HBRUSH)GetStockObject(BLACK_BRUSH));

            // Draw hazard character in the cell
            if (currentMaze[row][col] == -1) {
                HFONT hFont = CreateFont(
                    40,                  // Height of the font (adjust as needed to fit within the grid)
                    0,                   // Width of the font (0 for default aspect ratio)
                    0,                   // Escapement angle
                    0,                   // Orientation angle
                    FW_BOLD,             // Font weight (FW_NORMAL or FW_BOLD)
                    FALSE,               // Italic
                    FALSE,               // Underline
                    FALSE,               // Strikeout
                    DEFAULT_CHARSET,     // Character set
                    OUT_OUTLINE_PRECIS,  // Output precision
                    CLIP_DEFAULT_PRECIS, // Clipping precision
                    CLEARTYPE_QUALITY,   // Output quality
                    VARIABLE_PITCH,      // Pitch and family
                    L"Segoe UI Emoji"    // Font name (supports emoji)
                );

                HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

                // Hazard symbol (☠️)
                std::wstring hazardChar = L"☠️";

                // Calculate the position of the emoji within the grid cell
                SIZE textSize;
                GetTextExtentPoint32(hdc, hazardChar.c_str(), hazardChar.length(), &textSize);

                int textX = col * CELL_SIZE + (CELL_SIZE - textSize.cx) / 2; // Center horizontally
                int textY = row * CELL_SIZE + (CELL_SIZE - textSize.cy) / 2; // Center vertically

                // Set text properties
                SetBkMode(hdc, TRANSPARENT);          // Transparent background for text
                SetTextColor(hdc, RGB(0, 0, 0));    // Text color (Red for hazard)

                // Draw the hazard emoji
                TextOut(hdc, textX, textY, hazardChar.c_str(), hazardChar.length());

                // Restore the previous font and clean up
                SelectObject(hdc, oldFont);
                DeleteObject(hFont);
            }

            if (currentMaze[row][col] == 2) { // Power-up (collectible)
                brush = CreateSolidBrush(RGB(0, 255, 255)); // Power-up color (cyan)

                // Define the diamond symbol (🍒)
                HFONT hFont = CreateFont(
                    40,                  // Height of the font (adjust as needed to fit within the grid)
                    0,                   // Width of the font (0 for default aspect ratio)
                    0,                   // Escapement angle
                    0,                   // Orientation angle
                    FW_BOLD,             // Font weight (FW_NORMAL or FW_BOLD)
                    FALSE,               // Italic
                    FALSE,               // Underline
                    FALSE,               // Strikeout
                    DEFAULT_CHARSET,     // Character set
                    OUT_OUTLINE_PRECIS,  // Output precision
                    CLIP_DEFAULT_PRECIS, // Clipping precision
                    CLEARTYPE_QUALITY,   // Output quality
                    VARIABLE_PITCH,      // Pitch and family
                    L"Segoe UI Emoji"    // Font name (supports emoji)
                );

                HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

                // Diamond symbol (💎)
                std::wstring diamondChar = L"💎";

                // Calculate the position of the emoji within the grid cell
                SIZE textSize;
                GetTextExtentPoint32(hdc, diamondChar.c_str(), diamondChar.length(), &textSize);

                int textX = col * CELL_SIZE + (CELL_SIZE - textSize.cx) / 2; // Center horizontally
                int textY = row * CELL_SIZE + (CELL_SIZE - textSize.cy) / 2; // Center vertically

                // Set text properties
                SetBkMode(hdc, TRANSPARENT);          // Transparent background for text
                SetTextColor(hdc, RGB(35, 178, 255));  // Text color (Cyan for the diamond)

                // Draw the diamond emoji
                TextOut(hdc, textX, textY, diamondChar.c_str(), diamondChar.length());

                // Restore the previous font and clean up
                SelectObject(hdc, oldFont);
                DeleteObject(hFont);
            }

            if (currentMaze[row][col] == 3) { // Mini-dot
                HFONT hFont = CreateFont(
                    30,                  // Smaller font size for mini-dots
                    0,                   // Width of the font (0 for default aspect ratio)
                    0,                   // Escapement angle
                    0,                   // Orientation angle
                    FW_BOLD,             // Font weight
                    FALSE,               // Italic
                    FALSE,               // Underline
                    FALSE,               // Strikeout
                    DEFAULT_CHARSET,     // Character set
                    OUT_OUTLINE_PRECIS,  // Output precision
                    CLIP_DEFAULT_PRECIS, // Clipping precision
                    CLEARTYPE_QUALITY,   // Output quality
                    VARIABLE_PITCH,      // Pitch and family
                    L"Segoe UI Emoji"    // Font name
                );

                HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

                // Mini-dot symbol (•)
                std::wstring miniDotChar = L"•";

                // Calculate position within the cell
                SIZE textSize;
                GetTextExtentPoint32(hdc, miniDotChar.c_str(), miniDotChar.length(), &textSize);

                int textX = col * CELL_SIZE + (CELL_SIZE - textSize.cx) / 2; // Center horizontally
                int textY = row * CELL_SIZE + (CELL_SIZE - textSize.cy) / 2; // Center vertically

                // Set text properties
                SetBkMode(hdc, TRANSPARENT);          // Transparent background for text
                SetTextColor(hdc, RGB(255, 215, 0));  // Gold color for mini-dots

                // Draw the mini-dot
                TextOut(hdc, textX, textY, miniDotChar.c_str(), miniDotChar.length());

                // Restore font and clean up
                SelectObject(hdc, oldFont);
                DeleteObject(hFont);
            }

            // Draw the door character at the end position
            if (endPosition.x == col && endPosition.y == row) {
                HFONT hFont = CreateFont(
                    40,                  // Height of the font (adjust as needed to fit within the grid)
                    0,                   // Width of the font (0 for default aspect ratio)
                    0,                   // Escapement angle
                    0,                   // Orientation angle
                    FW_BOLD,             // Font weight (FW_NORMAL or FW_BOLD)
                    FALSE,               // Italic
                    FALSE,               // Underline
                    FALSE,               // Strikeout
                    DEFAULT_CHARSET,     // Character set
                    OUT_OUTLINE_PRECIS,  // Output precision
                    CLIP_DEFAULT_PRECIS, // Clipping precision
                    CLEARTYPE_QUALITY,   // Output quality
                    VARIABLE_PITCH,      // Pitch and family
                    L"Segoe UI Emoji"    // Font name (supports emoji)
                );

                HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

                // Door symbol (🚪)
                std::wstring doorChar = L"🚪";

                // Calculate the position of the emoji within the grid cell
                SIZE textSize;
                GetTextExtentPoint32(hdc, doorChar.c_str(), doorChar.length(), &textSize);

                int textX = col * CELL_SIZE + (CELL_SIZE - textSize.cx) / 2; // Center horizontally
                int textY = row * CELL_SIZE + (CELL_SIZE - textSize.cy) / 2; // Center vertically

                // Set text properties
                SetBkMode(hdc, TRANSPARENT);          // Transparent background for text
                SetTextColor(hdc, RGB(255, 255, 255)); // Text color (White for door)

                // Draw the door emoji
                TextOut(hdc, textX, textY, doorChar.c_str(), doorChar.length());

                // Restore the previous font and clean up
                SelectObject(hdc, oldFont);
                DeleteObject(hFont);
            }
        }
    }

    // Draw the player character using a custom font
    HFONT hFont = CreateFont(
        50,                  // Height of the font (increase for larger size)
        0,                   // Width of the font (0 for default aspect ratio)
        0,                   // Escapement angle
        0,                   // Orientation angle
        FW_BOLD,             // Font weight (FW_NORMAL or FW_BOLD)
        FALSE,               // Italic
        FALSE,               // Underline
        FALSE,               // Strikeout
        DEFAULT_CHARSET,     // Character set
        OUT_OUTLINE_PRECIS,  // Output precision
        CLIP_DEFAULT_PRECIS, // Clipping precision
        CLEARTYPE_QUALITY,   // Output quality
        VARIABLE_PITCH,      // Pitch and family
        L"Segoe UI Emoji"    // Font name (supports emoji)
    );

    HFONT oldFont = (HFONT)SelectObject(hdc, hFont);

    // Define the player emoji
    std::wstring playerChar = L"🤺";

    // Calculate the position of the emoji within the grid cell
    SIZE textSize;
    GetTextExtentPoint32(hdc, playerChar.c_str(), playerChar.length(), &textSize);

    int textX = playerPosition.x * CELL_SIZE + (CELL_SIZE - textSize.cx) / 2; // Center horizontally
    int textY = playerPosition.y * CELL_SIZE + (CELL_SIZE - textSize.cy) / 2; // Center vertically

    // Set text properties
    SetBkMode(hdc, TRANSPARENT);          // Transparent background for text
    SetTextColor(hdc, RGB(0, 25, 255));    // Text color (Blue for the player)

    // Draw the player emoji
    TextOut(hdc, textX, textY, playerChar.c_str(), playerChar.length());

    // Restore the previous font and clean up
    SelectObject(hdc, oldFont);
    DeleteObject(hFont);
}

void MovePlayer(int dx, int dy) {
    int newX = playerPosition.x + dx;
    int newY = playerPosition.y + dy;

    std::vector<std::vector<int>>& currentMaze = levels[currentLevel];

    if (newX >= 0 && newX < GRID_COLS && newY >= 0 && newY < GRID_ROWS) {
        if (currentMaze[newY][newX] == 0) { // Starting Point
            playerMoveHistory.push(playerPosition); // Save current position to the stack
            playerPosition.x = newX;
            playerPosition.y = newY;
        }
        else if (currentMaze[newY][newX] == -1) { // Hazard
            lives--; // Decrease a life on hazard hit
            eventQueue.push(L"You hit a hazard! Restarting level."); // Add event to queue
            PlayGameSound(L"hazard01.wav"); // Play hazard sound
            RestartLevel();
        }
        else if (currentMaze[newY][newX] == 2) { // Power-up
            lives++; // Increase lives
            timeLeft += 5; // Add extra time
            currentMaze[newY][newX] = 0; // Remove the power-up from the maze
            playerMoveHistory.push(playerPosition); // Save current position to the stack
            playerPosition.x = newX;
            playerPosition.y = newY;
            PlayGameSound(L"powerup.wav"); // Play power-up sound
            eventQueue.push(L"You collected a power-up! +1 life, +5 seconds."); // Add event to queue
        }
        else if (currentMaze[newY][newX] == 3) { // Mini-dot
            score += 1; // Increment score
            currentMaze[newY][newX] = 0; // Remove the mini-dot from the maze
            playerMoveHistory.push(playerPosition); // Save current position to the stack
            playerPosition.x = newX;
            playerPosition.y = newY;
            PlayGameSound(L"mov.wav"); // Play movement sound
        }
    }

    // Check if the player has reached the end position
    if (playerPosition.x == endPosition.x && playerPosition.y == endPosition.y) {
        if (currentLevel == levels.size() - 1) { // If last level
            PlayGameSound(L"win01.wav"); // Play winning sound
            eventQueue.push(L"You Won!"); // Add win event to queue
            Sleep(2000);
            PostQuitMessage(0); // End the game
        }
        else {
            PlayGameSound(L"lvl.wav"); // Play transfer(teleport) sound
            NextLevel();
        }
    }

    // Process queued events
    ProcessEvents();

    // Redraw the maze to reflect changes
    InvalidateRect(hWndMain, nullptr, TRUE);
}

void NextLevel() {
    currentLevel++;
    playerPosition = { 0, 0 }; // Reset player position
    timeLeft = 20; // Reset timer
}

void RestartLevel() {
    if (lives == 0)
    {
        MessageBox(hWndMain, L"Game Over! Exiting...", L"Maze Game", MB_OK);
        Sleep(1000);
        PostQuitMessage(0); // End the game
    }
    playerPosition = { 0, 0 }; // Reset player position
    timeLeft = 20; // Reset timer
    InvalidateRect(hWndMain, nullptr, TRUE);
}

std::wstring ConvertToWString(int number) {
    std::wstringstream ss;
    ss << number;
    return ss.str();
}
