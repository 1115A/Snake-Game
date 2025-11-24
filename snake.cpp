// Snake.cpp - 美化版本
#include "Snake.h"
#include <unordered_set>
#include <tuple>
#include <cmath>

namespace std
{
    template <>
    struct hash<tuple<int, int>>
    {
        size_t operator()(const tuple<int, int>& key) const
        {
            //将两个int合并为唯一哈希值
            const auto& [x, y] = key;
            return hash<int>()(x) ^ (hash<int>()(y) << 1);
        }
    };
}//全局区域特化哈希

//构造函数
Snake::Snake() : count(0), dirt(Direction::RIGHT), score(0), grow(false)
{
    Reset();
}

void Snake::Reset()
{
    node.clear();
    this->length = 3;
    SnakeNode temp_node;
    //下标是0的位置为蛇的头部
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(50, 200);

    for (int i = 0; i < length; i++)
    {
        temp_node.x = 60 - ((i + 1) * MYSIZE);
        temp_node.y = 60;
        temp_node.RGB[0] = dis(gen);
        temp_node.RGB[1] = dis(gen);
        temp_node.RGB[2] = dis(gen);
        temp_node.pulseOffset = i * 10; // 每个节点有不同的脉动偏移
        this->node.emplace_back(temp_node);
    }

    count = 0;
    dirt = Direction::RIGHT;
    score = 0;
    grow = false;
}

void Snake::setcount()
{
    this->count = 0;
}

size_t Snake::getsize()
{
    return this->node.size();
}

//移动
void Snake::Move()
{
    //保存当前头部位置
    SnakeNode head = node[0];

    // 根据方向移动头部
    switch (dirt)
    {
    case Direction::UP:
        head.y -= MYSIZE;
        break;
    case Direction::DOWN:
        head.y += MYSIZE;
        break;
    case Direction::LEFT:
        head.x -= MYSIZE;
        break;
    case Direction::RIGHT:
        head.x += MYSIZE;
        break;
    }

    //在头部插入新的节点
    node.insert(node.begin(), head);

    //如果不需要增长，则删除尾部节点
    if (!grow)
    {
        node.pop_back();
    }
    else
    {
        grow = false;//重置增长标志
        length++;//增加长度
    }
}

//设置方向
void Snake::SetDirection(Direction newDir)
{
    // 防止反向移动
    if ((dirt == Direction::UP && newDir == Direction::DOWN) ||
        (dirt == Direction::DOWN && newDir == Direction::UP) ||
        (dirt == Direction::LEFT && newDir == Direction::RIGHT) ||
        (dirt == Direction::RIGHT && newDir == Direction::LEFT))
    {
        return;
    }
    dirt = newDir;
}

//失败判定
bool Snake::Defeat() const
{
    //碰到边界
    if (this->node[0].x < 0 || this->node[0].x >= WIDTH ||
        this->node[0].y < 0 || this->node[0].y >= HEIGHT)
    {
        return true;
    }

    //碰到自己的身体
    for (size_t i = 1; i < this->node.size(); i++)
    {
        if (this->node[0].x == this->node[i].x && this->node[0].y == this->node[i].y)
        {
            return true;
        }
    }

    return false;
}

void Snake::showUI()
{
    //打印已经获得的分数
    settextcolor(WHITE);
    setbkmode(TRANSPARENT);
    settextstyle(16, 0, _T("宋体"));
    outtextxy(WIDTH - 120, 10, _T("分数:"));
    TCHAR scoreStr[10];
    _stprintf_s(scoreStr, _T("%d"), this->score);
    outtextxy(WIDTH - 50, 10, scoreStr);

    // 显示长度
    outtextxy(WIDTH - 120, 35, _T("长度:"));
    TCHAR lengthStr[10];
    _stprintf_s(lengthStr, _T("%zu"), this->node.size());
    outtextxy(WIDTH - 50, 35, lengthStr);
}

int Snake::GetCount() const
{
    return this->count;
}

int Snake::GetScore() const
{
    return this->score;
}

Food::Food(const std::unique_ptr<Snake>& snake)
{
    this->score = 1;

    std::unordered_set<std::tuple<int, int>> snakePositions;
    for (const auto& node : snake->node)
    {
        snakePositions.emplace(node.x, node.y);
    }

    //使用静态引擎避免重复初始化
    static std::mt19937 gen(std::random_device{}());
    const int gridWidth = WIDTH / MYSIZE;
    const int gridHeight = HEIGHT / MYSIZE;

    std::uniform_int_distribution<int> disX(1, gridWidth - 2);//BigFood-X
    std::uniform_int_distribution<int> disY(1, gridHeight - 2);//BigFood-Y

    //有限次尝试机制
    constexpr int MAX_ATTEMPTS = 1000;
    int attempts = 0;

    do
    {
        //生成网格坐标后转换为像素坐标
        const int gridX = disX(gen);
        const int gridY = disY(gen);
        this->x = gridX * MYSIZE;
        this->y = gridY * MYSIZE;

        if (++attempts > MAX_ATTEMPTS)
        {
            throw std::runtime_error("食物生成失败，可能地图已满");
        }
    } while (snakePositions.count({ x, y }) > 0);//使用哈希表以实现O(1)复杂度查询
}

