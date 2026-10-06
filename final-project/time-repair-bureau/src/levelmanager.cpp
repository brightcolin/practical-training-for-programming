#include "levelmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <queue>
#include <utility>

namespace {
constexpr int kBuiltInLevelCount = 6;

void setTile(std::vector<TileKind> &tiles, int rows, int cols, const QPoint &cell, TileKind kind)
{
    if (cell.x() < 0 || cell.x() >= cols || cell.y() < 0 || cell.y() >= rows) {
        return;
    }
    tiles[static_cast<size_t>(cell.y() * cols + cell.x())] = kind;
}

TileKind tileFromKey(const QString &key)
{
    if (key == QStringLiteral("path")) return TileKind::Path;
    if (key == QStringLiteral("discount")) return TileKind::Discount;
    if (key == QStringLiteral("anchor")) return TileKind::Anchor;
    if (key == QStringLiteral("accelerate")) return TileKind::Accelerate;
    if (key == QStringLiteral("portal")) return TileKind::Portal;
    return TileKind::Stable;
}

int cellIndex(const QPoint &cell, int cols)
{
    return cell.y() * cols + cell.x();
}

bool isInside(const QPoint &cell, int rows, int cols)
{
    return cell.x() >= 0 && cell.x() < cols && cell.y() >= 0 && cell.y() < rows;
}

bool isRouteTile(TileKind tile)
{
    return tile == TileKind::Path || tile == TileKind::Accelerate || tile == TileKind::Portal;
}

bool areAdjacent(const QPoint &a, const QPoint &b)
{
    return std::abs(a.x() - b.x()) + std::abs(a.y() - b.y()) == 1;
}

bool isValidPath(const std::vector<QPoint> &path, const std::vector<TileKind> &tiles, int rows, int cols)
{
    if (path.size() < 2) {
        return false;
    }
    for (size_t i = 0; i < path.size(); ++i) {
        if (!isInside(path[i], rows, cols)) {
            return false;
        }
        if (!isRouteTile(tiles[static_cast<size_t>(cellIndex(path[i], cols))])) {
            return false;
        }
        if (i > 0 && !areAdjacent(path[i - 1], path[i])) {
            return false;
        }
    }
    return true;
}

std::vector<QPoint> routePathFromTiles(const std::vector<TileKind> &tiles, int rows, int cols)
{
    std::vector<QPoint> routeCells;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            if (isRouteTile(tiles[static_cast<size_t>(row * cols + col)])) {
                routeCells.push_back(QPoint(col, row));
            }
        }
    }
    if (routeCells.size() < 2) {
        return {};
    }

    const int minX = std::min_element(routeCells.begin(), routeCells.end(), [](const QPoint &a, const QPoint &b) {
                         return a.x() < b.x();
                     })->x();
    const int maxX = std::max_element(routeCells.begin(), routeCells.end(), [](const QPoint &a, const QPoint &b) {
                         return a.x() < b.x();
                     })->x();

    std::vector<QPoint> starts;
    std::vector<QPoint> ends;
    for (const QPoint &cell : routeCells) {
        if (cell.x() == 0 || cell.x() == minX) {
            starts.push_back(cell);
        }
        if (cell.x() == cols - 1 || cell.x() == maxX) {
            ends.push_back(cell);
        }
    }

    auto isEnd = [&ends](const QPoint &cell) {
        return std::find(ends.begin(), ends.end(), cell) != ends.end();
    };

    std::vector<QPoint> bestPath;
    for (const QPoint &start : starts) {
        std::vector<int> previous(static_cast<size_t>(rows * cols), -1);
        std::vector<bool> visited(static_cast<size_t>(rows * cols), false);
        std::queue<QPoint> queue;
        queue.push(start);
        visited[static_cast<size_t>(cellIndex(start, cols))] = true;

        QPoint reached(-1, -1);
        while (!queue.empty()) {
            const QPoint current = queue.front();
            queue.pop();
            if (current != start && isEnd(current)) {
                reached = current;
                break;
            }

            const std::vector<QPoint> neighbors = {
                QPoint(current.x() + 1, current.y()),
                QPoint(current.x(), current.y() + 1),
                QPoint(current.x(), current.y() - 1),
                QPoint(current.x() - 1, current.y())
            };
            for (const QPoint &next : neighbors) {
                if (!isInside(next, rows, cols)) {
                    continue;
                }
                const int nextIndex = cellIndex(next, cols);
                if (visited[static_cast<size_t>(nextIndex)] || !isRouteTile(tiles[static_cast<size_t>(nextIndex)])) {
                    continue;
                }
                visited[static_cast<size_t>(nextIndex)] = true;
                previous[static_cast<size_t>(nextIndex)] = cellIndex(current, cols);
                queue.push(next);
            }
        }

        if (reached.x() < 0) {
            continue;
        }

        std::vector<QPoint> path;
        for (QPoint cell = reached; cell.x() >= 0;) {
            path.push_back(cell);
            const int prev = previous[static_cast<size_t>(cellIndex(cell, cols))];
            if (prev < 0) {
                break;
            }
            cell = QPoint(prev % cols, prev / cols);
        }
        std::reverse(path.begin(), path.end());
        if (path.size() > bestPath.size()) {
            bestPath = path;
        }
    }
    return bestPath;
}
}

