#include "editorwidget.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPainter>

#include <algorithm>
#include <queue>

namespace {
QString tileKey(EditorWidget::EditTile tile)
{
    switch (tile) {
    case EditorWidget::EditTile::Stable: return QStringLiteral("stable");
    case EditorWidget::EditTile::Path: return QStringLiteral("path");
    case EditorWidget::EditTile::Discount: return QStringLiteral("discount");
    case EditorWidget::EditTile::Anchor: return QStringLiteral("anchor");
    case EditorWidget::EditTile::Accelerate: return QStringLiteral("accelerate");
    case EditorWidget::EditTile::Portal: return QStringLiteral("portal");
    }
    return QStringLiteral("stable");
}

EditorWidget::EditTile tileFromKey(const QString &key)
{
    if (key == QStringLiteral("path")) return EditorWidget::EditTile::Path;
    if (key == QStringLiteral("discount")) return EditorWidget::EditTile::Discount;
    if (key == QStringLiteral("anchor")) return EditorWidget::EditTile::Anchor;
    if (key == QStringLiteral("accelerate")) return EditorWidget::EditTile::Accelerate;
    if (key == QStringLiteral("portal")) return EditorWidget::EditTile::Portal;
    return EditorWidget::EditTile::Stable;
}

int cellIndex(const QPoint &cell, int cols)
{
    return cell.y() * cols + cell.x();
}
}

EditorWidget::EditorWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    resetDefault();

    const std::vector<EditTile> tools = {
        EditTile::Stable, EditTile::Path, EditTile::Discount,
        EditTile::Anchor, EditTile::Accelerate, EditTile::Portal
    };

    for (EditTile tile : tools) {
        auto *button = new QPushButton(tileName(tile), this);
        button->setFocusPolicy(Qt::NoFocus);
        connect(button, &QPushButton::clicked, this, [this, tile]() {
            m_selected = tile;
            m_message = QStringLiteral("当前工具：%1").arg(tileName(tile));
            updateToolButtonStyles();
            update();
        });
        m_buttons.push_back(button);
    }

    m_saveButton = new QPushButton(QStringLiteral("保存地图"), this);
    m_loadButton = new QPushButton(QStringLiteral("加载地图"), this);
    m_playButton = new QPushButton(QStringLiteral("开始演练"), this);
    m_resetButton = new QPushButton(QStringLiteral("重置默认"), this);
    m_backButton = new QPushButton(QStringLiteral("返回"), this);

    connect(m_saveButton, &QPushButton::clicked, this, [this]() {
        saveLevel();
    });
    connect(m_loadButton, &QPushButton::clicked, this, &EditorWidget::loadLevel);
    connect(m_playButton, &QPushButton::clicked, this, [this]() {
        if (saveLevel()) {
            emit playRequested(customLevelPath());
        }
    });
    connect(m_resetButton, &QPushButton::clicked, this, [this]() {
        resetDefault();
        m_message = QStringLiteral("已重置为默认地图");
        update();
    });
    connect(m_backButton, &QPushButton::clicked, this, &EditorWidget::backRequested);
    updateToolButtonStyles();
}

QString EditorWidget::customLevelPath() const
{
    QDir dir(QCoreApplication::applicationDirPath() + QStringLiteral("/data"));
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }
    return dir.filePath(QStringLiteral("custom_level.json"));
}

void EditorWidget::resetDefault()
{
    m_tiles.assign(static_cast<size_t>(m_rows * m_cols), EditTile::Stable);
    const std::vector<QPoint> path = {
        {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3},
        {4, 2}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1},
        {8, 2}, {8, 3}, {9, 3}, {10, 3}, {11, 3}
    };
    for (const QPoint &cell : path) {
        m_tiles[static_cast<size_t>(cell.y() * m_cols + cell.x())] = EditTile::Path;
    }
    m_tiles[static_cast<size_t>(1 * m_cols + 5)] = EditTile::Portal;
    m_tiles[static_cast<size_t>(3 * m_cols + 8)] = EditTile::Portal;
}

