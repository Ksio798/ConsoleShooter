#include <iostream>
#include <Windows.h>
#include <chrono>
#include <cmath>
#include <vector>
#include <algorithm>
#include <deque>
#include <conio.h>
#include <unistd.h>

class Timer {
    std::chrono::system_clock::time_point tp1 = std::chrono::system_clock::now();
    std::chrono::system_clock::time_point tp2 = std::chrono::system_clock::now();
    bool *changingVar;
public:
    bool isStarted = false;
    int count;
public:
    void start(int c, bool *adds) {
        count = c;
        changingVar = adds;
        tp1 = std::chrono::system_clock::now();
        isStarted = true;
    }

    void update() {
        if (isStarted) {
            tp2 = std::chrono::system_clock::now();
            std::chrono::duration<float> duration = tp2 - tp1;
            if (duration.count() == count) {
                isStarted = false;
                if (changingVar != NULL)
                    *changingVar = true;
            }
        }
    }

    void stop() {
        isStarted = false;
        count = 0;
        if (changingVar != NULL)
            *changingVar = false;
    }
};

int ScreenWidth = 120;
int ScreenHeight = 40;
float VectorX = 1.0f;
float VectorY = 1.0f;
float VectorA = 0.0f;
const int gameHeight = 16;
const int gameWidth = 16;
float visibilityZone = 3.14159 / 3;
float visibilityDistance = 30.0f;
HANDLE Console;
auto screen = new char[ScreenWidth * ScreenHeight + 1];

std::string map, code = "IDDQD";
int codeIndex = 0, timeToWait = 5;
bool isTimeOver = false, isManualControl = true, isWin = false;
Timer timer;
char map2[gameWidth][gameHeight];
std::deque<std::pair<int, int>> previousSteps;
float targetX = 1.0f;
float targetY = 1.0f;
float targetA = 1.0f;
float difference = 0.05f;

void generateMap() {
    map += "################";
    map += "#..............#";
    for (int i = 0; i < gameHeight - 3; i++) {
        std::string a = "#";
        for (int j = 0; j < gameWidth - 2; ++j) {
            if ((rand() % 5) == 0) {
                a += "#";
            } else {
                a += ".";
            }
        }
        a += "#";
        map += a;
    }
    map += "########W#######";
}

void generateMap2() {
    map += "################";
    map += "#..............#";
    map += "#######.########";
    map += "#######.########";
    map += "#######....#####";
    map += "##########.#####";
    map += "##########.#####";
    map += "#######....#####";
    map += "####....########";
    map += "####.###########";
    map += "####.###########";
    map += "####.###########";
    map += "####.....#######";
    map += "#######..#######";
    map += "#######...######";
    map += "########W#######";




}

void move(float elapsedTime) {
    if (GetAsyncKeyState((int) 'A'))
        VectorA -= (1.5f) * elapsedTime;

    if (GetAsyncKeyState((int) 'D'))
        VectorA += (1.5f) * elapsedTime;

    if (GetAsyncKeyState((int) 'W')) {
        VectorX += sinf(VectorA) * 5.0f * elapsedTime;
        VectorY += cosf(VectorA) * 5.0f * elapsedTime;

        if (map[(int) VectorX * gameWidth + (int) VectorY] == '#') {
            VectorX -= sinf(VectorA) * 5.0f * elapsedTime;
            VectorY -= cosf(VectorA) * 5.0f * elapsedTime;
        }
    }
    if (GetAsyncKeyState((int) 'S')) {
        VectorX -= sinf(VectorA) * 5.0f * elapsedTime;
        VectorY -= cosf(VectorA) * 5.0f * elapsedTime;
        if (map[(int) VectorX * gameWidth + (int) VectorY] == '#') {
            VectorX += sinf(VectorA) * 5.0f * elapsedTime;
            VectorY += cosf(VectorA) * 5.0f * elapsedTime;
        }
    }
}