LevelData LevelManager::loadLevel(int levelIndex, const QString &customLevelPath, int rows, int cols)
{
    if (!customLevelPath.isEmpty()) {
        return customLevel(customLevelPath, rows, cols);
    }
    return builtInLevel(levelIndex, rows, cols);
}

int LevelManager::builtInLevelCount()
{
    return kBuiltInLevelCount;
}

QString LevelManager::levelTitle(int levelIndex)
{
    static const std::array<QString, kBuiltInLevelCount> titles = {
        QStringLiteral("钟楼回响"),
        QStringLiteral("断裂车站"),
        QStringLiteral("雾港传送站"),
        QStringLiteral("逆流档案馆"),
        QStringLiteral("深时能源井"),
        QStringLiteral("终末纪元")
    };
    return titles[static_cast<size_t>(std::clamp(levelIndex, 0, kBuiltInLevelCount - 1))];
}

QString LevelManager::levelDifficulty(int levelIndex)
{
    static const std::array<QString, kBuiltInLevelCount> difficulties = {
        QStringLiteral("入门 / 部署教学"),
        QStringLiteral("普通 / 节奏压力"),
        QStringLiteral("进阶 / 传送干扰"),
        QStringLiteral("困难 / 分裂抗性"),
        QStringLiteral("困难+ / 资源运营"),
        QStringLiteral("终局 / 协议汇合")
    };
    return difficulties[static_cast<size_t>(std::clamp(levelIndex, 0, kBuiltInLevelCount - 1))];
}

QString LevelManager::levelBriefing(int levelIndex)
{
    static const std::array<QString, kBuiltInLevelCount> briefings = {
        QStringLiteral("钟楼训练线被真实异常接管。这里原本用于教学，现在成为总部唯一可控的接入门。"),
        QStringLiteral("断裂车站被异常选为第一处城市节点。倒放广播、加速时流和密集人流残影证明它开始主动制造恐慌。"),
        QStringLiteral("雾港出现成对传送裂隙，入口与出口时间戳完全一致。幕后协议第一次表现出“铺路”能力。"),
        QStringLiteral("逆流档案馆的记录被提前改写，死亡名单反向生成分裂悖影。异常正在利用未来失败压迫现在。"),
        QStringLiteral("深时能源井仍在供能，却被污染协议夺取调度权。修补行动从单线防守升级为资源运营和 Boss 应急。"),
        QStringLiteral("终末纪元汇合了所有异常协议。它不是单个敌人，而是一条倒灌回来的失败时间线。")
    };
    return briefings[static_cast<size_t>(std::clamp(levelIndex, 0, kBuiltInLevelCount - 1))];
}

