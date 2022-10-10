#include <iostream>
#include <Windows.h>
#include <chrono>
#include <cmath>
#include <vector>
#include <cstdio>
#include <utility>
#include <algorithm>

int ScreenWidth = 120;
int ScreenHeight = 40;
float VectorX = 1.0f;
float VectorY = 1.0f;
float VectorA = 0.0f;
int gameHeight = 16;
int gameWidth = 16;
float visibilityZone = 3.14159 / 3;
float visibilityDistance = 30.0f;

std::string generateMap() {
    std::string map;
    map += "################";
    map += "#..............#";
    for (int i = 0; i < gameHeight - 3; i++) {
        std::string a = "#";
        for (int j = 0; j < gameWidth - 2; ++j) {
            if ((rand() % 2) == 1) {
                a += ".";
            } else {
                a += "#";
            }
        }
        a += "#";
        map += a;
    }


    map += "#######...######";
    return map;
}

int main() {
    auto screen = new char[ScreenWidth * ScreenHeight + 1];
    HANDLE Console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(Console);
    DWORD dwBytesWritten = 0;
    std::string map;
    map = generateMap();


    auto tp1 = std::chrono::system_clock::now();
    auto tp2 = std::chrono::system_clock::now();

    while (true) {
        tp2 = std::chrono::system_clock::now();
        std::chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float ElapsedTime = elapsedTime.count();

        if (GetAsyncKeyState((unsigned short) 'A') & 0x8000)
            VectorA -= (1.5f) * ElapsedTime;

        if (GetAsyncKeyState((unsigned short) 'D') & 0x8000)
            VectorA += (1.5f) * ElapsedTime;

        if (GetAsyncKeyState((unsigned short) 'W') & 0x8000) {
            VectorX += sinf(VectorA) * 5.0f * ElapsedTime;
            VectorY += cosf(VectorA) * 5.0f * ElapsedTime;

            if (map.c_str()[(int) VectorX * gameWidth + (int) VectorY] == '#') {
                VectorX -= sinf(VectorA) * 5.0f * ElapsedTime;
                VectorY -= cosf(VectorA) * 5.0f * ElapsedTime;
            }
        }

        if (GetAsyncKeyState((unsigned short) 'S') & 0x8000) {
            VectorX -= sinf(VectorA) * 5.0f * ElapsedTime;
            VectorY -= cosf(VectorA) * 5.0f * ElapsedTime;
            if (map.c_str()[(int) VectorX * gameWidth + (int) VectorY] == '#') {
                VectorX += sinf(VectorA) * 5.0f * ElapsedTime;
                VectorY += cosf(VectorA) * 5.0f * ElapsedTime;
            }
        }

        for (int x = 0; x < ScreenWidth; x++) {
            float fRayAngle = (VectorA - visibilityZone / 2.0f) + ((float) x / (float) ScreenWidth) * visibilityZone;

            float DistanceToWall = 0.0f;
            bool isHitWall = false;
            bool isBoundary = false;
            float fEyeX = sinf(fRayAngle);
            float fEyeY = cosf(fRayAngle);
            while (!isHitWall && DistanceToWall < visibilityDistance) {
                DistanceToWall += 0.1f;

                int nTestX = (int) (VectorX + fEyeX * DistanceToWall);
                int nTestY = (int) (VectorY + fEyeY * DistanceToWall);

                if (nTestX < 0 || nTestX >= gameWidth || nTestY < 0 || nTestY >= gameHeight) {
                    isHitWall = true;
                    DistanceToWall = visibilityDistance;
                } else {

                    if (map.c_str()[nTestX * gameWidth + nTestY] == '#') {

                        isHitWall = true;
                        std::vector<std::pair<float, float>> p;

                        for (int tx = 0; tx < 2; tx++)
                            for (int ty = 0; ty < 2; ty++) {
                                float vy = (float) nTestY + ty - VectorY;
                                float vx = (float) nTestX + tx - VectorX;
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p.push_back(std::make_pair(d, dot));
                            }

                        sort(p.begin(), p.end(),
                             [](const std::pair<float, float> &left, const std::pair<float, float> &right) {
                                 return left.first < right.first;
                             });

                        float fBound = 0.005;
                        if (acos(p.at(0).second) < fBound) isBoundary = true;
                        if (acos(p.at(1).second) < fBound) isBoundary = true;
                        if (acos(p.at(2).second) < fBound) isBoundary = true;
                    }
                }
            }

            int nCeiling = (float) (ScreenHeight / 2.0) - ScreenHeight / ((float) DistanceToWall);
            int nFloor = ScreenHeight - nCeiling;

            char nShade = ' ';

            if (DistanceToWall <= visibilityDistance / 4.0f) nShade = char(219);
            else if (DistanceToWall < visibilityDistance / 3.0f) nShade = char(178);
            else if (DistanceToWall < visibilityDistance / 2.0f) nShade = char(177);
            else if (DistanceToWall < visibilityDistance) nShade = char(176);
            else nShade = ' ';
            if (isBoundary) nShade = char(179);//линия границы блока
            for (int y = 0; y < ScreenHeight; y++) {
                if (y <= nCeiling) {
                    screen[y * ScreenWidth + x] = char(197);//символ неба
                } else if (y > nCeiling && y <= nFloor) {
                    screen[y * ScreenWidth + x] = nShade;
                } else {
                    float b = 1.0f - ((float) y - ScreenHeight / 2.0f) / ((float) ScreenHeight / 2.0f);
                    if (b < 0.25) nShade = '#';
                    else if (b < 0.5) nShade = 'x';
                    else if (b < 0.75) nShade = '~';
                    else if (b < 0.9) nShade = '-';
                    else nShade = ' ';

                    screen[y * ScreenWidth + x] = nShade;
                }
            }
        }

        for (int nx = 0; nx < gameWidth; nx++)
            for (int ny = 0; ny < gameWidth; ny++) {
                screen[(ny + 1) * ScreenWidth + nx] = map[ny * gameWidth + nx];
            }
        screen[((int) VectorX + 1) * ScreenWidth + (int) VectorY] = 'P';

        screen[ScreenWidth * ScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacter(Console, reinterpret_cast<LPCSTR>(screen), ScreenWidth * ScreenHeight,
                                    {0, 0}, &dwBytesWritten);

    }
    return 0;
}