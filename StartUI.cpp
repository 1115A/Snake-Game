// StartUI.cpp - 完整实现
#include "StartUI.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <random>

StartUI::StartUI(int w, int h) : width(w), height(h), startGame(false),
snakeDirection(0), animationCounter(0) {
    // 初始化颜色
    snakeColor = RGB(0, 255, 128);      // 亮绿色
    foodColor = RGB(255, 100, 100);     // 红色
    backgroundColor = RGB(10, 20, 30);  // 深蓝色背景

    resetAnimation();
}

bool StartUI::showStartScreen() {
    initgraph(width, height);
    setbkcolor(backgroundColor);
    cleardevice();

    startGame = false;

    // 使用时间戳控制动画，不依赖鼠标移动
    auto lastUpdateTime = std::chrono::steady_clock::now();

    // 主循环
    while (!startGame) {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastUpdateTime).count();

        // 每50毫秒更新一次动画
        if (elapsed >= 50) {
            BeginBatchDraw();

            // 绘制动态背景
            drawBackground();

            // 绘制动画元素
            drawAnimatedSnake();
            drawFoodParticles();

            // 绘制UI元素
            drawTitle();
            drawStartButton();

            // 更新动画
            updateAnimation();

            EndBatchDraw();
            lastUpdateTime = currentTime;
        }

        // 处理输入（不阻塞）
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                if (isPointInButton(msg.x, msg.y)) {
                    startGame = true;
                }
            }
        }

        // 检查ESC键退出
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            break;
        }

        // 短暂休眠以减少CPU占用
        Sleep(10);
    }

    if (startGame) {
        // 添加一个简单的过渡效果
        for (int i = 0; i < 20; i++) {
            BeginBatchDraw();
            drawBackground();
            drawAnimatedSnake();
            drawFoodParticles();

            // 逐渐变暗
            setfillcolor(RGB(0, 0, 0));
            solidrectangle(0, 0, width, height);

            EndBatchDraw();
            Sleep(30);
        }
    }

    closegraph();
    return startGame;
}

void StartUI::drawBackground() {
    // 绘制渐变背景 - 调暗背景让网格更明显
    drawGradientBackground();

    // 绘制网格
    drawGridPattern();

    // 绘制装饰性粒子 - 减少数量避免干扰
    setfillcolor(RGB(80, 100, 180));
    for (size_t i = 0; i < particles.size() && i < 30; i++) {
        solidcircle(particles[i].x, particles[i].y, 1);
    }
}

void StartUI::drawGradientBackground() {
    // 更暗的渐变背景
    for (int y = 0; y < height; y++) {
        int r = 5 + (y * 8 / height);
        int g = 10 + (y * 12 / height);
        int b = 15 + (y * 16 / height);
        setlinecolor(RGB(r, g, b));
        line(0, y, width, y);
    }
}

//void StartUI::drawGradientBackground() {
//    // 简单的渐变背景
//    for (int y = 0; y < height; y++) {
//        int r = 10 + (y * 10 / height);
//        int g = 20 + (y * 15 / height);
//        int b = 30 + (y * 20 / height);
//        setlinecolor(RGB(r, g, b));
//        line(0, y, width, y);
//    }
//}

void StartUI::drawGridPattern() {
    // 绘制更明显的网格线
    setlinecolor(RGB(60, 70, 100));  // 更亮的颜色

    // 主网格线
    for (int x = 0; x < width; x += 40) {
        line(x, 0, x, height);
    }
    for (int y = 0; y < height; y += 40) {
        line(0, y, width, y);
    }

    // 次网格线
    setlinecolor(RGB(40, 50, 80));
    for (int x = 0; x < width; x += 20) {
        line(x, 0, x, height);
    }
    for (int y = 0; y < height; y += 20) {
        line(0, y, width, y);
    }

    // 绘制边框
    setlinecolor(RGB(100, 120, 180));
    setlinestyle(PS_SOLID, 3);
    rectangle(0, 0, width - 1, height - 1);
    setlinestyle(PS_SOLID, 1);
}