bool EditorWidget::saveLevel()
{
    const std::vector<QPoint> generatedPath = routePath();
    if (generatedPath.size() < 2) {
        m_message = QStringLiteral("路线未连通：请用时间裂缝/加速时流/跃迁裂隙画出连续路线");
        update();
        return false;
    }
    if (generatedPath.size() < 8) {
        m_message = QStringLiteral("路线太短：请至少画出 8 格连续路线，避免开局瞬间失败");
        update();
        return false;
    }

    QJsonObject root;
    root.insert(QStringLiteral("name"), QStringLiteral("自定义时间线"));
    root.insert(QStringLiteral("rows"), m_rows);
    root.insert(QStringLiteral("cols"), m_cols);
    root.insert(QStringLiteral("initialEnergy"), 260);

    QJsonArray tiles;
    for (EditTile tile : m_tiles) {
        tiles.append(tileKey(tile));
    }
    root.insert(QStringLiteral("tiles"), tiles);

    const std::vector<QPoint> portals = portalCellsOnRoute(generatedPath);

    QJsonArray path;
    for (const QPoint &cell : generatedPath) {
        path.append(QJsonArray{cell.x(), cell.y()});
    }
    root.insert(QStringLiteral("path"), path);

    if (portals.size() >= 2) {
        root.insert(QStringLiteral("portalFrom"), QJsonArray{portals[0].x(), portals[0].y()});
        root.insert(QStringLiteral("portalTo"), QJsonArray{portals[1].x(), portals[1].y()});
    } else {
        root.insert(QStringLiteral("portalFrom"), QJsonArray{-1, -1});
        root.insert(QStringLiteral("portalTo"), QJsonArray{-1, -1});
    }

    QFile file(customLevelPath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        m_message = QStringLiteral("已保存自定义地图，路线长度 %1 格").arg(path.size());
        update();
        return true;
    } else {
        m_message = QStringLiteral("保存失败");
    }
    update();
    return false;
}

void EditorWidget::loadLevel()
{
    QFile file(customLevelPath());
    if (!file.open(QIODevice::ReadOnly)) {
        m_message = QStringLiteral("还没有可加载的自定义地图");
        update();
        return;
    }

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonArray tiles = root.value(QStringLiteral("tiles")).toArray();
    if (tiles.size() == m_rows * m_cols) {
        for (int i = 0; i < tiles.size(); ++i) {
            m_tiles[static_cast<size_t>(i)] = tileFromKey(tiles.at(i).toString());
        }
        m_message = QStringLiteral("已加载自定义地图");
    }
    update();
}

void EditorWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
    gradient.setColorAt(0.0, QColor(5, 13, 28));
    gradient.setColorAt(1.0, QColor(10, 31, 49));
    painter.fillRect(rect(), gradient);

    painter.setPen(QColor(255, 225, 154));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 24, QFont::Bold));
    painter.drawText(QRectF(32, 24, 360, 40), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("关卡编辑器"));

    painter.setPen(QColor(187, 223, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    painter.drawText(QRectF(34, 64, 620, 28), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("选择右侧工具后可点击或拖拽涂格，右键擦除；黄色连线就是敌人实际路线。"));

    painter.setPen(QColor(255, 225, 154));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));
    painter.drawText(QRectF(34, 90, 360, 24), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("当前工具：%1").arg(tileName(m_selected)));

    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            const QRectF r = cellRect(row, col).adjusted(2, 2, -2, -2);
            const EditTile tile = m_tiles[static_cast<size_t>(row * m_cols + col)];
            painter.setBrush(tileColor(tile));
            painter.setPen(QPen(QColor(83, 185, 214, 100), 1));
            painter.drawRoundedRect(r, 6, 6);
            painter.setPen(QColor(255, 238, 180));
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
            painter.drawText(r, Qt::AlignCenter, tileName(tile).left(2));
        }
    }

    const std::vector<QPoint> path = routePath();
    if (path.size() >= 2) {
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(255, 225, 154, 210), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        for (size_t i = 1; i < path.size(); ++i) {
            painter.drawLine(cellRect(path[i - 1].y(), path[i - 1].x()).center(),
                             cellRect(path[i].y(), path[i].x()).center());
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(78, 218, 255, 220));
        painter.drawEllipse(cellRect(path.front().y(), path.front().x()).center(), 9, 9);
        painter.setBrush(QColor(255, 105, 128, 220));
        painter.drawEllipse(cellRect(path.back().y(), path.back().x()).center(), 11, 11);
    }

    if (!m_message.isEmpty()) {
        painter.setPen(QColor(255, 225, 154));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
        painter.drawText(QRectF(0, height() - 46, width(), 26), Qt::AlignCenter, m_message);
    }
}

void EditorWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        paintCellAt(event->pos(), m_selected);
    } else if (event->button() == Qt::RightButton) {
        paintCellAt(event->pos(), EditTile::Stable);
    }
}

void EditorWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::LeftButton)) {
        paintCellAt(event->pos(), m_selected);
    } else if (event->buttons().testFlag(Qt::RightButton)) {
        paintCellAt(event->pos(), EditTile::Stable);
    }
}

void EditorWidget::resizeEvent(QResizeEvent *)
{
    const int panelX = width() - 178;
    const int usableWidth = width() - 230;
    const int usableHeight = height() - 170;
    m_cellSize = std::max(42, std::min(usableWidth / m_cols, usableHeight / m_rows));
    m_origin = QPoint(36, 118 + (usableHeight - m_cellSize * m_rows) / 2);

    int y = 126;
    for (auto *button : m_buttons) {
        button->setGeometry(panelX, y, 138, 32);
        y += 40;
    }
    y += 12;
    m_saveButton->setGeometry(panelX, y, 138, 32);
    m_loadButton->setGeometry(panelX, y + 40, 138, 32);
    m_playButton->setGeometry(panelX, y + 80, 138, 32);
    m_resetButton->setGeometry(panelX, y + 120, 138, 32);
    m_backButton->setGeometry(panelX, y + 160, 138, 32);
    updateToolButtonStyles();
}

void EditorWidget::paintCellAt(const QPoint &pos, EditTile tile)
{
    const QPoint cell = cellAt(pos);
    if (cell.x() < 0) {
        return;
    }
    m_tiles[static_cast<size_t>(cell.y() * m_cols + cell.x())] = tile;
    update();
}

void EditorWidget::updateToolButtonStyles()
{
    const QString selectedStyle =
        QStringLiteral("QPushButton { background: #315d78; color: #ffe19a; border: 1px solid #ffe19a; border-radius: 6px; }");
    const QString normalStyle =
        QStringLiteral("QPushButton { background: #102842; color: #dff8ff; border: 1px solid #50d4ff; border-radius: 6px; } QPushButton:hover { background: #18395c; }");
    const QString commandStyle =
        QStringLiteral("QPushButton { background: #142c45; color: #dff8ff; border: 1px solid #6aa8c8; border-radius: 6px; } QPushButton:hover { background: #1b3b5b; }");

    for (auto *button : m_buttons) {
        button->setStyleSheet(button->text() == tileName(m_selected) ? selectedStyle : normalStyle);
    }
    for (auto *button : {m_saveButton, m_loadButton, m_playButton, m_resetButton, m_backButton}) {
        if (button) {
            button->setStyleSheet(commandStyle);
        }
    }
}