QString LevelManager::levelSelectSummary(int levelIndex)
{
    static const std::array<QString, kBuiltInLevelCount> summaries = {
        QStringLiteral("交互式训练：完成输出、经济、减速、升级和冻结的完整入门链路。"),
        QStringLiteral("节奏压力：快速敌人与加速格压缩反应时间，范围塔处理密集站台。"),
        QStringLiteral("空间扰动：传送裂隙改变战线位置，穿透塔负责封锁长直线。"),
        QStringLiteral("抗性扩散：分裂与抗滞叠加，屏障、范围和减速必须形成交叉火力。"),
        QStringLiteral("运营考验：补给格、汲取仪和 Boss 协议同时检验资源转化速度。"),
        QStringLiteral("终局协议：Boss 与多类型异常混合出现，考验完整阵线和冻结时机。")
    };
    return summaries[static_cast<size_t>(std::clamp(levelIndex, 0, kBuiltInLevelCount - 1))];
}

QString LevelManager::levelNextHook(int levelIndex)
{
    static const std::array<QString, kBuiltInLevelCount> hooks = {
        QStringLiteral("下一站，旧城车站的广播开始倒放同一句话：不要抵达。"),
        QStringLiteral("雾港传来同频折跃，异常不再只是前进，它开始选择捷径。"),
        QStringLiteral("档案馆记录被人动过，死亡名单里出现了尚未抵达战场的敌人。"),
        QStringLiteral("被改写的记录全部指向深时能源井，那里仍在给城市供能。"),
        QStringLiteral("能源井底部的协议签名亮起，终末纪元不是地点，而是结局。"),
        QStringLiteral("主时间线恢复连续，但黑匣仍保留一段未命名协议，等待后续扩展。")
    };
    return hooks[static_cast<size_t>(std::clamp(levelIndex, 0, kBuiltInLevelCount - 1))];
}

QString LevelManager::levelVictoryText(int levelIndex)
{
    static const std::array<QString, kBuiltInLevelCount> texts = {
        QStringLiteral("钟声重新按秒针落下，训练线恢复稳定。总部确认：你已经从旁听见习生转为临时修补员。"),
        QStringLiteral("车站广播重新同步，站台裂缝被临时因果桥固定。异常选择交通节点的证据已经上传。"),
        QStringLiteral("雾港裂隙被锁入闭环，传送异常失去跳跃锚点。刻度捕捉到一段被隐藏的协议签名。"),
        QStringLiteral("档案页码回到正序，逆流记录被封存进黑匣。有人提前写下下一次污染的证据仍未删除。"),
        QStringLiteral("能源井恢复供能，新的时能配额回流总部。终末纪元的门因此被迫显形。"),
        QStringLiteral("终末纪元完成封存，主时间线重新获得连续性。修补局保住了城市的今天，也留下下一次追查的线索。")
    };
    return texts[static_cast<size_t>(std::clamp(levelIndex, 0, kBuiltInLevelCount - 1))];
}

