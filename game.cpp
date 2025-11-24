#include "Game.h"

Game::Game() : gameover(false), should_break(false), bigFoodActive(false),
startUI(800, 600)  // 初始化开始界面
{
    Initialize();
}
Game::~Game()
{
    saveGameResult(); // 确保游戏退出时保存进度
    closegraph();
}

std::string Game::generatePlayerName() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1000, 9999);
    return "Player_" + std::to_string(dis(gen));
}

void Game::saveGameResult()
{
    if (currentRecordId > 0 && !gameover) {
        database.endGame(currentRecordId,
            snake->GetScore(),
            static_cast<int>(snake->getsize()),
            snake->GetCount(),
            0,
            "PAUSED");
    }
}

void Game::showGameStatistics()
{
    auto leaderboard = database.getLeaderboard();

    cleardevice();
    setbkcolor(BLACK);
    settextcolor(WHITE);
    settextstyle(20, 0, _T("宋体"));

    int y = 50;
    outtextxy(100, y, L"游戏统计");
    y += 40;

    settextstyle(16, 0, _T("宋体"));
    outtextxy(100, y, L"排行榜:");
    y += 30;

    for (size_t i = 0; i < leaderboard.size() && i < 5; ++i) {
        TCHAR entry[100];
        _stprintf_s(entry, _T("%d. %s - %d分"),
            i + 1,
            leaderboard[i].first.c_str(),
            leaderboard[i].second);
        outtextxy(120, y, entry);
        y += 25;
    }

    y += 20;
    outtextxy(100, y, L"按任意键返回...");

    while (!_kbhit()) {
        Sleep(100);
    }
    _getch();
}

void Game::showPlayerStats() {
    auto stats = database.getPlayerStats(currentPlayerId);
    auto achievements = database.getPlayerAchievements(currentPlayerId);

    cleardevice();
    setbkcolor(BLACK);
    settextcolor(WHITE);
    settextstyle(20, 0, _T("宋体"));

    int y = 50;
    outtextxy(100, y, L"玩家统计");
    y += 40;

    settextstyle(16, 0, _T("宋体"));

    // 显示基本统计
    for (const auto& stat : stats) {
        TCHAR statText[100];
        _stprintf_s(statText, _T("%s: %s"), stat.first.c_str(), stat.second.c_str());
        outtextxy(100, y, statText);
        y += 25;
    }

    y += 20;
    outtextxy(100, y, L"成就:");
    y += 25;

    // 显示成就
    if (achievements.empty()) {
        outtextxy(120, y, L"暂无成就");
        y += 25;
    }
    else {
        for (const auto& achievement : achievements) {
            TCHAR achievementText[100];
            _stprintf_s(achievementText, _T("✓ %s"), achievement.c_str());
            outtextxy(120, y, achievementText);
            y += 25;
        }
    }

    y += 20;
    outtextxy(100, y, L"按任意键返回...");

    while (!_kbhit()) {
        Sleep(100);
    }
    _getch();
}

void Game::Initialize()
{
    std::cout << "开始初始化游戏..." << std::endl;

    // 显示开始界面
    std::cout << "显示开始界面..." << std::endl;
    if (!startUI.showStartScreen()) {
        std::cout << "用户取消开始游戏" << std::endl;
        should_break = true;
        return;
    }
    std::cout << "开始界面显示完成" << std::endl;

    // 初始化数据库
    std::cout << "初始化数据库..." << std::endl;
    if (!database.initialize()) {
        std::cerr << "数据库初始化失败!" << std::endl;
        should_break = true;
        return;
    }
    std::cout << "数据库初始化成功!" << std::endl;

    // 生成玩家名并获取玩家ID
    std::cout << "创建玩家..." << std::endl;
    currentUsername = generatePlayerName();
    if (!database.createPlayer(currentUsername, currentPlayerId)) {
        std::cerr << "创建玩家失败!" << std::endl;
        should_break = true;
        return;
    }
    std::cout << "创建玩家成功: " << currentUsername << " (ID: " << currentPlayerId << ")" << std::endl;

    // 初始化游戏窗口
    std::cout << "初始化游戏窗口..." << std::endl;
    initgraph(WIDTH, HEIGHT);

    // 检查窗口是否创建成功
    HWND hwnd = GetHWnd();
    if (hwnd == NULL) {
        std::cerr << "游戏窗口创建失败!" << std::endl;
        should_break = true;
        return;
    }
    std::cout << "游戏窗口创建成功" << std::endl;

    setbkcolor(BLACK);
    cleardevice();

    // 显示加载界面
    std::cout << "显示加载界面..." << std::endl;
    settextcolor(WHITE);
    settextstyle(24, 0, _T("宋体"));
    outtextxy(WIDTH / 2 - 60, HEIGHT / 2 - 50, _T("游戏加载中..."));

    settextstyle(16, 0, _T("宋体"));
    outtextxy(WIDTH / 2 - 100, HEIGHT / 2, _T("正在初始化游戏环境"));
    FlushBatchDraw();

    // 模拟加载过程
    for (int i = 0; i < 3; i++) {
        outtextxy(WIDTH / 2 - 20 + i * 20, HEIGHT / 2 + 30, _T("."));
        Sleep(300);
        FlushBatchDraw();
    }

    // 开始游戏记录
    std::cout << "开始游戏记录..." << std::endl;
    currentRecordId = database.startGame(currentPlayerId);
    std::cout << "游戏记录ID: " << currentRecordId << std::endl;

    // 创建蛇和食物
    std::cout << "创建蛇和食物..." << std::endl;
    snake = std::make_unique<Snake>();
    SpawnFood();

    std::cout << "游戏初始化完成! 玩家: " << currentUsername << " (ID: " << currentPlayerId << ")" << std::endl;

    // 直接进入游戏，不显示"按任意键开始"
    std::cout << "进入游戏主循环..." << std::endl;
}   