std::vector<QPoint> EditorWidget::routePath() const
{
    std::vector<QPoint> routeCells;
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            const EditTile tile = m_tiles[static_cast<size_t>(row * m_cols + col)];
            if (isRouteTile(tile)) {
                routeCells.push_back(QPoint(col, row));
            }
        }
    }
    if (routeCells.size() < 2) {
        return routeCells;
    }

    std::vector<QPoint> starts;
    std::vector<QPoint> ends;
    const int minX = std::min_element(routeCells.begin(), routeCells.end(), [](const QPoint &a, const QPoint &b) {
                         return a.x() < b.x();
                     })->x();
    const int maxX = std::max_element(routeCells.begin(), routeCells.end(), [](const QPoint &a, const QPoint &b) {
                         return a.x() < b.x();
                     })->x();

    for (const QPoint &cell : routeCells) {
        if (cell.x() == 0 || cell.x() == minX) {
            starts.push_back(cell);
        }
        if (cell.x() == m_cols - 1 || cell.x() == maxX) {
            ends.push_back(cell);
        }
    }

    auto isEnd = [&ends](const QPoint &cell) {
        return std::find(ends.begin(), ends.end(), cell) != ends.end();
    };

    std::vector<QPoint> bestPath;
    for (const QPoint &start : starts) {
        std::vector<int> previous(static_cast<size_t>(m_rows * m_cols), -1);
        std::vector<bool> visited(static_cast<size_t>(m_rows * m_cols), false);
        std::queue<QPoint> queue;
        queue.push(start);
        visited[static_cast<size_t>(cellIndex(start, m_cols))] = true;

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
                if (next.x() < 0 || next.x() >= m_cols || next.y() < 0 || next.y() >= m_rows) {
                    continue;
                }
                const int nextIndex = cellIndex(next, m_cols);
                if (visited[static_cast<size_t>(nextIndex)]) {
                    continue;
                }
                if (!isRouteTile(m_tiles[static_cast<size_t>(nextIndex)])) {
                    continue;
                }
                visited[static_cast<size_t>(nextIndex)] = true;
                previous[static_cast<size_t>(nextIndex)] = cellIndex(current, m_cols);
                queue.push(next);
            }
        }

        if (reached.x() < 0) {
            continue;
        }

        std::vector<QPoint> path;
        for (QPoint cell = reached; cell.x() >= 0;) {
            path.push_back(cell);
            const int prev = previous[static_cast<size_t>(cellIndex(cell, m_cols))];
            if (prev < 0) {
                break;
            }
            cell = QPoint(prev % m_cols, prev / m_cols);
        }
        std::reverse(path.begin(), path.end());
        if (path.size() > bestPath.size()) {
            bestPath = path;
        }
    }

    return bestPath;
}

std::vector<QPoint> EditorWidget::portalCellsOnRoute(const std::vector<QPoint> &path) const
{
    std::vector<QPoint> portals;
    for (const QPoint &cell : path) {
        const int index = cellIndex(cell, m_cols);
        if (m_tiles[static_cast<size_t>(index)] == EditTile::Portal) {
            portals.push_back(cell);
        }
    }
    return portals;
}

bool EditorWidget::isRouteTile(EditTile tile) const
{
    return tile == EditTile::Path || tile == EditTile::Accelerate || tile == EditTile::Portal;
}

QPoint EditorWidget::cellAt(const QPoint &pos) const
{
    const int col = (pos.x() - m_origin.x()) / m_cellSize;
    const int row = (pos.y() - m_origin.y()) / m_cellSize;
    if (col < 0 || col >= m_cols || row < 0 || row >= m_rows) {
        return QPoint(-1, -1);
    }
    if (!cellRect(row, col).contains(pos)) {
        return QPoint(-1, -1);
    }
    return QPoint(col, row);
}

QRectF EditorWidget::cellRect(int row, int col) const
{
    return QRectF(m_origin.x() + col * m_cellSize,
                  m_origin.y() + row * m_cellSize,
                  m_cellSize,
                  m_cellSize);
}

QColor EditorWidget::tileColor(EditTile tile) const
{
    switch (tile) {
    case EditTile::Stable: return QColor(20, 50, 64, 215);
    case EditTile::Path: return QColor(42, 76, 110, 230);
    case EditTile::Discount: return QColor(34, 86, 64, 230);
    case EditTile::Anchor: return QColor(48, 54, 62, 230);
    case EditTile::Accelerate: return QColor(44, 92, 132, 240);
    case EditTile::Portal: return QColor(82, 48, 118, 240);
    }
    return QColor(20, 50, 64, 215);
}

QString EditorWidget::tileName(EditTile tile) const
{
    switch (tile) {
    case EditTile::Stable: return QStringLiteral("稳定时域");
    case EditTile::Path: return QStringLiteral("时间裂缝");
    case EditTile::Discount: return QStringLiteral("时能节点");
    case EditTile::Anchor: return QStringLiteral("时间锚石");
    case EditTile::Accelerate: return QStringLiteral("加速时流");
    case EditTile::Portal: return QStringLiteral("跃迁裂隙");
    }
    return QString();
}
