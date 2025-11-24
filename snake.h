#pragma once
#include "common.h"
#include <graphics.h>
#include <Windows.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <random>
#include <stdexcept>
#include <functional>
#include <chrono>
class Snake;
// 在Snake.h中修改SnakeNode类
class SnakeNode
{
public:
    int x{ 0 };
    int y{ 0 };
    int RGB[3];
    int pulseOffset; // 新增：脉动效果偏移量
    SnakeNode();
};
//创建蛇的类
class Snake
{
    friend class Food;
    friend class BigFood;

private:
    int score;                  //分数
    int count;                  //辅助判断大食物的生成
    Direction dirt;             //蛇的朝向
    int length;                 //长度        感觉并不需要单独存在，但是为了初始化方便还是留了  node.size()=length实际上
    std::vector<SnakeNode> node; //蛇的结点
    bool grow;                  //标记是否需要增长
public:
    Snake();                                    //初始化
    template <class T>
    bool Eat(const std::unique_ptr<T>& food);   //吃食物
    void Move();                                //移动
    void Show();                                //绘制蛇
    bool Defeat() const;                        //失败判定
    void showUI();                              //打印分数等UI
    int GetCount() const;
    int GetScore() const;
    void SetDirection(Direction newDir);        //设置方向
    void Reset();                               //重置蛇
    void setcount();
    size_t getsize();
};

class BaseFood
{
public:
    int x, y;
    int score; // 分数
    BaseFood();
    virtual ~BaseFood() = default;
    virtual void Show() {};
    virtual bool ShouldRemove() const { return false; };
};

// 创建食物的类
class Food : public BaseFood
{
    friend class Snake;

public:
    Food(const std::unique_ptr<Snake>& snake);
    void Show() override; //绘制食物
    ~Food();
};

class BigFood : public BaseFood
{
    friend class Snake;
private:
    std::chrono::steady_clock::time_point spawnTime; //生成时间

public:
    BigFood(const std::unique_ptr<Snake>& snake);
    void Show() override; //绘制食物
    ~BigFood();
    bool ShouldRemove() const override; //检查是否应该移除
};

//模板函数或类的定义通常需要在头文件中进行而不是源文件。
//这是因为模板实例化是在编译时进行的
//并且编译器需要看到模板的完整定义以便于为特定类型的参数生成代码
template <class T>
bool Snake::Eat(const std::unique_ptr<T>& food)
{
    if (food->x == this->node[0].x && food->y == this->node[0].y)
    {
        this->score += food->score;
        this->count++;
        this->grow = true; //标记需要增长
        return true;
    }
    return false;
}