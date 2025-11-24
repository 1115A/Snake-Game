#include "AdvancedSQLiteDB.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>

AdvancedSQLiteDB::AdvancedSQLiteDB(const std::string& path) : db(nullptr), dbPath(path) {}

AdvancedSQLiteDB::~AdvancedSQLiteDB() {
    close();
}

bool AdvancedSQLiteDB::open() {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc) {
        std::cerr << "无法打开数据库: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // 启用外键约束
    sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    // 注册自定义函数
    sqlite3_create_function(db, "CALCULATE_LEVEL", 1, SQLITE_UTF8, nullptr,
        &AdvancedSQLiteDB::calculateLevel, nullptr, nullptr);
    sqlite3_create_function(db, "FORMAT_PLAY_TIME", 1, SQLITE_UTF8, nullptr,
        &AdvancedSQLiteDB::formatPlayTime, nullptr, nullptr);

    return true;
}

void AdvancedSQLiteDB::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool AdvancedSQLiteDB::initialize() {
    if (!open()) return false;

    const char* sql = R"(
        -- 玩家表（简化版，无密码）
        CREATE TABLE IF NOT EXISTS players (
            player_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            level INTEGER DEFAULT 1,
            total_score INTEGER DEFAULT 0,
            total_games INTEGER DEFAULT 0,
            highest_score INTEGER DEFAULT 0,
            register_date DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_login DATETIME,
            status TEXT DEFAULT 'ACTIVE'
        );
        
        -- 游戏记录表
        CREATE TABLE IF NOT EXISTS game_records (
            record_id INTEGER PRIMARY KEY AUTOINCREMENT,
            player_id INTEGER,
            start_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            end_time DATETIME,
            score INTEGER DEFAULT 0,
            snake_length INTEGER,
            food_eaten INTEGER,
            big_food_eaten INTEGER,
            game_duration INTEGER,
            game_status TEXT DEFAULT 'COMPLETED',
            FOREIGN KEY(player_id) REFERENCES players(player_id) ON DELETE CASCADE
        );
        
        -- 食物记录表
        CREATE TABLE IF NOT EXISTS food_records (
            food_id INTEGER PRIMARY KEY AUTOINCREMENT,
            record_id INTEGER,
            food_type TEXT NOT NULL,
            spawn_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            eaten_time DATETIME,
            position_x INTEGER,
            position_y INTEGER,
            score_value INTEGER,
            FOREIGN KEY(record_id) REFERENCES game_records(record_id) ON DELETE CASCADE
        );
        
        -- 系统配置表
        CREATE TABLE IF NOT EXISTS system_config (
            config_id INTEGER PRIMARY KEY AUTOINCREMENT,
            config_key TEXT UNIQUE NOT NULL,
            config_value TEXT,
            description TEXT,
            last_modified DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        
        -- 成就表
        CREATE TABLE IF NOT EXISTS achievements (
            achievement_id INTEGER PRIMARY KEY AUTOINCREMENT,
            player_id INTEGER,
            achievement_name TEXT NOT NULL,
            achievement_desc TEXT,
            unlock_date DATETIME DEFAULT CURRENT_TIMESTAMP,
            score_required INTEGER,
            FOREIGN KEY(player_id) REFERENCES players(player_id) ON DELETE CASCADE
        )
    )";

    char* errorMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL错误（表创建）: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        return false;
    }

    // 创建索引
    const char* indexSql = R"(
        CREATE INDEX IF NOT EXISTS idx_players_username ON players(username);
        CREATE INDEX IF NOT EXISTS idx_game_records_score ON game_records(score);
        CREATE INDEX IF NOT EXISTS idx_game_records_player ON game_records(player_id)
    )";

    rc = sqlite3_exec(db, indexSql, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL错误（索引创建）: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
    }

    // 创建视图
    const char* viewSql = R"(
        CREATE VIEW IF NOT EXISTS player_leaderboard AS
        SELECT 
            p.username,
            p.level,
            MAX(g.score) as best_score,
            COUNT(g.record_id) as games_played,
            RANK() OVER (ORDER BY MAX(g.score) DESC) as rank
        FROM players p
        LEFT JOIN game_records g ON p.player_id = g.player_id
        WHERE p.status = 'ACTIVE'
        GROUP BY p.player_id
        ORDER BY best_score DESC
    )";

    rc = sqlite3_exec(db, viewSql, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL错误（视图创建）: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
    }

    // 创建触发器
    const char* triggerSql = R"(
        CREATE TRIGGER IF NOT EXISTS update_player_stats
        AFTER INSERT ON game_records
        FOR EACH ROW
        BEGIN
            UPDATE players 
            SET total_score = total_score + NEW.score,
                total_games = total_games + 1,
                highest_score = MAX(highest_score, NEW.score),
                last_login = CURRENT_TIMESTAMP
            WHERE player_id = NEW.player_id;
        END
    )";

    rc = sqlite3_exec(db, triggerSql, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL错误（触发器创建）: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
    }

    // 插入默认配置
    const char* configSql = R"(
        INSERT OR IGNORE INTO system_config (config_key, config_value, description) VALUES 
        ('GAME_SPEED', '150', '游戏速度'),
        ('BIG_FOOD_SCORE', '5', '大食物分数'),
        ('INITIAL_LENGTH', '3', '初始长度'),
        ('MAX_LENGTH', '50', '最大长度'),
        ('BIG_FOOD_SPAWN', '4', '大食物生成间隔')
    )";

    rc = sqlite3_exec(db, configSql, nullptr, nullptr, &errorMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL错误（配置插入）: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
    }

    std::cout << "数据库初始化成功!" << std::endl;
    return true;
}