std::array<TowerSpec, 6> LevelManager::loadTowerSpecs()
{
    std::array<TowerSpec, 6> specs = {
        TowerSpec{60, 155.0, 30.0, 0.58, 100.0},
        TowerSpec{85, 145.0, 15.0, 0.95, 90.0},
        TowerSpec{120, 165.0, 26.0, 1.35, 100.0},
        TowerSpec{150, 190.0, 24.0, 1.05, 100.0},
        TowerSpec{125, 0.0, 0.0, 0.0, 85.0},
        TowerSpec{50, 0.0, 0.0, 0.0, 260.0}
    };

    const QString path = configPath(QStringLiteral("towers.json"));
    if (path.isEmpty()) {
        return specs;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return specs;
    }

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const std::vector<std::pair<QString, TowerKind>> keys = {
        {QStringLiteral("shooter"), TowerKind::Shooter},
        {QStringLiteral("slow"), TowerKind::Slow},
        {QStringLiteral("splash"), TowerKind::Splash},
        {QStringLiteral("laser"), TowerKind::Laser},
        {QStringLiteral("resource"), TowerKind::Resource},
        {QStringLiteral("wall"), TowerKind::Wall}
    };

    for (const auto &entry : keys) {
        const QJsonObject obj = root.value(entry.first).toObject();
        if (obj.isEmpty()) {
            continue;
        }
        TowerSpec &spec = specs[static_cast<size_t>(entry.second)];
        spec.cost = obj.value(QStringLiteral("cost")).toInt(spec.cost);
        spec.range = obj.value(QStringLiteral("range")).toDouble(spec.range);
        spec.damage = obj.value(QStringLiteral("damage")).toDouble(spec.damage);
        spec.cooldown = obj.value(QStringLiteral("cooldown")).toDouble(spec.cooldown);
        spec.hp = obj.value(QStringLiteral("hp")).toDouble(spec.hp);
    }

    return specs;
}

QString LevelManager::configPath(const QString &fileName)
{
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/config/"),
        QDir::currentPath() + QStringLiteral("/config/"),
        QDir::currentPath() + QStringLiteral("/../config/"),
        QDir::currentPath() + QStringLiteral("/../../project/src/config/"),
        QDir::currentPath() + QStringLiteral("/project/src/config/")
    };

    for (const QString &root : roots) {
        const QString candidate = QDir::cleanPath(root + fileName);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return QString();
}

