#pragma once

#include<Windows.h>
//定义场景大小
constexpr auto WIDTH = 1040;
constexpr auto HEIGHT = 640;

//定义食物以及蛇的单位大小
constexpr auto MYSIZE = 20;

enum class Direction //枚举定义蛇的朝向
{
    UP,
    DOWN,
    RIGHT,
    LEFT
};

constexpr auto MAXSIZE = 1600; //相当于蛇的最大长度
constexpr auto SPEED = 150;    //速度
constexpr auto BIGFOOD_DURATION = 5000; //BigFood显示ms时间
// 在common.h中添加颜色常量
constexpr auto SNAKE_HEAD_COLOR = RGB(0, 255, 128);
constexpr auto SNAKE_BODY_COLOR = RGB(0, 200, 100);
constexpr auto FOOD_COLOR = RGB(255, 50, 50);
constexpr auto BIG_FOOD_COLOR = RGB(255, 215, 0);
constexpr auto BACKGROUND_COLOR = RGB(10, 20, 30);