void Game::ProcessInput()
{
    // 使用非阻塞方式检测按键
    if (_kbhit()) {
        int key = _getch();
        // 处理按键...
    }

    //使用GetAsyncKeyState检测按键（非阻塞）
    if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000)
    {
        snake->SetDirection(Direction::UP);
    }
    else if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000)
    {
        snake->SetDirection(Direction::DOWN);
    }
    else if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000)
    {
        snake->SetDirection(Direction::LEFT);
    }
    else if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000)
    {
        snake->SetDirection(Direction::RIGHT);
    }

    //检查ESC键
    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
    {
        should_break = true;
    }

    //检查R键重开
    if (gameover && (GetAsyncKeyState('R') & 0x8000))
    {
        snake->Reset();
        gameover = false;
        SpawnFood();
        if (Bfood != nullptr)
            Bfood.reset();
        bigFoodActive = false;
    }
}

void Game::SpawnFood()
{
    if (food == nullptr && Bfood == nullptr)
    {
        food = std::make_unique<Food>(snake);
    }
}

void Game::CheckBigFood()
{
    if (snake->GetCount() % 4 == 0 && snake->GetCount() > 0 && !bigFoodActive && Bfood == nullptr)
    {
        Bfood = std::make_unique<BigFood>(snake);
        bigFoodActive = true;
    }

    if (Bfood != nullptr && Bfood->ShouldRemove())
    {
        Bfood.reset();
        snake->setcount();
        bigFoodActive = false;
        SpawnFood();
    }
}

void Game::Update()
{
    if (gameover)
    {
        if (currentRecordId > 0) {
            database.endGame(currentRecordId,
                snake->GetScore(),
                static_cast<int>(snake->getsize()),
                snake->GetCount(),
                0,
                "COMPLETED");
        }

        HandleGameOver();
        return;
    }

    CheckBigFood();

    if (food != nullptr && snake->Eat(food))
    {
        // 记录食物被吃
        database.addFoodRecord(currentRecordId, "NORMAL", 1, food->x, food->y);
        food.reset();
        CheckBigFood();
        if (Bfood == nullptr)
            SpawnFood();
    }
    else if (Bfood != nullptr && snake->Eat(Bfood))
    {
        // 记录大食物被吃
        database.addFoodRecord(currentRecordId, "BIG", 5, Bfood->x, Bfood->y);
        Bfood.reset();
        bigFoodActive = false;
        SpawnFood();
    }

    if (snake->Defeat())
    {
        gameover = true;

        if (currentRecordId > 0) {
            database.endGame(currentRecordId,
                snake->GetScore(),
                static_cast<int>(snake->getsize()),
                snake->GetCount(),
                0,
                "FAILED");
        }
        return;
    }

    snake->Move();
    Sleep(SPEED);
}