// 自定义函数实现
void AdvancedSQLiteDB::calculateLevel(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc == 1 && sqlite3_value_type(argv[0]) == SQLITE_INTEGER) {
        int totalScore = sqlite3_value_int(argv[0]);
        int level = 1;

        if (totalScore >= 1000) level = 5;
        else if (totalScore >= 500) level = 4;
        else if (totalScore >= 200) level = 3;
        else if (totalScore >= 100) level = 2;

        sqlite3_result_int(context, level);
    }
    else {
        sqlite3_result_null(context);
    }
}

void AdvancedSQLiteDB::formatPlayTime(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc == 1 && sqlite3_value_type(argv[0]) == SQLITE_INTEGER) {
        int seconds = sqlite3_value_int(argv[0]);
        int minutes = seconds / 60;
        int remainingSeconds = seconds % 60;

        std::stringstream ss;
        ss << minutes << "分" << remainingSeconds << "秒";
        sqlite3_result_text(context, ss.str().c_str(), -1, SQLITE_TRANSIENT);
    }
    else {
        sqlite3_result_null(context);
    }
}

bool AdvancedSQLiteDB::createPlayer(const std::string& username, int& playerId) {
    std::string sql = "INSERT INTO players (username) VALUES (?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "创建玩家失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    playerId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);

    std::cout << "创建玩家成功: " << username << " (ID: " << playerId << ")" << std::endl;
    return true;
}

int AdvancedSQLiteDB::startGame(int playerId) {
    std::string sql = "INSERT INTO game_records (player_id, start_time) VALUES (?, CURRENT_TIMESTAMP);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    sqlite3_bind_int(stmt, 1, playerId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "执行SQL语句失败: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return -1;
    }

    int recordId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);

    std::cout << "开始游戏记录 - 玩家ID: " << playerId << " 记录ID: " << recordId << std::endl;
    return recordId;
}

bool AdvancedSQLiteDB::endGame(int recordId, int score, int length, int food, int bigFood, const std::string& status) {
    std::string sql = R"(
        UPDATE game_records 
        SET end_time = CURRENT_TIMESTAMP,
            score = ?,
            snake_length = ?,
            food_eaten = ?,
            big_food_eaten = ?,
            game_status = ?,
            game_duration = CAST((julianday(CURRENT_TIMESTAMP) - julianday(start_time)) * 24 * 60 * 60 AS INTEGER)
        WHERE record_id = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "准备SQL语句失败: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, score);
    sqlite3_bind_int(stmt, 2, length);
    sqlite3_bind_int(stmt, 3, food);
    sqlite3_bind_int(stmt, 4, bigFood);
    sqlite3_bind_text(stmt, 5, status.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, recordId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success) {
        // 检查成就
        checkAchievements(recordId, score);
        std::cout << "结束游戏记录 - 记录ID: " << recordId << " 分数: " << score << " 状态: " << status << std::endl;
    }
    else {
        std::cerr << "更新游戏记录失败: " << sqlite3_errmsg(db) << std::endl;
    }

    return success;
}

