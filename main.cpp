
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

// Function to get console dimensions
void getConsoleSize(int& width, int& height) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
#else
    struct winsize ws;
    ioctl(STDOUT_FILING, TIOCGWINSZ, &ws);
    width = ws.ws_col;
    height = ws.ws_row;
#endif
}

int main() {
    // Game setup
    int nScreenWidth, nScreenHeight;
    getConsoleSize(nScreenWidth, nScreenHeight);

    // Player position and direction
    float fPlayerX = 8.0f;
    float fPlayerY = 8.0f;
    float fPlayerA = 0.0f; // Player Angle

    // Field of View
    float fFOV = 3.14159f / 4.0f;

    // Maximum rendering distance
    float fDepth = 16.0f;

    // Map dimensions
    int nMapWidth = 16;
    int nMapHeight = 16;

    // Game map
    std::wstring map;
    map += L"################";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"#..............#";
    map += L"################";

    // Screen buffer
    wchar_t* screen = new wchar_t[nScreenWidth * nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    auto tp1 = std::chrono::system_clock::now();
    auto tp2 = std::chrono::system_clock::now();

    // Game loop
    while (1) {
        tp2 = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();

        // Handle player input
        // Rotation
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000) {
            fPlayerA -= (0.8f) * fElapsedTime;
        }
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000) {
            fPlayerA += (0.8f) * fElapsedTime;
        }

        // Movement
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000) {
            fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
            if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
                fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }
        if (GetAsyncKeyState((unsigned short)'S') & 0x8000) {
            fPlayerX -= sinf(fPlayerA) * 5.0f * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * 5.0f * fElapsedTime;
            if (map[(int)fPlayerY * nMapWidth + (int)fPlayerX] == '#') {
                fPlayerX += sinf(fPlayerA) * 5.0f * fElapsedTime;
                fPlayerY += cosf(fPlayerA) * 5.0f * fElapsedTime;
            }
        }

        // Raycasting loop
        for (int x = 0; x < nScreenWidth; x++) {
            // Calculate ray angle
            float fRayAngle = (fPlayerA - fFOV / 2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            float fStepSize = 0.1f;
            float fDistanceToWall = 0.0f;
            bool bHitWall = false;
            bool bBoundary = false;

            float fEyeX = sinf(fRayAngle);
            float fEyeY = cosf(fRayAngle);

            while (!bHitWall && fDistanceToWall < fDepth) {
                fDistanceToWall += fStepSize;
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Test if ray is out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight) {
                    bHitWall = true;
                    fDistanceToWall = fDepth;
                }
                else {
                    // Ray is in bounds, test if it hits a wall
                    if (map[nTestY * nMapWidth + nTestX] == '#') {
                        bHitWall = true;

                        // Check for boundary conditions
                        std::vector<std::pair<float, float>> p;

                        for (int tx = 0; tx < 2; tx++) {
                            for (int ty = 0; ty < 2; ty++) {
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (fEyeX * vx + fEyeY * vy) / d;
                                p.push_back(std::make_pair(d, dot));
                            }
                        }

                        // Sort pairs by distance
                        std::sort(p.begin(), p.end(), [](const std::pair<float, float>& left, const std::pair<float, float>& right) {
                            return left.first < right.first;
                        });

                        float fBound = 0.01f;
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                        if (acos(p.at(2).second) < fBound) bBoundary = true;
                    }
                }
            }

            // Calculate ceiling and floor
            int nCeiling = (float)(nScreenHeight / 2.0) - nScreenHeight / ((float)fDistanceToWall);
            int nFloor = nScreenHeight - nCeiling;

            // Render wall based on distance
            short nShade = ' ';
            if (fDistanceToWall <= fDepth / 4.0f)       nShade = 0x2588; // Very close
            else if (fDistanceToWall < fDepth / 3.0f)  nShade = 0x259B;
            else if (fDistanceToWall < fDepth / 2.0f)  nShade = 0x2592;
            else if (fDistanceToWall < fDepth / 1.0f)  nShade = 0x2591; // Far
            else                                       nShade = ' ';

            if (bBoundary) nShade = ' ';

            for (int y = 0; y < nScreenHeight; y++) {
                if (y < nCeiling) {
                    screen[y * nScreenWidth + x] = ' ';
                }
                else if (y > nCeiling && y <= nFloor) {
                    screen[y * nScreenWidth + x] = nShade;
                }
                else {
                    // Render floor
                    short nFloorShade = ' ';
                    float b = 1.0f - (((float)y - nScreenHeight / 2.0f) / ((float)nScreenHeight / 2.0f));
                    if (b < 0.25)       nFloorShade = '#';
                    else if (b < 0.5)   nFloorShade = 'x';
                    else if (b < 0.75)  nFloorShade = '.';
                    else if (b < 0.9)   nFloorShade = '-';
                    else                nFloorShade = ' ';
                    screen[y * nScreenWidth + x] = nFloorShade;
                }
            }
        }

        // Display map
        for (int nx = 0; nx < nMapWidth; nx++) {
            for (int ny = 0; ny < nMapHeight; ny++) {
                screen[(ny + 1) * nScreenWidth + nx] = map[ny * nMapWidth + nx];
            }
        }

        screen[((int)fPlayerY + 1) * nScreenWidth + (int)fPlayerX] = 'P';

        // Display frame rate
        swprintf(screen, 40, L"X=%0.2f, Y=%0.2f, A=%0.2f, FPS=%0.2f", fPlayerX, fPlayerY, fPlayerA, 1.0f / fElapsedTime);

        // Display screen
        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
    }

    return 0;
}