void StartUI::drawAnimatedSnake() {
    if (snakeSegments.empty()) return;

    // 绘制蛇身
    for (size_t i = 0; i < snakeSegments.size(); i++) {
        int alpha = 255 - (i * 200 / snakeSegments.size());
        if (alpha < 50) alpha = 50;

        COLORREF segmentColor = RGB(
            GetRValue(snakeColor) * alpha / 255,
            GetGValue(snakeColor) * alpha / 255,
            GetBValue(snakeColor) * alpha / 255
        );

        setfillcolor(segmentColor);
        setlinecolor(segmentColor);

        int size = 15 - (i * 8 / snakeSegments.size());
        if (size < 5) size = 5;

        // 绘制圆角矩形作为蛇身段
        fillroundrect(
            snakeSegments[i].x - size,
            snakeSegments[i].y - size,
            snakeSegments[i].x + size,
            snakeSegments[i].y + size,
            8, 8
        );
    }

    // 绘制蛇头
    setfillcolor(snakeColor);
    setlinecolor(RGB(255, 255, 255));
    fillroundrect(
        snakeSegments[0].x - 18,
        snakeSegments[0].y - 18,
        snakeSegments[0].x + 18,
        snakeSegments[0].y + 18,
        12, 12
    );

    // 绘制眼睛
    setfillcolor(RGB(255, 255, 255));
    int eyeOffsetX = 0, eyeOffsetY = 0;
    switch (snakeDirection) {
    case 0: eyeOffsetX = 6; break;  // 右
    case 1: eyeOffsetY = 6; break;  // 下
    case 2: eyeOffsetX = -6; break; // 左
    case 3: eyeOffsetY = -6; break; // 上
    }
    solidcircle(snakeSegments[0].x + eyeOffsetX - 3, snakeSegments[0].y + eyeOffsetY - 3, 4);
    solidcircle(snakeSegments[0].x + eyeOffsetX + 3, snakeSegments[0].y + eyeOffsetY - 3, 4);

    setfillcolor(RGB(0, 0, 0));
    solidcircle(snakeSegments[0].x + eyeOffsetX - 3, snakeSegments[0].y + eyeOffsetY - 3, 2);
    solidcircle(snakeSegments[0].x + eyeOffsetX + 3, snakeSegments[0].y + eyeOffsetY - 3, 2);
}

void StartUI::drawFoodParticles() {
    for (size_t i = 0; i < foods.size(); i++) {
        // 闪烁效果
        int pulse = (animationCounter + i * 20) % 100;
        int brightness = 150 + 105 * sin(pulse * 3.14159 / 50);

        COLORREF currentColor = RGB(
            GetRValue(foodColor) * brightness / 255,
            GetGValue(foodColor) * brightness / 255,
            GetBValue(foodColor) * brightness / 255
        );

        setfillcolor(currentColor);
        setlinecolor(RGB(255, 255, 255));

        // 绘制食物（苹果形状）
        int size = 12 + 3 * sin((animationCounter + i * 10) * 3.14159 / 30);

        // 主体
        solidcircle(foods[i].x, foods[i].y, size);

        // 高光
        setfillcolor(RGB(255, 255, 255));
        solidcircle(foods[i].x - size / 3, foods[i].y - size / 3, size / 4);

        // 茎
        setlinecolor(RGB(100, 200, 100));
        setlinestyle(PS_SOLID, 3);
        line(foods[i].x, foods[i].y - size, foods[i].x, foods[i].y - size - 8);
        setlinestyle(PS_SOLID, 1);
    }
}

void StartUI::drawTitle() {
    // 主标题
    settextcolor(RGB(255, 255, 255));
    settextstyle(48, 0, _T("华文行楷"));
    setbkmode(TRANSPARENT);

    // 文字阴影
    settextcolor(RGB(0, 100, 200));
    outtextxy(width / 2 - 98, 102, _T("贪吃蛇大作战"));

    settextcolor(RGB(255, 255, 255));
    outtextxy(width / 2 - 100, 100, _T("贪吃蛇大作战"));

    // 副标题
    settextstyle(20, 0, _T("宋体"));
    settextcolor(RGB(200, 200, 255));
    outtextxy(width / 2 - 80, 160, _T(""));

    // 版本信息
    settextstyle(14, 0, _T("宋体"));
    settextcolor(RGB(150, 150, 200));
    outtextxy(width - 150, height - 30, _T("版本 3.0"));
}