void Snake::Show()
{
    // 使用时间戳创建脉动效果
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // 绘制蛇身
    for (size_t i = 0; i < node.size(); i++)
    {
        // 计算脉动效果
        double pulse = sin((ms + node[i].pulseOffset) * 0.01) * 0.1 + 0.9;

        // 计算颜色渐变：从头部到尾部逐渐变暗
        double alpha = 1.0 - (i * 0.7 / node.size());
        if (alpha < 0.3) alpha = 0.3;

        COLORREF segmentColor = RGB(
            static_cast<int>(node[i].RGB[0] * alpha * pulse),
            static_cast<int>(node[i].RGB[1] * alpha * pulse),
            static_cast<int>(node[i].RGB[2] * alpha * pulse)
        );

        setfillcolor(segmentColor);
        setlinecolor(RGB(255, 255, 255));  // 白色边框让蛇更明显

        // 计算大小（头部更大）
        int size = MYSIZE;
        if (i == 0) {
            size = static_cast<int>(MYSIZE * 1.2); // 头部更大
        }
        else {
            size = static_cast<int>(MYSIZE * (1.0 - i * 0.1 / node.size()));
            if (size < MYSIZE * 0.7) size = static_cast<int>(MYSIZE * 0.7);
        }

        // 绘制圆角矩形作为蛇身段
        int offset = (MYSIZE - size) / 2;
        fillroundrect(
            node[i].x + offset,
            node[i].y + offset,
            node[i].x + MYSIZE - offset,
            node[i].y + MYSIZE - offset,
            8, 8
        );

        // 绘制边框
        setlinecolor(RGB(255, 255, 255));
        roundrect(
            node[i].x + offset,
            node[i].y + offset,
            node[i].x + MYSIZE - offset,
            node[i].y + MYSIZE - offset,
            8, 8
        );
    }

    // 绘制蛇头特征（眼睛）- 保持不变
    if (!node.empty()) {
        // 根据方向确定眼睛位置
        int eyeSize = MYSIZE / 5;
        int eyeOffset = MYSIZE / 3;

        POINT leftEye, rightEye;

        switch (dirt) {
        case Direction::UP:
            leftEye = { node[0].x + eyeOffset, node[0].y + eyeOffset };
            rightEye = { node[0].x + MYSIZE - eyeOffset, node[0].y + eyeOffset };
            break;
        case Direction::DOWN:
            leftEye = { node[0].x + eyeOffset, node[0].y + MYSIZE - eyeOffset };
            rightEye = { node[0].x + MYSIZE - eyeOffset, node[0].y + MYSIZE - eyeOffset };
            break;
        case Direction::LEFT:
            leftEye = { node[0].x + eyeOffset, node[0].y + eyeOffset };
            rightEye = { node[0].x + eyeOffset, node[0].y + MYSIZE - eyeOffset };
            break;
        case Direction::RIGHT:
            leftEye = { node[0].x + MYSIZE - eyeOffset, node[0].y + eyeOffset };
            rightEye = { node[0].x + MYSIZE - eyeOffset, node[0].y + MYSIZE - eyeOffset };
            break;
        }

        // 绘制眼睛
        setfillcolor(RGB(255, 255, 255));
        solidcircle(leftEye.x, leftEye.y, eyeSize);
        solidcircle(rightEye.x, rightEye.y, eyeSize);

        setfillcolor(RGB(0, 0, 0));
        solidcircle(leftEye.x, leftEye.y, eyeSize / 2);
        solidcircle(rightEye.x, rightEye.y, eyeSize / 2);
    }
}

Food::~Food()
{
}

