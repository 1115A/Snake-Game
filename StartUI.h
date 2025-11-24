// StartUI.h - 确保所有方法都有声明
#pragma once
#include <graphics.h>
#include <windows.h>
#include <vector>
#include <string>
#include <random>
#include <functional>

class StartUI {
private:
    int width, height;
    bool startGame;

    // 动画相关变量
    std::vector<POINT> snakeSegments;
    std::vector<POINT> foods;
    std::vector<POINT> particles;
    int snakeDirection;
    int animationCounter;
    COLORREF snakeColor;
    COLORREF foodColor;
    COLORREF backgroundColor;

public:
    StartUI(int w = 800, int h = 600);
    bool showStartScreen();

private:
    void drawBackground();
    void drawAnimatedSnake();
    void drawFoodParticles();
    void drawStartButton();
    void drawTitle();
    void updateAnimation();  // 确保这行存在
    void resetAnimation();
    bool isPointInButton(int x, int y);
    COLORREF getRandomColor();
    void drawGradientBackground();
    void drawGridPattern();
};