void StartUI::drawStartButton() {
    int buttonX = width / 2 - 80;
    int buttonY = height / 2 + 50;
    int buttonWidth = 160;
    int buttonHeight = 50;

    // 按钮发光效果
    for (int i = 0; i < 3; i++) {
        int glowAlpha = 50 - i * 15;
        setlinecolor(RGB(0, 200 + i * 20, 100 + i * 10));
        setlinestyle(PS_SOLID, 2);
        roundrect(buttonX - i, buttonY - i,
            buttonX + buttonWidth + i, buttonY + buttonHeight + i,
            15, 15);
    }

    // 按钮主体（渐变填充）
    for (int y = buttonY; y < buttonY + buttonHeight; y++) {
        int green = 100 + (y - buttonY) * 155 / buttonHeight;
        setlinecolor(RGB(0, green, 50));
        line(buttonX, y, buttonX + buttonWidth, y);
    }

    // 按钮边框
    setlinestyle(PS_SOLID, 2);
    setlinecolor(RGB(0, 255, 150));
    roundrect(buttonX, buttonY, buttonX + buttonWidth, buttonY + buttonHeight, 15, 15);

    // 按钮文字
    settextstyle(24, 0, _T("宋体"));
    settextcolor(RGB(255, 255, 255));
    outtextxy(buttonX + 45, buttonY + 12, _T("开始游戏"));

    // 鼠标悬停效果
    MOUSEMSG mouseMsg;
    if (PeekMouseMsg(&mouseMsg)) {
        if (isPointInButton(mouseMsg.x, mouseMsg.y)) {
            settextcolor(RGB(255, 255, 0));
            outtextxy(buttonX + 45, buttonY + 12, _T("开始游戏"));

            // 绘制小手光标提示
            settextstyle(12, 0, _T("宋体"));
            settextcolor(RGB(255, 255, 0));
            outtextxy(buttonX + buttonWidth + 10, buttonY + 15, _T("点击开始"));
        }
    }
}

// 添加缺失的 updateAnimation 方法实现
void StartUI::updateAnimation() {
    animationCounter++;

    // 更新蛇的位置
    if (!snakeSegments.empty()) {
        POINT newHead = snakeSegments[0];

        // 根据方向移动头部
        switch (snakeDirection) {
        case 0: newHead.x += 3; break; // 右
        case 1: newHead.y += 3; break; // 下
        case 2: newHead.x -= 3; break; // 左
        case 3: newHead.y -= 3; break; // 上
        }

        // 边界检查
        if (newHead.x < -50) newHead.x = width + 50;
        if (newHead.x > width + 50) newHead.x = -50;
        if (newHead.y < -50) newHead.y = height + 50;
        if (newHead.y > height + 50) newHead.y = -50;

        // 随机改变方向
        if (rand() % 100 == 0) {
            snakeDirection = rand() % 4;
        }

        // 更新蛇身
        snakeSegments.insert(snakeSegments.begin(), newHead);
        if (snakeSegments.size() > 15) {
            snakeSegments.pop_back();
        }
    }

    // 更新粒子
    if (particles.size() < 100 && rand() % 3 == 0) {
        particles.push_back({ rand() % width, rand() % height });
    }

    // 随机移除粒子
    if (!particles.empty() && rand() % 10 == 0) {
        particles.erase(particles.begin());
    }

    // 更新食物位置（轻微浮动）
    for (size_t i = 0; i < foods.size(); i++) {
        foods[i].x += rand() % 3 - 1;
        foods[i].y += rand() % 3 - 1;

        // 边界检查
        if (foods[i].x < 0) foods[i].x = 0;
        if (foods[i].x > width) foods[i].x = width;
        if (foods[i].y < 0) foods[i].y = 0;
        if (foods[i].y > height) foods[i].y = height;
    }
}

void StartUI::resetAnimation() {
    // 重置蛇
    snakeSegments.clear();
    int startX = width / 4;
    int startY = height / 2;
    for (int i = 0; i < 10; i++) {
        snakeSegments.push_back({ startX - i * 15, startY });
    }
    snakeDirection = 0; // 向右

    // 重置食物
    foods.clear();
    foods.push_back({ width * 3 / 4, height / 3 });
    foods.push_back({ width * 2 / 3, height * 2 / 3 });
    foods.push_back({ width * 4 / 5, height / 2 });

    // 重置粒子
    particles.clear();
    for (int i = 0; i < 50; i++) {
        particles.push_back({ rand() % width, rand() % height });
    }
}

bool StartUI::isPointInButton(int x, int y) {
    int buttonX = width / 2 - 80;
    int buttonY = height / 2 + 50;
    int buttonWidth = 160;
    int buttonHeight = 50;

    return (x >= buttonX && x <= buttonX + buttonWidth &&
        y >= buttonY && y <= buttonY + buttonHeight);
}

COLORREF StartUI::getRandomColor() {
    return RGB(rand() % 256, rand() % 256, rand() % 256);
}