BigFood::BigFood(const std::unique_ptr<Snake>& snake)
{
    this->score = 5;
    this->spawnTime = std::chrono::steady_clock::now();

    std::unordered_set<std::tuple<int, int>> snakePositions;
    for (const auto& node : snake->node)
    {
        snakePositions.emplace(node.x, node.y);//直接插入坐标元组
    }

    //使用静态引擎避免重复初始化
    static std::mt19937 gen(std::random_device{}());
    const int gridWidth = WIDTH / MYSIZE;
    const int gridHeight = HEIGHT / MYSIZE;

    std::uniform_int_distribution<int> disX(0, gridWidth - 1);
    std::uniform_int_distribution<int> disY(0, gridHeight - 1);

    //有限次尝试机制
    constexpr int MAX_ATTEMPTS = 1000;
    int attempts = 0;

    do
    {
        //生成网格坐标后转换为像素坐标
        const int gridX = disX(gen);
        const int gridY = disY(gen);
        this->x = gridX * MYSIZE;
        this->y = gridY * MYSIZE;

        if (++attempts > MAX_ATTEMPTS)
        {
            throw std::runtime_error("BigFood生成失败，可能地图已满");
        }
    } while (snakePositions.count({ x, y }) > 0);//使用哈希表以实现O(1)复杂度查询
}

void BigFood::Show()
{
    // 使用时间戳创建闪烁效果
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // 更强烈的脉动效果
    double pulse = sin(ms * 0.015) * 0.3 + 0.7;

    // 基础颜色（金色）
    COLORREF baseColor = RGB(255, 215, 0);
    COLORREF currentColor = RGB(
        static_cast<int>(GetRValue(baseColor) * pulse),
        static_cast<int>(GetGValue(baseColor) * pulse),
        static_cast<int>(GetBValue(baseColor) * pulse)
    );

    setfillcolor(currentColor);
    setlinecolor(RGB(255, 255, 255));

    // 绘制大食物主体（星星形状）
    int centerX = this->x + MYSIZE / 2;
    int centerY = this->y + MYSIZE / 2;
    int outerRadius = static_cast<int>(MYSIZE * 0.5 * pulse);
    int innerRadius = static_cast<int>(outerRadius * 0.4);

    // 绘制五角星
    POINT points[10];
    for (int i = 0; i < 10; i++) {
        double angle = i * 3.14159 / 5 - 3.14159 / 2;
        int radius = (i % 2 == 0) ? outerRadius : innerRadius;
        points[i].x = centerX + static_cast<int>(radius * cos(angle));
        points[i].y = centerY + static_cast<int>(radius * sin(angle));
    }

    solidpolygon(points, 10);

    // 添加发光效果
    setlinecolor(RGB(255, 255, 0));
    setlinestyle(PS_SOLID, 2);
    for (int i = 0; i < 5; i++) {
        double angle = i * 2 * 3.14159 / 5 - 3.14159 / 2;
        int endX = centerX + static_cast<int>((outerRadius + 5) * cos(angle));
        int endY = centerY + static_cast<int>((outerRadius + 5) * sin(angle));
        line(centerX, centerY, endX, endY);
    }
    setlinestyle(PS_SOLID, 1);
}

BigFood::~BigFood()
{
}

bool BigFood::ShouldRemove() const
{
    auto now = std::chrono::steady_clock::now();//获取当前时间
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - spawnTime).count();//时间差
    return duration > BIGFOOD_DURATION;
}

BaseFood::BaseFood() : score(0), x(0), y(0)
{
}

SnakeNode::SnakeNode() : x(0), y(0), pulseOffset(0)
{
    for (size_t i = 0; i < 3; ++i)
        this->RGB[i] = 0;
}

void Food::Show()
{
    // 使用时间戳创建闪烁效果
    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    // 脉动效果
    double pulse = sin(ms * 0.01) * 0.2 + 0.8;

    // 基础颜色（苹果红）
    COLORREF baseColor = RGB(255, 50, 50);
    COLORREF currentColor = RGB(
        static_cast<int>(GetRValue(baseColor) * pulse),
        static_cast<int>(GetGValue(baseColor) * pulse),
        static_cast<int>(GetBValue(baseColor) * pulse)
    );

    setfillcolor(currentColor);
    setlinecolor(RGB(255, 255, 255));

    // 绘制食物主体（圆形）
    int centerX = this->x + MYSIZE / 2;
    int centerY = this->y + MYSIZE / 2;
    int radius = static_cast<int>(MYSIZE * 0.4 * pulse);

    solidcircle(centerX, centerY, radius);

    // 绘制高光
    setfillcolor(RGB(255, 255, 255));
    solidcircle(centerX - radius / 3, centerY - radius / 3, radius / 4);

    // 绘制茎
    setlinecolor(RGB(100, 200, 100));
    setlinestyle(PS_SOLID, 2);
    line(centerX, centerY - radius, centerX, centerY - radius - 5);
    setlinestyle(PS_SOLID, 1);

    // 绘制叶子
    setfillcolor(RGB(100, 200, 100));
    solidcircle(centerX + 2, centerY - radius - 3, 3);
}