void Game::Render()
{
    BeginBatchDraw();

    // 绘制渐变背景 - 调暗背景让网格更明显
    for (int y = 0; y < HEIGHT; y++) {
        int r = 5 + (y * 5 / HEIGHT);
        int g = 10 + (y * 8 / HEIGHT);
        int b = 15 + (y * 10 / HEIGHT);
        setlinecolor(RGB(r, g, b));
        line(0, y, WIDTH, y);
    }

    // 绘制更明显的网格 - 使用更亮的颜色和更粗的线
    setlinecolor(RGB(60, 70, 90));  // 更亮的网格颜色

    // 主网格线（粗一些）
    setlinestyle(PS_SOLID, 1);
    for (int x = 0; x < WIDTH; x += 40) {
        line(x, 0, x, HEIGHT);
    }
    for (int y = 0; y < HEIGHT; y += 40) {
        line(0, y, WIDTH, y);
    }

    // 次网格线（细一些，更密集）
    setlinecolor(RGB(40, 50, 70));
    setlinestyle(PS_SOLID, 1);
    for (int x = 0; x < WIDTH; x += 20) {
        line(x, 0, x, HEIGHT);
    }
    for (int y = 0; y < HEIGHT; y += 20) {
        line(0, y, WIDTH, y);
    }

    // 绘制边界线
    setlinecolor(RGB(100, 120, 150));
    setlinestyle(PS_SOLID, 2);
    rectangle(0, 0, WIDTH - 1, HEIGHT - 1);
    setlinestyle(PS_SOLID, 1);

    // 先绘制食物（在蛇下面）
    if (Bfood != nullptr)
    {
        Bfood->Show();
    }
    else if (food != nullptr)
    {
        food->Show();
    }

    // 再绘制蛇（在食物上面）
    snake->Show();
    snake->showUI();

    // 显示玩家信息
    settextcolor(WHITE);
    settextstyle(14, 0, _T("宋体"));
    TCHAR playerInfo[100];
    _stprintf_s(playerInfo, _T("玩家: %s"), currentUsername.c_str());
    outtextxy(10, 10, playerInfo);

    // 显示操作提示
    settextcolor(RGB(200, 200, 200));
    settextstyle(12, 0, _T("宋体"));
    outtextxy(10, HEIGHT - 60, _T("控制: W/A/S/D 或 方向键"));
    outtextxy(10, HEIGHT - 40, _T("退出: ESC"));
    outtextxy(10, HEIGHT - 20, _T("重新开始: R"));

    EndBatchDraw();
}

void Game::HandleGameOver()
{
    cleardevice();

    settextcolor(RED);
    settextstyle(24, 0, _T("宋体"));
    outtextxy(WIDTH / 2 - 80, 50, _T("游戏结束!"));

    settextcolor(WHITE);
    settextstyle(20, 0, _T("宋体"));
    TCHAR scoreText[50];
    _stprintf_s(scoreText, _T("最终分数: %d"), snake->GetScore());
    outtextxy(WIDTH / 2 - 80, 100, scoreText);

    TCHAR playerText[50];
    _stprintf_s(playerText, _T("玩家: %s"), currentUsername.c_str());
    outtextxy(WIDTH / 2 - 80, 130, playerText);

    auto leaderboard = database.getLeaderboard();
    settextstyle(16, 0, _T("宋体"));

    outtextxy(WIDTH / 2 - 80, 180, _T("排行榜"));
    int y = 210;
    for (size_t i = 0; i < leaderboard.size() && i < 5; ++i) {
        TCHAR entry[100];
        _stprintf_s(entry, _T("%d. %s - %d分"),
            i + 1,
            leaderboard[i].first.c_str(),
            leaderboard[i].second);
        outtextxy(WIDTH / 2 - 80, y, entry);
        y += 25;
    }

    y += 30;
    settextcolor(YELLOW);
    outtextxy(WIDTH / 2 - 120, y, _T("按 R 重新开始"));
    y += 25;
    outtextxy(WIDTH / 2 - 120, y, _T("按 S 查看统计"));
    y += 25;
    outtextxy(WIDTH / 2 - 120, y, _T("按 P 查看个人数据"));
    y += 25;
    outtextxy(WIDTH / 2 - 120, y, _T("按 ESC 退出"));

    while (true) {
        if (GetAsyncKeyState('R') & 0x8000) {
            snake->Reset();
            gameover = false;
            if (Bfood != nullptr)
                Bfood.reset();
            bigFoodActive = false;

            currentRecordId = database.startGame(currentPlayerId);
            SpawnFood();
            break;
        }
        else if (GetAsyncKeyState('S') & 0x8000) {
            showGameStatistics();
            HandleGameOver();
            break;
        }
        else if (GetAsyncKeyState('P') & 0x8000) {
            showPlayerStats();
            HandleGameOver();
            break;
        }
        else if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            should_break = true;
            break;
        }
        Sleep(100);
    }
}

void Game::Run()
{
    std::cout << "开始游戏主循环..." << std::endl;

    int frameCount = 0;
    while (!should_break)
    {
        frameCount++;
        if (frameCount % 100 == 0) {
            std::cout << "游戏运行中，帧数: " << frameCount << std::endl;
        }

        try {
            ProcessInput();
            Update();
            Render();
        }
        catch (const std::exception& e) {
            std::cerr << "游戏循环异常: " << e.what() << std::endl;
            break;
        }
        catch (...) {
            std::cerr << "未知异常发生在游戏循环中" << std::endl;
            break;
        }

        // 添加帧率控制
        Sleep(10);
    }

    std::cout << "游戏循环结束" << std::endl;
}