// Game.h - 添加开始界面
#pragma once
#include "Snake.h"
#include "AdvancedSQLiteDB.h"
#include "StartUI.h"  // 新增
#include <conio.h>
#include <string>
#include <memory>
#include <random>
#include<iostream>

class Game
{
private:
    std::unique_ptr<Snake> snake;
    std::unique_ptr<Food> food;
    std::unique_ptr<BigFood> Bfood;

    AdvancedSQLiteDB database;
    StartUI startUI;  // 新增

    bool gameover;
    bool should_break;
    bool bigFoodActive;
    int currentPlayerId;
    int currentRecordId;
    std::string currentUsername;

    void Initialize();
    void ProcessInput();
    void Update();
    void Render();
    void HandleGameOver();
    void SpawnFood();
    void CheckBigFood();
    void saveGameResult();
    void showGameStatistics();
    void showPlayerStats();
    std::string generatePlayerName();

public:
    Game();
    ~Game();
    void Run();
};