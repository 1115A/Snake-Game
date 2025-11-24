// AdvancedSQLiteDB.h - 完整数据库版本
#pragma once
#include "sqlite3.h"
#include <string>
#include <vector>
#include <utility>

class AdvancedSQLiteDB {
private:
    sqlite3* db;
    std::string dbPath;

    // 自定义SQLite函数
    static void calculateLevel(sqlite3_context* context, int argc, sqlite3_value** argv);
    static void formatPlayTime(sqlite3_context* context, int argc, sqlite3_value** argv);

    // 私有辅助方法
    void checkAchievements(int recordId, int score);
    bool execute(const std::string& sql, const std::vector<std::string>& params = {});

public:
    AdvancedSQLiteDB(const std::string& path = "snake_game.db");
    ~AdvancedSQLiteDB();

    bool open();
    void close();
    bool initialize();

    // 玩家管理
    bool createPlayer(const std::string& username, int& playerId);

    // 游戏记录操作
    int startGame(int playerId);
    bool endGame(int recordId, int score, int length, int food, int bigFood, const std::string& status = "COMPLETED");
    bool addFoodRecord(int recordId, const std::string& foodType, int scoreValue, int x, int y);

    // 查询功能
    std::vector<std::pair<std::string, int>> getLeaderboard();
    std::vector<std::pair<std::string, std::string>> getPlayerStats(int playerId);
    std::vector<std::string> getPlayerAchievements(int playerId);

    // 管理功能
    bool executeSQL(const std::string& sql);
    void populateTestData();

    // 配置管理
    bool getConfig(const std::string& key, std::string& value);
    bool setConfig(const std::string& key, const std::string& value);
};