LevelData LevelManager::builtInLevel(int levelIndex, int rows, int cols)
{
    LevelData data;
    data.tiles.assign(static_cast<size_t>(rows * cols), TileKind::Stable);
    levelIndex = std::clamp(levelIndex, 0, kBuiltInLevelCount - 1);
    data.name = levelTitle(levelIndex);
    data.briefing = levelBriefing(levelIndex);
    data.victoryText = levelVictoryText(levelIndex);

    std::vector<QPoint> anchors;
    std::vector<QPoint> discounts;
    std::vector<QPoint> accelerates;

    if (levelIndex == 0) {
        data.initialEnergy = 240;
        data.pathCells = {
            {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3},
            {4, 2}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1},
            {8, 2}, {8, 3}, {9, 3}, {10, 3}, {11, 3}
        };
        anchors = {{2, 1}, {2, 5}, {6, 4}, {9, 5}};
        discounts = {{1, 1}, {3, 5}, {7, 4}, {10, 1}};
        accelerates = {{4, 2}, {8, 2}};
        data.portalFrom = QPoint(5, 1);
        data.portalTo = QPoint(8, 3);
    } else if (levelIndex == 1) {
        data.initialEnergy = 220;
        data.pathCells = {
            {0, 1}, {1, 1}, {2, 1}, {3, 1}, {3, 2}, {3, 3},
            {4, 3}, {5, 3}, {6, 3}, {6, 4}, {6, 5}, {7, 5},
            {8, 5}, {9, 5}, {9, 4}, {9, 3}, {10, 3}, {11, 3}
        };
        anchors = {{1, 4}, {4, 1}, {5, 5}, {8, 2}, {10, 5}};
        discounts = {{2, 3}, {5, 1}, {7, 2}};
        accelerates = {{3, 2}, {6, 4}, {9, 4}};
        data.portalFrom = QPoint(5, 3);
        data.portalTo = QPoint(9, 3);
    } else if (levelIndex == 2) {
        data.initialEnergy = 245;
        data.pathCells = {
            {0, 4}, {1, 4}, {2, 4}, {3, 4}, {3, 3}, {3, 2},
            {4, 2}, {5, 2}, {6, 2}, {6, 3}, {6, 4}, {7, 4},
            {8, 4}, {8, 3}, {8, 2}, {9, 2}, {10, 2}, {11, 2}
        };
        anchors = {{1, 1}, {2, 6}, {5, 5}, {9, 5}, {10, 1}};
        discounts = {{2, 2}, {4, 5}, {7, 1}, {10, 4}};
        accelerates = {{3, 3}, {6, 3}, {8, 3}};
        data.portalFrom = QPoint(5, 2);
        data.portalTo = QPoint(8, 4);
    } else if (levelIndex == 3) {
        data.initialEnergy = 235;
        data.pathCells = {
            {0, 2}, {1, 2}, {2, 2}, {2, 3}, {2, 4}, {3, 4},
            {4, 4}, {5, 4}, {5, 5}, {6, 5}, {7, 5}, {7, 4},
            {7, 3}, {8, 3}, {9, 3}, {10, 3}, {11, 3}
        };
        anchors = {{1, 5}, {3, 1}, {4, 2}, {6, 2}, {9, 5}};
        discounts = {{1, 1}, {4, 6}, {8, 1}, {10, 5}};
        accelerates = {{2, 3}, {5, 5}, {7, 4}};
        data.portalFrom = QPoint(4, 4);
        data.portalTo = QPoint(7, 3);
    } else if (levelIndex == 4) {
        data.initialEnergy = 300;
        data.pathCells = {
            {0, 1}, {1, 1}, {1, 2}, {1, 3}, {2, 3}, {3, 3},
            {4, 3}, {4, 4}, {4, 5}, {5, 5}, {6, 5}, {7, 5},
            {8, 5}, {8, 4}, {8, 3}, {9, 3}, {10, 3}, {11, 3}
        };
        anchors = {{2, 1}, {3, 5}, {6, 2}, {9, 1}, {10, 5}};
        discounts = {{2, 5}, {5, 2}, {7, 3}, {9, 5}, {10, 1}};
        accelerates = {{1, 2}, {4, 4}, {8, 4}};
        data.portalFrom = QPoint(6, 5);
        data.portalTo = QPoint(8, 3);
    } else {
        data.initialEnergy = 260;
        data.pathCells = {
            {0, 5}, {1, 5}, {2, 5}, {2, 4}, {2, 3}, {3, 3},
            {4, 3}, {5, 3}, {5, 2}, {5, 1}, {6, 1}, {7, 1},
            {8, 1}, {8, 2}, {8, 3}, {9, 3}, {10, 3}, {11, 3}
        };
        anchors = {{1, 1}, {4, 5}, {6, 4}, {9, 1}, {10, 5}};
        discounts = {{3, 1}, {6, 2}, {7, 5}};
        accelerates = {{2, 4}, {5, 2}, {8, 2}, {9, 3}};
        data.portalFrom = QPoint(4, 3);
        data.portalTo = QPoint(8, 1);
    }

    for (const QPoint &cell : data.pathCells) {
        setTile(data.tiles, rows, cols, cell, TileKind::Path);
    }
    for (const QPoint &cell : anchors) {
        setTile(data.tiles, rows, cols, cell, TileKind::Anchor);
    }
    for (const QPoint &cell : discounts) {
        setTile(data.tiles, rows, cols, cell, TileKind::Discount);
    }
    for (const QPoint &cell : accelerates) {
        setTile(data.tiles, rows, cols, cell, TileKind::Accelerate);
    }
    setTile(data.tiles, rows, cols, data.portalFrom, TileKind::Portal);
    setTile(data.tiles, rows, cols, data.portalTo, TileKind::Portal);
    data.waves = wavesForLevel(levelIndex);
    return data;
}