void drawLevel(int x, float DistanceToWall, bool isBoundary) {
    int nCeiling = (float) (ScreenHeight / 2.0) - ScreenHeight / ((float) DistanceToWall); //дистанция до неба
    int nFloor = ScreenHeight - nCeiling;//дистанция до пола
    char nShade;
    if (DistanceToWall <= visibilityDistance / 4.0f) nShade = char(219);
    else if (DistanceToWall < visibilityDistance / 3.0f) nShade = char(178);
    else if (DistanceToWall < visibilityDistance / 2.0f) nShade = char(177);
    else if (DistanceToWall < visibilityDistance) nShade = char(176);
    else nShade = char(197);//символ неба
    if (isBoundary)
        nShade = char(179);//линия границы блока
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

void rayCasting() {
    for (int x = 0; x < ScreenWidth; x++) {
        float fRayAngle = (VectorA - visibilityZone / 2.0f) +
                          ((float) x / (float) ScreenWidth) * visibilityZone; //угол видимости для каждого столбца
        float DistanceToWall = 0.0f;
        bool isHitWall = false; //попадение луча в стену
        bool isBoundary = false;//попадение луча между стенами
        float fEyeX = sinf(fRayAngle); //вектор луча X
        float fEyeY = cosf(fRayAngle);//вектор луча Y
        while (!isHitWall &&
               DistanceToWall < visibilityDistance) { //пока не столкнулись со стеной и стена в облисти видимости
            DistanceToWall += 0.1f;

            int nTestX = (int) (VectorX + fEyeX * DistanceToWall);//координаты точки в которую попал луч
            int nTestY = (int) (VectorY + fEyeY * DistanceToWall);

            if (map[nTestX * gameWidth + nTestY] == '#') {
                isHitWall = true;
                std::vector<float> p;
                for (int tx = 0; tx < 2; tx++)
                    for (int ty = 0; ty < 2; ty++) {
                        float vy = (float) nTestY + ty - VectorY; //координаты вектора от игрока до ребра
                        float vx = (float) nTestX + tx - VectorX;
                        float d = sqrt(vx * vx + vy * vy); //длина вектора (модудь вектора)
                        float dot = (fEyeX * vx / d) + (fEyeY * vy / d); //скалярное произведение векторов
                        p.push_back(dot);
                    }

                float fBound = 0.005;
                if (acos(p.at(0)) < fBound) isBoundary = true;
                if (acos(p.at(1)) < fBound) isBoundary = true;
                if (acos(p.at(2)) < fBound) isBoundary = true;
            }
        }
        drawLevel(x, DistanceToWall, isBoundary);//рисовние уровня
    }
}

void minMap() {
    for (int nx = 0; nx < gameWidth; nx++)
        for (int ny = 0; ny < gameWidth; ny++) {
            screen[(ny + 1) * ScreenWidth + nx] = map[ny * gameWidth + nx];
        }
    screen[((int) VectorX + 1) * ScreenWidth + (int) VectorY] = 'P';
}

void copyMap() {
    for (int i = 0; i < gameWidth; ++i) {
        for (int j = 0; j < gameHeight; ++j) {
            if (map[j + gameWidth * i] == '#')
                map2[i][j] = '#';
            else if (map[j + gameWidth * i] == '.')
                map2[i][j] = '.';
            else
                map2[i][j] = 'W';
        }
    }
}

void findWay() {
    copyMap();
    int currentX = VectorX, currentY = VectorY;
    map2[currentX][currentY] = '*';
    previousSteps.clear();
    previousSteps.push_back(std::make_pair(currentX, currentY));
    while (true) {
        std::vector<std::pair<int, int>> ways;
        if (map2[currentX + 1][currentY] != '*' && map2[currentX + 1][currentY] != '#' && currentX < gameWidth - 1)
            ways.push_back(std::make_pair(currentX + 1, currentY));
        if (map2[currentX - 1][currentY] != '*' && map2[currentX - 1][currentY] != '#' && currentX > 1)
            ways.push_back(std::make_pair(currentX - 1, currentY));
        if (map2[currentX][currentY + 1] != '*' && map2[currentX][currentY + 1] != '#' && currentY < gameHeight - 1)
            ways.push_back(std::make_pair(currentX, currentY + 1));
        if (map2[currentX][currentY - 1] != '*' && map2[currentX][currentY - 1] != '#' && currentY > 1)
            ways.push_back(std::make_pair(currentX, currentY - 1));

        if (map2[currentX + 1][currentY] == 'W') {
            previousSteps.push_back(std::make_pair(currentX + 1, currentY));
            break;
        }
        if (map2[currentX - 1][currentY] == 'W') {
            previousSteps.push_back(std::make_pair(currentX - 1, currentY));
            break;
        }
        if (map2[currentX][currentY + 1] == 'W') {
            previousSteps.push_back(std::make_pair(currentX, currentY + 1));
            break;
        }
        if (map2[currentX][currentY - 1] == 'W') {
            previousSteps.push_back(std::make_pair(currentX, currentY - 1));
            break;
        }

        if (ways.size() > 0) {
            auto way = ways.at(rand() % ways.size());
            currentX = way.first;
            currentY = way.second;
            previousSteps.push_back(std::make_pair(currentX, currentY));
            if (map2[currentX][currentY] == 'W') {
                break;
            }
            map2[currentX][currentY] = '*';
        } else if (!previousSteps.empty()) {
            auto way = previousSteps.back();
            previousSteps.pop_back();
            currentX = way.first;
            currentY = way.second;
        } else {
            break;
        }
    }
    previousSteps.pop_front();
}

void setTarget() {
    if (!previousSteps.empty()) {
        auto target = previousSteps.front();
        previousSteps.pop_front();
        targetX = (float) target.first;
        targetY = (float) target.second;
        float x = (targetY - VectorY) /
                  sqrtf((targetX - VectorX) * (targetX - VectorX) + (targetY - VectorY) * (targetY - VectorY));
        targetA = acos(x);
    } else {
        //isManualControl = true;
        targetY = -1;
        targetX = -1;
    }

}

void getCode() {

    if (GetAsyncKeyState((int) code[codeIndex])) {
        codeIndex++;
        if (!timer.isStarted)
            timer.start(timeToWait, &isTimeOver);
        if (codeIndex == code.size()) {
            codeIndex = 0;
            isManualControl = false;
            findWay();
            setTarget();
        }
    }
    if (isTimeOver) {
        codeIndex = 0;
        isTimeOver = false;
    }
}

bool checkButton() {
    bool res = false;
    if (GetAsyncKeyState((int) 'W')) {
        res = true;
    }
    return res;
}

void autoMove(float elapsedTime) {
    if (abs(targetA - abs(VectorA)) > difference) {
        VectorA += (1.5f) * elapsedTime;
    } else {
        if (abs(targetX - VectorX) <= difference && abs(targetY - VectorY) <= difference) {
            VectorX = targetX;
            VectorY = targetY;
            setTarget();
        } else {
            VectorX += sinf(VectorA) * 5.0f * elapsedTime;
            VectorY += cosf(VectorA) * 5.0f * elapsedTime;
            if (map[(int) VectorX * gameWidth + (int) VectorY] == '#') {
                VectorX -= sinf(VectorA) * 5.0f * elapsedTime;
                VectorY -= cosf(VectorA) * 5.0f * elapsedTime;
                findWay();
                setTarget();
            }
        }
    }
    if (targetX == -1 && targetY == -1) {
        VectorX += sinf(VectorA) * 5.0f * elapsedTime;
        VectorY += cosf(VectorA) * 5.0f * elapsedTime;
    }
    if (checkButton())
        isManualControl = true;
}

int main() {
    Console = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0,
                                        NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(Console);
    DWORD dwBytesWritten = 0;

    generateMap2();

    auto tp1 = std::chrono::system_clock::now();
    auto tp2 = std::chrono::system_clock::now();

    while (true) {
        if (!isWin) {
            tp2 = std::chrono::system_clock::now();//расчет разницы во времени для постепенного перемещения
            std::chrono::duration<float> elapsedTime = tp2 - tp1;
            tp1 = tp2;
            float ElapsedTime = elapsedTime.count();

            timer.update();
            getCode();
            if (std::abs(VectorA) >= 6.28f)
                VectorA = 0.0f;

            if (isManualControl)
                move(ElapsedTime);//движение
            else
                autoMove(ElapsedTime);

            rayCasting();

            minMap();//отрисовка миникарты
        } else {
            std::string win = "YOU WIN";
            for (int i = 0; i < win.size(); ++i) {
                screen[2460 + i] = win[i];
            }
        }

        if (map[(int) VectorX * gameWidth + (int) VectorY] == 'W') {
            isWin = true;
        }
        screen[ScreenWidth * ScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacter(Console, reinterpret_cast<LPCSTR>(screen),
                                    ScreenWidth * ScreenHeight, {0, 0},
                                    &dwBytesWritten);
    }
}