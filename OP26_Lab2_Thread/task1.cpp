#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <cmath>
#include <iomanip>

// Константи екрану (приблизно)
const int WIDTH = 60;
const int HEIGHT = 25;
const double PI = 3.14159265358979323846;

std::mutex console_mtx;

// Функція переміщення курсору
void setCursor(int x, int y) {
    std::cout << "\033[" << y + 1 << ";" << x + 1 << "H";
}

// Безпечне малювання символу
void drawEntity(int x, int y, char symbol, int oldX, int oldY) {
    std::lock_guard<std::mutex> lock(console_mtx);

    // Перевірка меж екрану
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;

    // Очищення старої позиції
    if (oldX != -1 && oldY != -1) {
        setCursor(oldX, oldY);
        std::cout << " ";
    }

    // Малювання нової
    setCursor(x, y);
    std::cout << symbol;

    // Прибираємо курсор у безпечне місце
    setCursor(0, HEIGHT + 1);
    std::cout.flush();
}

// --- ЛОГІКА МУРАХИ-РОБОЧОГО (Лінійний рух туди-назад) ---
void workerAntTask(int startX, int startY, int speedV) {
    // Цільова точка (лівий верхній кут)
    int targetX = 0;
    int targetY = 0;

    double currentX = startX;
    double currentY = startY;

    int prevX = -1, prevY = -1;
    int delay = 1000 / speedV;

    // Вектор руху ДО цілі
    double dx = targetX - startX;
    double dy = targetY - startY;
    double distance = std::sqrt(dx*dx + dy*dy);
    double steps = distance * 1.5; // Кількість кроків залежить від відстані

    double stepX = dx / steps;
    double stepY = dy / steps;

    // 1. Рух ДО кута [0;0]
    for (int i = 0; i <= steps; ++i) {
        int drawX = static_cast<int>(currentX);
        int drawY = static_cast<int>(currentY);

        drawEntity(drawX, drawY, 'W', prevX, prevY);
        prevX = drawX; prevY = drawY;

        currentX += stepX;
        currentY += stepY;

        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    // Невелика пауза в кутку
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 2. Рух НАЗАД до народження (інвертуємо кроки)
    for (int i = 0; i <= steps; ++i) {
        int drawX = static_cast<int>(currentX);
        int drawY = static_cast<int>(currentY);

        drawEntity(drawX, drawY, 'W', prevX, prevY);
        prevX = drawX; prevY = drawY;

        // Рухаємось назад
        currentX -= stepX;
        currentY -= stepY;

        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    // Фінальне стирання
    drawEntity(-1, -1, ' ', prevX, prevY);
}

// --- ЛОГІКА МУРАХИ-ВОЇНА (Рух по колу) ---
void warriorAntTask(int centerX, int centerY, int radius, int speedV) {
    double angle = 0.0;
    int prevX = -1, prevY = -1;

    // Швидкість зміни кута залежить від V
    // Чим більший радіус, тим повільніше змінюється кут при тій самій лінійній швидкості,
    // але для спрощення зробимо залежність прямою від V.
    double angleStep = 0.1;
    int delay = 1000 / speedV;

    // Нехай зробить 3 повних кола (3 * 2PI)
    while (angle < 6 * PI) {
        // Параметричне рівняння кола
        // Множимо X на 2, бо в консолі символи витягнуті вертикально (корекція aspect ratio)
        int x = centerX + static_cast<int>(radius * 2 * std::cos(angle));
        int y = centerY + static_cast<int>(radius * std::sin(angle));

        drawEntity(x, y, '@', prevX, prevY);
        prevX = x; prevY = y;

        angle += angleStep;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    }

    // Фінальне стирання
    drawEntity(-1, -1, ' ', prevX, prevY);
}

int main() {
    // Очищення екрану
    std::cout << "\033[2J";

    std::vector<std::thread> colony;

    // --- Створення потоків ---

    // 1. Робочі мурахи (startX, startY, speed)
    // Вони стартують з різних точок і йдуть в [0;0]
    colony.emplace_back(workerAntTask, 20, 10, 20);
    colony.emplace_back(workerAntTask, 30, 15, 15);
    colony.emplace_back(workerAntTask, 10, 20, 25); // Швидкий

    // 2. Воїни (centerX, centerY, radius, speed)
    // Вони кружляють навколо центру карти
    colony.emplace_back(warriorAntTask, 30, 12, 5, 30); // Мале коло, швидко
    colony.emplace_back(warriorAntTask, 30, 12, 8, 15); // Велике коло, повільно

    // --- Очікування завершення ---
    for (auto& ant : colony) {
        if (ant.joinable()) {
            ant.join();
        }
    }

    setCursor(0, HEIGHT + 2);
    std::cout << "Симуляцію завершено." << std::endl;

    return 0;
}