LevelData LevelManager::customLevel(const QString &path, int rows, int cols)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return builtInLevel(0, rows, cols);
    }

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    LevelData data;
    data.name = root.value(QStringLiteral("name")).toString(QStringLiteral("自定义时间线"));
    data.briefing = QStringLiteral("自定义关卡已载入。系统会根据你编辑的路线重新校验异常行进路径。");
    data.victoryText = QStringLiteral("自定义时间线完成修补，关卡路径与防线均通过实战校验。");
    data.initialEnergy = root.value(QStringLiteral("initialEnergy")).toInt(260);
    data.portalFrom = QPoint(-1, -1);
    data.portalTo = QPoint(-1, -1);
    data.tiles.assign(static_cast<size_t>(rows * cols), TileKind::Stable);

    const QJsonArray tiles = root.value(QStringLiteral("tiles")).toArray();
    if (tiles.size() == rows * cols) {
        for (int i = 0; i < tiles.size(); ++i) {
            data.tiles[static_cast<size_t>(i)] = tileFromKey(tiles.at(i).toString());
        }
    }

    const QJsonArray pathArray = root.value(QStringLiteral("path")).toArray();
    for (const QJsonValue &value : pathArray) {
        const QJsonArray point = value.toArray();
        if (point.size() >= 2) {
            data.pathCells.push_back(QPoint(point.at(0).toInt(), point.at(1).toInt()));
        }
    }
    if (!isValidPath(data.pathCells, data.tiles, rows, cols)) {
        data.pathCells = routePathFromTiles(data.tiles, rows, cols);
    }
    if (data.pathCells.size() < 2) {
        data.pathCells.clear();
        const int fallbackRow = std::clamp(rows / 2, 0, std::max(0, rows - 1));
        for (int column = 0; column < cols; ++column) {
            const QPoint cell(column, fallbackRow);
            data.pathCells.push_back(cell);
            setTile(data.tiles, rows, cols, cell, TileKind::Path);
        }
    } else {
        for (const QPoint &cell : data.pathCells) {
            const int index = cellIndex(cell, cols);
            const TileKind tile = data.tiles[static_cast<size_t>(index)];
            if (tile != TileKind::Portal && tile != TileKind::Accelerate) {
                data.tiles[static_cast<size_t>(index)] = TileKind::Path;
            }
        }
    }

    const QJsonArray portalFrom = root.value(QStringLiteral("portalFrom")).toArray();
    const QJsonArray portalTo = root.value(QStringLiteral("portalTo")).toArray();
    if (portalFrom.size() >= 2) {
        data.portalFrom = QPoint(portalFrom.at(0).toInt(), portalFrom.at(1).toInt());
    }
    if (portalTo.size() >= 2) {
        data.portalTo = QPoint(portalTo.at(0).toInt(), portalTo.at(1).toInt());
    }

    data.waves = wavesForLevel(2);
    return data;
}