bool AdvancedSQLiteDB::addFoodRecord(int recordId, const std::string& foodType, int scoreValue, int x, int y) {
    std::string sql = R"(
        INSERT INTO food_records (record_id, food_type, score_value, position_x, position_y, eaten_time)
        VALUES (?, ?, ?, ?, ?, CURRENT_TIMESTAMP);
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, recordId);
    sqlite3_bind_text(stmt, 2, foodType.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, scoreValue);
    sqlite3_bind_int(stmt, 4, x);
    sqlite3_bind_int(stmt, 5, y);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

void AdvancedSQLiteDB::checkAchievements(int recordId, int score) {
    // 检查并解锁成就
    if (score >= 100) {
        std::string sql = R"(
            INSERT OR IGNORE INTO achievements (player_id, achievement_name, achievement_desc, score_required)
            SELECT player_id, '百分达人', '单局游戏得分超过100分', 100
            FROM game_records 
            WHERE record_id = ? AND player_id NOT IN (
                SELECT player_id FROM achievements WHERE achievement_name = '百分达人'
            )
        )";
        execute(sql, { std::to_string(recordId) });
    }

    if (score >= 200) {
        std::string sql = R"(
            INSERT OR IGNORE INTO achievements (player_id, achievement_name, achievement_desc, score_required)
            SELECT player_id, '两百分高手', '单局游戏得分超过200分', 200
            FROM game_records 
            WHERE record_id = ? AND player_id NOT IN (
                SELECT player_id FROM achievements WHERE achievement_name = '两百分高手'
            )
        )";
        execute(sql, { std::to_string(recordId) });
    }

    if (score >= 500) {
        std::string sql = R"(
            INSERT OR IGNORE INTO achievements (player_id, achievement_name, achievement_desc, score_required)
            SELECT player_id, '五百分大师', '单局游戏得分超过500分', 500
            FROM game_records 
            WHERE record_id = ? AND player_id NOT IN (
                SELECT player_id FROM achievements WHERE achievement_name = '五百分大师'
            )
        )";
        execute(sql, { std::to_string(recordId) });
    }
}

bool AdvancedSQLiteDB::execute(const std::string& sql, const std::vector<std::string>& params) {
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    for (size_t i = 0; i < params.size(); i++) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_STATIC);
    }

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

std::vector<std::pair<std::string, int>> AdvancedSQLiteDB::getLeaderboard() {
    std::vector<std::pair<std::string, int>> leaderboard;

    const char* sql = "SELECT username, best_score FROM player_leaderboard ORDER BY rank LIMIT 10;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* name = sqlite3_column_text(stmt, 0);
            if (name != nullptr) {
                std::string username = reinterpret_cast<const char*>(name);
                int score = sqlite3_column_int(stmt, 1);
                leaderboard.emplace_back(username, score);
            }
        }
        sqlite3_finalize(stmt);
    }
    else {
        // 备用查询
        const char* backupSql =
            "SELECT p.username, MAX(g.score) as best_score "
            "FROM players p JOIN game_records g ON p.player_id = g.player_id "
            "GROUP BY p.username ORDER BY best_score DESC LIMIT 10;";

        sqlite3_stmt* backupStmt;
        if (sqlite3_prepare_v2(db, backupSql, -1, &backupStmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(backupStmt) == SQLITE_ROW) {
                const unsigned char* name = sqlite3_column_text(backupStmt, 0);
                if (name != nullptr) {
                    std::string username = reinterpret_cast<const char*>(name);
                    int score = sqlite3_column_int(backupStmt, 1);
                    leaderboard.emplace_back(username, score);
                }
            }
            sqlite3_finalize(backupStmt);
        }
    }

    return leaderboard;
}

std::vector<std::pair<std::string, std::string>> AdvancedSQLiteDB::getPlayerStats(int playerId) {
    std::vector<std::pair<std::string, std::string>> stats;

    const char* sql = R"(
        SELECT 
            username,
            level,
            total_score,
            total_games,
            highest_score
        FROM players
        WHERE player_id = ?
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, playerId);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* username = sqlite3_column_text(stmt, 0);
            if (username != nullptr) {
                stats.emplace_back("用户名", reinterpret_cast<const char*>(username));
            }
            else {
                stats.emplace_back("用户名", "未知");
            }
            stats.emplace_back("等级", std::to_string(sqlite3_column_int(stmt, 1)));
            stats.emplace_back("总分数", std::to_string(sqlite3_column_int(stmt, 2)));
            stats.emplace_back("总游戏数", std::to_string(sqlite3_column_int(stmt, 3)));
            stats.emplace_back("最高分", std::to_string(sqlite3_column_int(stmt, 4)));

            // 获取游戏次数和平均分
            const char* gameStatsSql = R"(
                SELECT 
                    COUNT(*) as games_played,
                    AVG(score) as avg_score,
                    MAX(score) as best_score
                FROM game_records 
                WHERE player_id = ?
            )";

            sqlite3_stmt* gameStmt;
            if (sqlite3_prepare_v2(db, gameStatsSql, -1, &gameStmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_int(gameStmt, 1, playerId);

                if (sqlite3_step(gameStmt) == SQLITE_ROW) {
                    stats.emplace_back("游戏次数", std::to_string(sqlite3_column_int(gameStmt, 0)));
                    double avgScore = sqlite3_column_double(gameStmt, 1);
                    stats.emplace_back("平均分", std::to_string(static_cast<int>(avgScore)));
                    stats.emplace_back("最佳成绩", std::to_string(sqlite3_column_int(gameStmt, 2)));
                }
                sqlite3_finalize(gameStmt);
            }
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}

std::vector<std::string> AdvancedSQLiteDB::getPlayerAchievements(int playerId) {
    std::vector<std::string> achievements;

    const char* sql = "SELECT achievement_name FROM achievements WHERE player_id = ? ORDER BY unlock_date DESC;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, playerId);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const unsigned char* name = sqlite3_column_text(stmt, 0);
            if (name != nullptr) {
                achievements.push_back(reinterpret_cast<const char*>(name));
            }
        }
        sqlite3_finalize(stmt);
    }

    return achievements;
}

bool AdvancedSQLiteDB::getConfig(const std::string& key, std::string& value) {
    std::string sql = "SELECT config_value FROM system_config WHERE config_key = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

    bool found = (sqlite3_step(stmt) == SQLITE_ROW);
    if (found) {
        value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return found;
}

bool AdvancedSQLiteDB::setConfig(const std::string& key, const std::string& value) {
    std::string sql = R"(
        INSERT OR REPLACE INTO system_config (config_key, config_value, last_modified)
        VALUES (?, ?, CURRENT_TIMESTAMP);
    )";

    return execute(sql, { key, value });
}

bool AdvancedSQLiteDB::executeSQL(const std::string& sql) {
    char* errorMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errorMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL执行错误: " << errorMsg << std::endl;
        sqlite3_free(errorMsg);
        return false;
    }
    return true;
}

void AdvancedSQLiteDB::populateTestData() {
    // 检查是否已有数据
    const char* checkSql = "SELECT COUNT(*) FROM players;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            if (count > 0) {
                sqlite3_finalize(stmt);
                return; // 已有数据，不重复插入
            }
        }
        sqlite3_finalize(stmt);
    }

    // 插入测试玩家
    const char* players[] = {
        "INSERT INTO players (username, total_score, total_games, highest_score) VALUES ('SnakeMaster', 450, 10, 120);",
        "INSERT INTO players (username, total_score, total_games, highest_score) VALUES ('FoodHunter', 320, 8, 95);",
        "INSERT INTO players (username, total_score, total_games, highest_score) VALUES ('SpeedRunner', 280, 6, 85);",
        "INSERT INTO players (username, total_score, total_games, highest_score) VALUES ('NewPlayer', 150, 3, 60);",
        "INSERT INTO players (username, total_score, total_games, highest_score) VALUES ('ProGamer', 520, 12, 150);"
    };

    char* errorMsg = nullptr;
    for (int i = 0; i < 5; i++) {
        sqlite3_exec(db, players[i], nullptr, nullptr, &errorMsg);
    }
}