std::vector<std::vector<WaveEntry>> LevelManager::wavesForLevel(int levelIndex)
{
    if (levelIndex == 0) {
        return {
            {{EnemyKind::Normal, 5, 1.12}},
            {{EnemyKind::Normal, 7, 0.96}, {EnemyKind::Fast, 2, 0.86}},
            {{EnemyKind::Fast, 5, 0.74}, {EnemyKind::Normal, 5, 0.84}},
            {{EnemyKind::Armored, 4, 1.06}, {EnemyKind::Normal, 6, 0.78}},
            {{EnemyKind::Fast, 6, 0.62}, {EnemyKind::Armored, 4, 0.90}, {EnemyKind::Normal, 6, 0.62}},
            {{EnemyKind::Normal, 8, 0.58}, {EnemyKind::Boss, 1, 0.95}, {EnemyKind::Fast, 4, 0.48}}
        };
    }
    if (levelIndex == 1) {
        return {
            {{EnemyKind::Normal, 9, 0.78}},
            {{EnemyKind::Normal, 9, 0.66}, {EnemyKind::Fast, 5, 0.58}},
            {{EnemyKind::Fast, 10, 0.46}, {EnemyKind::Normal, 8, 0.56}},
            {{EnemyKind::Armored, 5, 0.86}, {EnemyKind::Fast, 7, 0.48}},
            {{EnemyKind::Normal, 12, 0.42}, {EnemyKind::Fast, 10, 0.38}, {EnemyKind::Armored, 4, 0.72}},
            {{EnemyKind::Fast, 12, 0.34}, {EnemyKind::Boss, 1, 0.88}, {EnemyKind::Normal, 10, 0.42}}
        };
    }
    if (levelIndex == 2) {
        return {
            {{EnemyKind::Normal, 8, 0.76}, {EnemyKind::Fast, 4, 0.58}},
            {{EnemyKind::Armored, 6, 0.82}, {EnemyKind::Normal, 8, 0.60}},
            {{EnemyKind::Fast, 8, 0.42}, {EnemyKind::Resistant, 5, 0.66}},
            {{EnemyKind::Resistant, 8, 0.54}, {EnemyKind::Armored, 5, 0.68}, {EnemyKind::Normal, 8, 0.48}},
            {{EnemyKind::Fast, 12, 0.36}, {EnemyKind::Resistant, 8, 0.48}, {EnemyKind::Armored, 6, 0.58}},
            {{EnemyKind::Resistant, 8, 0.42}, {EnemyKind::Boss, 1, 0.82}, {EnemyKind::Fast, 8, 0.34}}
        };
    }
    if (levelIndex == 3) {
        return {
            {{EnemyKind::Normal, 9, 0.72}, {EnemyKind::Fast, 4, 0.58}},
            {{EnemyKind::Resistant, 5, 0.76}, {EnemyKind::Normal, 8, 0.58}},
            {{EnemyKind::Splitter, 5, 0.74}, {EnemyKind::Fast, 8, 0.44}},
            {{EnemyKind::Splitter, 8, 0.60}, {EnemyKind::Armored, 6, 0.70}},
            {{EnemyKind::Resistant, 10, 0.46}, {EnemyKind::Splitter, 8, 0.52}, {EnemyKind::Fast, 8, 0.36}},
            {{EnemyKind::Splitter, 10, 0.46}, {EnemyKind::Boss, 1, 0.78}, {EnemyKind::Resistant, 8, 0.40}}
        };
    }
    if (levelIndex == 4) {
        return {
            {{EnemyKind::Normal, 10, 0.58}, {EnemyKind::Fast, 6, 0.42}},
            {{EnemyKind::Armored, 8, 0.68}, {EnemyKind::Resistant, 6, 0.50}},
            {{EnemyKind::Splitter, 8, 0.56}, {EnemyKind::Normal, 10, 0.42}},
            {{EnemyKind::Fast, 14, 0.30}, {EnemyKind::Resistant, 9, 0.40}, {EnemyKind::Armored, 6, 0.54}},
            {{EnemyKind::Splitter, 10, 0.42}, {EnemyKind::Resistant, 10, 0.38}, {EnemyKind::Fast, 12, 0.30}},
            {{EnemyKind::Armored, 8, 0.52}, {EnemyKind::Boss, 1, 0.72}, {EnemyKind::Splitter, 8, 0.36}}
        };
    }
    return {
        {{EnemyKind::Normal, 10, 0.60}, {EnemyKind::Fast, 8, 0.40}},
        {{EnemyKind::Armored, 8, 0.72}, {EnemyKind::Resistant, 8, 0.52}},
        {{EnemyKind::Splitter, 8, 0.58}, {EnemyKind::Fast, 12, 0.34}},
        {{EnemyKind::Armored, 9, 0.58}, {EnemyKind::Resistant, 10, 0.44}, {EnemyKind::Splitter, 6, 0.54}},
        {{EnemyKind::Fast, 16, 0.26}, {EnemyKind::Resistant, 10, 0.36}, {EnemyKind::Boss, 1, 0.78}},
        {{EnemyKind::Splitter, 11, 0.36}, {EnemyKind::Armored, 10, 0.46}, {EnemyKind::Boss, 2, 0.92}, {EnemyKind::Fast, 12, 0.26}}
    };
}
