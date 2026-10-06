#include "gamewidget.h"

#include "entityfactory.h"
#include "entityrules.h"
#include "levelmanager.h"
#include "progressmanager.h"
#include "soundmanager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>

namespace {
constexpr double kFrameSeconds = 1.0 / 60.0;
constexpr double kDesignCellSize = 58.0;
constexpr int kBaseTowerButtonWidth = 132;

QPointF normalized(const QPointF &v)
{
    const double len = std::sqrt(v.x() * v.x() + v.y() * v.y());
    if (len <= 0.001) {
        return QPointF();
    }
    return v / len;
}

QColor alphaColor(QColor color, int alpha)
{
    color.setAlpha(alpha);
    return color;
}

void drawGlassPanel(QPainter &painter, const QRectF &rect, const QColor &accent)
{
    QLinearGradient fill(rect.topLeft(), rect.bottomRight());
    fill.setColorAt(0.0, QColor(11, 29, 48, 235));
    fill.setColorAt(1.0, QColor(6, 16, 31, 225));
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawRoundedRect(rect, 8, 8);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(alphaColor(accent, 150), 1));
    painter.drawRoundedRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), 8, 8);
    painter.setPen(QPen(QColor(255, 225, 154, 42), 1));
    painter.drawLine(rect.topLeft() + QPointF(12, 1), rect.topRight() + QPointF(-12, 1));
}

void drawHoverPanel(QPainter &painter, const QRectF &rect, const QColor &accent)
{
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 118));
    painter.drawRoundedRect(rect.translated(0, 8), 12, 12);

    QLinearGradient fill(rect.topLeft(), rect.bottomRight());
    fill.setColorAt(0.0, QColor(6, 18, 34, 248));
    fill.setColorAt(0.58, QColor(9, 28, 45, 248));
    fill.setColorAt(1.0, QColor(4, 10, 22, 250));
    painter.setBrush(fill);
    painter.drawRoundedRect(rect, 12, 12);

    QLinearGradient edge(rect.topLeft(), rect.topRight());
    edge.setColorAt(0.0, alphaColor(accent.lighter(125), 210));
    edge.setColorAt(0.48, QColor(255, 225, 154, 150));
    edge.setColorAt(1.0, alphaColor(accent, 95));
    painter.setPen(QPen(QBrush(edge), 1.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(rect.adjusted(0.7, 0.7, -0.7, -0.7), 12, 12);

    painter.setPen(Qt::NoPen);
    painter.setBrush(alphaColor(accent, 175));
    painter.drawRoundedRect(QRectF(rect.left(), rect.top() + 12, 4, rect.height() - 24), 2, 2);
    painter.setBrush(QColor(255, 225, 154, 54));
    painter.drawRoundedRect(QRectF(rect.left() + 14, rect.top() + 12, rect.width() - 28, 1.4), 1, 1);
    painter.restore();
}

void drawStatusBar(QPainter &painter, const QRectF &rect, double ratio, const QColor &fill)
{
    const double clamped = std::clamp(ratio, 0.0, 1.0);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(18, 20, 31, 230));
    painter.drawRoundedRect(rect, 2, 2);
    QLinearGradient gradient(rect.topLeft(), rect.topRight());
    gradient.setColorAt(0.0, fill.lighter(135));
    gradient.setColorAt(1.0, fill.darker(115));
    painter.setBrush(gradient);
    painter.drawRoundedRect(QRectF(rect.x(), rect.y(), rect.width() * clamped, rect.height()), 2, 2);
}

QString gameActionButtonStyle()
{
    return QStringLiteral(
        "QPushButton { color: #dff8ff; background: rgba(12, 32, 52, 220);"
        " border: 1px solid rgba(80, 212, 255, 170); border-radius: 8px;"
        " font-size: 13px; font-weight: 700; padding: 4px 10px; }"
        "QPushButton:hover { color: #ffe19a; background: rgba(31, 70, 103, 235); border-color: #ffe19a; }"
        "QPushButton:pressed { background: rgba(7, 17, 31, 245); border-color: #9ed8ff; }");
}

QPolygonF diamondAt(const QPointF &center, double radius)
{
    QPolygonF polygon;
    polygon << center + QPointF(0, -radius)
            << center + QPointF(radius, 0)
            << center + QPointF(0, radius)
            << center + QPointF(-radius, 0);
    return polygon;
}

QString assetPath(const QString &fileName)
{
    const QString relative = QStringLiteral("assets/ai/") + fileName;
    const QStringList roots = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../src/"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../../src/"),
        QDir::currentPath() + QStringLiteral("/"),
        QDir::currentPath() + QStringLiteral("/../src/"),
        QDir::currentPath() + QStringLiteral("/../../src/"),
        QDir::currentPath() + QStringLiteral("/../"),
        QDir::currentPath() + QStringLiteral("/../../"),
        QDir::currentPath() + QStringLiteral("/../../../"),
        QDir::currentPath() + QStringLiteral("/../../../../"),
        QDir::currentPath() + QStringLiteral("/../../project/src/"),
        QDir::currentPath() + QStringLiteral("/project/src/")
    };
    for (const QString &root : roots) {
        const QString candidate = QDir::cleanPath(root + relative);
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return QString();
}

void drawCoverPixmap(QPainter &painter, const QPixmap &pixmap, const QRectF &target)
{
    if (pixmap.isNull()) {
        return;
    }
    const double sourceRatio = static_cast<double>(pixmap.width()) / std::max(1, pixmap.height());
    const double targetRatio = target.width() / std::max(1.0, target.height());
    QRect source = pixmap.rect();
    if (sourceRatio > targetRatio) {
        const int newW = static_cast<int>(pixmap.height() * targetRatio);
        source.setX((pixmap.width() - newW) / 2);
        source.setWidth(newW);
    } else {
        const int newH = static_cast<int>(pixmap.width() / targetRatio);
        source.setY((pixmap.height() - newH) / 2);
        source.setHeight(newH);
    }
    painter.drawPixmap(target, pixmap, source);
}

double towerIconVerticalOffset(TowerKind kind, double cellSize)
{
    if (kind == TowerKind::Splash) {
        return cellSize * 0.105;
    }
    if (kind == TowerKind::Wall) {
        return cellSize * 0.075;
    }
    return 0.0;
}

int towerUnlockLevel(TowerKind kind)
{
    switch (kind) {
    case TowerKind::Shooter:
    case TowerKind::Slow:
    case TowerKind::Resource:
        return 0;
    case TowerKind::Splash:
        return 1;
    case TowerKind::Laser:
        return 2;
    case TowerKind::Wall:
        return 3;
    }
    return 0;
}

[[maybe_unused]] QString towerAuthorizationText(int levelIndex)
{
    switch (std::clamp(levelIndex, 0, LevelManager::builtInLevelCount() - 1)) {
    case 0:
        return QStringLiteral("初始授权：输出、减速、经济三件套。");
    case 1:
        return QStringLiteral("新装置：悖论震荡器，处理密集波。");
    case 2:
        return QStringLiteral("新装置：因果切割器，封锁长直线。");
    case 3:
        return QStringLiteral("新装置：锚点屏障，拖住冲线敌人。");
    default:
        return QStringLiteral("全装置开放：按路线组合火力。");
    }
}

QString towerIntroTitle(TowerKind kind)
{
    switch (kind) {
    case TowerKind::Shooter: return QStringLiteral("指针炮台");
    case TowerKind::Slow: return QStringLiteral("凝滞棱镜");
    case TowerKind::Splash: return QStringLiteral("悖论震荡器");
    case TowerKind::Laser: return QStringLiteral("因果切割器");
    case TowerKind::Resource: return QStringLiteral("时能汲取仪");
    case TowerKind::Wall: return QStringLiteral("锚点屏障");
    }
    return QStringLiteral("修补装置");
}

QString enemyIntroTitle(EnemyKind kind)
{
    switch (kind) {
    case EnemyKind::Normal: return QStringLiteral("敌情识别：裂隙残影");
    case EnemyKind::Fast: return QStringLiteral("敌情识别：加速残影");
    case EnemyKind::Armored: return QStringLiteral("敌情识别：固化悖论");
    case EnemyKind::Resistant: return QStringLiteral("敌情识别：抗滞异常体");
    case EnemyKind::Splitter: return QStringLiteral("敌情识别：分叉时间体");
    case EnemyKind::Boss: return QStringLiteral("敌情识别：纪元崩坏体");
    }
    return QStringLiteral("敌情识别");
}

QString bossProtocolName(int levelIndex)
{
    static const std::array<QString, 6> names = {
        QStringLiteral("校准守门者"),
        QStringLiteral("疾行站台长"),
        QStringLiteral("雾港折跃核"),
        QStringLiteral("逆流编目者"),
        QStringLiteral("深井护盾体"),
        QStringLiteral("纪元主宰")
    };
    return names[static_cast<size_t>(std::clamp(levelIndex, 0, 5))];
}

[[maybe_unused]] QString levelStrategyText(int levelIndex)
{
    static const std::array<QString, 6> texts = {
        QStringLiteral("先用指针炮台守住拐角，再用减速和冻结处理关底守门者。"),
        QStringLiteral("出口前提前放减速，密集波用悖论震荡器覆盖站台拥堵点。"),
        QStringLiteral("传送出口必须有第二层火力，因果切割器优先放长直线。"),
        QStringLiteral("第一波先稳住基础输出，后续用屏障拖住分裂与抗性组合。"),
        QStringLiteral("经济塔要早放，但核心前至少保留两层升级火力。"),
        QStringLiteral("全装置协同，保留冻结给纪元主宰和最终混合波。")
    };
    return texts[static_cast<size_t>(std::clamp(levelIndex, 0, 5))];
}

QString tutorialFeedback(int step)
{
    switch (step) {
    case 1: return QStringLiteral("目标：看时能，按空格键");
    case 2: return QStringLiteral("目标：看路线，按空格键");
    case 3: return QStringLiteral("目标：左键在高亮格建炮台");
    case 4: return QStringLiteral("目标：看敌人标签，按空格键");
    case 5: return QStringLiteral("目标：右键升级炮台");
    case 6: return QStringLiteral("目标：鼠标点击“时能汲取仪”");
    case 7: return QStringLiteral("目标：左键部署时能汲取仪");
    case 8: return QStringLiteral("目标：鼠标点击“凝滞棱镜”");
    case 9: return QStringLiteral("目标：左键部署凝滞棱镜");
    case 10: return QStringLiteral("目标：按 F 冻结全场");
    }
    return QString();
}

QString tutorialTitle(int step)
{
    switch (step) {
    case 1: return QStringLiteral("教学 1/10：时能与防线");
    case 2: return QStringLiteral("教学 2/10：路线与格子");
    case 3: return QStringLiteral("教学 3/10：指针炮台");
    case 4: return QStringLiteral("教学 4/10：敌人属性");
    case 5: return QStringLiteral("教学 5/10：指针炮台");
    case 6: return QStringLiteral("教学 6/10：时能汲取仪");
    case 7: return QStringLiteral("教学 7/10：时能汲取仪");
    case 8: return QStringLiteral("教学 8/10：凝滞棱镜");
    case 9: return QStringLiteral("教学 9/10：凝滞棱镜");
    case 10: return QStringLiteral("教学 10/10：主动技能");
    }
    return QStringLiteral("教学");
}

QString tutorialBody(int step)
{
    switch (step) {
    case 1:
        return QStringLiteral("左上角是关卡、波次和时能。时能用于建塔和升级。按空格键继续。");
    case 2:
        return QStringLiteral("蓝色轨道是敌人路线；普通塔放稳定格，补给格更便宜。按空格键继续。");
    case 3:
        return QStringLiteral("定位：低费持续伤害。左键点击高亮格，把第一座炮台放到拐角火力位。");
    case 4:
        return QStringLiteral("敌人看生命、速度、护盾和抗性。高速怕减速，装甲怕升级火力。");
    case 5:
        return QStringLiteral("升级会提高持续输出。右键刚才的炮台，把核心拐角变成主火力点。");
    case 6:
        return QStringLiteral("定位：收集时能，不攻击。用鼠标点击右侧按钮，准备补经济。");
    case 7:
        return QStringLiteral("左键点击高亮格。经济装置放在安全格，负责长期补给而不是拦敌。");
    case 8:
        return QStringLiteral("定位：减速控制，低伤害。用鼠标点击右侧按钮，让敌人多停在火力区。");
    case 9:
        return QStringLiteral("左键点击高亮格。把减速放在火力前方，延长炮台输出时间。");
    case 10:
        return QStringLiteral("按 F 释放全场冻结。完成后进入 3 秒倒计时。");
    }
    return QString();
}

QString towerDrillRoleText(TowerKind kind)
{
    switch (kind) {
    case TowerKind::Splash: return QStringLiteral("范围伤害，附带持续灼蚀，专门清密集波。");
    case TowerKind::Laser: return QStringLiteral("直线穿透，适合长直线、传送出口和收尾。");
    case TowerKind::Wall: return QStringLiteral("路径屏障，阻挡敌人，为后排火力买时间。");
    case TowerKind::Slow: return QStringLiteral("减速控制，低伤害，让敌人在火力区停更久。");
    case TowerKind::Resource: return QStringLiteral("收集时能，不攻击，越早部署越能补经济。");
    case TowerKind::Shooter: return QStringLiteral("低费持续伤害，稳定处理单体和开局压力。");
    }
    return QString();
}

const std::array<EnemyKind, 6> &reverseEnemyOrder()
{
    static const std::array<EnemyKind, 6> order = {
        EnemyKind::Normal,
        EnemyKind::Fast,
        EnemyKind::Armored,
        EnemyKind::Resistant,
        EnemyKind::Splitter,
        EnemyKind::Boss
    };
    return order;
}

int reverseScenarioClamp(int scenarioIndex)
{
    return std::clamp(scenarioIndex, 0, 2);
}
}

GameWidget::GameWidget(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    m_pauseButton = new QPushButton(QStringLiteral("暂停"), this);
    m_restartButton = new QPushButton(QStringLiteral("重新开始"), this);
    m_menuButton = new QPushButton(QStringLiteral("返回"), this);
    m_settingsButton = new QPushButton(QStringLiteral("选项"), this);
    m_skipTutorialButton = new QPushButton(QStringLiteral("跳过教学"), this);
    m_reverseLaunchButton = new QPushButton(QStringLiteral("开始本波"), this);
    m_pauseButton->setStyleSheet(gameActionButtonStyle());
    m_restartButton->setStyleSheet(gameActionButtonStyle());
    m_menuButton->setStyleSheet(gameActionButtonStyle());
    m_settingsButton->setStyleSheet(gameActionButtonStyle());
    m_settingsButton->setFocusPolicy(Qt::NoFocus);
    m_settingsButton->setVisible(false);
    m_skipTutorialButton->setStyleSheet(gameActionButtonStyle());
    m_skipTutorialButton->setFocusPolicy(Qt::NoFocus);
    m_skipTutorialButton->setVisible(false);
    m_reverseLaunchButton->setStyleSheet(gameActionButtonStyle());
    m_reverseLaunchButton->setFocusPolicy(Qt::NoFocus);
    m_reverseLaunchButton->setVisible(false);
    m_commandInput = new QLineEdit(this);
    m_commandInput->setPlaceholderText(QStringLiteral("输入命令：energy / clear / next / win"));
    m_commandInput->setVisible(false);
    m_commandInput->setFocusPolicy(Qt::StrongFocus);
    m_commandInput->setStyleSheet(QStringLiteral(
        "QLineEdit { background: #07111f; color: #ffe19a; border: 1px solid #50d4ff;"
        " border-radius: 8px; padding: 8px; font-size: 16px; }"));
    setupTowerButtons();

    connect(m_pauseButton, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        m_paused = !m_paused;
        m_pauseButton->setText(m_paused ? QStringLiteral("继续") : QStringLiteral("暂停"));
        updateControlButtonGeometry();
        update();
    });
    connect(m_restartButton, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        resetGame();
    });
    connect(m_menuButton, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        emit backToMenu();
    });
    connect(m_settingsButton, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        emit settingsRequested();
    });
    connect(m_skipTutorialButton, &QPushButton::clicked, this, &GameWidget::skipTutorial);
    connect(m_reverseLaunchButton, &QPushButton::clicked, this, &GameWidget::startReverseWave);
    connect(m_commandInput, &QLineEdit::returnPressed, this, &GameWidget::executeCommandInput);

    connect(&m_timer, &QTimer::timeout, this, &GameWidget::updateGame);
    m_timer.start(16);

    loadVisualAssets();
    loadTowerConfig();
    resetGame();
}

void GameWidget::setupTowerButtons()
{
    for (TowerKind kind : EntityRules::towerBuildOrder()) {
        auto *button = new QPushButton(towerShortName(kind), this);
        button->setFocusPolicy(Qt::NoFocus);
        button->setIconSize(QSize(34, 34));
        button->setMouseTracking(true);
        button->installEventFilter(this);
        connect(button, &QPushButton::clicked, this, [this, kind]() {
            if (m_reverseMode) {
                if (!m_reversePlanning || m_finished) {
                    m_feedback = QStringLiteral("当前波次已经展开，等待战斗结束后继续编排。");
                    m_feedbackTime = 1.6;
                    update();
                    return;
                }
                const auto &towerOrder = EntityRules::towerBuildOrder();
                const auto towerIt = std::find(towerOrder.begin(), towerOrder.end(), kind);
                const int buttonIndex = static_cast<int>(std::distance(towerOrder.begin(), towerIt));
                const EnemyKind enemyKind = reverseEnemyKindForButton(buttonIndex);
                if (!reverseEnemyUnlocked(enemyKind)) {
                    m_feedback = enemyKind == EnemyKind::Boss
                                     ? QStringLiteral("Boss 协议只允许在第 6 波投放。")
                                     : QStringLiteral("%1 尚未授权：继续推进波次后开放。").arg(enemyName(enemyKind));
                    m_feedbackTime = 1.8;
                    SoundManager::play(SoundCue::ErrorDeny);
                    update();
                    return;
                }
                const int cost = reverseEnemyCost(enemyKind);
                if (m_reverseRiftPoints < cost) {
                    m_feedback = QStringLiteral("裂隙点数不足：%1 需要 %2").arg(enemyName(enemyKind)).arg(cost);
                    m_feedbackTime = 1.6;
                    SoundManager::play(SoundCue::ErrorDeny);
                    update();
                    return;
                }
                auto &plan = m_reversePlans[static_cast<size_t>(m_currentWave)];
                int alreadyQueued = 0;
                for (const WaveEntry &entry : plan) {
                    if (entry.kind == enemyKind) {
                        alreadyQueued += entry.count;
                    }
                }
                const int waveLimit = reverseEnemyWaveLimit(enemyKind);
                if (alreadyQueued >= waveLimit) {
                    m_feedback = QStringLiteral("%1 本波上限 %2，换一种异常组合。")
                                     .arg(enemyName(enemyKind))
                                     .arg(waveLimit);
                    m_feedbackTime = 1.8;
                    SoundManager::play(SoundCue::ErrorDeny);
                    update();
                    return;
                }
                const double interval = enemyKind == EnemyKind::Fast ? 0.42
                                      : enemyKind == EnemyKind::Boss ? 0.82
                                      : enemyKind == EnemyKind::Armored ? 0.68
                                      : enemyKind == EnemyKind::Resistant ? 0.56
                                      : enemyKind == EnemyKind::Splitter ? 0.54
                                                                          : 0.50;
                plan.push_back(WaveEntry{enemyKind, 1, interval});
                m_reverseRiftPoints -= cost;
                m_reverseSpentThisWave += cost;
                SoundManager::play(SoundCue::UiClick);
                m_feedback = QStringLiteral("已加入队列：%1  -%2 裂隙点").arg(enemyName(enemyKind)).arg(cost);
                m_feedbackTime = 1.4;
                updateReverseButtonStates();
                update();
                return;
            }
            if (!isTowerUnlocked(kind)) {
                m_feedback = towerUnlockHint(kind);
                m_feedbackTime = 2.4;
                SoundManager::play(SoundCue::ErrorDeny);
                update();
                return;
            }
            if (m_towerDrillActive && kind != m_towerDrillKind) {
                m_feedback = QStringLiteral("授权演练中：请先使用高亮要求的 %1").arg(towerName(m_towerDrillKind));
                m_feedbackTime = 999.0;
                update();
                return;
            }
            if (m_towerDrillActive && kind == m_towerDrillKind && m_towerDrillStage == 1) {
                m_towerDrillStage = 2;
                m_selectedTower = kind;
                m_towerDrillBody = towerDrillRoleText(kind);
                updateTowerButtonStates();
                update();
                return;
            }
            if (m_tutorialStep > 0 && m_tutorialStep != 6 && m_tutorialStep != 8) {
                syncTutorialFeedback();
                update();
                return;
            }
            if (m_tutorialStep == 6 && kind != TowerKind::Resource) {
                syncTutorialFeedback();
                update();
                return;
            }
            if (m_tutorialStep == 8 && kind != TowerKind::Slow) {
                syncTutorialFeedback();
                update();
                return;
            }
            m_selectedTower = kind;
            if (m_tutorialStep == 6 && kind == TowerKind::Resource) {
                m_tutorialStep = 7;
                if (m_tutorialResourceCell.x() >= 0) {
                    m_energy = std::max(m_energy, buildCost(TowerKind::Resource, m_tutorialResourceCell));
                }
                syncTutorialFeedback();
                updateTowerButtonStates();
                update();
                return;
            }
            if (m_tutorialStep == 8 && kind == TowerKind::Slow) {
                m_tutorialStep = 9;
                if (m_tutorialSlowCell.x() >= 0) {
                    m_energy = std::max(m_energy, buildCost(TowerKind::Slow, m_tutorialSlowCell));
                }
                syncTutorialFeedback();
                updateTowerButtonStates();
                update();
                return;
            }
            m_feedback = QStringLiteral("已选择：%1").arg(towerName(kind));
            m_feedbackTime = 1.4;
            updateTowerButtonStates();
            update();
        });
        m_towerButtons.push_back(button);
    }
}

void GameWidget::updateTowerButtonStates()
{
    if (m_reverseMode) {
        updateReverseButtonStates();
        return;
    }
    const auto &order = EntityRules::towerBuildOrder();
    TowerKind tutorialTarget = m_selectedTower;
    bool hasTutorialTarget = false;
    if (m_tutorialStep == 3 || m_tutorialStep == 5) {
        tutorialTarget = TowerKind::Shooter;
        hasTutorialTarget = true;
    } else if (m_tutorialStep == 6 || m_tutorialStep == 7) {
        tutorialTarget = TowerKind::Resource;
        hasTutorialTarget = true;
    } else if (m_tutorialStep == 8 || m_tutorialStep == 9) {
        tutorialTarget = TowerKind::Slow;
        hasTutorialTarget = true;
    }
    const bool guidedTowerChoice = (m_towerDrillActive && m_towerDrillStage == 1)
                                   || m_tutorialStep == 3
                                   || m_tutorialStep == 6
                                   || m_tutorialStep == 8;
    for (size_t i = 0; i < m_towerButtons.size() && i < order.size(); ++i) {
        QPushButton *button = m_towerButtons[i];
        const TowerKind kind = order[i];
        const bool unlocked = isTowerUnlocked(kind);
        const bool drillButton = m_towerDrillActive && m_towerDrillStage == 1 && kind == m_towerDrillKind;
        const bool tutorialButton = hasTutorialTarget && kind == tutorialTarget;
        const bool guidedTarget = drillButton || tutorialButton;
        const int baseCost = m_towerSpecs[static_cast<size_t>(kind)].cost;
        const bool affordable = unlocked && m_energy >= baseCost;
        const bool selected = unlocked && kind == m_selectedTower;
        const int fontSize = std::clamp(static_cast<int>(m_cellSize * 0.17), 12, 15);
        button->setEnabled(unlocked && (!guidedTowerChoice || guidedTarget));
        const int iconIndex = static_cast<int>(kind);
        if (iconIndex >= 0 && iconIndex < static_cast<int>(m_towerIcons.size()) && !m_towerIcons[static_cast<size_t>(iconIndex)].isNull()) {
            button->setIcon(QIcon(m_towerIcons[static_cast<size_t>(iconIndex)]));
        }
        button->setText(unlocked
                            ? QStringLiteral("%1\n费用 %2").arg(towerName(kind)).arg(baseCost)
                            : QStringLiteral("锁定"));
        button->setToolTip(QString());
        const QString baseStyle = QStringLiteral(
            "QPushButton { text-align: left; padding-left: 10px; padding-right: 6px;"
            " font-size: %1px; line-height: 115%; border-radius: 8px; }").arg(fontSize);
        button->setStyleSheet(guidedTowerChoice && !guidedTarget
                                  ? baseStyle + QStringLiteral("QPushButton { background: rgba(4, 12, 24, 138); color: rgba(190, 208, 220, 92); border: 1px solid rgba(80, 212, 255, 42); }")
                                  : (drillButton || tutorialButton)
                                  ? baseStyle + QStringLiteral("QPushButton { background: #614618; color: #fff2bc; border: 2px solid #ffe19a; font-weight: 700; }")
                                  : selected
                                  ? baseStyle + (affordable
                                                     ? QStringLiteral("QPushButton { background: #173754; color: #ffe19a; border: 1px solid #ffe19a; font-weight: 700; }")
                                                     : QStringLiteral("QPushButton { background: rgba(45, 39, 28, 205); color: #d8c690; border: 1px solid rgba(255, 225, 154, 120); font-weight: 700; }"))
                                  : !affordable
                                  ? baseStyle + QStringLiteral("QPushButton { background: rgba(8, 18, 30, 188); color: rgba(190, 208, 220, 150); border: 1px solid rgba(80, 212, 255, 65); }")
                                  : baseStyle + QStringLiteral("QPushButton { background: rgba(12, 32, 52, 210); color: #dff8ff; border: 1px solid rgba(80, 212, 255, 120); }"
                                                               "QPushButton:disabled { color: rgba(190, 208, 220, 125); border-color: rgba(126, 148, 165, 80); background: rgba(8, 18, 30, 165); }"));
    }
}

void GameWidget::updateReverseButtonStates()
{
    const auto &order = reverseEnemyOrder();
    for (size_t i = 0; i < m_towerButtons.size() && i < order.size(); ++i) {
        QPushButton *button = m_towerButtons[i];
        const EnemyKind kind = order[i];
        const int iconIndex = static_cast<int>(kind);
        const int count = (m_reversePlanning && m_currentWave < static_cast<int>(m_reversePlans.size()))
                              ? [&]() {
                                    const auto &plan = m_reversePlans[static_cast<size_t>(m_currentWave)];
                                    int total = 0;
                                    for (const WaveEntry &entry : plan) {
                                        if (entry.kind == kind) {
                                            total += entry.count;
                                        }
                                    }
                                    return total;
                                }()
                              : 0;
        const int cost = reverseEnemyCost(kind);
        const bool progressionLocked = !reverseEnemyUnlocked(kind);
        const int waveLimit = reverseEnemyWaveLimit(kind);
        const bool limitReached = !progressionLocked && count >= waveLimit;
        const bool enabled = m_reversePlanning
                             && !m_finished
                             && !progressionLocked
                             && !limitReached
                             && m_reverseRiftPoints >= cost;
        const int fontSize = std::clamp(static_cast<int>(m_cellSize * 0.17), 12, 15);

        button->setEnabled(enabled || (m_reversePlanning && !m_finished && !progressionLocked && !limitReached));
        if (iconIndex >= 0 && iconIndex < static_cast<int>(m_enemyIcons.size()) && !m_enemyIcons[static_cast<size_t>(iconIndex)].isNull()) {
            button->setIcon(QIcon(m_enemyIcons[static_cast<size_t>(iconIndex)]));
        }
        button->setText(progressionLocked
                            ? QStringLiteral("%1\n第 %2 波开放")
                                  .arg(enemyName(kind))
                                  .arg(kind == EnemyKind::Armored ? 2
                                       : kind == EnemyKind::Resistant ? 3
                                       : kind == EnemyKind::Splitter ? 4
                                                                       : 6)
                            : limitReached
                            ? QStringLiteral("%1\n本波已满").arg(enemyName(kind))
                            : reverseEnemyButtonText(kind, count));
        button->setToolTip(QString());
        const QString baseStyle = QStringLiteral(
            "QPushButton { text-align: left; padding-left: 10px; padding-right: 6px;"
            " font-size: %1px; line-height: 115%; border-radius: 8px; }").arg(fontSize);
        button->setStyleSheet(!m_reversePlanning || progressionLocked || limitReached
                                  ? baseStyle + QStringLiteral("QPushButton { background: rgba(4, 12, 24, 138); color: rgba(190, 208, 220, 92); border: 1px solid rgba(80, 212, 255, 42); }")
                              : enabled
                                  ? baseStyle + QStringLiteral("QPushButton { background: rgba(74, 24, 58, 218); color: #ffd6f3; border: 1px solid rgba(255, 92, 190, 145); font-weight: 700; }")
                                  : baseStyle + QStringLiteral("QPushButton { background: rgba(8, 18, 30, 188); color: rgba(190, 208, 220, 150); border: 1px solid rgba(80, 212, 255, 65); }"));
    }
}

bool GameWidget::eventFilter(QObject *watched, QEvent *event)
{
    const auto &order = EntityRules::towerBuildOrder();
    for (size_t i = 0; i < m_towerButtons.size() && i < order.size(); ++i) {
        if (watched != m_towerButtons[i]) {
            continue;
        }
        if (event->type() == QEvent::Enter) {
            m_hoveredTowerButtonIndex = static_cast<int>(i);
            update();
        } else if (event->type() == QEvent::Leave) {
            if (m_hoveredTowerButtonIndex == static_cast<int>(i)) {
                m_hoveredTowerButtonIndex = -1;
                update();
            }
        } else if (event->type() == QEvent::ToolTip) {
            return true;
        }
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void GameWidget::syncTutorialFeedback()
{
    if (m_tutorialStep <= 0) {
        if (m_skipTutorialButton) {
            m_skipTutorialButton->setVisible(false);
        }
        return;
    }
    m_feedback = tutorialFeedback(m_tutorialStep);
    m_feedbackTime = 999.0;
    if (m_skipTutorialButton) {
        m_skipTutorialButton->setVisible(true);
    }
    updateControlButtonGeometry();
    updateTowerButtonStates();
}

void GameWidget::skipTutorial()
{
    if (m_tutorialStep <= 0 || m_finished) {
        return;
    }
    SoundManager::play(SoundCue::UiClick);
    m_tutorialStep = 0;
    m_tutorialAnimTime = 0.0;
    m_deploymentIntroTimer = 3.0;
    m_feedback = QStringLiteral("教学已跳过：3 秒后开始第一波异常协议");
    m_feedbackTime = 2.6;
    ProgressManager::markTutorialFlag(QStringLiteral("towerIntroductions"), static_cast<int>(TowerKind::Shooter));
    ProgressManager::markTutorialFlag(QStringLiteral("towerIntroductions"), static_cast<int>(TowerKind::Slow));
    ProgressManager::markTutorialFlag(QStringLiteral("towerIntroductions"), static_cast<int>(TowerKind::Resource));
    if (m_skipTutorialButton) {
        m_skipTutorialButton->setVisible(false);
    }
    updateControlButtonGeometry();
    updateTowerButtonStates();
    update();
}

void GameWidget::queuePrompt(const QString &title,
                             const QString &body,
                             const QColor &accent,
                             double duration,
                             PromptLane lane)
{
    if (blocksTransientPrompt() && lane != PromptLaneBoss) {
        return;
    }

    PromptMessage message;
    message.title = title;
    message.body = body;
    message.accent = accent;
    message.duration = std::max(1.0, duration);
    message.lane = lane;

    if (m_promptTime <= 0.0 && m_promptTitle.isEmpty()) {
        m_promptTitle = message.title;
        m_promptBody = message.body;
        m_promptAccent = message.accent;
        m_promptDuration = message.duration;
        m_promptTime = message.duration;
        m_promptLane = message.lane;
        return;
    }

    if (m_promptQueue.size() >= 3) {
        m_promptQueue.erase(m_promptQueue.begin());
    }
    m_promptQueue.push_back(message);
}

void GameWidget::clearPrompts()
{
    m_promptQueue.clear();
    m_promptTitle.clear();
    m_promptBody.clear();
    m_promptTime = 0.0;
    m_promptDuration = 0.0;
    m_promptLane = PromptLaneBottom;
}

void GameWidget::updatePrompt(double dt)
{
    if (blocksTransientPrompt()) {
        return;
    }
    if (m_promptTime > 0.0) {
        m_promptTime = std::max(0.0, m_promptTime - dt);
    }
    if (m_promptTime > 0.0 || m_promptQueue.empty()) {
        return;
    }

    const PromptMessage message = m_promptQueue.front();
    m_promptQueue.erase(m_promptQueue.begin());
    m_promptTitle = message.title;
    m_promptBody = message.body;
    m_promptAccent = message.accent;
    m_promptDuration = message.duration;
    m_promptTime = message.duration;
    m_promptLane = message.lane;
}

bool GameWidget::blocksTransientPrompt() const
{
    return m_paused
           || m_infoCardActive
           || m_enemyGuideActive
           || m_towerDrillActive
           || m_tutorialStep > 0
           || m_deploymentIntroTimer > 0.0
           || m_bossAlertTimer > 0.0;
}

void GameWidget::queueFirstRunIntroductions()
{
    if (m_reverseMode
        || !ProgressManager::tutorialHintsEnabled()
        || !m_customLevelPath.isEmpty()
        || m_infoCardActive
        || m_tutorialStep > 0
        || m_finished
        || m_towerDrillActive) {
        return;
    }
    if (startTowerDrillIfNeeded()) {
        return;
    }
}

bool GameWidget::startTowerDrillIfNeeded()
{
    for (TowerKind kind : EntityRules::towerBuildOrder()) {
        const int index = static_cast<int>(kind);
        if (!isTowerUnlocked(kind)
            || towerUnlockLevel(kind) != m_levelIndex
            || m_towerIntroShownThisRun[static_cast<size_t>(index)]
            || ProgressManager::tutorialFlag(QStringLiteral("towerIntroductions"), index)) {
            continue;
        }
        m_towerIntroShownThisRun[static_cast<size_t>(index)] = true;
        beginTowerDrill(kind);
        return true;
    }
    return false;
}

void GameWidget::beginTowerDrill(TowerKind kind)
{
    m_towerDrillKind = kind;
    m_towerDrillCell = recommendedDrillCell(kind);
    if (m_towerDrillCell.x() < 0) {
        ProgressManager::markTutorialFlag(QStringLiteral("towerIntroductions"), static_cast<int>(kind));
        showIntelBanner(towerIntroTitle(kind),
                        towerIntroText(kind) + QStringLiteral("\n\n当前地图没有合适演练格，装置资料已写入图鉴。"),
                        towerColor(kind));
        return;
    }
    m_towerDrillActive = true;
    m_towerDrillStage = 1;
    m_energy = std::max(m_energy, buildCost(kind, m_towerDrillCell));
    m_deploymentIntroTimer = 0.0;
    m_feedback.clear();
    m_feedbackTime = 0.0;
    m_towerDrillTitle = QStringLiteral("装置教学：%1").arg(towerName(kind));
    m_towerDrillBody = towerDrillRoleText(kind);
    updateTowerButtonStates();
}

void GameWidget::completeTowerDrill()
{
    if (!m_towerDrillActive) {
        return;
    }
    ProgressManager::markTutorialFlag(QStringLiteral("towerIntroductions"), static_cast<int>(m_towerDrillKind));
    SoundManager::play(SoundCue::CodexUnlock);
    m_towerDrillActive = false;
    m_towerDrillStage = 0;
    m_visualPulses.push_back(VisualPulse{cellCenter(m_towerDrillCell), towerColor(m_towerDrillKind), 0.0, 0.85, scaledDistance(96.0)});
    m_deploymentIntroTimer = 3.0;
    m_towerDrillCell = QPoint(-1, -1);
    updateTowerButtonStates();
}

void GameWidget::closeEnemyGuide()
{
    if (!m_enemyGuideActive) {
        return;
    }
    const bool recognized = m_enemyGuideStage >= 2;
    m_enemyGuideActive = false;
    m_enemyGuideId = -1;
    m_enemyGuideStage = 1;
    m_enemyGuideTitle.clear();
    m_enemyGuideBody.clear();
    SoundManager::play(recognized ? SoundCue::CodexUnlock : SoundCue::UiClick);
    update();
}

QPoint GameWidget::recommendedDrillCell(TowerKind kind) const
{
    if (kind == TowerKind::Wall) {
        for (int i = 1; i + 1 < static_cast<int>(m_pathCells.size()); ++i) {
            const QPoint cell = m_pathCells[static_cast<size_t>(i)];
            if (canBuildAt(cell, kind)) {
                return cell;
            }
        }
        return QPoint(-1, -1);
    }

    const std::array<QPoint, 8> offsets = {
        QPoint(0, -1), QPoint(0, 1), QPoint(1, 0), QPoint(-1, 0),
        QPoint(1, -1), QPoint(-1, -1), QPoint(1, 1), QPoint(-1, 1)
    };
    const int start = std::max(0, static_cast<int>(m_pathCells.size()) / 3 - 1);
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = start; i < static_cast<int>(m_pathCells.size()); ++i) {
            const QPoint path = m_pathCells[static_cast<size_t>(i)];
            for (const QPoint &offset : offsets) {
                const QPoint cell = path + offset;
                if (m_levelIndex == 0
                    && ((kind == TowerKind::Resource && cell == m_tutorialCell)
                        || (kind == TowerKind::Slow && (cell == m_tutorialCell || cell == m_tutorialResourceCell)))) {
                    continue;
                }
                if (canBuildAt(cell, kind)) {
                    return cell;
                }
            }
        }
        for (int i = 0; i < start; ++i) {
            const QPoint path = m_pathCells[static_cast<size_t>(i)];
            for (const QPoint &offset : offsets) {
                const QPoint cell = path + offset;
                if (m_levelIndex == 0
                    && ((kind == TowerKind::Resource && cell == m_tutorialCell)
                        || (kind == TowerKind::Slow && (cell == m_tutorialCell || cell == m_tutorialResourceCell)))) {
                    continue;
                }
                if (canBuildAt(cell, kind)) {
                    return cell;
                }
            }
        }
    }
    return QPoint(-1, -1);
}

void GameWidget::loadVisualAssets()
{
    m_backgroundImage.load(assetPath(QStringLiteral("bg-main.png")));
    const std::array<QString, 6> towerFiles = {
        QStringLiteral("tower-shooter.png"),
        QStringLiteral("tower-slow.png"),
        QStringLiteral("tower-splash.png"),
        QStringLiteral("tower-laser.png"),
        QStringLiteral("tower-resource.png"),
        QStringLiteral("tower-wall.png")
    };
    const std::array<QString, 6> enemyFiles = {
        QStringLiteral("enemy-normal.png"),
        QStringLiteral("enemy-fast.png"),
        QStringLiteral("enemy-armored.png"),
        QStringLiteral("enemy-resistant.png"),
        QStringLiteral("enemy-splitter.png"),
        QStringLiteral("enemy-boss.png")
    };
    const std::array<QString, 6> enemyV2Files = {
        QStringLiteral("enemy-normal-v2.png"),
        QStringLiteral("enemy-fast-v2.png"),
        QStringLiteral("enemy-armored-v2.png"),
        QStringLiteral("enemy-resistant-v2.png"),
        QStringLiteral("enemy-splitter-v2.png"),
        QStringLiteral("enemy-boss-v2.png")
    };
    const std::array<QString, 6> tileFiles = {
        QStringLiteral("tile-stable.png"),
        QStringLiteral("tile-path.png"),
        QStringLiteral("tile-discount.png"),
        QStringLiteral("tile-anchor.png"),
        QStringLiteral("tile-accelerate.png"),
        QStringLiteral("tile-portal.png")
    };
    for (size_t i = 0; i < towerFiles.size(); ++i) {
        m_towerIcons[i].load(assetPath(towerFiles[i]));
        if (!m_enemyIcons[i].load(assetPath(enemyV2Files[i]))) {
            m_enemyIcons[i].load(assetPath(enemyFiles[i]));
        }
        m_tileIcons[i].load(assetPath(tileFiles[i]));
    }
    rebuildScaledVisualCache();
}

void GameWidget::resetGame()
{
    const LevelData level = LevelManager::loadLevel(m_reverseMode ? reverseScenarioLevelIndex() : m_levelIndex, m_customLevelPath, m_rows, m_cols);
    m_tiles = level.tiles;
    m_pathCells = level.pathCells;
    m_portalFrom = level.portalFrom;
    m_portalTo = level.portalTo;
    m_levelName = m_reverseMode ? reverseScenarioName() : level.name;
    m_initialEnergy = m_reverseMode ? reverseInitialDefenseEnergy() : level.initialEnergy;
    m_waves = m_reverseMode ? std::vector<std::vector<WaveEntry>>(6) : level.waves;
    rebuildPathMetrics();

    m_enemies.clear();
    m_pendingEnemies.clear();
    m_towers.clear();
    m_projectiles.clear();
    m_nextEnemyId = 1;
    m_towersBuilt = 0;
    m_enemiesDefeated = 0;
    m_currentWave = 0;
    m_currentEntry = 0;
    m_entrySpawned = 0;
    m_spawnTimer = 0.0;
    m_waitingNextWave = false;
    m_nextWaveTimer = 0.0;
    m_reversePlans.clear();
    m_reversePlanning = false;
    m_reverseRiftPoints = 0;
    m_reverseSpentThisWave = 0;
    m_tutorialStep = (!m_reverseMode
                      && m_levelIndex == 0
                      && m_customLevelPath.isEmpty()
                      && ProgressManager::tutorialHintsEnabled()) ? 1 : 0;
    m_tutorialCell = QPoint(2, 2);
    m_tutorialResourceCell = recommendedDrillCell(TowerKind::Resource);
    m_tutorialSlowCell = recommendedDrillCell(TowerKind::Slow);
    m_tutorialAnimTime = 0.0;
    m_towerDrillActive = false;
    m_towerDrillStage = 0;
    m_towerDrillCell = QPoint(-1, -1);
    m_towerDrillTitle.clear();
    m_towerDrillBody.clear();
    m_enemyGuideActive = false;
    m_enemyGuideId = -1;
    m_enemyGuideStage = 1;
    m_enemyGuideTitle.clear();
    m_enemyGuideBody.clear();
    m_deploymentIntroTimer = (m_reverseMode || m_tutorialStep > 0) ? 0.0 : 3.0;
    m_freezePulseTimer = 0.0;
    m_bossAlertTimer = 0.0;
    m_waveBannerTimer = 0.0;
    m_waveBannerText.clear();
    m_enemyIntelShown.fill(false);
    m_towerIntroShownThisRun.fill(false);
    m_intelBannerTimer = 0.0;
    m_intelBannerTitle.clear();
    m_intelBannerBody.clear();
    clearPrompts();
    m_infoCardActive = false;
    m_infoCardTitle.clear();
    m_infoCardBody.clear();
    m_visualPulses.clear();
    m_energy = m_initialEnergy;
    m_energyTick = 0.0;
    m_elapsedSeconds = 0.0;
    m_freezeSkillCooldown = 0.0;
    closeCommandInput();
    m_paused = false;
    m_finished = false;
    m_selectedTower = TowerKind::Shooter;
    m_feedback = m_tutorialStep > 0 ? tutorialFeedback(m_tutorialStep) : QString();
    m_feedbackTime = m_tutorialStep > 0 ? 999.0 : 0.0;
    m_summary = GameSummary{m_levelName, m_levelIndex, m_reverseMode, false, m_energy, 0, 0, 0, static_cast<int>(m_waves.size()), 0.0};
    m_pauseButton->setText(QStringLiteral("暂停"));
    if (m_reverseMode) {
        configureReverseMode();
    }
    updateControlButtonGeometry();
    updateTowerButtonStates();
    if (m_reverseMode) {
        // Reverse mode owns its opening prompt inside configureReverseMode().
    } else if (m_tutorialStep <= 0) {
        if (m_skipTutorialButton) {
            m_skipTutorialButton->setVisible(false);
        }
        queueFirstRunIntroductions();
    } else {
        syncTutorialFeedback();
    }
    m_clock.restart();
    update();
}

void GameWidget::setLevelIndex(int index)
{
    m_reverseMode = false;
    m_levelIndex = std::clamp(index, 0, LevelManager::builtInLevelCount() - 1);
    m_customLevelPath.clear();
    resetGame();
}

void GameWidget::startReverseMode(int scenarioIndex)
{
    m_reverseMode = true;
    m_reverseScenarioIndex = reverseScenarioClamp(scenarioIndex);
    m_levelIndex = reverseScenarioLevelIndex();
    m_customLevelPath.clear();
    resetGame();
}

void GameWidget::loadCustomLevel(const QString &path)
{
    m_reverseMode = false;
    m_customLevelPath = path;
    m_levelIndex = LevelManager::builtInLevelCount();
    resetGame();
}

void GameWidget::setRunning(bool running)
{
    m_running = running;
    if (m_running) {
        m_clock.restart();
    } else {
        closeCommandInput();
    }
}

void GameWidget::saveProgress(bool cleared)
{
    if (!m_customLevelPath.isEmpty()) {
        return;
    }
    ProgressManager::saveLevelResult(m_levelIndex,
                                     m_levelName,
                                     cleared,
                                     m_energy,
                                     m_elapsedSeconds,
                                     std::min(m_currentWave, static_cast<int>(m_waves.size())));
}

void GameWidget::updateControlButtonGeometry()
{
    const int buttonHeight = std::clamp(static_cast<int>(height() * 0.045), 32, 44);
    if (m_paused) {
        const int buttonW = 212;
        const int buttonH = 42;
        const int gap = 20;
        const int x = width() / 2 - buttonW / 2;
        const int y0 = height() / 2 - (buttonH * 4 + gap * 3) / 2 + 54;
        m_pauseButton->setGeometry(x, y0, buttonW, buttonH);
        m_restartButton->setGeometry(x, y0 + (buttonH + gap), buttonW, buttonH);
        m_settingsButton->setGeometry(x, y0 + (buttonH + gap) * 2, buttonW, buttonH);
        m_menuButton->setGeometry(x, y0 + (buttonH + gap) * 3, buttonW, buttonH);
        m_settingsButton->setVisible(true);
        m_skipTutorialButton->setVisible(false);
        m_reverseLaunchButton->setVisible(false);
        return;
    }

    const int topButtonY = 30;
    m_menuButton->setGeometry(width() - 98, topButtonY, 74, buttonHeight);
    m_restartButton->setGeometry(width() - 202, topButtonY, 94, buttonHeight);
    m_pauseButton->setGeometry(width() - 288, topButtonY, 76, buttonHeight);
    m_settingsButton->setVisible(false);

    const int tutorialPanelWidth = std::min(width() - 52, 840);
    const int skipW = 104;
    const int skipH = std::clamp(static_cast<int>(height() * 0.04), 30, 38);
    m_skipTutorialButton->setGeometry(width() / 2 + tutorialPanelWidth / 2 - skipW - 22,
                                      height() - 218,
                                      skipW,
                                      skipH);
    m_skipTutorialButton->setVisible(m_tutorialStep > 0 && !m_finished);

    const int launchW = std::clamp(static_cast<int>(width() * 0.18), 178, 230);
    const int launchH = std::clamp(static_cast<int>(height() * 0.045), 34, 42);
    if (m_reverseMode) {
        const QRectF queuePanel = reverseQueuePanelRect();
        m_reverseLaunchButton->setGeometry(static_cast<int>(queuePanel.right() - launchW - 20),
                                           static_cast<int>(queuePanel.bottom() - launchH - 18),
                                           launchW,
                                           launchH);
    } else {
        m_reverseLaunchButton->setGeometry(width() / 2 - launchW / 2,
                                           height() - launchH - 28,
                                           launchW,
                                           launchH);
    }
    m_reverseLaunchButton->setVisible(m_reverseMode && m_reversePlanning && !m_finished);
}

void GameWidget::openCommandInput()
{
    if (m_finished) {
        return;
    }
    m_commandInput->clear();
    m_commandInput->setVisible(true);
    m_commandInput->setFocus();
    m_feedback.clear();
    m_feedbackTime = 0.0;
    update();
}

void GameWidget::closeCommandInput()
{
    if (!m_commandInput) {
        return;
    }
    m_commandInput->clear();
    m_commandInput->setVisible(false);
    setFocus();
    update();
}

void GameWidget::executeCommandInput()
{
    const QString command = m_commandInput->text().trimmed().toLower();
    closeCommandInput();
    executeCheatCommand(command);
}

void GameWidget::executeCheatCommand(const QString &command)
{
    if (command.isEmpty() || command == QStringLiteral("cancel") || m_finished) {
        return;
    }

    if (command == QStringLiteral("energy")) {
        m_energy += 1000;
        m_feedback = QStringLiteral("指令生效：时能 +1000");
        m_feedbackTime = 2.0;
    } else if (command == QStringLiteral("clear")) {
        m_enemies.clear();
        m_pendingEnemies.clear();
        m_projectiles.clear();
        m_feedback = QStringLiteral("指令生效：清除当前异常体");
        m_feedbackTime = 2.0;
    } else if (command == QStringLiteral("next")) {
        if (m_currentWave < static_cast<int>(m_waves.size()) - 1) {
            ++m_currentWave;
            m_currentEntry = 0;
            m_entrySpawned = 0;
            m_spawnTimer = 0.0;
            m_waitingNextWave = false;
        }
        m_feedback = QStringLiteral("指令生效：进入下一波");
        m_feedbackTime = 2.0;
    } else if (command == QStringLiteral("win")) {
        m_currentWave = static_cast<int>(m_waves.size());
        m_enemies.clear();
        m_pendingEnemies.clear();
        m_projectiles.clear();
        checkEndConditions();
    } else {
        m_feedback = QStringLiteral("未知命令：%1").arg(command);
        m_feedbackTime = 2.0;
    }
}

void GameWidget::loadTowerConfig()
{
    m_towerSpecs = LevelManager::loadTowerSpecs();
}

void GameWidget::paintEvent(QPaintEvent *)
{
    if (m_cachedCellSize != m_cellSize || m_cachedBackgroundSize != size()) {
        rebuildScaledVisualCache();
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    drawBackground(painter);
    drawHud(painter);
    drawMap(painter);
    drawTowers(painter);
    drawEnemies(painter);
    drawProjectiles(painter);
    drawCinematicEffects(painter);
    drawReversePlanningOverlay(painter);
    drawIntelBanner(painter);
    drawTutorialOverlay(painter);
    drawTowerDrillOverlay(painter);
    drawEnemyGuideOverlay(painter);
    drawInfoCard(painter);
    drawMessage(painter);
    drawTowerButtonHoverCard(painter);
}

void GameWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_infoCardActive) {
        closeInfoCard();
        return;
    }
    if (m_enemyGuideActive) {
        const Enemy *enemy = enemyById(m_enemyGuideId);
        QPointF target = m_pathCells.empty() ? QPointF(width() / 2.0, height() / 2.0) : cellCenter(m_pathCells.front());
        if (enemy) {
            target = pathPosition(enemy->progress);
        }
        if (m_enemyGuideStage == 1) {
            if (distance(event->pos(), target) <= scaledDistance(58.0)) {
                m_enemyGuideStage = 2;
                m_enemyGuideBody = enemyIntelText(m_enemyGuideKind);
            } else {
                m_enemyGuideBody = QStringLiteral("请点击高亮异常体完成识别。");
            }
            update();
            return;
        }
        closeEnemyGuide();
        return;
    }
    if (m_finished) {
        return;
    }

    if (m_reverseMode) {
        if (m_reversePlanning && event->button() == Qt::LeftButton) {
            const int queueIndex = reverseQueueIndexAt(event->pos(), false);
            if (queueIndex >= 0) {
                m_reverseDragIndex = queueIndex;
                m_reverseDragHoverIndex = queueIndex;
                m_feedback = QStringLiteral("拖动队列图标可调整出场顺序。");
                m_feedbackTime = 1.4;
                update();
                return;
            }
        }
        if (m_reversePlanning && event->button() == Qt::RightButton && removeReverseQueuedEnemyAt(event->pos())) {
            update();
            return;
        }
        if (!towerPanelRect().contains(event->pos()) && event->button() == Qt::LeftButton) {
            m_feedback = m_reversePlanning
                             ? QStringLiteral("点击右侧异常加入队列；右键底部队列图标可撤回。")
                             : QStringLiteral("本波已展开，观察系统防守与异常突破。");
            m_feedbackTime = 1.8;
            update();
        }
        return;
    }

    if (m_deploymentIntroTimer > 0.0 && !m_towerDrillActive) {
        m_feedback = QStringLiteral("任务投影同步中，倒计时结束后开始部署");
        m_feedbackTime = 1.4;
        update();
        return;
    }

    const QPoint cell = cellAt(event->pos());
    if (cell.x() < 0 || towerPanelRect().contains(event->pos())) {
        return;
    }

    if (m_tutorialStep > 0 && m_tutorialStep != 3 && m_tutorialStep != 5 && m_tutorialStep != 7 && m_tutorialStep != 9) {
        syncTutorialFeedback();
        update();
        return;
    }

    if (m_tutorialStep == 3 && (event->button() != Qt::LeftButton || cell != m_tutorialCell)) {
        syncTutorialFeedback();
        update();
        return;
    }
    if (m_tutorialStep == 3) {
        m_selectedTower = TowerKind::Shooter;
    }

    if (m_tutorialStep == 7 && (event->button() != Qt::LeftButton || cell != m_tutorialResourceCell)) {
        syncTutorialFeedback();
        update();
        return;
    }
    if (m_tutorialStep == 7) {
        m_selectedTower = TowerKind::Resource;
    }

    if (m_tutorialStep == 9 && (event->button() != Qt::LeftButton || cell != m_tutorialSlowCell)) {
        syncTutorialFeedback();
        update();
        return;
    }
    if (m_tutorialStep == 9) {
        m_selectedTower = TowerKind::Slow;
    }

    if (m_towerDrillActive && m_towerDrillStage == 1) {
        m_feedback = QStringLiteral("授权演练：请先在右侧装置库选择 %1。").arg(towerName(m_towerDrillKind));
        m_feedbackTime = 999.0;
        update();
        return;
    }
    if (m_towerDrillActive && (event->button() != Qt::LeftButton || cell != m_towerDrillCell)) {
        m_feedback = QStringLiteral("授权演练：请把 %1 部署到高亮格。").arg(towerName(m_towerDrillKind));
        m_feedbackTime = 999.0;
        update();
        return;
    }
    if (m_towerDrillActive) {
        m_selectedTower = m_towerDrillKind;
    }

    if (m_tutorialStep == 5 && event->button() != Qt::RightButton) {
        syncTutorialFeedback();
        update();
        return;
    }

    if (event->button() == Qt::RightButton) {
        const int index = towerIndexAt(cell);
        if (m_tutorialStep == 5 && (cell != m_tutorialCell || index < 0)) {
            syncTutorialFeedback();
            update();
            return;
        }
        if (index >= 0) {
            upgradeTower(index);
            if (m_tutorialStep == 5) {
                m_tutorialStep = 6;
                syncTutorialFeedback();
            }
        }
        return;
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (!isTowerUnlocked(m_selectedTower)) {
        m_feedback = towerUnlockHint(m_selectedTower);
        m_feedbackTime = 2.4;
        SoundManager::play(SoundCue::ErrorDeny);
        update();
        return;
    }

    if (!canBuildAt(cell, m_selectedTower)) {
        m_feedback = QStringLiteral("该格不能部署 %1").arg(towerName(m_selectedTower));
        m_feedbackTime = 1.6;
        SoundManager::play(SoundCue::ErrorDeny);
        update();
        return;
    }

    const int cost = buildCost(m_selectedTower, cell);
    if (m_energy < cost) {
        m_feedback = QStringLiteral("时能不足：%1 需要 %2").arg(towerName(m_selectedTower)).arg(cost);
        m_feedbackTime = 1.6;
        SoundManager::play(SoundCue::ErrorDeny);
        update();
        return;
    }

    m_towers.push_back(makeTower(m_selectedTower, cell));
    m_visualPulses.push_back(VisualPulse{cellCenter(cell), QColor(255, 225, 154), 0.0, 0.65, scaledDistance(76.0)});
    ++m_towersBuilt;
    m_energy -= cost;
    SoundManager::play(SoundCue::Build);
    m_feedback = QStringLiteral("%1 校准完成，消耗 %2 时能").arg(towerName(m_selectedTower)).arg(cost);
    if (m_tutorialStep == 3) {
        m_tutorialStep = 4;
        syncTutorialFeedback();
    } else if (m_tutorialStep == 7) {
        m_tutorialStep = 8;
        syncTutorialFeedback();
    } else if (m_tutorialStep == 9) {
        m_tutorialStep = 10;
        syncTutorialFeedback();
    } else {
        if (m_towerDrillActive && m_selectedTower == m_towerDrillKind && cell == m_towerDrillCell) {
            completeTowerDrill();
            update();
            return;
        }
        m_feedbackTime = 1.4;
    }
    update();
}

void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_reverseMode && m_reversePlanning && m_reverseDragIndex >= 0) {
        const int hoverIndex = reverseQueueIndexAt(event->pos(), true);
        if (hoverIndex >= 0 && hoverIndex != m_reverseDragHoverIndex) {
            m_reverseDragHoverIndex = hoverIndex;
            update();
        }
        return;
    }

    const QPoint cell = cellAt(event->pos());
    const QPoint nextHover = towerPanelRect().contains(event->pos()) ? QPoint(-1, -1) : cell;
    if (nextHover != m_hoverCell) {
        m_hoverCell = nextHover;
        update();
    }
}

void GameWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_reverseMode && m_reversePlanning && event->button() == Qt::LeftButton && m_reverseDragIndex >= 0) {
        int target = reverseQueueIndexAt(event->pos(), true);
        if (target < 0) {
            target = m_reverseDragHoverIndex;
        }
        if (m_currentWave < static_cast<int>(m_reversePlans.size())) {
            auto &plan = m_reversePlans[static_cast<size_t>(m_currentWave)];
            if (m_reverseDragIndex >= 0 && m_reverseDragIndex < static_cast<int>(plan.size())) {
                target = std::clamp(target, 0, static_cast<int>(plan.size()) - 1);
                if (target != m_reverseDragIndex) {
                    WaveEntry moved = plan[static_cast<size_t>(m_reverseDragIndex)];
                    plan.erase(plan.begin() + m_reverseDragIndex);
                    target = std::clamp(target, 0, static_cast<int>(plan.size()));
                    plan.insert(plan.begin() + target, moved);
                    m_feedback = QStringLiteral("队列顺序已调整。");
                    m_feedbackTime = 1.2;
                    SoundManager::play(SoundCue::UiClick);
                }
            }
        }
        m_reverseDragIndex = -1;
        m_reverseDragHoverIndex = -1;
        update();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void GameWidget::leaveEvent(QEvent *)
{
    if (m_reverseDragIndex >= 0) {
        m_reverseDragIndex = -1;
        m_reverseDragHoverIndex = -1;
    }
    if (m_hoverCell.x() >= 0 || m_hoveredTowerButtonIndex >= 0) {
        m_hoverCell = QPoint(-1, -1);
        m_hoveredTowerButtonIndex = -1;
        update();
    }
}

void GameWidget::resizeEvent(QResizeEvent *)
{
    const double oldLength = pathLength();
    const int panelWidth = std::clamp(static_cast<int>(width() * 0.135), 184, 220);
    const int topReserve = std::clamp(static_cast<int>(height() * 0.15), 136, 178);
    const int bottomReserve = std::clamp(static_cast<int>(height() * 0.08), 48, 90);
    const int usableWidth = width() - panelWidth - 90;
    const int usableHeight = height() - topReserve - bottomReserve;
    m_cellSize = std::max(42, std::min(usableWidth / m_cols, usableHeight / m_rows));
    const int gridWidth = m_cellSize * m_cols;
    const int gridHeight = m_cellSize * m_rows;
    const int leftArea = width() - panelWidth - 40;
    m_origin = QPoint(std::max(34, (leftArea - gridWidth) / 2),
                      topReserve + std::max(0, height() - topReserve - bottomReserve - gridHeight) / 2);
    rebuildPathMetrics(oldLength);

    updateControlButtonGeometry();
    const int commandPanelWidth = std::min(width() - 96, 760);
    m_commandInput->setGeometry(width() / 2 - commandPanelWidth / 2 + 24,
                                height() - 104,
                                commandPanelWidth - 48,
                                38);

    const QRect panel = towerPanelRect();
    const int towerButtonHeight = std::clamp(static_cast<int>(m_cellSize * 0.58), 48, 56);
    const int towerButtonGap = std::clamp(static_cast<int>(m_cellSize * 0.10), 8, 11);
    int y = panel.y() + std::clamp(static_cast<int>(m_cellSize * 0.72), 68, 78);
    for (auto *button : m_towerButtons) {
        button->setGeometry(panel.x() + 18, y, panel.width() - 36, towerButtonHeight);
        const int iconSide = std::clamp(towerButtonHeight - 16, 30, 42);
        button->setIconSize(QSize(iconSide, iconSide));
        y += towerButtonHeight + towerButtonGap;
    }
    rebuildScaledVisualCache();
}

void GameWidget::rebuildScaledVisualCache()
{
    m_cachedCellSize = m_cellSize;
    m_cachedBackgroundSize = size();

    if (width() > 0 && height() > 0) {
        m_cachedBackground = QPixmap(size());
        m_cachedBackground.fill(Qt::transparent);
        QPainter cachePainter(&m_cachedBackground);
        cachePainter.setRenderHint(QPainter::Antialiasing, true);
        renderBackground(cachePainter);
    } else {
        m_cachedBackground = QPixmap();
    }

    const int enemySide = std::max(1, static_cast<int>(std::round(std::clamp(m_cellSize * 0.82, 52.0, 132.0))));
    const int bossSide = std::max(1, static_cast<int>(std::round(std::clamp(m_cellSize * 1.16, 74.0, 178.0))));
    const int tileSide = std::max(1, static_cast<int>(std::round(m_cellSize * 1.18)));
    for (size_t i = 0; i < m_towerIcons.size(); ++i) {
        const TowerKind kind = static_cast<TowerKind>(i);
        const int towerSide = std::max(1, static_cast<int>(std::round(m_cellSize * (kind == TowerKind::Wall ? 1.02 : 0.95))));
        m_scaledTowerIcons[i] = m_towerIcons[i].isNull()
                                    ? QPixmap()
                                    : m_towerIcons[i].scaled(towerSide,
                                                            towerSide,
                                                            Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation);
        m_scaledEnemyIcons[i] = m_enemyIcons[i].isNull()
                                    ? QPixmap()
                                    : m_enemyIcons[i].scaled(enemySide,
                                                            enemySide,
                                                            Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation);
        m_scaledBossEnemyIcons[i] = m_enemyIcons[i].isNull()
                                        ? QPixmap()
                                        : m_enemyIcons[i].scaled(bossSide,
                                                                bossSide,
                                                                Qt::KeepAspectRatio,
                                                                Qt::SmoothTransformation);
        m_scaledTileIcons[i] = m_tileIcons[i].isNull()
                                   ? QPixmap()
                                   : m_tileIcons[i].scaled(tileSide,
                                                           tileSide,
                                                           Qt::KeepAspectRatio,
                                                           Qt::SmoothTransformation);
    }
}

void GameWidget::rebuildPathMetrics(double oldLength)
{
    const double oldTotal = oldLength > 0.0 ? oldLength : pathLength();
    m_pathSegmentLengths.clear();
    for (size_t i = 1; i < m_pathCells.size(); ++i) {
        m_pathSegmentLengths.push_back(distance(cellCenter(m_pathCells[i - 1]), cellCenter(m_pathCells[i])));
    }

    const double newTotal = pathLength();
    if (oldTotal > 0.0 && newTotal > 0.0 && std::abs(oldTotal - newTotal) > 0.001) {
        const double ratio = newTotal / oldTotal;
        for (auto &enemy : m_enemies) {
            enemy->progress *= ratio;
        }
    }
}

double GameWidget::mapScale() const
{
    return std::max(0.1, m_cellSize / kDesignCellSize);
}

double GameWidget::scaledDistance(double designPixels) const
{
    return designPixels * mapScale();
}

double GameWidget::scaledTowerRange(const Tower &tower) const
{
    return scaledDistance(tower.range);
}

void GameWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_commandInput->isVisible()) {
        if (event->key() == Qt::Key_Escape) {
            closeCommandInput();
        } else {
            QWidget::keyPressEvent(event);
        }
        return;
    }

    if (m_infoCardActive) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
            || event->key() == Qt::Key_Escape) {
            closeInfoCard();
            return;
        }
        return;
    }
    if (m_enemyGuideActive) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
            || event->key() == Qt::Key_Escape) {
            if (m_enemyGuideStage == 1) {
                m_enemyGuideBody = QStringLiteral("请先点击高亮异常体完成识别。");
                update();
            } else {
                closeEnemyGuide();
            }
        }
        return;
    }

    if (event->key() == Qt::Key_Slash || event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (m_tutorialStep == 1 || m_tutorialStep == 2 || m_tutorialStep == 4) {
            ++m_tutorialStep;
            syncTutorialFeedback();
            update();
            return;
        }
        openCommandInput();
    } else if (event->key() == Qt::Key_Space) {
        if (m_reverseMode && m_reversePlanning) {
            startReverseWave();
            return;
        }
        if (m_tutorialStep == 1 || m_tutorialStep == 2 || m_tutorialStep == 4) {
            ++m_tutorialStep;
            syncTutorialFeedback();
            update();
            return;
        }
        m_paused = !m_paused;
        m_pauseButton->setText(m_paused ? QStringLiteral("继续") : QStringLiteral("暂停"));
        updateControlButtonGeometry();
        update();
    } else if (event->key() == Qt::Key_R && event->modifiers().testFlag(Qt::ControlModifier)) {
        resetGame();
    } else if (event->key() == Qt::Key_F) {
        if (m_reverseMode) {
            m_feedback = QStringLiteral("逆向推演中冻结由系统掌握，你负责投放异常。");
            m_feedbackTime = 1.6;
            SoundManager::play(SoundCue::ErrorDeny);
            update();
            return;
        }
        if (m_tutorialStep == 10) {
            m_tutorialStep = 0;
            m_freezePulseTimer = 1.05;
            m_deploymentIntroTimer = 3.0;
            if (m_skipTutorialButton) {
                m_skipTutorialButton->setVisible(false);
            }
            m_feedback = QStringLiteral("教学完成：3 秒后开始第一波异常协议");
            m_feedbackTime = 2.4;
            SoundManager::play(SoundCue::Freeze);
            ProgressManager::markTutorialFlag(QStringLiteral("towerIntroductions"), static_cast<int>(TowerKind::Shooter));
            ProgressManager::markTutorialFlag(QStringLiteral("towerIntroductions"), static_cast<int>(TowerKind::Slow));
            ProgressManager::markTutorialFlag(QStringLiteral("towerIntroductions"), static_cast<int>(TowerKind::Resource));
            queueFirstRunIntroductions();
            update();
            return;
        }
        activateFreezeSkill();
    } else if (event->key() >= Qt::Key_1 && event->key() <= Qt::Key_6) {
        if (m_reverseMode) {
            const int index = event->key() - Qt::Key_1;
            if (index >= 0 && index < static_cast<int>(m_towerButtons.size())) {
                m_towerButtons[static_cast<size_t>(index)]->click();
            }
            return;
        }
        const int index = event->key() - Qt::Key_1;
        const TowerKind kind = EntityRules::towerBuildOrder()[static_cast<size_t>(index)];
        if (!isTowerUnlocked(kind)) {
            m_feedback = towerUnlockHint(kind);
            m_feedbackTime = 2.4;
            SoundManager::play(SoundCue::ErrorDeny);
            update();
            return;
        }
        if (m_towerDrillActive && kind != m_towerDrillKind) {
            m_feedback = QStringLiteral("授权演练中：请先使用高亮要求的 %1").arg(towerName(m_towerDrillKind));
            m_feedbackTime = 999.0;
            update();
            return;
        }
        if (m_towerDrillActive && kind == m_towerDrillKind && m_towerDrillStage == 1) {
            m_towerDrillStage = 2;
            m_selectedTower = kind;
            m_towerDrillBody = towerDrillRoleText(kind);
            updateTowerButtonStates();
            update();
            return;
        }
        if (m_tutorialStep > 0 && m_tutorialStep != 6 && m_tutorialStep != 8) {
            syncTutorialFeedback();
            update();
            return;
        }
        if (m_tutorialStep == 6) {
            if (kind != TowerKind::Resource) {
                syncTutorialFeedback();
                update();
                return;
            }
            m_tutorialStep = 7;
            m_selectedTower = kind;
            if (m_tutorialResourceCell.x() >= 0) {
                m_energy = std::max(m_energy, buildCost(TowerKind::Resource, m_tutorialResourceCell));
            }
            syncTutorialFeedback();
            updateTowerButtonStates();
            update();
            return;
        }
        if (m_tutorialStep == 8) {
            if (kind != TowerKind::Slow) {
                syncTutorialFeedback();
                update();
                return;
            }
            m_tutorialStep = 9;
            m_selectedTower = kind;
            if (m_tutorialSlowCell.x() >= 0) {
                m_energy = std::max(m_energy, buildCost(TowerKind::Slow, m_tutorialSlowCell));
            }
            syncTutorialFeedback();
            updateTowerButtonStates();
            update();
            return;
        }
        m_selectedTower = kind;
        updateTowerButtonStates();
        m_feedback = QStringLiteral("已选择：%1").arg(towerName(m_selectedTower));
        m_feedbackTime = 1.2;
        update();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void GameWidget::updateGame()
{
    const double dt = m_clock.isValid() ? std::min(0.05, m_clock.restart() / 1000.0) : kFrameSeconds;

    if (!m_running) {
        return;
    }

    if (!m_paused) {
        m_freezePulseTimer = std::max(0.0, m_freezePulseTimer - dt);
        m_bossAlertTimer = std::max(0.0, m_bossAlertTimer - dt);
        m_waveBannerTimer = std::max(0.0, m_waveBannerTimer - dt);
        if (!m_infoCardActive && !m_enemyGuideActive) {
            m_intelBannerTimer = std::max(0.0, m_intelBannerTimer - dt);
        }
        updatePrompt(dt);
        for (VisualPulse &pulse : m_visualPulses) {
            pulse.age += dt;
        }
        m_visualPulses.erase(std::remove_if(m_visualPulses.begin(), m_visualPulses.end(), [](const VisualPulse &pulse) {
                                return pulse.age >= pulse.duration;
                            }),
                            m_visualPulses.end());
    }

    if (!m_finished && !m_paused && !m_infoCardActive) {
        if (m_tutorialStep > 0) {
            m_tutorialAnimTime += dt;
            update();
            return;
        }
        if (m_enemyGuideActive) {
            m_tutorialAnimTime += dt;
            update();
            return;
        }
        if (m_towerDrillActive) {
            m_tutorialAnimTime += dt;
            update();
            return;
        }

        if (m_deploymentIntroTimer > 0.0) {
            m_deploymentIntroTimer = std::max(0.0, m_deploymentIntroTimer - dt);
            if (m_deploymentIntroTimer <= 0.0) {
                m_waveBannerText = QStringLiteral("异常协议 1/%1 展开").arg(m_waves.size());
                m_waveBannerTimer = 2.4;
                SoundManager::play(SoundCue::MissionStart);
            }
            update();
            return;
        }

        m_elapsedSeconds += dt;
        m_freezeSkillCooldown = std::max(0.0, m_freezeSkillCooldown - dt);
        m_energyTick += dt;
        if (m_energyTick >= 1.0) {
            m_energy += 8;
            m_energyTick = 0.0;
        }

        updateWaveSpawner(dt);
        if (m_infoCardActive || m_enemyGuideActive) {
            update();
            return;
        }
        updateEnemies(dt);
        updateTowers(dt);
        updateProjectiles(dt);
        cleanupDeadEnemies();
        cleanupDestroyedTowers();
        checkEndConditions();
    }

    if (m_feedbackTime > 0.0) {
        m_feedbackTime = std::max(0.0, m_feedbackTime - dt);
    }

    update();
}

void GameWidget::updateWaveSpawner(double dt)
{
    if (m_reverseMode && m_reversePlanning) {
        return;
    }

    if (m_currentWave >= static_cast<int>(m_waves.size())) {
        return;
    }

    if (m_waitingNextWave) {
        m_nextWaveTimer -= dt;
        if (m_nextWaveTimer <= 0.0) {
            m_waitingNextWave = false;
            m_currentEntry = 0;
            m_entrySpawned = 0;
            m_spawnTimer = 0.0;
            const bool finalWave = m_currentWave == static_cast<int>(m_waves.size()) - 1;
            m_waveBannerText = finalWave
                                   ? QStringLiteral("关底协议启动：%1").arg(bossProtocolName(m_levelIndex))
                                   : QStringLiteral("异常协议 %1/%2 展开").arg(m_currentWave + 1).arg(m_waves.size());
            m_waveBannerTimer = 2.4;
            SoundManager::play(SoundCue::MissionStart);
        }
        return;
    }

    const auto &wave = m_waves[static_cast<size_t>(m_currentWave)];
    if (m_currentEntry >= static_cast<int>(wave.size())) {
        if (m_enemies.empty() && m_projectiles.empty()) {
            ++m_currentWave;
            if (m_reverseMode) {
                if (m_currentWave < static_cast<int>(m_waves.size())) {
                    prepareReversePlanningWave();
                }
                return;
            }
            if (m_currentWave < static_cast<int>(m_waves.size())) {
                m_waitingNextWave = true;
                const bool finalWave = m_currentWave == static_cast<int>(m_waves.size()) - 1;
                m_nextWaveTimer = finalWave ? 4.0 : 3.0;
                if (finalWave) {
                    m_waveBannerText.clear();
                    m_waveBannerTimer = 0.0;
                    SoundManager::play(SoundCue::BossAlert);
                }
            }
        }
        return;
    }

    m_spawnTimer -= dt;
    if (m_spawnTimer > 0.0) {
        return;
    }

    const WaveEntry &entry = wave[static_cast<size_t>(m_currentEntry)];
    m_enemies.push_back(makeEnemy(entry.kind));
    const int spawnedEnemyId = m_enemies.back()->id;
    const size_t enemyIndex = static_cast<size_t>(entry.kind);
    if (enemyIndex < m_enemyIntelShown.size() && !m_enemyIntelShown[enemyIndex]) {
        m_enemyIntelShown[enemyIndex] = true;
        const int flagIndex = static_cast<int>(entry.kind);
        if (!m_reverseMode
            && ProgressManager::tutorialHintsEnabled()
            && m_customLevelPath.isEmpty()
            && !ProgressManager::tutorialFlag(QStringLiteral("enemyIntroductions"), flagIndex)) {
            ProgressManager::markTutorialFlag(QStringLiteral("enemyIntroductions"), flagIndex);
            m_enemyGuideActive = true;
            m_enemyGuideKind = entry.kind;
            m_enemyGuideId = spawnedEnemyId;
            m_enemyGuideStage = 1;
            m_enemyGuideTitle = enemyIntroTitle(entry.kind);
            m_enemyGuideBody = enemyIntelText(entry.kind);
            SoundManager::play(SoundCue::Dialogue);
        }
    }
    if (entry.kind == EnemyKind::Boss) {
        const QString bossName = m_reverseMode ? QStringLiteral("逆向终局协议") : bossProtocolName(m_levelIndex);
        SoundManager::startMusic(MusicCue::Boss);
        clearPrompts();
        m_bossAlertTimer = 2.2;
        m_waveBannerTimer = 0.0;
        m_waveBannerText.clear();
        const QPointF breach = pathPosition(m_enemies.back()->progress);
        m_visualPulses.push_back(VisualPulse{breach, QColor(255, 90, 105), 0.0, 1.2, scaledDistance(170.0)});
        m_visualPulses.push_back(VisualPulse{breach, QColor(255, 225, 154), 0.0, 0.9, scaledDistance(120.0)});
        Q_UNUSED(bossName);
        SoundManager::play(SoundCue::BossStinger);
        SoundManager::play(SoundCue::BossAlert);
    }
    ++m_entrySpawned;
    m_spawnTimer = entry.interval;

    if (m_entrySpawned >= entry.count) {
        ++m_currentEntry;
        m_entrySpawned = 0;
        m_spawnTimer = 0.45;
    }
}

void GameWidget::updateEnemies(double dt)
{
    const double totalLength = pathLength();
    for (auto &enemy : m_enemies) {
        enemy->attackCooldown = std::max(0.0, enemy->attackCooldown - dt);
        enemy->portalCooldown = std::max(0.0, enemy->portalCooldown - dt);
        enemy->slowTime = std::max(0.0, enemy->slowTime - dt);
        enemy->burnTime = std::max(0.0, enemy->burnTime - dt);
        enemy->hitFlash = std::max(0.0, enemy->hitFlash - dt);

        if (enemy->burnTime > 0.0) {
            applyDamage(*enemy, enemy->burnDps * dt);
        }
        if (enemy->dead()) {
            continue;
        }

        const QPointF pos = pathPosition(enemy->progress);
        const int wallIndex = blockingWallIndex(pos);
        if (wallIndex >= 0) {
            if (enemy->attackCooldown <= 0.0) {
                m_towers[static_cast<size_t>(wallIndex)].hp -= EntityRules::enemy(enemy->kind).wallAttackDamage();
                enemy->attackCooldown = 0.75;
            }
            continue;
        }

        QPoint pathCell = pathCellForProgress(enemy->progress);
        TileKind tile = TileKind::Path;
        if (pathCell.x() >= 0) {
            tile = m_tiles[static_cast<size_t>(pathCell.y() * m_cols + pathCell.x())];
        }

        double speed = scaledDistance(enemy->baseSpeed);
        if (enemy->slowTime > 0.0) {
            speed *= enemy->slowFactor;
        }
        if (tile == TileKind::Accelerate) {
            speed *= 1.42;
        }
        updateBossMechanic(*enemy, speed, dt);

        enemy->progress += speed * dt;
        if (tile == TileKind::Portal && pathCell == m_portalFrom && enemy->portalCooldown <= 0.0) {
            enemy->progress = progressAtPathCell(m_portalTo) + m_cellSize * 0.35;
            enemy->portalCooldown = 1.0;
        }

        if (enemy->progress >= totalLength) {
            enemy->reachedCore = true;
        }
    }

    for (auto &enemy : m_pendingEnemies) {
        m_enemies.push_back(std::move(enemy));
    }
    m_pendingEnemies.clear();
}

void GameWidget::updateTowers(double dt)
{
    for (Tower &tower : m_towers) {
        tower.cooldownLeft = std::max(0.0, tower.cooldownLeft - dt);

        if (tower.kind == TowerKind::Resource) {
            tower.resourceTimer += dt;
            const double interval = std::max(4.6, 7.2 - tower.level * 0.65);
            if (tower.resourceTimer >= interval) {
                m_energy += 10 + tower.level * 5;
                tower.resourceTimer = 0.0;
            }
            continue;
        }
        if (tower.kind == TowerKind::Wall || tower.cooldownLeft > 0.0) {
            continue;
        }

        const QPointF towerPos = cellCenter(tower.cell);
        const double range = scaledTowerRange(tower);
        Enemy *target = nearestEnemy(towerPos, range);
        if (!target) {
            continue;
        }

        if (tower.kind == TowerKind::Laser) {
            const QPoint targetCell = pathCellForProgress(target->progress);
            for (Enemy *enemy : enemiesInRange(towerPos, range)) {
                const QPoint enemyCell = pathCellForProgress(enemy->progress);
                if (enemyCell.x() == targetCell.x() || enemyCell.y() == targetCell.y()) {
                    applyDamage(*enemy, tower.damage);
                }
            }
            tower.cooldownLeft = tower.cooldown;
            continue;
        }

        Projectile projectile;
        projectile.pos = towerPos;
        projectile.targetId = target->id;
        projectile.damage = tower.damage;
        if (tower.kind == TowerKind::Slow) {
            projectile.kind = ProjectileKind::Slow;
            projectile.speed = 390.0;
        } else if (tower.kind == TowerKind::Splash) {
            projectile.kind = ProjectileKind::Splash;
            projectile.speed = 330.0;
        }
        m_projectiles.push_back(projectile);
        tower.cooldownLeft = tower.cooldown;
    }
}

void GameWidget::updateBossMechanic(Enemy &enemy, double &speed, double dt)
{
    if (enemy.kind != EnemyKind::Boss || enemy.dead()) {
        return;
    }

    enemy.abilityTimer = std::max(0.0, enemy.abilityTimer - dt);
    const int idx = std::clamp(m_levelIndex, 0, 5);
    const QPointF center = pathPosition(enemy.progress);

    auto pulse = [&](const QColor &color, double radius) {
        m_visualPulses.push_back(VisualPulse{center, color, 0.0, 0.72, scaledDistance(radius)});
    };

    if (idx == 0) {
        if (enemy.phase == 0 && enemy.shield <= 0.0) {
            enemy.phase = 1;
            enemy.shield += 72.0;
            queuePrompt(QStringLiteral("护盾"),
                        QStringLiteral("守门者重启"),
                        QColor(255, 225, 154),
                        1.4,
                        PromptLaneBoss);
            pulse(QColor(255, 225, 154), 118.0);
        }
        return;
    }

    if (idx == 1) {
        if (enemy.abilityTimer <= 0.0) {
            enemy.abilityTimer = 5.8;
            queuePrompt(QStringLiteral("冲刺"),
                        QStringLiteral("站台长加速"),
                        QColor(82, 218, 255),
                        1.2,
                        PromptLaneBoss);
            pulse(QColor(82, 218, 255), 132.0);
        }
        if (enemy.abilityTimer > 4.55) {
            speed *= 1.68;
        }
        return;
    }

    if (idx == 2) {
        if (enemy.phase == 0 && enemy.hp < enemy.maxHp * 0.68) {
            enemy.phase = 1;
            enemy.progress = std::min(pathLength() - scaledDistance(80.0),
                                      enemy.progress + m_cellSize * 2.35);
            enemy.shield += 115.0;
            queuePrompt(QStringLiteral("跃迁"),
                        QStringLiteral("折跃核前移"),
                        QColor(156, 105, 255),
                        1.4,
                        PromptLaneBoss);
            pulse(QColor(156, 105, 255), 150.0);
        }
        return;
    }

    if (idx == 3) {
        if (enemy.phase == 0 && enemy.hp < enemy.maxHp * 0.62) {
            enemy.phase = 1;
            queuePrompt(QStringLiteral("复制"),
                        QStringLiteral("编目者召唤"),
                        QColor(205, 104, 220),
                        1.4,
                        PromptLaneBoss);
            spawnEnemyAt(EnemyKind::Fast, enemy.progress - scaledDistance(20.0), 0.70, 1.04);
            spawnEnemyAt(EnemyKind::Fast, enemy.progress - scaledDistance(54.0), 0.70, 1.04);
            spawnEnemyAt(EnemyKind::Resistant, enemy.progress - scaledDistance(90.0), 0.78, 0.96);
            pulse(QColor(205, 104, 220), 150.0);
        } else if (enemy.phase == 1 && enemy.hp < enemy.maxHp * 0.32) {
            enemy.phase = 2;
            spawnEnemyAt(EnemyKind::Splitter, enemy.progress - scaledDistance(44.0), 0.82, 0.96);
            spawnEnemyAt(EnemyKind::Splitter, enemy.progress - scaledDistance(92.0), 0.82, 0.96);
            pulse(QColor(255, 150, 92), 132.0);
        }
        return;
    }

    if (idx == 4) {
        if (enemy.abilityTimer <= 0.0) {
            enemy.abilityTimer = 5.2;
            enemy.shield = std::min(enemy.shield + 88.0, 430.0);
            queuePrompt(QStringLiteral("回充"),
                        QStringLiteral("护盾体补盾"),
                        QColor(255, 225, 154),
                        1.2,
                        PromptLaneBoss);
            pulse(QColor(255, 225, 154), 140.0);
        }
        if (enemy.phase == 0 && enemy.hp < enemy.maxHp * 0.45) {
            enemy.phase = 1;
            enemy.shield += 170.0;
            pulse(QColor(255, 245, 196), 170.0);
        }
        speed *= enemy.shield > 0.0 ? 0.92 : 1.08;
        return;
    }

    if (enemy.phase == 0 && enemy.hp < enemy.maxHp * 0.72) {
        enemy.phase = 1;
        enemy.shield += 135.0;
        queuePrompt(QStringLiteral("阶段 I"),
                    QStringLiteral("纪元主宰展开"),
                    QColor(255, 76, 132),
                    1.4,
                    PromptLaneBoss);
        spawnEnemyAt(EnemyKind::Fast, enemy.progress - scaledDistance(34.0), 0.82, 1.12);
        spawnEnemyAt(EnemyKind::Resistant, enemy.progress - scaledDistance(80.0), 0.88, 1.0);
        pulse(QColor(255, 76, 132), 170.0);
    } else if (enemy.phase == 1 && enemy.hp < enemy.maxHp * 0.42) {
        enemy.phase = 2;
        enemy.shield += 190.0;
        queuePrompt(QStringLiteral("终末"),
                    QStringLiteral("混合召唤"),
                    QColor(255, 225, 154),
                    1.5,
                    PromptLaneBoss);
        spawnEnemyAt(EnemyKind::Splitter, enemy.progress - scaledDistance(40.0), 0.88, 1.0);
        spawnEnemyAt(EnemyKind::Armored, enemy.progress - scaledDistance(100.0), 0.82, 0.96);
        pulse(QColor(255, 225, 154), 190.0);
    }
    if (enemy.shield <= 0.0) {
        speed *= 1.12;
    }
}

void GameWidget::updateProjectiles(double dt)
{
    for (Projectile &projectile : m_projectiles) {
        Enemy *target = enemyById(projectile.targetId);
        if (!target || target->dead()) {
            projectile.expired = true;
            continue;
        }

        const QPointF targetPos = pathPosition(target->progress);
        const QPointF delta = targetPos - projectile.pos;
        const double len = distance(projectile.pos, targetPos);
        if (len < scaledDistance(12.0)) {
            if (projectile.kind == ProjectileKind::Splash) {
                for (Enemy *enemy : enemiesInRange(targetPos, scaledDistance(82.0))) {
                    applyDamage(*enemy, projectile.damage);
                    applyBurn(*enemy, 2.0, 8.0);
                }
            } else {
                applyDamage(*target, projectile.damage);
                if (projectile.kind == ProjectileKind::Slow) {
                    applySlow(*target, 2.4, 0.48);
                }
            }
            projectile.expired = true;
            continue;
        }

        const double step = scaledDistance(projectile.speed) * dt;
        projectile.pos += normalized(delta) * std::min(step, len);
    }

    m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(), [](const Projectile &p) {
                            return p.expired;
                        }),
                        m_projectiles.end());
}

void GameWidget::cleanupDeadEnemies()
{
    std::vector<std::unique_ptr<Enemy>> spawned;
    for (auto &enemy : m_enemies) {
        const auto children = EntityRules::splitChildren(*enemy);
        if (enemy->dead() && !children.empty()) {
            enemy->splitCreated = true;
            const QPointF splitPoint = pathPosition(enemy->progress);
            m_visualPulses.push_back(VisualPulse{splitPoint, QColor(255, 150, 92), 0.0, 0.72, scaledDistance(88.0)});
            for (const SplitEnemySpec &spec : children) {
                auto child = makeEnemy(spec.kind);
                child->hp = spec.hp;
                child->maxHp = spec.hp;
                child->baseSpeed = spec.baseSpeed;
                child->progress = std::max(0.0, enemy->progress + scaledDistance(spec.progressOffset));
                spawned.push_back(std::move(child));
            }
        }
    }

    int defeatedNow = 0;
    bool defeatedBoss = false;
    for (const auto &enemy : m_enemies) {
        if (!enemy->dead()) {
            continue;
        }
        ++defeatedNow;
        const QPointF center = pathPosition(enemy->progress);
        const bool boss = enemy->kind == EnemyKind::Boss;
        defeatedBoss = defeatedBoss || boss;
        m_visualPulses.push_back(VisualPulse{center,
                                             enemyColor(enemy->kind).lighter(145),
                                             0.0,
                                             boss ? 1.25 : 0.62,
                                             scaledDistance(boss ? 170.0 : 62.0)});
    }
    m_enemiesDefeated += defeatedNow;

    m_enemies.erase(std::remove_if(m_enemies.begin(), m_enemies.end(), [](const std::unique_ptr<Enemy> &enemy) {
                        return enemy->dead();
                    }),
                    m_enemies.end());

    for (auto &enemy : spawned) {
        m_enemies.push_back(std::move(enemy));
    }

    if (defeatedBoss && !m_finished) {
        const bool bossStillActive = std::any_of(m_enemies.begin(), m_enemies.end(), [](const std::unique_ptr<Enemy> &enemy) {
            return enemy->kind == EnemyKind::Boss && !enemy->dead();
        });
        if (!bossStillActive) {
            SoundManager::startMusic(MusicCue::Battle);
        }
    }
}

void GameWidget::cleanupDestroyedTowers()
{
    m_towers.erase(std::remove_if(m_towers.begin(), m_towers.end(), [](const Tower &tower) {
                       return tower.hp <= 0.0;
                   }),
                   m_towers.end());
}

void GameWidget::configureReverseMode()
{
    m_reversePlans.assign(6, {});
    m_reversePlanning = true;
    m_reverseSpentThisWave = 0;
    m_reverseRiftPoints = reverseWaveBudget(0);
    m_energy = reverseInitialDefenseEnergy();
    m_freezeSkillCooldown = 999.0;
    m_feedback = QStringLiteral("%1：用裂隙点数编排第 1 波，系统会自动建塔防守。").arg(reverseScenarioName());
    m_feedbackTime = 4.0;
    queuePrompt(reverseScenarioName(),
                QStringLiteral("你负责投放异常，系统负责建塔。6 波内突破核心即胜利。"),
                QColor(255, 92, 190),
                3.2,
                PromptLaneTop);
}

void GameWidget::prepareReversePlanningWave()
{
    if (!m_reverseMode || m_currentWave >= static_cast<int>(m_waves.size())) {
        return;
    }
    m_reversePlanning = true;
    m_reverseSpentThisWave = 0;
    m_reverseRiftPoints = reverseWaveBudget(m_currentWave);
    m_energy += reverseWaveDefenseEnergy(m_currentWave);
    m_projectiles.clear();
    m_feedback = QStringLiteral("第 %1 波编排开始：选择异常后点击“开始本波”。").arg(m_currentWave + 1);
    m_feedbackTime = 3.0;
    updateControlButtonGeometry();
    updateReverseButtonStates();
}

void GameWidget::startReverseWave()
{
    if (!m_reverseMode || !m_reversePlanning || m_finished || m_currentWave >= static_cast<int>(m_waves.size())) {
        return;
    }

    auto &plan = m_reversePlans[static_cast<size_t>(m_currentWave)];
    if (plan.empty()) {
        m_feedback = QStringLiteral("至少投放一个异常体才能开始本波。");
        m_feedbackTime = 1.8;
        SoundManager::play(SoundCue::ErrorDeny);
        update();
        return;
    }

    m_waves[static_cast<size_t>(m_currentWave)] = plan;
    autoDeployReverseDefense();
    m_reversePlanning = false;
    m_currentEntry = 0;
    m_entrySpawned = 0;
    m_spawnTimer = 0.15;
    m_waitingNextWave = false;
    m_waveBannerText = m_currentWave == static_cast<int>(m_waves.size()) - 1
                           ? QStringLiteral("最终逆向协议展开")
                           : QStringLiteral("逆向协议 %1/%2 展开").arg(m_currentWave + 1).arg(m_waves.size());
    m_waveBannerTimer = 2.4;
    m_feedback = QStringLiteral("系统防守 AI 已部署，逆向波次开始。");
    m_feedbackTime = 2.2;
    SoundManager::play(SoundCue::MissionStart);
    updateControlButtonGeometry();
    updateReverseButtonStates();
    update();
}

void GameWidget::autoDeployReverseDefense()
{
    const int scenario = reverseScenarioClamp(m_reverseScenarioIndex);
    if (scenario == 0) {
        std::vector<std::pair<TowerKind, double>> trainingBlueprint = {
            {TowerKind::Shooter, 0.24},
            {TowerKind::Slow, 0.58},
            {TowerKind::Shooter, 0.82}
        };
        if (m_currentWave >= 2) {
            trainingBlueprint.push_back({TowerKind::Slow, 0.38});
        }
        if (m_currentWave >= 4) {
            trainingBlueprint.push_back({TowerKind::Shooter, 0.92});
        }

        const int buildLimit = std::clamp(2 + m_currentWave / 2, 2, 5);
        int built = 0;
        for (const auto &entry : trainingBlueprint) {
            if (built >= buildLimit) {
                break;
            }
            if (tryAutoBuildTower(entry.first, entry.second)) {
                ++built;
            }
        }
        if (m_currentWave >= 4) {
            tryAutoUpgradeTower();
        }
        return;
    }

    std::vector<std::pair<TowerKind, double>> blueprint = {
        {TowerKind::Shooter, 0.16},
        {TowerKind::Slow, 0.30},
        {TowerKind::Shooter, 0.48},
        {TowerKind::Slow, 0.66},
        {TowerKind::Shooter, 0.82}
    };
    if (scenario >= 1) {
        if (m_currentWave >= 1) {
            blueprint.push_back({TowerKind::Splash, 0.38});
        }
        if (m_currentWave >= 2) {
            blueprint.push_back({TowerKind::Wall, 0.58});
        }
    }
    if (scenario >= 2) {
        if (m_currentWave >= 1) {
            blueprint.push_back({TowerKind::Laser, 0.24});
            blueprint.push_back({TowerKind::Splash, 0.72});
        }
        if (m_currentWave >= 2) {
            blueprint.push_back({TowerKind::Wall, 0.88});
        }
    }
    if (m_currentWave >= 1) {
        blueprint.push_back({TowerKind::Shooter, 0.92});
        blueprint.push_back({TowerKind::Slow, 0.76});
    }
    if (m_currentWave >= 2) {
        blueprint.push_back({TowerKind::Splash, 0.52});
        blueprint.push_back({TowerKind::Laser, 0.42});
    }
    if (m_currentWave >= 3) {
        blueprint.push_back({TowerKind::Wall, 0.70});
        blueprint.push_back({TowerKind::Laser, 0.18});
    }
    if (m_currentWave >= 4) {
        blueprint.push_back({TowerKind::Splash, 0.86});
        blueprint.push_back({TowerKind::Slow, 0.12});
    }

    const int buildLimit = scenario == 0
                               ? std::clamp(4 + m_currentWave, 4, 8)
                               : scenario == 1
                                     ? std::clamp(4 + m_currentWave, 4, 9)
                                     : std::clamp(5 + m_currentWave, 5, 10);
    int built = 0;
    for (const auto &entry : blueprint) {
        if (built >= buildLimit) {
            break;
        }
        if (tryAutoBuildTower(entry.first, entry.second)) {
            ++built;
        } else {
            tryAutoUpgradeTower();
        }
    }
    const int upgrades = scenario == 0
                             ? std::clamp(m_currentWave / 2, 0, 3)
                             : scenario == 1
                                   ? std::clamp((m_currentWave + 1) / 3, 0, 3)
                                   : std::clamp(m_currentWave / 2, 0, 4);
    for (int i = 0; i < upgrades; ++i) {
        tryAutoUpgradeTower();
    }
}

int GameWidget::reverseScenarioLevelIndex() const
{
    static const std::array<int, 3> levels = {0, 3, 5};
    return levels[static_cast<size_t>(reverseScenarioClamp(m_reverseScenarioIndex))];
}

QString GameWidget::reverseScenarioName() const
{
    switch (reverseScenarioClamp(m_reverseScenarioIndex)) {
    case 0:
        return QStringLiteral("逆向训练线");
    case 1:
        return QStringLiteral("裂隙压测线");
    case 2:
        return QStringLiteral("终局破防线");
    }
    return QStringLiteral("逆向推演");
}

int GameWidget::reverseInitialDefenseEnergy() const
{
    static const std::array<int, 3> energy = {280, 420, 480};
    return energy[static_cast<size_t>(reverseScenarioClamp(m_reverseScenarioIndex))];
}

int GameWidget::reverseWaveDefenseEnergy(int waveIndex) const
{
    const int wave = std::clamp(waveIndex, 0, 5);
    switch (reverseScenarioClamp(m_reverseScenarioIndex)) {
    case 0:
        return 36 + wave * 10;
    case 1:
        return 64 + wave * 15;
    case 2:
        return 74 + wave * 17;
    }
    return 70 + wave * 16;
}

bool GameWidget::tryAutoBuildTower(TowerKind kind, double preferredProgress)
{
    const int pathIndex = std::clamp(static_cast<int>(preferredProgress * std::max(1, static_cast<int>(m_pathCells.size()) - 1)),
                                     0,
                                     std::max(0, static_cast<int>(m_pathCells.size()) - 1));
    const std::array<QPoint, 8> offsets = {
        QPoint(0, -1), QPoint(0, 1), QPoint(1, 0), QPoint(-1, 0),
        QPoint(1, -1), QPoint(-1, -1), QPoint(1, 1), QPoint(-1, 1)
    };
    for (int radius = 0; radius < static_cast<int>(m_pathCells.size()); ++radius) {
        const int forward = pathIndex + radius;
        const int backward = pathIndex - radius;
        const std::array<int, 2> candidates = {forward, backward};
        for (int index : candidates) {
            if (index < 0 || index >= static_cast<int>(m_pathCells.size())) {
                continue;
            }
            const QPoint path = m_pathCells[static_cast<size_t>(index)];
            const std::vector<QPoint> cells = kind == TowerKind::Wall
                                                  ? std::vector<QPoint>{path}
                                                  : std::vector<QPoint>(offsets.begin(), offsets.end());
            for (const QPoint &offsetOrCell : cells) {
                const QPoint cell = kind == TowerKind::Wall ? offsetOrCell : path + offsetOrCell;
                if (!canBuildAt(cell, kind)) {
                    continue;
                }
                const int cost = buildCost(kind, cell);
                if (m_energy >= cost) {
                    m_towers.push_back(makeTower(kind, cell));
                    ++m_towersBuilt;
                    m_energy -= cost;
                    m_visualPulses.push_back(VisualPulse{cellCenter(cell), towerColor(kind), 0.0, 0.72, scaledDistance(86.0)});
                    return true;
                }
            }
        }
    }
    return false;
}

bool GameWidget::tryAutoUpgradeTower()
{
    int bestIndex = -1;
    int bestCost = 0;
    double bestScore = -1.0;
    for (int i = 0; i < static_cast<int>(m_towers.size()); ++i) {
        const int cost = upgradeCost(m_towers[static_cast<size_t>(i)]);
        if (cost <= 0 || cost > m_energy) {
            continue;
        }
        const double progress = progressAtPathCell(m_towers[static_cast<size_t>(i)].cell);
        const double score = m_towers[static_cast<size_t>(i)].damage + progress * 0.01 + m_towers[static_cast<size_t>(i)].level * 20.0;
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
            bestCost = cost;
        }
    }
    if (bestIndex < 0) {
        return false;
    }
    upgradeTower(bestIndex);
    m_energy = std::max(0, m_energy); // upgradeTower already subtracts; keep the intent explicit for AI spending.
    Q_UNUSED(bestCost);
    return true;
}

EnemyKind GameWidget::reverseEnemyKindForButton(int index) const
{
    const auto &order = reverseEnemyOrder();
    return order[static_cast<size_t>(std::clamp(index, 0, static_cast<int>(order.size()) - 1))];
}

bool GameWidget::reverseEnemyUnlocked(EnemyKind kind) const
{
    switch (kind) {
    case EnemyKind::Normal:
        return true;
    case EnemyKind::Fast:
        return true;
    case EnemyKind::Armored:
        return m_currentWave >= 1;
    case EnemyKind::Resistant:
        return m_currentWave >= 2;
    case EnemyKind::Splitter:
        return m_currentWave >= 3;
    case EnemyKind::Boss:
        return m_currentWave >= static_cast<int>(m_waves.size()) - 1;
    }
    return false;
}

int GameWidget::reverseEnemyWaveLimit(EnemyKind kind) const
{
    const int wave = std::clamp(m_currentWave, 0, 5);
    const int scenario = reverseScenarioClamp(m_reverseScenarioIndex);
    switch (kind) {
    case EnemyKind::Normal:
        return scenario == 0 ? 6 + wave : 5 + wave;
    case EnemyKind::Fast:
        return scenario == 0 ? 3 + wave / 2 : 3 + wave / 2;
    case EnemyKind::Armored:
        return scenario == 0 ? 3 + wave / 2 : 2 + wave / 2;
    case EnemyKind::Resistant:
        return scenario == 0 ? 2 + (wave >= 4 ? 1 : 0) : 2 + (wave >= 4 ? 1 : 0);
    case EnemyKind::Splitter:
        return wave >= 4 ? 2 : 1;
    case EnemyKind::Boss:
        return 1;
    }
    return 1;
}

QRectF GameWidget::reverseQueuePanelRect() const
{
    const double panelWidth = std::clamp(width() - 300.0, 820.0, 1280.0);
    const double panelHeight = 190.0;
    return QRectF(width() / 2.0 - panelWidth / 2.0,
                  height() - panelHeight - 34.0,
                  panelWidth,
                  panelHeight);
}

int GameWidget::reverseQueueIndexAt(const QPoint &pos, bool insertion) const
{
    if (!m_reverseMode || !m_reversePlanning || m_currentWave >= static_cast<int>(m_reversePlans.size())) {
        return -1;
    }
    const auto &plan = m_reversePlans[static_cast<size_t>(m_currentWave)];
    if (plan.empty()) {
        return -1;
    }

    const QRectF panel = reverseQueuePanelRect();
    const double chip = 58.0;
    const double gap = 10.0;
    const double y = panel.top() + 104.0;
    const double startX = panel.left() + 24.0;
    const int visible = std::min(static_cast<int>(plan.size()),
                                 std::max(1, static_cast<int>((panel.width() - 310.0) / (chip + gap))));
    if (pos.y() < y - 10.0 || pos.y() > y + chip + 10.0 || pos.x() < startX - 8.0) {
        return -1;
    }

    if (insertion) {
        const int index = static_cast<int>(std::floor((pos.x() - startX + (chip + gap) / 2.0) / (chip + gap)));
        return std::clamp(index, 0, std::max(0, visible - 1));
    }

    for (int i = 0; i < visible; ++i) {
        if (QRectF(startX + i * (chip + gap), y, chip, chip).contains(pos)) {
            return i;
        }
    }
    return -1;
}

bool GameWidget::removeReverseQueuedEnemyAt(const QPoint &pos)
{
    if (!m_reverseMode || !m_reversePlanning || m_currentWave >= static_cast<int>(m_reversePlans.size())) {
        return false;
    }
    auto &plan = m_reversePlans[static_cast<size_t>(m_currentWave)];
    if (plan.empty()) {
        return false;
    }

    const int index = reverseQueueIndexAt(pos, false);
    if (index < 0 || index >= static_cast<int>(plan.size())) {
        return false;
    }

    const EnemyKind kind = plan[static_cast<size_t>(index)].kind;
    m_reverseRiftPoints += reverseEnemyCost(kind);
    m_reverseSpentThisWave = std::max(0, m_reverseSpentThisWave - reverseEnemyCost(kind));
    plan.erase(plan.begin() + index);
    m_reverseDragIndex = -1;
    m_reverseDragHoverIndex = -1;
    m_feedback = QStringLiteral("已撤回：%1  +%2 裂隙点").arg(enemyName(kind)).arg(reverseEnemyCost(kind));
    m_feedbackTime = 1.4;
    SoundManager::play(SoundCue::UiClick);
    updateReverseButtonStates();
    return true;
}

int GameWidget::reverseEnemyCost(EnemyKind kind) const
{
    switch (kind) {
    case EnemyKind::Normal: return 12;
    case EnemyKind::Fast: return 18;
    case EnemyKind::Armored: return 32;
    case EnemyKind::Resistant: return 39;
    case EnemyKind::Splitter: return 44;
    case EnemyKind::Boss: return 108;
    }
    return 12;
}

int GameWidget::reverseWaveBudget(int waveIndex) const
{
    static const std::array<std::array<int, 6>, 3> budgets = {{
        {78, 124, 178, 240, 314, 420},
        {68, 122, 176, 236, 304, 398},
        {62, 118, 174, 238, 310, 410}
    }};
    return budgets[static_cast<size_t>(reverseScenarioClamp(m_reverseScenarioIndex))]
                  [static_cast<size_t>(std::clamp(waveIndex, 0, 5))];
}

QString GameWidget::reverseEnemyButtonText(EnemyKind kind, int count) const
{
    return QStringLiteral("%1\n点数 %2  %3/%4")
        .arg(enemyName(kind))
        .arg(reverseEnemyCost(kind))
        .arg(count)
        .arg(reverseEnemyWaveLimit(kind));
}

QString GameWidget::reverseEnemyHint(EnemyKind kind) const
{
    switch (kind) {
    case EnemyKind::Normal: return QStringLiteral("铺量压测：低价填充队列，适合试探系统开局火力。");
    case EnemyKind::Fast: return QStringLiteral("速度突防：快速压线，逼迫系统在出口前补减速。");
    case EnemyKind::Armored: return QStringLiteral("高血消耗：拖住单体输出，让后续异常穿过火力空窗。");
    case EnemyKind::Resistant: return QStringLiteral("反控制：削弱减速与伤害，专门冲破凝滞防线。");
    case EnemyKind::Splitter: return QStringLiteral("死亡分裂：制造多目标压力，克制单点高伤防线。");
    case EnemyKind::Boss: return QStringLiteral("终局突破：高血量协议，适合与混合异常同时压核心。");
    }
    return QString();
}

void GameWidget::checkEndConditions()
{
    for (const auto &enemy : m_enemies) {
        if (enemy->reachedCore) {
            m_finished = true;
            SoundManager::play(SoundCue::CoreDamage);
            m_summary = GameSummary{m_levelName,
                                    m_levelIndex,
                                    m_reverseMode,
                                    m_reverseMode,
                                    m_reverseMode ? m_reverseRiftPoints : m_energy,
                                    m_towersBuilt,
                                    m_enemiesDefeated,
                                    std::min(m_currentWave + 1, static_cast<int>(m_waves.size())),
                                    static_cast<int>(m_waves.size()),
                                    m_elapsedSeconds};
            if (!m_reverseMode) {
                saveProgress(false);
                emit failure();
            } else {
                emit victory();
            }
            return;
        }
    }

    if (m_currentWave >= static_cast<int>(m_waves.size()) && m_enemies.empty() && m_projectiles.empty()) {
        m_finished = true;
        m_summary = GameSummary{m_levelName,
                                m_levelIndex,
                                m_reverseMode,
                                !m_reverseMode,
                                m_reverseMode ? m_reverseRiftPoints : m_energy,
                                m_towersBuilt,
                                m_enemiesDefeated,
                                static_cast<int>(m_waves.size()),
                                static_cast<int>(m_waves.size()),
                                m_elapsedSeconds};
        if (!m_reverseMode) {
            saveProgress(true);
            emit victory();
        } else {
            emit failure();
        }
    }
}

QPointF GameWidget::cellCenter(const QPoint &cell) const
{
    return QPointF(m_origin.x() + cell.x() * m_cellSize + m_cellSize / 2.0,
                   m_origin.y() + cell.y() * m_cellSize + m_cellSize / 2.0);
}

QRectF GameWidget::cellRect(int row, int col) const
{
    return QRectF(m_origin.x() + col * m_cellSize,
                  m_origin.y() + row * m_cellSize,
                  m_cellSize,
                  m_cellSize);
}

QRect GameWidget::towerPanelRect() const
{
    const int panelWidth = std::clamp(static_cast<int>(width() * 0.135), 184, 220);
    const int buttonHeight = std::clamp(static_cast<int>(m_cellSize * 0.58), 48, 56);
    const int buttonGap = std::clamp(static_cast<int>(m_cellSize * 0.10), 8, 11);
    const int titleArea = std::clamp(static_cast<int>(m_cellSize * 0.72), 68, 78);
    const int buttonCount = std::max(1, static_cast<int>(m_towerButtons.size()));
    const int naturalHeight = titleArea + buttonCount * buttonHeight + (buttonCount - 1) * buttonGap + 24;
    const int top = std::clamp(static_cast<int>(height() * 0.155), 110, 150);
    const int panelHeight = std::min(std::clamp(naturalHeight, 388, 486), std::max(320, height() - top - 24));
    return QRect(width() - panelWidth - 18, top, panelWidth, panelHeight);
}

QPoint GameWidget::cellAt(const QPoint &pos) const
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

QPoint GameWidget::pathCellForProgress(double progress) const
{
    if (m_pathCells.empty()) {
        return QPoint(-1, -1);
    }

    int index = 0;
    double remaining = progress;
    while (index < static_cast<int>(m_pathSegmentLengths.size()) && remaining > m_pathSegmentLengths[static_cast<size_t>(index)]) {
        remaining -= m_pathSegmentLengths[static_cast<size_t>(index)];
        ++index;
    }
    index = std::clamp(index, 0, static_cast<int>(m_pathCells.size()) - 1);
    return m_pathCells[static_cast<size_t>(index)];
}

QPointF GameWidget::pathPosition(double progress) const
{
    if (m_pathCells.empty()) {
        return QPointF();
    }

    double remaining = progress;
    for (size_t i = 0; i < m_pathSegmentLengths.size(); ++i) {
        const double segment = m_pathSegmentLengths[i];
        if (remaining <= segment) {
            const QPointF a = cellCenter(m_pathCells[i]);
            const QPointF b = cellCenter(m_pathCells[i + 1]);
            const double t = segment > 0.0 ? remaining / segment : 0.0;
            return a + (b - a) * t;
        }
        remaining -= segment;
    }

    return cellCenter(m_pathCells.back());
}

double GameWidget::progressAtPathCell(const QPoint &cell) const
{
    double progress = 0.0;
    for (size_t i = 0; i < m_pathCells.size(); ++i) {
        if (m_pathCells[i] == cell) {
            return progress;
        }
        if (i < m_pathSegmentLengths.size()) {
            progress += m_pathSegmentLengths[i];
        }
    }
    return progress;
}

double GameWidget::pathLength() const
{
    double total = 0.0;
    for (double segment : m_pathSegmentLengths) {
        total += segment;
    }
    return total;
}

bool GameWidget::isPathCell(const QPoint &cell) const
{
    return std::find(m_pathCells.begin(), m_pathCells.end(), cell) != m_pathCells.end();
}

bool GameWidget::isEndpointCell(const QPoint &cell) const
{
    return !m_pathCells.empty() && (cell == m_pathCells.front() || cell == m_pathCells.back());
}

bool GameWidget::hasTowerAt(const QPoint &cell) const
{
    return std::any_of(m_towers.begin(), m_towers.end(), [&cell](const Tower &tower) {
        return tower.cell == cell;
    });
}

bool GameWidget::isTowerUnlocked(TowerKind kind) const
{
    if (m_reverseMode) {
        return true;
    }
    if (!m_customLevelPath.isEmpty()) {
        return true;
    }
    return m_levelIndex >= towerUnlockLevel(kind);
}

bool GameWidget::canBuildAt(const QPoint &cell, TowerKind kind) const
{
    if (!isTowerUnlocked(kind)) {
        return false;
    }
    if (cell.x() < 0 || cell.x() >= m_cols || cell.y() < 0 || cell.y() >= m_rows || hasTowerAt(cell)) {
        return false;
    }

    const TileKind tile = m_tiles[static_cast<size_t>(cell.y() * m_cols + cell.x())];
    if (kind == TowerKind::Wall) {
        return isPathCell(cell) && !isEndpointCell(cell) && tile != TileKind::Portal;
    }
    return tile == TileKind::Stable || tile == TileKind::Discount;
}

int GameWidget::buildCost(TowerKind kind, const QPoint &cell) const
{
    int cost = m_towerSpecs[static_cast<size_t>(kind)].cost;
    const TileKind tile = m_tiles[static_cast<size_t>(cell.y() * m_cols + cell.x())];
    if (tile == TileKind::Discount) {
        cost = static_cast<int>(std::round(cost * 0.7));
    }
    return cost;
}

int GameWidget::upgradeCost(const Tower &tower) const
{
    if (tower.level >= 3) {
        return 0;
    }

    const int baseCost = m_towerSpecs[static_cast<size_t>(tower.kind)].cost;
    const double levelFactor = tower.level == 1 ? 1.05 : 1.55;
    double roleFactor = 1.0;
    switch (tower.kind) {
    case TowerKind::Shooter: roleFactor = 0.95; break;
    case TowerKind::Slow: roleFactor = 0.98; break;
    case TowerKind::Splash: roleFactor = 1.0; break;
    case TowerKind::Laser: roleFactor = 1.05; break;
    case TowerKind::Resource: roleFactor = 1.0; break;
    case TowerKind::Wall: roleFactor = 0.92; break;
    }
    return std::max(40, static_cast<int>(std::round(baseCost * levelFactor * roleFactor / 5.0)) * 5);
}

int GameWidget::towerIndexAt(const QPoint &cell)
{
    for (size_t i = 0; i < m_towers.size(); ++i) {
        if (m_towers[i].cell == cell) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int GameWidget::blockingWallIndex(const QPointF &enemyPos)
{
    for (size_t i = 0; i < m_towers.size(); ++i) {
        const Tower &tower = m_towers[i];
        if (tower.kind == TowerKind::Wall && distance(cellCenter(tower.cell), enemyPos) < m_cellSize * 0.48) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Enemy *GameWidget::enemyById(int id) const
{
    for (const auto &enemy : m_enemies) {
        if (enemy->id == id) {
            return enemy.get();
        }
    }
    return nullptr;
}

Enemy *GameWidget::nearestEnemy(const QPointF &from, double range) const
{
    Enemy *best = nullptr;
    double bestDistance = range;
    for (const auto &enemy : m_enemies) {
        if (enemy->dead()) {
            continue;
        }
        const double d = distance(from, pathPosition(enemy->progress));
        if (d <= bestDistance) {
            bestDistance = d;
            best = enemy.get();
        }
    }
    return best;
}

std::vector<Enemy *> GameWidget::enemiesInRange(const QPointF &from, double range) const
{
    std::vector<Enemy *> result;
    for (const auto &enemy : m_enemies) {
        if (!enemy->dead() && distance(from, pathPosition(enemy->progress)) <= range) {
            result.push_back(enemy.get());
        }
    }
    return result;
}

Tower GameWidget::makeTower(TowerKind kind, const QPoint &cell) const
{
    return EntityFactory::createTower(kind, cell, m_towerSpecs[static_cast<size_t>(kind)]);
}

void GameWidget::upgradeTower(int index)
{
    if (index < 0 || index >= static_cast<int>(m_towers.size())) {
        return;
    }

    Tower &tower = m_towers[static_cast<size_t>(index)];
    if (tower.level >= 3) {
        m_feedback = QStringLiteral("%1 已达到最高等级").arg(towerName(tower.kind));
        m_feedbackTime = 1.5;
        SoundManager::play(SoundCue::ErrorDeny);
        return;
    }

    const int cost = upgradeCost(tower);
    if (m_energy < cost) {
        m_feedback = QStringLiteral("时能不足：升级需要 %1").arg(cost);
        m_feedbackTime = 1.5;
        SoundManager::play(SoundCue::ErrorDeny);
        return;
    }

    m_energy -= cost;
    EntityFactory::applyUpgrade(tower);
    const QPointF center = cellCenter(tower.cell);
    m_visualPulses.push_back(VisualPulse{center, QColor(255, 245, 196), 0.0, 0.38, scaledDistance(64.0)});
    m_visualPulses.push_back(VisualPulse{center, QColor(80, 212, 255), 0.0, 0.82, scaledDistance(112.0)});
    m_visualPulses.push_back(VisualPulse{center, QColor(255, 189, 92), 0.0, 1.05, scaledDistance(146.0)});
    SoundManager::play(SoundCue::Upgrade);

    m_feedback = QStringLiteral("%1 校准到 Lv.%2").arg(towerName(tower.kind)).arg(tower.level);
    m_feedbackTime = 1.6;
}

void GameWidget::activateFreezeSkill()
{
    if (m_finished) {
        return;
    }
    if (m_freezeSkillCooldown > 0.0) {
        m_feedback = QStringLiteral("时序冻结冷却中：%1 秒").arg(static_cast<int>(std::ceil(m_freezeSkillCooldown)));
        m_feedbackTime = 1.5;
        SoundManager::play(SoundCue::ErrorDeny);
        return;
    }

    for (auto &enemy : m_enemies) {
        applySlow(*enemy, 3.0, 0.12);
    }
    m_freezeSkillCooldown = 28.0;
    m_freezePulseTimer = 1.05;
    SoundManager::play(SoundCue::Freeze);
    m_feedback = QStringLiteral("主动技能：时序冻结");
    m_feedbackTime = 2.0;
}

std::unique_ptr<Enemy> GameWidget::makeEnemy(EnemyKind kind)
{
    auto enemy = EntityFactory::createEnemy(kind, m_nextEnemyId++);
    if (kind == EnemyKind::Boss) {
        static const std::array<double, 6> hpScale = {0.58, 0.78, 0.98, 1.15, 1.35, 1.55};
        static const std::array<double, 6> shieldScale = {0.45, 0.65, 0.85, 1.05, 1.35, 1.70};
        static const std::array<double, 6> speedScale = {0.90, 1.12, 1.00, 0.94, 0.88, 1.06};
        const int idx = std::clamp(m_levelIndex, 0, 5);
        enemy->hp *= hpScale[static_cast<size_t>(idx)];
        enemy->maxHp = enemy->hp;
        enemy->shield *= shieldScale[static_cast<size_t>(idx)];
        enemy->baseSpeed *= speedScale[static_cast<size_t>(idx)];
    }
    return enemy;
}

void GameWidget::spawnEnemyAt(EnemyKind kind, double progress, double hpScale, double speedScale)
{
    auto enemy = makeEnemy(kind);
    enemy->hp *= hpScale;
    enemy->maxHp = enemy->hp;
    enemy->baseSpeed *= speedScale;
    enemy->progress = std::clamp(progress, 0.0, std::max(0.0, pathLength() - scaledDistance(46.0)));
    m_pendingEnemies.push_back(std::move(enemy));
}

void GameWidget::applyDamage(Enemy &enemy, double damage)
{
    damage *= EntityRules::enemy(enemy.kind).incomingDamageMultiplier();
    if (enemy.shield > 0.0) {
        const double blocked = std::min(enemy.shield, damage);
        enemy.shield -= blocked;
        damage -= blocked;
    }
    enemy.hp -= damage;
    enemy.hitFlash = std::max(enemy.hitFlash, 0.16);
}

void GameWidget::applySlow(Enemy &enemy, double seconds, double factor)
{
    EntityRules::enemy(enemy.kind).adjustSlow(seconds, factor);

    const QPoint cell = pathCellForProgress(enemy.progress);
    if (cell.x() >= 0) {
        const TileKind tile = m_tiles[static_cast<size_t>(cell.y() * m_cols + cell.x())];
        if (tile == TileKind::Accelerate) {
            factor *= 0.7;
            seconds *= 1.25;
        }
    }

    enemy.slowTime = std::max(enemy.slowTime, seconds);
    enemy.slowFactor = std::min(enemy.slowFactor, factor);
}

void GameWidget::applyBurn(Enemy &enemy, double seconds, double dps)
{
    enemy.burnTime = std::max(enemy.burnTime, seconds);
    enemy.burnDps = std::max(enemy.burnDps, dps);
}

QString GameWidget::towerName(TowerKind kind) const
{
    return EntityRules::tower(kind).name();
}

QString GameWidget::towerShortName(TowerKind kind) const
{
    return EntityRules::tower(kind).shortName();
}

QString GameWidget::towerUnlockHint(TowerKind kind) const
{
    const int unlockLevel = towerUnlockLevel(kind);
    if (unlockLevel <= 0) {
        return QStringLiteral("%1 已在初始授权中开放。").arg(towerName(kind));
    }
    return QStringLiteral("%1 尚未授权：通关至第 %2 关主线后开放。")
        .arg(towerName(kind))
        .arg(unlockLevel + 1);
}

QString GameWidget::towerIntroText(TowerKind kind) const
{
    const TowerSpec &spec = m_towerSpecs[static_cast<size_t>(kind)];
    const QString base = QStringLiteral("费用 %1 / 射程 %2 / 伤害 %3 / 冷却 %4 秒\n")
                             .arg(spec.cost)
                             .arg(static_cast<int>(std::round(spec.range)))
                             .arg(static_cast<int>(std::round(spec.damage)))
                             .arg(QString::number(spec.cooldown, 'f', 2));
    switch (kind) {
    case TowerKind::Shooter:
        return base + QStringLiteral("定位：低费持续伤害。\n要点：守拐角和出口，优先升级主火力位。");
    case TowerKind::Slow:
        return base + QStringLiteral("定位：减速控制，低伤害。\n要点：放在火力区前方或 Boss 前。");
    case TowerKind::Splash:
        return base + QStringLiteral("定位：范围伤害，附带持续灼蚀。\n要点：处理密集波和分裂点。");
    case TowerKind::Laser:
        return base + QStringLiteral("定位：直线穿透，多目标命中。\n要点：守长线、传送出口和终点直道。");
    case TowerKind::Resource:
        return QStringLiteral("费用 %1 / 收集时能 / 无攻击\n定位：周期性补经济。\n要点：早放有收益，但不能替代火力。").arg(spec.cost);
    case TowerKind::Wall:
        return QStringLiteral("费用 %1 / 路径屏障 / 生命 %2\n定位：阻挡和拖延。\n要点：拦 Boss 和漏怪，不能放起终点。")
            .arg(spec.cost)
            .arg(static_cast<int>(std::round(spec.hp)));
    }
    return QString();
}

QString GameWidget::towerButtonTooltip(TowerKind kind) const
{
    const TowerSpec &spec = m_towerSpecs[static_cast<size_t>(kind)];
    const Tower preview = makeTower(kind, QPoint(-1, -1));
    const int upgradeLv2 = upgradeCost(preview);
    Tower upgraded = preview;
    upgraded.level = 2;
    const int upgradeLv3 = upgradeCost(upgraded);
    const QString status = m_energy >= spec.cost
                               ? QStringLiteral("状态：时能充足，可部署")
                               : QStringLiteral("状态：时能不足");

    if (kind == TowerKind::Resource) {
        return QStringLiteral("%1\n费用：%2\n定位：经济装置，无攻击\n升级：Lv.2 %3 / Lv.3 %4\n%5")
            .arg(towerName(kind))
            .arg(spec.cost)
            .arg(upgradeLv2)
            .arg(upgradeLv3)
            .arg(status);
    }
    if (kind == TowerKind::Wall) {
        return QStringLiteral("%1\n费用：%2\n生命：%3\n升级：Lv.2 %4 / Lv.3 %5\n%6")
            .arg(towerName(kind))
            .arg(spec.cost)
            .arg(static_cast<int>(std::round(spec.hp)))
            .arg(upgradeLv2)
            .arg(upgradeLv3)
            .arg(status);
    }
    return QStringLiteral("%1\n费用：%2\n射程：%3  伤害：%4  冷却：%5 秒\n升级：Lv.2 %6 / Lv.3 %7\n%8")
        .arg(towerName(kind))
        .arg(spec.cost)
        .arg(static_cast<int>(std::round(spec.range)))
        .arg(static_cast<int>(std::round(spec.damage)))
        .arg(QString::number(spec.cooldown, 'f', 2))
        .arg(upgradeLv2)
        .arg(upgradeLv3)
        .arg(status);
}

QString GameWidget::enemyName(EnemyKind kind) const
{
    return EntityRules::enemy(kind).name();
}

QString GameWidget::enemyIntelText(EnemyKind kind) const
{
    switch (kind) {
    case EnemyKind::Normal:
        return QStringLiteral("生命中 / 速度中 / 炮台守拐角");
    case EnemyKind::Fast:
        return QStringLiteral("高速 / 怕减速 / 出口前放凝滞棱镜");
    case EnemyKind::Armored:
        return QStringLiteral("高生命 / 速度慢 / 优先升级输出");
    case EnemyKind::Resistant:
        return QStringLiteral("减伤 / 抗减速 / 多层火力覆盖");
    case EnemyKind::Splitter:
        return QStringLiteral("死亡分裂 / 怕范围 / 守住死亡点");
    case EnemyKind::Boss:
        return QStringLiteral("Boss / 护盾 / 冻结留给它");
    }
    return QString();
}

QColor GameWidget::towerColor(TowerKind kind) const
{
    return EntityRules::tower(kind).color();
}

QColor GameWidget::enemyColor(EnemyKind kind) const
{
    return EntityRules::enemy(kind).color();
}

QColor GameWidget::tileColor(TileKind kind) const
{
    switch (kind) {
    case TileKind::Stable: return QColor(20, 50, 64, 215);
    case TileKind::Path: return QColor(42, 76, 110, 230);
    case TileKind::Discount: return QColor(34, 86, 64, 230);
    case TileKind::Anchor: return QColor(48, 54, 62, 230);
    case TileKind::Accelerate: return QColor(44, 92, 132, 240);
    case TileKind::Portal: return QColor(82, 48, 118, 240);
    }
    return QColor(20, 50, 64, 215);
}

void GameWidget::drawBackground(QPainter &painter)
{
    if (!m_cachedBackground.isNull() && m_cachedBackgroundSize == size()) {
        painter.drawPixmap(0, 0, m_cachedBackground);
        return;
    }
    renderBackground(painter);
}

void GameWidget::renderBackground(QPainter &painter)
{
    if (!m_backgroundImage.isNull()) {
        drawCoverPixmap(painter, m_backgroundImage, rect());
        QLinearGradient veil(rect().topLeft(), rect().bottomRight());
        veil.setColorAt(0.0, QColor(4, 10, 24, 155));
        veil.setColorAt(0.55, QColor(4, 12, 26, 196));
        veil.setColorAt(1.0, QColor(8, 14, 26, 172));
        painter.fillRect(rect(), veil);
    } else {
    QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
    gradient.setColorAt(0.0, QColor(4, 10, 24));
    gradient.setColorAt(0.48, QColor(8, 24, 42));
    gradient.setColorAt(1.0, QColor(18, 18, 35));
    painter.fillRect(rect(), gradient);
    }

    QRadialGradient coreGlow(QPointF(width() * 0.72, height() * 0.18), width() * 0.58);
    coreGlow.setColorAt(0.0, QColor(58, 175, 218, 54));
    coreGlow.setColorAt(0.55, QColor(20, 72, 110, 18));
    coreGlow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(rect(), coreGlow);

    QRadialGradient goldGlow(QPointF(width() * 0.18, height() * 0.82), width() * 0.45);
    goldGlow.setColorAt(0.0, QColor(255, 204, 112, 34));
    goldGlow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.fillRect(rect(), goldGlow);

    painter.setPen(QPen(QColor(90, 190, 230, 26), 1));
    for (int x = 0; x < width(); x += 44) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += 44) {
        painter.drawLine(0, y, width(), y);
    }

    painter.setPen(QPen(QColor(255, 225, 154, 36), 1));
    for (int i = 0; i < 22; ++i) {
        const int x = (i * 137 + 41) % std::max(1, width());
        const int y = (i * 83 + 29) % std::max(1, height());
        painter.drawPoint(x, y);
        if (i % 5 == 0) {
            painter.drawEllipse(QPointF(x, y), 1.5, 1.5);
        }
    }
}

void GameWidget::drawHud(QPainter &painter)
{
    const double hudHeight = std::clamp(height() * 0.096, 78.0, 96.0);
    drawGlassPanel(painter, QRectF(24, 18, width() - 48, hudHeight), QColor(80, 212, 255));

    painter.setPen(QColor(255, 225, 154));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), std::clamp(static_cast<int>(height() * 0.025), 21, 26), QFont::Bold));
    painter.drawText(QRectF(42, 24, 320, 32), Qt::AlignLeft | Qt::AlignVCenter,
                     m_reverseMode ? m_levelName : QStringLiteral("时间修补局"));

    const int shownWave = std::min(m_currentWave + 1, static_cast<int>(m_waves.size()));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), std::clamp(static_cast<int>(height() * 0.014), 12, 15)));
    painter.setPen(QColor(187, 223, 255));
    const int actionLeft = m_pauseButton ? m_pauseButton->geometry().left() : width() - 330;
    const double energyWidth = std::clamp(width() * 0.145, 164.0, 220.0);
    const double energyX = std::max(460.0, static_cast<double>(actionLeft) - energyWidth - 22.0);
    const double subtitleWidth = std::max(250.0, energyX - 62.0);
    painter.drawText(QRectF(44, 54, subtitleWidth, 24), Qt::AlignLeft | Qt::AlignVCenter,
                     m_reverseMode
                         ? QStringLiteral("异常协议编排 | 波次 %1/%2 | 系统塔数 %3")
                               .arg(shownWave)
                               .arg(m_waves.size())
                               .arg(m_towers.size())
                         : QStringLiteral("%1 | 波次 %2/%3")
                               .arg(m_levelName)
                               .arg(shownWave)
                               .arg(m_waves.size()));

    QRectF energyPill(energyX, 31, energyWidth, std::clamp(height() * 0.044, 36.0, 46.0));
    QLinearGradient energyFill(energyPill.topLeft(), energyPill.bottomRight());
    energyFill.setColorAt(0.0, QColor(58, 184, 132, 150));
    energyFill.setColorAt(1.0, QColor(255, 225, 154, 80));
    painter.setBrush(energyFill);
    painter.setPen(QPen(QColor(255, 225, 154, 150), 1));
    painter.drawRoundedRect(energyPill, 8, 8);
    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), std::clamp(static_cast<int>(height() * 0.018), 15, 19), QFont::Bold));
    painter.drawText(energyPill, Qt::AlignCenter,
                     m_reverseMode
                         ? QStringLiteral("裂隙点数 %1").arg(m_reverseRiftPoints)
                         : QStringLiteral("时能储备 %1").arg(m_energy));

    const double hintRight = energyPill.left() - 14.0;
    const double hintLeft = 300.0;
    const double hintWidth = hintRight - hintLeft;
    if (hintWidth >= 210.0 && m_tutorialStep <= 0 && !m_towerDrillActive && !m_enemyGuideActive) {
        painter.setPen(QColor(151, 236, 255));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), std::clamp(static_cast<int>(height() * 0.013), 10, 14)));
        painter.drawText(QRectF(hintLeft, 31, hintWidth, 22), Qt::AlignRight | Qt::AlignVCenter,
                         m_reverseMode ? QStringLiteral("点击右侧异常，编排本波攻势")
                                       : QStringLiteral("/ 指令台 | Ctrl+R 回溯"));

        painter.setPen(m_reverseMode ? QColor(255, 225, 154)
                                     : (m_freezeSkillCooldown <= 0.0 ? QColor(255, 225, 154) : QColor(150, 170, 185)));
        painter.drawText(QRectF(hintLeft, 54, hintWidth, 22), Qt::AlignRight | Qt::AlignVCenter,
                         m_reverseMode
                             ? (m_reversePlanning ? QStringLiteral("系统待命：开始本波前自动布防")
                                                  : QStringLiteral("系统防守中：观察是否突破核心"))
                             : (m_freezeSkillCooldown <= 0.0
                                    ? QStringLiteral("时序冻结：就绪")
                                    : QStringLiteral("时序冻结：%1 秒").arg(static_cast<int>(std::ceil(m_freezeSkillCooldown)))));
    }

    const QRect panel = towerPanelRect();
    drawGlassPanel(painter, panel, QColor(80, 212, 255));
    painter.setPen(QColor(255, 225, 154));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), std::clamp(static_cast<int>(m_cellSize * 0.19), 14, 17), QFont::Bold));
    painter.drawText(QRectF(panel.x() + 12, panel.y() + 14, panel.width() - 24, 34),
                     Qt::AlignCenter, m_reverseMode ? QStringLiteral("异常投放库") : QStringLiteral("修补装置库"));

    updateTowerButtonStates();
}

void GameWidget::drawTowerButtonHoverCard(QPainter &painter)
{
    if (m_reverseMode) {
        const auto &order = reverseEnemyOrder();
        if (m_hoveredTowerButtonIndex < 0
            || m_hoveredTowerButtonIndex >= static_cast<int>(m_towerButtons.size())
            || m_hoveredTowerButtonIndex >= static_cast<int>(order.size())) {
            return;
        }
        const EnemyKind kind = order[static_cast<size_t>(m_hoveredTowerButtonIndex)];
        const QColor accent = enemyColor(kind);
        const QString role = [kind]() {
            switch (kind) {
            case EnemyKind::Normal: return QStringLiteral("定位：铺量压测");
            case EnemyKind::Fast: return QStringLiteral("定位：速度突防");
            case EnemyKind::Armored: return QStringLiteral("定位：高血消耗");
            case EnemyKind::Resistant: return QStringLiteral("定位：反控制突破");
            case EnemyKind::Splitter: return QStringLiteral("定位：分裂扰乱");
            case EnemyKind::Boss: return QStringLiteral("定位：终局核心");
            }
            return QString();
        }();
        const QString plan = [kind]() {
            switch (kind) {
            case EnemyKind::Normal: return QStringLiteral("编队：用 3-5 个测试系统火力密度。");
            case EnemyKind::Fast: return QStringLiteral("编队：夹在基础单位后，冲刺出口空窗。");
            case EnemyKind::Armored: return QStringLiteral("编队：放在队首吃伤害，掩护后续单位。");
            case EnemyKind::Resistant: return QStringLiteral("编队：专压减速区和凝滞塔覆盖点。");
            case EnemyKind::Splitter: return QStringLiteral("编队：跟随高血单位，制造清场压力。");
            case EnemyKind::Boss: return QStringLiteral("编队：终局波搭配速度或分裂单位同步进场。");
            }
            return QString();
        }();
        const QString unlock = reverseEnemyUnlocked(kind)
                                   ? QStringLiteral("授权：当前波可投放")
                                   : QStringLiteral("授权：继续推进波次后开放");
        const QPushButton *button = m_towerButtons[static_cast<size_t>(m_hoveredTowerButtonIndex)];
        const double cardWidth = std::clamp(width() * 0.34, 430.0, 520.0);
        const double cardHeight = 244.0;
        QRectF card(towerPanelRect().left() - cardWidth - 16.0,
                    button->geometry().center().y() - cardHeight / 2.0,
                    cardWidth,
                    cardHeight);
        if (card.left() < 28.0) {
            card.moveLeft(28.0);
        }
        if (card.top() < 104.0) {
            card.moveTop(104.0);
        }
        if (card.bottom() > height() - 28.0) {
            card.moveBottom(height() - 28.0);
        }
        drawHoverPanel(painter, card, accent);

        const int iconIndex = static_cast<int>(kind);
        if (iconIndex >= 0 && iconIndex < static_cast<int>(m_scaledEnemyIcons.size()) && !m_scaledEnemyIcons[static_cast<size_t>(iconIndex)].isNull()) {
            const QRectF iconBox(card.left() + 20, card.top() + 24, 62, 62);
            painter.setPen(QPen(alphaColor(accent, 135), 1));
            painter.setBrush(QColor(4, 12, 24, 175));
            painter.drawRoundedRect(iconBox, 8, 8);
            const QPixmap &icon = m_scaledEnemyIcons[static_cast<size_t>(iconIndex)];
            const QPixmap scaled = icon.scaled(52, 52, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            painter.drawPixmap(QPointF(iconBox.center().x() - scaled.width() / 2.0,
                                       iconBox.center().y() - scaled.height() / 2.0),
                               scaled);
        }

        painter.setPen(QColor(255, 225, 154));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 20, QFont::Bold));
        painter.drawText(QRectF(card.left() + 98, card.top() + 22, card.width() - 122, 34),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         enemyName(kind));
        painter.setPen(QColor(accent.red(), accent.green(), accent.blue(), 235));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));
        painter.drawText(QRectF(card.left() + 98, card.top() + 54, card.width() - 122, 22),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         role);
        painter.setPen(QColor(223, 248, 255));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12));
        painter.drawText(QRectF(card.left() + 98, card.top() + 80, card.width() - 122, 48),
                         Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                         reverseEnemyHint(kind));
        painter.setPen(QColor(255, 245, 196));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));
        painter.drawText(QRectF(card.left() + 20, card.top() + 138, card.width() - 40, 72),
                         Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                         QStringLiteral("消耗 %1 裂隙点。\n%2\n%3")
                             .arg(reverseEnemyCost(kind))
                             .arg(unlock, plan));
        return;
    }

    const auto &order = EntityRules::towerBuildOrder();
    if (m_hoveredTowerButtonIndex < 0
        || m_hoveredTowerButtonIndex >= static_cast<int>(m_towerButtons.size())
        || m_hoveredTowerButtonIndex >= static_cast<int>(order.size())) {
        return;
    }

    const TowerKind kind = order[static_cast<size_t>(m_hoveredTowerButtonIndex)];
    const bool unlocked = isTowerUnlocked(kind);
    const TowerSpec &spec = m_towerSpecs[static_cast<size_t>(kind)];
    const int baseCost = spec.cost;
    const bool affordable = unlocked && m_energy >= baseCost;
    const QColor accent = unlocked ? towerColor(kind) : QColor(118, 128, 145);
    const QPushButton *button = m_towerButtons[static_cast<size_t>(m_hoveredTowerButtonIndex)];

    const double cardWidth = std::clamp(width() * 0.34, 410.0, 500.0);
    const double cardHeight = 256.0;
    QRectF card(towerPanelRect().left() - cardWidth - 16.0,
                button->geometry().center().y() - cardHeight / 2.0,
                cardWidth,
                cardHeight);
    if (card.left() < 28.0) {
        card.moveLeft(28.0);
    }
    if (card.top() < 104.0) {
        card.moveTop(104.0);
    }
    if (card.bottom() > height() - 28.0) {
        card.moveBottom(height() - 28.0);
    }

    drawHoverPanel(painter, card, accent);

    const int iconIndex = static_cast<int>(kind);
    const bool hasIcon = iconIndex >= 0
                         && iconIndex < static_cast<int>(m_scaledTowerIcons.size())
                         && !m_scaledTowerIcons[static_cast<size_t>(iconIndex)].isNull();
    const QRectF iconBox(card.left() + 20, card.top() + 22, 66, 66);
    painter.setPen(QPen(alphaColor(accent, 135), 1));
    painter.setBrush(QColor(4, 12, 24, 175));
    painter.drawRoundedRect(iconBox, 8, 8);
    if (hasIcon) {
        const QPixmap &icon = m_scaledTowerIcons[static_cast<size_t>(iconIndex)];
        const QSize targetSize(54, 54);
        const QPixmap scaled = icon.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap(QPointF(iconBox.center().x() - scaled.width() / 2.0,
                                   iconBox.center().y() - scaled.height() / 2.0),
                           scaled);
    } else {
        painter.setPen(QPen(accent.lighter(150), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(iconBox.center(), 18, 18);
    }

    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 18, QFont::Bold));
    painter.drawText(QRectF(card.left() + 104, card.top() + 18, card.width() - 228, 36),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     towerName(kind));

    const QString state = !unlocked
                              ? QStringLiteral("权限锁定")
                              : affordable ? QStringLiteral("可部署")
                                           : QStringLiteral("时能不足");
    const QColor stateColor = !unlocked ? QColor(150, 165, 178)
                          : affordable ? QColor(95, 220, 155)
                                       : QColor(255, 196, 115);
    QRectF statePill(card.right() - 116, card.top() + 24, 92, 28);
    painter.setPen(QPen(alphaColor(stateColor, 160), 1));
    painter.setBrush(alphaColor(stateColor.darker(165), 70));
    painter.drawRoundedRect(statePill, 12, 12);
    painter.setPen(stateColor.lighter(125));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 10, QFont::Bold));
    painter.drawText(statePill, Qt::AlignCenter, state);

    painter.setPen(QColor(190, 225, 240));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Medium));
    painter.drawText(QRectF(card.left() + 104, card.top() + 58, card.width() - 128, 48),
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     towerDrillRoleText(kind));

    const Tower preview = makeTower(kind, QPoint(-1, -1));
    Tower upgraded = preview;
    upgraded.level = 2;
    const int upgradeLv2 = upgradeCost(preview);
    const int upgradeLv3 = upgradeCost(upgraded);
    const QString primaryStats = kind == TowerKind::Resource
                                     ? QStringLiteral("费用 %1    收集时能    无攻击").arg(baseCost)
                                 : kind == TowerKind::Wall
                                     ? QStringLiteral("费用 %1    生命 %2    路径阻挡").arg(baseCost).arg(static_cast<int>(std::round(spec.hp)))
                                     : QStringLiteral("费用 %1    射程 %2    伤害 %3").arg(baseCost)
                                           .arg(static_cast<int>(std::round(spec.range)))
                                           .arg(static_cast<int>(std::round(spec.damage)));
    const QString availability = unlocked
                                     ? affordable
                                           ? QStringLiteral("当前状态：时能充足，可立即部署")
                                           : QStringLiteral("当前状态：时能不足，按钮保持灰暗")
                                     : QStringLiteral("解锁条件：通关第 %1 关主线").arg(towerUnlockLevel(kind) + 1);
    const QRectF infoPanel(card.left() + 18, card.top() + 118, card.width() - 36, 112);
    painter.setPen(QPen(alphaColor(accent, 72), 1));
    painter.setBrush(QColor(4, 12, 24, 132));
    painter.drawRoundedRect(infoPanel, 8, 8);

    const QRectF info(infoPanel.left() + 16, infoPanel.top() + 12, infoPanel.width() - 32, infoPanel.height() - 22);
    painter.setPen(QColor(223, 248, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 10, QFont::Medium));
    painter.drawText(info,
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     QStringLiteral("%1\n升级费用：Lv.2 %2    Lv.3 %3\n%4")
                         .arg(primaryStats)
                         .arg(upgradeLv2)
                         .arg(upgradeLv3)
                         .arg(availability));
}

void GameWidget::drawMap(QPainter &painter)
{
    const double boardPad = std::max(10.0, scaledDistance(10.0));
    QRectF board(m_origin.x() - boardPad,
                 m_origin.y() - boardPad,
                 m_cols * m_cellSize + boardPad * 2.0,
                 m_rows * m_cellSize + boardPad * 2.0);
    drawGlassPanel(painter, board, QColor(255, 225, 154));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(3, 8, 18, 112));
    const double innerPad = std::max(7.0, scaledDistance(7.0));
    painter.drawRoundedRect(board.adjusted(innerPad, innerPad, -innerPad, -innerPad), scaledDistance(7.0), scaledDistance(7.0));

    painter.setPen(QPen(QColor(80, 212, 255, 58), std::max(1.0, scaledDistance(1.0))));
    for (int col = 0; col <= m_cols; ++col) {
        const double x = m_origin.x() + col * m_cellSize;
        painter.drawLine(QPointF(x, board.top() + innerPad), QPointF(x, board.top() + scaledDistance(15.0)));
        painter.drawLine(QPointF(x, board.bottom() - innerPad), QPointF(x, board.bottom() - scaledDistance(15.0)));
    }
    for (int row = 0; row <= m_rows; ++row) {
        const double y = m_origin.y() + row * m_cellSize;
        painter.drawLine(QPointF(board.left() + innerPad, y), QPointF(board.left() + scaledDistance(15.0), y));
        painter.drawLine(QPointF(board.right() - innerPad, y), QPointF(board.right() - scaledDistance(15.0), y));
    }

    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            const double tileInset = std::max(2.0, scaledDistance(2.0));
            const QRectF r = cellRect(row, col).adjusted(tileInset, tileInset, -tileInset, -tileInset);
            const TileKind kind = m_tiles[static_cast<size_t>(row * m_cols + col)];
            QLinearGradient tileFill(r.topLeft(), r.bottomRight());
            tileFill.setColorAt(0.0, tileColor(kind).lighter(118));
            tileFill.setColorAt(1.0, tileColor(kind).darker(128));
            painter.setBrush(tileFill);
            painter.setPen(QPen(kind == TileKind::Path || kind == TileKind::Portal
                                    ? QColor(255, 225, 154, 130)
                                    : QColor(83, 185, 214, 85),
                                std::max(1.0, scaledDistance(1.0))));
            painter.drawRoundedRect(r, scaledDistance(6.0), scaledDistance(6.0));

            const int tileIconIndex = static_cast<int>(kind);
            if (tileIconIndex >= 0
                && tileIconIndex < static_cast<int>(m_scaledTileIcons.size())
                && !m_scaledTileIcons[static_cast<size_t>(tileIconIndex)].isNull()) {
                painter.save();
                const double iconOpacity = kind == TileKind::Stable ? 0.22
                                         : kind == TileKind::Path ? 0.36
                                         : kind == TileKind::Portal ? 0.54
                                         : 0.44;
                painter.setOpacity(iconOpacity);
                const QPixmap &icon = m_scaledTileIcons[static_cast<size_t>(tileIconIndex)];
                const QPointF topLeft(r.center().x() - icon.width() / 2.0,
                                      r.center().y() - icon.height() / 2.0);
                painter.drawPixmap(topLeft, icon);
                painter.restore();
            }

            painter.setPen(QPen(QColor(255, 238, 180, 145), std::max(1.5, scaledDistance(2.0))));
            if (kind == TileKind::Discount) {
                painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), std::clamp(static_cast<int>(m_cellSize * 0.15), 8, 14), QFont::Bold));
                painter.drawText(r, Qt::AlignCenter, QStringLiteral("-30%"));
            } else if (kind == TileKind::Anchor) {
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(r.center(), m_cellSize * 0.17, m_cellSize * 0.17);
                painter.drawLine(r.center() + QPointF(-scaledDistance(8.0), 0), r.center() + QPointF(scaledDistance(8.0), 0));
                painter.drawLine(r.center() + QPointF(0, -scaledDistance(8.0)), r.center() + QPointF(0, scaledDistance(8.0)));
            } else if (kind == TileKind::Accelerate) {
                painter.drawLine(r.center() + QPointF(-scaledDistance(14.0), scaledDistance(9.0)), r.center() + QPointF(scaledDistance(13.0), 0));
                painter.drawLine(r.center() + QPointF(-scaledDistance(14.0), -scaledDistance(9.0)), r.center() + QPointF(scaledDistance(13.0), 0));
                painter.drawLine(r.center() + QPointF(-scaledDistance(2.0), scaledDistance(9.0)), r.center() + QPointF(scaledDistance(25.0), 0));
                painter.drawLine(r.center() + QPointF(-scaledDistance(2.0), -scaledDistance(9.0)), r.center() + QPointF(scaledDistance(25.0), 0));
            } else if (kind == TileKind::Portal) {
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(r.center(), m_cellSize * 0.25, m_cellSize * 0.25);
                painter.drawEllipse(r.center(), m_cellSize * 0.13, m_cellSize * 0.13);
                const double portalInset = scaledDistance(10.0);
                painter.drawArc(r.adjusted(portalInset, portalInset, -portalInset, -portalInset), 20 * 16, 250 * 16);
            }
        }
    }

    if (m_portalFrom.x() >= 0 && m_portalTo.x() >= 0 && m_portalFrom != m_portalTo) {
        const QPointF from = cellCenter(m_portalFrom);
        const QPointF to = cellCenter(m_portalTo);
        const QPointF delta = to - from;
        const QPointF direction = normalized(delta);
        const QPointF perp = normalized(QPointF(-delta.y(), delta.x()));
        const double bend = std::clamp(distance(from, to) * 0.22, scaledDistance(34.0), scaledDistance(94.0));
        QPainterPath link(from);
        link.cubicTo(from + delta * 0.24 + perp * bend,
                     to - delta * 0.24 + perp * bend,
                     to);

        painter.save();
        painter.setBrush(Qt::NoBrush);
        QPen glow(QColor(174, 96, 255, 46),
                  std::max(8.0, scaledDistance(9.0)),
                  Qt::DashLine,
                  Qt::RoundCap,
                  Qt::RoundJoin);
        glow.setDashPattern({scaledDistance(5.0), scaledDistance(12.0)});
        glow.setDashOffset(-m_elapsedSeconds * scaledDistance(18.0));
        painter.setPen(glow);
        painter.drawPath(link);

        QPen linkPen(QColor(122, 232, 255, 152),
                     std::max(2.0, scaledDistance(2.6)),
                     Qt::DashLine,
                     Qt::RoundCap,
                     Qt::RoundJoin);
        linkPen.setDashPattern({scaledDistance(7.0), scaledDistance(10.0)});
        linkPen.setDashOffset(-m_elapsedSeconds * scaledDistance(34.0));
        painter.setPen(linkPen);
        painter.drawPath(link);

        const QPointF arrowBase = to - direction * scaledDistance(18.0);
        const QPointF side(-direction.y(), direction.x());
        QPolygonF arrow;
        arrow << to - direction * scaledDistance(8.0)
              << arrowBase - direction * scaledDistance(12.0) + side * scaledDistance(7.0)
              << arrowBase - direction * scaledDistance(12.0) - side * scaledDistance(7.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(122, 232, 255, 170));
        painter.drawPolygon(arrow);

        const double particleT = 0.5 + std::sin(m_elapsedSeconds * 2.8) * 0.5;
        const QPointF particle = link.pointAtPercent(std::clamp(particleT, 0.0, 1.0));
        QRadialGradient spark(particle, scaledDistance(11.0));
        spark.setColorAt(0.0, QColor(255, 225, 154, 210));
        spark.setColorAt(1.0, QColor(122, 232, 255, 0));
        painter.setBrush(spark);
        painter.drawEllipse(particle, scaledDistance(11.0), scaledDistance(11.0));
        painter.restore();
    }

    auto isPortalJumpSegment = [this](const QPoint &a, const QPoint &b) {
        return a == m_portalFrom && b == m_portalTo;
    };

    painter.setPen(QPen(QColor(255, 219, 122, 58), std::max(10.0, scaledDistance(12.0)), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (size_t i = 1; i < m_pathCells.size(); ++i) {
        if (isPortalJumpSegment(m_pathCells[i - 1], m_pathCells[i])) {
            continue;
        }
        painter.drawLine(cellCenter(m_pathCells[i - 1]), cellCenter(m_pathCells[i]));
    }
    painter.setPen(QPen(QColor(255, 219, 122, 210), std::max(3.0, scaledDistance(4.0)), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    for (size_t i = 1; i < m_pathCells.size(); ++i) {
        if (isPortalJumpSegment(m_pathCells[i - 1], m_pathCells[i])) {
            continue;
        }
        painter.drawLine(cellCenter(m_pathCells[i - 1]), cellCenter(m_pathCells[i]));
    }
    QPen flowPen(QColor(94, 235, 255, 155), std::max(2.0, scaledDistance(2.6)), Qt::CustomDashLine, Qt::RoundCap, Qt::RoundJoin);
    flowPen.setDashPattern({scaledDistance(9.0), scaledDistance(12.0)});
    flowPen.setDashOffset(-m_elapsedSeconds * scaledDistance(34.0));
    painter.setPen(flowPen);
    for (size_t i = 1; i < m_pathCells.size(); ++i) {
        if (isPortalJumpSegment(m_pathCells[i - 1], m_pathCells[i])) {
            continue;
        }
        painter.drawLine(cellCenter(m_pathCells[i - 1]), cellCenter(m_pathCells[i]));
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 238, 168, 185));
    for (size_t i = 1; i < m_pathCells.size(); ++i) {
        if (isPortalJumpSegment(m_pathCells[i - 1], m_pathCells[i])) {
            continue;
        }
        const QPointF a = cellCenter(m_pathCells[i - 1]);
        const QPointF b = cellCenter(m_pathCells[i]);
        const QPointF dir = normalized(b - a);
        const QPointF perp(-dir.y(), dir.x());
        const double t = 0.48 + std::sin(m_elapsedSeconds * 3.4 + static_cast<double>(i) * 0.85) * 0.12;
        const QPointF p = a + (b - a) * t;
        const double arrowLength = scaledDistance(10.0);
        const double arrowWidth = scaledDistance(5.0);
        QPolygonF arrow;
        arrow << p + dir * arrowLength
              << p - dir * arrowLength * 0.58 + perp * arrowWidth
              << p - dir * arrowLength * 0.58 - perp * arrowWidth;
        painter.drawPolygon(arrow);
    }

    painter.setPen(Qt::NoPen);
    const double startRadius = scaledDistance(24.0);
    QRadialGradient startGlow(cellCenter(m_pathCells.front()), startRadius);
    startGlow.setColorAt(0.0, QColor(78, 218, 255, 230));
    startGlow.setColorAt(1.0, QColor(78, 218, 255, 0));
    painter.setBrush(startGlow);
    painter.drawEllipse(cellCenter(m_pathCells.front()), startRadius, startRadius);
    painter.setBrush(QColor(78, 218, 255, 230));
    painter.drawEllipse(cellCenter(m_pathCells.front()), scaledDistance(8.0), scaledDistance(8.0));

    const double coreRadius = scaledDistance(32.0);
    QRadialGradient coreGlow(cellCenter(m_pathCells.back()), coreRadius);
    coreGlow.setColorAt(0.0, QColor(255, 211, 119, 245));
    coreGlow.setColorAt(0.5, QColor(255, 105, 128, 100));
    coreGlow.setColorAt(1.0, QColor(255, 105, 128, 0));
    painter.setBrush(coreGlow);
    painter.drawEllipse(cellCenter(m_pathCells.back()), coreRadius, coreRadius);
    painter.setBrush(QColor(255, 211, 119, 240));
    painter.drawPolygon(diamondAt(cellCenter(m_pathCells.back()), scaledDistance(13.0)));

    if (m_hoverCell.x() >= 0 && !m_finished && towerIndexAt(m_hoverCell) < 0) {
        const bool legal = canBuildAt(m_hoverCell, m_selectedTower);
        const QRectF hoverRect = cellRect(m_hoverCell.y(), m_hoverCell.x()).adjusted(3, 3, -3, -3);
        const QColor hoverColor = legal ? QColor(94, 235, 255) : QColor(255, 92, 132);
        painter.setPen(QPen(alphaColor(hoverColor, legal ? 230 : 190), std::max(2.0, scaledDistance(2.2))));
        painter.setBrush(alphaColor(hoverColor, legal ? 42 : 34));
        painter.drawRoundedRect(hoverRect, 7, 7);
        if (legal) {
            const double range = scaledDistance(m_towerSpecs[static_cast<size_t>(m_selectedTower)].range);
            if (range > 0.0) {
                painter.setPen(QPen(QColor(94, 235, 255, 72), std::max(1.0, scaledDistance(1.4)), Qt::DashLine));
                painter.setBrush(Qt::NoBrush);
                painter.drawEllipse(cellCenter(m_hoverCell), range, range);
            }
            painter.setPen(QPen(QColor(255, 225, 154, 180), std::max(1.0, scaledDistance(1.2))));
            painter.setBrush(QColor(4, 12, 24, 130));
            painter.drawPolygon(diamondAt(cellCenter(m_hoverCell), m_cellSize * 0.22));
        }
    }
}

void GameWidget::drawTowers(QPainter &painter)
{
    for (const Tower &tower : m_towers) {
        const QPointF c = cellCenter(tower.cell);
        const double range = scaledTowerRange(tower);
        if (range > 0.0) {
            painter.setPen(QPen(QColor(92, 213, 255, 34), std::max(1.5, scaledDistance(2.0)), Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(c, range, range);
        }

        if (tower.kind == TowerKind::Laser && tower.cooldownLeft > tower.cooldown * 0.68) {
            if (Enemy *target = nearestEnemy(c, range)) {
                const QPointF targetPos = pathPosition(target->progress);
                painter.setPen(QPen(QColor(255, 225, 154, 70), std::max(6.0, scaledDistance(10.0)), Qt::SolidLine, Qt::RoundCap));
                painter.drawLine(c, targetPos);
                painter.setPen(QPen(QColor(255, 248, 205, 220), std::max(2.0, scaledDistance(3.0)), Qt::SolidLine, Qt::RoundCap));
                painter.drawLine(c, targetPos);
            }
        }
        if (tower.kind == TowerKind::Resource) {
            const double pulse = 0.35 + std::abs(std::sin(m_elapsedSeconds * 3.0 + tower.cell.x())) * 0.65;
            painter.setPen(QPen(QColor(95, 220, 155, 95), std::max(1.5, scaledDistance(2.0))));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(c, scaledDistance(17.0 + 7.0 * pulse), scaledDistance(17.0 + 7.0 * pulse));
        }
        if (tower.kind == TowerKind::Slow && tower.cooldownLeft > tower.cooldown * 0.55) {
            painter.setPen(QPen(QColor(105, 210, 255, 90), std::max(2.0, scaledDistance(3.0))));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(c, scaledDistance(28.0), scaledDistance(28.0));
        }

        const QColor base = towerColor(tower.kind);
        const double idle = 0.5 + std::sin(m_elapsedSeconds * 2.4 + tower.cell.x() * 0.7 + tower.cell.y()) * 0.5;
        painter.setPen(QPen(QColor(255, 225, 154, static_cast<int>(42 + idle * 72)),
                            std::max(1.0, scaledDistance(1.5)),
                            Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(c,
                            scaledDistance(24.0 + idle * 5.0),
                            scaledDistance(10.0 + idle * 2.0));
        if (tower.level >= 2) {
            const int levelAlpha = tower.level >= 3 ? 150 : 105;
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(alphaColor(base.lighter(170), levelAlpha),
                                std::max(1.5, scaledDistance(2.0)),
                                Qt::SolidLine));
            painter.drawEllipse(c,
                                scaledDistance(26.0 + tower.level * 2.5 + idle * 3.0),
                                scaledDistance(26.0 + tower.level * 2.5 + idle * 3.0));
            painter.setPen(QPen(QColor(255, 225, 154, tower.level >= 3 ? 160 : 95),
                                std::max(1.0, scaledDistance(1.6)),
                                Qt::DashLine));
            painter.drawEllipse(c,
                                scaledDistance(31.0 + idle * 4.0),
                                scaledDistance(13.0 + idle * 2.0));
        }
        if (tower.level >= 3) {
            const double spin = m_elapsedSeconds * 2.35 + tower.cell.x() * 0.6 + tower.cell.y() * 0.35;
            QRadialGradient ascensionGlow(c, scaledDistance(36.0));
            ascensionGlow.setColorAt(0.0, QColor(255, 245, 190, 105));
            ascensionGlow.setColorAt(0.55, alphaColor(base.lighter(190), 54));
            ascensionGlow.setColorAt(1.0, alphaColor(base, 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(ascensionGlow);
            painter.drawEllipse(c, scaledDistance(38.0), scaledDistance(38.0));

            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(QColor(255, 225, 154, 170),
                                std::max(1.4, scaledDistance(1.8)),
                                Qt::SolidLine,
                                Qt::RoundCap));
            for (int i = 0; i < 3; ++i) {
                const double a1 = spin + i * 2.09439510239;
                const double a2 = a1 + 0.72;
                const QPointF start(c.x() + std::cos(a1) * scaledDistance(36.0),
                                    c.y() + std::sin(a1) * scaledDistance(15.0));
                const QPointF end(c.x() + std::cos(a2) * scaledDistance(36.0),
                                  c.y() + std::sin(a2) * scaledDistance(15.0));
                painter.drawLine(start, end);
                painter.setBrush(QColor(255, 245, 190, 185));
                painter.drawEllipse(start, scaledDistance(2.8), scaledDistance(2.8));
                painter.setBrush(Qt::NoBrush);
            }

            painter.setPen(QPen(QColor(255, 245, 190, 130),
                                std::max(1.0, scaledDistance(1.3)),
                                Qt::DashLine));
            painter.drawEllipse(c,
                                scaledDistance(40.0 + idle * 4.0),
                                scaledDistance(17.0 + idle * 2.5));
        }
        QRadialGradient glow(c, scaledDistance(tower.kind == TowerKind::Wall ? 34.0 : 30.0));
        glow.setColorAt(0.0, alphaColor(base.lighter(150), 120));
        glow.setColorAt(1.0, alphaColor(base, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(c,
                            scaledDistance(tower.kind == TowerKind::Wall ? 35.0 : 31.0),
                            scaledDistance(tower.kind == TowerKind::Wall ? 35.0 : 31.0));

        const int towerIconIndex = static_cast<int>(tower.kind);
        const bool useAiTower = towerIconIndex >= 0
                                && towerIconIndex < static_cast<int>(m_scaledTowerIcons.size())
                                && !m_scaledTowerIcons[static_cast<size_t>(towerIconIndex)].isNull();
        if (useAiTower) {
            const QPixmap &icon = m_scaledTowerIcons[static_cast<size_t>(towerIconIndex)];
            const double yOffset = towerIconVerticalOffset(tower.kind, m_cellSize);
            const QRectF target(c.x() - icon.width() / 2.0,
                                c.y() - icon.height() / 2.0,
                                icon.width(),
                                icon.height());
            painter.setPen(QPen(QColor(255, 225, 154, 95), std::max(1.0, scaledDistance(1.0))));
            painter.setBrush(QColor(4, 10, 20, 120));
            const double frameInset = std::max(3.0, scaledDistance(3.0));
            painter.drawRoundedRect(target.adjusted(frameInset, frameInset, -frameInset, -frameInset),
                                    scaledDistance(8.0),
                                    scaledDistance(8.0));
            painter.drawPixmap(target.topLeft() + QPointF(0, yOffset), icon);
        } else {
        QLinearGradient body(c + QPointF(-20, -20), c + QPointF(20, 22));
        body.setColorAt(0.0, base.lighter(135));
        body.setColorAt(1.0, base.darker(135));
        painter.setBrush(body);
        painter.setPen(QPen(QColor(220, 244, 255, 210), 2));
        if (tower.kind == TowerKind::Wall) {
            painter.drawRoundedRect(QRectF(c.x() - 22, c.y() - 22, 44, 44), 4, 4);
        } else {
            painter.drawPolygon(diamondAt(c, 23));
        }

        painter.setPen(QPen(QColor(255, 235, 176), 3, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        if (tower.kind == TowerKind::Laser) {
            painter.drawLine(c + QPointF(-13, -13), c + QPointF(13, 13));
            painter.drawLine(c + QPointF(-13, 13), c + QPointF(13, -13));
        } else if (tower.kind == TowerKind::Resource) {
            painter.drawEllipse(c, 10, 10);
            painter.drawEllipse(c, 5, 5);
        } else if (tower.kind == TowerKind::Splash) {
            painter.drawEllipse(c, 12, 12);
            painter.drawEllipse(c, 5, 5);
        } else if (tower.kind == TowerKind::Slow) {
            painter.drawArc(QRectF(c.x() - 13, c.y() - 13, 26, 26), 20 * 16, 270 * 16);
            painter.drawLine(c, c + QPointF(0, -11));
        } else {
            painter.drawLine(c + QPointF(-10, 0), c + QPointF(10, 0));
            painter.drawLine(c + QPointF(0, -10), c + QPointF(0, 10));
        }
        }

        if (tower.hp < tower.maxHp) {
            const double ratio = std::clamp(tower.hp / tower.maxHp, 0.0, 1.0);
            QRectF bar(c.x() - scaledDistance(22.0),
                       c.y() + scaledDistance(27.0),
                       scaledDistance(44.0),
                       std::max(5.0, scaledDistance(5.0)));
            drawStatusBar(painter, bar, ratio, QColor(95, 220, 155));
        }

        const QRectF levelBadge(c.x() + scaledDistance(8.0),
                                c.y() - scaledDistance(32.0),
                                std::max(34.0, scaledDistance(34.0)),
                                std::max(16.0, scaledDistance(16.0)));
        const QColor badgeFill = tower.level >= 3 ? QColor(72, 38, 13, 235)
                               : tower.level == 2 ? QColor(21, 44, 54, 235)
                                                  : QColor(8, 18, 31, 230);
        painter.setPen(QPen(tower.level >= 2 ? QColor(255, 225, 154, 235)
                                             : QColor(255, 225, 154, 210),
                            std::max(1.0, scaledDistance(1.0))));
        painter.setBrush(badgeFill);
        painter.drawRoundedRect(levelBadge, scaledDistance(4.0), scaledDistance(4.0));
        painter.setPen(QColor(255, 245, 190));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), std::clamp(static_cast<int>(m_cellSize * 0.14), 8, 13), QFont::Bold));
        painter.drawText(levelBadge, Qt::AlignCenter, QStringLiteral("Lv%1").arg(tower.level));
    }

    const int hoverTowerIndex = m_hoverCell.x() >= 0 ? towerIndexAt(m_hoverCell) : -1;
    if (hoverTowerIndex >= 0 && hoverTowerIndex < static_cast<int>(m_towers.size()) && !m_finished) {
        const Tower &tower = m_towers[static_cast<size_t>(hoverTowerIndex)];
        const QPointF c = cellCenter(tower.cell);
        const bool maxLevel = tower.level >= 3;
        const int cost = maxLevel ? 0 : upgradeCost(tower);
        const bool enough = maxLevel || m_energy >= cost;
        const QColor accent = maxLevel ? QColor(255, 225, 154)
                            : enough ? QColor(95, 220, 155)
                                     : QColor(160, 180, 194);
        QRectF card(c.x() + scaledDistance(28.0), c.y() - scaledDistance(64.0), 258.0, 96.0);
        if (card.right() > width() - 18.0) {
            card.moveLeft(c.x() - card.width() - scaledDistance(28.0));
        }
        if (card.top() < 96.0) {
            card.moveTop(c.y() + scaledDistance(32.0));
        }
        drawHoverPanel(painter, card, accent);
        painter.setPen(QColor(255, 245, 196));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));
        painter.drawText(card.adjusted(18, 10, -18, -62),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("%1  Lv.%2").arg(towerName(tower.kind)).arg(tower.level));

        const QString tag = maxLevel ? QStringLiteral("满级")
                          : enough ? QStringLiteral("可升级")
                                   : QStringLiteral("时能不足");
        QRectF tagRect(card.right() - 78, card.top() + 12, 58, 22);
        painter.setPen(QPen(alphaColor(accent, 150), 1));
        painter.setBrush(alphaColor(accent.darker(160), 72));
        painter.drawRoundedRect(tagRect, 11, 11);
        painter.setPen(accent.lighter(140));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9, QFont::Bold));
        painter.drawText(tagRect, Qt::AlignCenter, tag);

        painter.setPen(enough ? QColor(223, 248, 255) : QColor(178, 194, 205));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 10, QFont::Medium));
        const QString body = maxLevel
                                 ? QStringLiteral("已达最高等级")
                                 : enough
                                       ? QStringLiteral("右键升级：消耗 %1 时能").arg(cost)
                                       : QStringLiteral("升级需要 %1 时能").arg(cost);
        painter.drawText(card.adjusted(18, 46, -18, -12),
                         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                         body);
    }
}

void GameWidget::drawEnemies(QPainter &painter)
{
    for (const auto &enemy : m_enemies) {
        const QPointF c = pathPosition(enemy->progress);
        const QColor base = enemyColor(enemy->kind);
        const bool isBoss = enemy->kind == EnemyKind::Boss;
        const double radius = isBoss
                                  ? std::clamp(m_cellSize * 0.39, 24.0, 64.0)
                                  : std::clamp(m_cellSize * 0.28, 16.0, 44.0);
        const double markerRadius = isBoss
                                        ? std::clamp(m_cellSize * 0.62, 38.0, 96.0)
                                        : std::clamp(m_cellSize * 0.46, 28.0, 72.0);
        const double iconSize = isBoss
                                    ? std::clamp(m_cellSize * 1.16, 74.0, 178.0)
                                    : std::clamp(m_cellSize * 0.82, 52.0, 132.0);
        const double strokeWidth = std::clamp(m_cellSize * 0.035, 2.0, 6.0);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 145));
        painter.drawEllipse(c + QPointF(markerRadius * 0.11, markerRadius * 0.21),
                            markerRadius * 0.92,
                            markerRadius * 0.42);

        QRadialGradient threatGlow(c, markerRadius);
        threatGlow.setColorAt(0.0, alphaColor(base.lighter(150), isBoss ? 170 : 120));
        threatGlow.setColorAt(0.62, alphaColor(base, isBoss ? 72 : 50));
        threatGlow.setColorAt(1.0, alphaColor(base, 0));
        painter.setBrush(threatGlow);
        painter.drawEllipse(c, markerRadius, markerRadius);

        painter.setBrush(QColor(4, 8, 18, isBoss ? 178 : 150));
        painter.setPen(QPen(alphaColor(base.lighter(165), isBoss ? 230 : 205),
                            isBoss ? strokeWidth + 1.0 : strokeWidth));
        painter.drawEllipse(c, markerRadius * 0.76, markerRadius * 0.76);

        painter.setPen(QPen(QColor(255, 225, 154, isBoss ? 160 : 95), std::max(1.0, strokeWidth * 0.45)));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(c, markerRadius * 0.54, markerRadius * 0.54);
        const double orbit = 0.5 + std::sin(m_elapsedSeconds * (isBoss ? 2.0 : 3.4) + enemy->id * 0.47) * 0.5;
        painter.setPen(QPen(QColor(255, 255, 255, static_cast<int>(34 + orbit * 74)),
                            std::max(1.0, strokeWidth * 0.35),
                            Qt::DashLine));
        painter.drawEllipse(c, markerRadius * (0.88 + orbit * 0.08), markerRadius * (0.28 + orbit * 0.04));

        if (enemy->slowTime > 0.0) {
            painter.setPen(QPen(QColor(105, 210, 255, 170), strokeWidth, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(c, radius + m_cellSize * 0.12, radius + m_cellSize * 0.12);
        }
        if (enemy->burnTime > 0.0) {
            painter.setPen(QPen(QColor(196, 90, 255, 150), strokeWidth));
            painter.drawLine(c + QPointF(-radius, -radius), c + QPointF(radius, radius));
        }
        if (enemy->shield > 0.0) {
            painter.setPen(QPen(QColor(255, 225, 154, 190), strokeWidth + 1.0));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(c, radius + m_cellSize * 0.07, radius + m_cellSize * 0.07);
            const double shieldRadius = radius + m_cellSize * 0.14;
            painter.drawArc(QRectF(c.x() - shieldRadius,
                                   c.y() - shieldRadius,
                                   shieldRadius * 2,
                                   shieldRadius * 2),
                            40 * 16, 230 * 16);
        }

        const int enemyIconIndex = static_cast<int>(enemy->kind);
        const bool useAiEnemy = enemyIconIndex >= 0
                                && enemyIconIndex < static_cast<int>(m_scaledEnemyIcons.size())
                                && !(isBoss
                                         ? m_scaledBossEnemyIcons[static_cast<size_t>(enemyIconIndex)].isNull()
                                         : m_scaledEnemyIcons[static_cast<size_t>(enemyIconIndex)].isNull());
        if (useAiEnemy) {
            const QPixmap &icon = isBoss
                                      ? m_scaledBossEnemyIcons[static_cast<size_t>(enemyIconIndex)]
                                      : m_scaledEnemyIcons[static_cast<size_t>(enemyIconIndex)];
            const QRectF target(c.x() - icon.width() / 2.0,
                                c.y() - icon.height() / 2.0 - m_cellSize * (isBoss ? 0.03 : 0.02),
                                icon.width(),
                                icon.height());
            painter.setPen(QPen(QColor(255, 246, 205, isBoss ? 145 : 100), strokeWidth));
            painter.setBrush(Qt::NoBrush);
            const double inset = std::clamp(m_cellSize * 0.08, 5.0, 9.0);
            painter.drawRoundedRect(target.adjusted(inset, inset, -inset, -inset), 9, 9);
            painter.drawPixmap(target.topLeft(), icon);
        } else {
        QRadialGradient body(c + QPointF(-radius * 0.35, -radius * 0.35), radius * 1.5);
        body.setColorAt(0.0, base.lighter(145));
        body.setColorAt(1.0, base.darker(125));
        painter.setBrush(body);
        painter.setPen(QPen(QColor(225, 202, 255, 220), 2));
        if (enemy->kind == EnemyKind::Fast) {
            QPolygonF tri;
            tri << c + QPointF(0, -radius - 3)
                << c + QPointF(radius + 6, radius * 0.75)
                << c + QPointF(-radius - 6, radius * 0.75);
            painter.drawPolygon(tri);
        } else if (enemy->kind == EnemyKind::Armored) {
            painter.drawRoundedRect(QRectF(c.x() - radius, c.y() - radius, radius * 2, radius * 2), 4, 4);
        } else if (enemy->kind == EnemyKind::Splitter) {
            painter.drawEllipse(c + QPointF(-6, 0), radius * 0.82, radius);
            painter.drawEllipse(c + QPointF(6, 0), radius * 0.82, radius);
        } else if (enemy->kind == EnemyKind::Boss) {
            painter.drawPolygon(diamondAt(c, radius + 5));
            painter.setPen(QPen(QColor(255, 225, 154), 2));
            painter.drawLine(c + QPointF(-10, -radius - 7), c + QPointF(0, -radius - 15));
            painter.drawLine(c + QPointF(0, -radius - 15), c + QPointF(10, -radius - 7));
        } else {
            painter.drawEllipse(c, radius, radius);
        }
        }

        if (enemy->hitFlash > 0.0) {
            const int alpha = static_cast<int>(std::clamp(enemy->hitFlash / 0.16, 0.0, 1.0) * 170.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(255, 255, 255, alpha));
            painter.drawEllipse(c, markerRadius * 0.58, markerRadius * 0.58);
        }

        const double ratio = std::clamp(enemy->hp / enemy->maxHp, 0.0, 1.0);
        const double barWidth = iconSize * (isBoss ? 0.92 : 1.05);
        const double barHeight = std::clamp(m_cellSize * (isBoss ? 0.09 : 0.075), 6.0, 10.0);
        QRectF bar(c.x() - barWidth / 2.0,
                   c.y() - radius - m_cellSize * (isBoss ? 0.35 : 0.28),
                   barWidth,
                   barHeight);
        drawStatusBar(painter, bar, ratio, QColor(255, 105, 128));
        painter.setPen(QPen(QColor(255, 245, 210, 155), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(bar.adjusted(-1, -1, 1, 1), 3, 3);
    }
}

void GameWidget::drawProjectiles(QPainter &painter)
{
    for (const Projectile &projectile : m_projectiles) {
        QColor color;
        if (projectile.kind == ProjectileKind::Slow) {
            color = QColor(105, 210, 255);
        } else if (projectile.kind == ProjectileKind::Splash) {
            color = QColor(194, 103, 255);
        } else {
            color = QColor(255, 226, 132);
        }

        painter.setPen(QPen(alphaColor(color, 85), 7, Qt::SolidLine, Qt::RoundCap));
        if (Enemy *target = enemyById(projectile.targetId)) {
            const QPointF targetPos = pathPosition(target->progress);
            const QPointF tail = projectile.pos - normalized(targetPos - projectile.pos) * 16.0;
            painter.drawLine(tail, projectile.pos);
        }
        painter.setPen(Qt::NoPen);
        QRadialGradient glow(projectile.pos, 10);
        glow.setColorAt(0.0, color.lighter(140));
        glow.setColorAt(1.0, alphaColor(color, 0));
        painter.setBrush(glow);
        painter.drawEllipse(projectile.pos, 10, 10);
        painter.setBrush(color);
        painter.drawEllipse(projectile.pos, 5, 5);
    }
}

void GameWidget::drawCinematicEffects(QPainter &painter)
{
    for (const VisualPulse &pulse : m_visualPulses) {
        const double t = std::clamp(pulse.age / std::max(0.01, pulse.duration), 0.0, 1.0);
        const double r = pulse.maxRadius * t;
        const int alpha = static_cast<int>((1.0 - t) * 190.0);
        QRadialGradient flash(pulse.center, std::max(1.0, r * 0.72));
        flash.setColorAt(0.0, QColor(pulse.color.red(), pulse.color.green(), pulse.color.blue(), alpha / 3));
        flash.setColorAt(1.0, QColor(pulse.color.red(), pulse.color.green(), pulse.color.blue(), 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(flash);
        painter.drawEllipse(pulse.center, r * 0.72, r * 0.72);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(pulse.color.red(), pulse.color.green(), pulse.color.blue(), alpha),
                            std::max(2.0, scaledDistance(3.0))));
        painter.drawEllipse(pulse.center, r, r);
        painter.setPen(QPen(QColor(255, 255, 255, alpha / 2), std::max(1.0, scaledDistance(1.0))));
        painter.drawEllipse(pulse.center, r * 0.56, r * 0.56);
        painter.setPen(QPen(QColor(255, 245, 196, alpha / 3), std::max(1.0, scaledDistance(1.4)), Qt::DashLine));
        painter.drawEllipse(pulse.center, r * 0.82, r * 0.34);
    }

    if (m_freezePulseTimer > 0.0) {
        const double t = 1.0 - m_freezePulseTimer / 1.05;
        const double radius = std::hypot(width(), height()) * std::clamp(t, 0.0, 1.0);
        painter.fillRect(rect(), QColor(60, 180, 255, static_cast<int>((1.0 - t) * 52)));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(145, 228, 255, static_cast<int>((1.0 - t) * 210)),
                            std::max(4.0, scaledDistance(8.0))));
        painter.drawEllipse(QPointF(width() / 2.0, height() / 2.0), radius, radius * 0.62);
    }

    const bool majorOverlay = m_paused
                              || m_infoCardActive
                              || m_enemyGuideActive
                              || m_towerDrillActive
                              || m_tutorialStep > 0
                              || (m_commandInput && m_commandInput->isVisible());
    if (majorOverlay) {
        return;
    }

    if (m_bossAlertTimer > 0.0) {
        const double pulse = 0.5 + std::sin(m_bossAlertTimer * 18.0) * 0.5;
        painter.fillRect(rect(), QColor(88, 0, 14, static_cast<int>(42 + 34 * pulse)));
        QRectF box(width() / 2.0 - 310, 150, 620, 62);
        drawGlassPanel(painter, box, QColor(255, 90, 105));
        painter.setPen(QColor(255, 214, 220));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 21, QFont::Bold));
        painter.drawText(box.adjusted(16, 0, -16, 0),
                         Qt::AlignCenter | Qt::TextWordWrap,
                         QStringLiteral("警报：%1 接近")
                             .arg(m_reverseMode ? QStringLiteral("逆向终局协议") : bossProtocolName(m_levelIndex)));
    }

    if (m_waveBannerTimer > 0.0 && !m_waveBannerText.isEmpty()) {
        const double t = std::clamp(m_waveBannerTimer / 2.4, 0.0, 1.0);
        const double slide = (1.0 - std::min(1.0, t * 2.2)) * 80.0;
        QRectF banner(width() / 2.0 - 330 + slide, 132, 660, 56);
        drawGlassPanel(painter, banner, QColor(255, 225, 154));
        painter.setPen(QColor(255, 245, 196, static_cast<int>(210 * std::min(1.0, t * 1.6))));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 23, QFont::Bold));
        painter.drawText(banner, Qt::AlignCenter, m_waveBannerText);
    }

    if (m_deploymentIntroTimer > 0.0 && !m_finished) {
        drawDeploymentIntro(painter);
    }
}

void GameWidget::drawDeploymentIntro(QPainter &painter)
{
    painter.fillRect(rect(), QColor(2, 6, 14, 170));
    const QRectF panel(width() / 2.0 - 330, height() / 2.0 - 180, 660, 318);
    drawGlassPanel(painter, panel, QColor(80, 212, 255));

    painter.setPen(QColor(255, 225, 154));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 28, QFont::Bold));
    painter.drawText(panel.adjusted(24, 20, -24, -250), Qt::AlignCenter,
                     QStringLiteral("%1  任务投影").arg(m_levelName));

    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 15));
    painter.setPen(QColor(223, 248, 255));
    painter.drawText(panel.adjusted(48, 84, -48, -182), Qt::AlignCenter | Qt::TextWordWrap,
                     QStringLiteral("路径扫描完成 / 装置权限开放 / 第一波异常等待同步"));

    const double progress = std::clamp((3.0 - m_deploymentIntroTimer) / 3.0, 0.0, 1.0);
    const QRectF bar(panel.left() + 80, panel.top() + 154, panel.width() - 160, 16);
    painter.setPen(QPen(QColor(80, 212, 255, 150), 1));
    painter.setBrush(QColor(4, 12, 24, 210));
    painter.drawRoundedRect(bar, 8, 8);
    QLinearGradient fill(bar.topLeft(), bar.topRight());
    fill.setColorAt(0.0, QColor(80, 212, 255, 210));
    fill.setColorAt(1.0, QColor(255, 225, 154, 230));
    painter.setBrush(fill);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(bar.left(), bar.top(), bar.width() * progress, bar.height()), 8, 8);

    painter.setPen(QColor(157, 216, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 14, QFont::Bold));
    const QStringList steps = {
        QStringLiteral("异常节点锁定"),
        QStringLiteral("因果路径生成"),
        QStringLiteral("修补倒计时")
    };
    for (int i = 0; i < steps.size(); ++i) {
        const bool active = progress >= (i + 1) / 4.0;
        QRectF step(panel.left() + 84 + i * 170, panel.top() + 196, 142, 42);
        painter.setPen(QPen(active ? QColor(255, 225, 154) : QColor(80, 212, 255, 90), 1));
        painter.setBrush(active ? QColor(255, 225, 154, 28) : QColor(5, 14, 28, 130));
        painter.drawRoundedRect(step, 8, 8);
        painter.setPen(active ? QColor(255, 245, 196) : QColor(157, 216, 255));
        painter.drawText(step, Qt::AlignCenter, steps[i]);
    }

    painter.setPen(QColor(255, 225, 154));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 24, QFont::Bold));
    painter.drawText(panel.adjusted(24, 248, -24, -24), Qt::AlignCenter,
                     QStringLiteral("%1").arg(static_cast<int>(std::ceil(m_deploymentIntroTimer))));
}

void GameWidget::drawReversePlanningOverlay(QPainter &painter)
{
    if (!m_reverseMode || !m_reversePlanning || m_finished || m_paused) {
        return;
    }

    const QRectF panel = reverseQueuePanelRect();
    drawGlassPanel(painter, panel, QColor(255, 92, 190));

    painter.setPen(QColor(255, 225, 154));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 18, QFont::Bold));
    painter.drawText(panel.adjusted(24, 14, -24, -124),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("逆向编排 %1/%2  |  剩余裂隙点 %3")
                         .arg(m_currentWave + 1)
                         .arg(m_waves.size())
                         .arg(m_reverseRiftPoints));

    static const std::array<std::array<QString, 3>, 6> waveTags = {{
        {QStringLiteral("低价铺量"), QStringLiteral("试探火力"), QStringLiteral("保留点数")},
        {QStringLiteral("速度突防"), QStringLiteral("压减速区"), QStringLiteral("拉开队列")},
        {QStringLiteral("护甲推进"), QStringLiteral("吃单体伤害"), QStringLiteral("后排跟进")},
        {QStringLiteral("抗控突破"), QStringLiteral("穿减速区"), QStringLiteral("混合施压")},
        {QStringLiteral("分裂扰乱"), QStringLiteral("多目标"), QStringLiteral("压范围塔")},
        {QStringLiteral("Boss 协议"), QStringLiteral("混合波"), QStringLiteral("冲核心")}
    }};
    const auto &tags = waveTags[static_cast<size_t>(std::clamp(m_currentWave, 0, 5))];
    for (int i = 0; i < static_cast<int>(tags.size()); ++i) {
        const QRectF tag(panel.left() + 24.0 + i * 102.0, panel.top() + 58.0, 92.0, 24.0);
        painter.setPen(QPen(QColor(255, 225, 154, 145), 1));
        painter.setBrush(QColor(5, 14, 28, 188));
        painter.drawRoundedRect(tag, 7, 7);
        painter.setPen(QColor(255, 245, 196));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 10, QFont::Bold));
        painter.drawText(tag.adjusted(6, 0, -6, 0), Qt::AlignCenter, tags[static_cast<size_t>(i)]);
    }

    const double chip = 58.0;
    const double gap = 10.0;
    const double startX = panel.left() + 24.0;
    const double y = panel.top() + 104.0;
    const double available = panel.width() - 310.0;
    const int maxVisible = std::max(1, static_cast<int>(available / (chip + gap)));
    int queueSize = 0;
    int queueCost = 0;
    if (m_currentWave < static_cast<int>(m_reversePlans.size())) {
        const auto &plan = m_reversePlans[static_cast<size_t>(m_currentWave)];
        queueSize = static_cast<int>(plan.size());
        for (const WaveEntry &entry : plan) {
            queueCost += reverseEnemyCost(entry.kind) * entry.count;
        }
        for (int i = 0; i < std::min(queueSize, maxVisible); ++i) {
            const EnemyKind kind = plan[static_cast<size_t>(i)].kind;
            const QRectF chipRect(startX + i * (chip + gap), y, chip, chip);
            const bool dragHover = m_reverseDragIndex >= 0 && m_reverseDragHoverIndex == i;
            painter.setPen(QPen(dragHover ? QColor(255, 225, 154) : enemyColor(kind).lighter(135),
                                dragHover ? 2 : 1));
            painter.setBrush(dragHover ? QColor(42, 34, 20, 220) : QColor(4, 12, 24, 185));
            painter.drawRoundedRect(chipRect, 8, 8);
            const int iconIndex = static_cast<int>(kind);
            if (iconIndex >= 0 && iconIndex < static_cast<int>(m_enemyIcons.size()) && !m_enemyIcons[static_cast<size_t>(iconIndex)].isNull()) {
                const QPixmap scaled = m_enemyIcons[static_cast<size_t>(iconIndex)].scaled(46, 46, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                painter.drawPixmap(QPointF(chipRect.center().x() - scaled.width() / 2.0,
                                           chipRect.center().y() - scaled.height() / 2.0),
                                   scaled);
            }
            painter.setPen(QColor(255, 245, 196, 230));
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 8, QFont::Bold));
            painter.drawText(chipRect.adjusted(4, 2, -4, -28),
                             Qt::AlignLeft | Qt::AlignTop,
                             QString::number(i + 1));
        }
        if (m_reverseDragIndex >= 0 && m_reverseDragHoverIndex >= 0) {
            const int hover = std::clamp(m_reverseDragHoverIndex, 0, std::min(queueSize, maxVisible) - 1);
            const double lineX = startX + hover * (chip + gap) - gap / 2.0;
            painter.setPen(QPen(QColor(255, 225, 154, 230), 3));
            painter.drawLine(QPointF(lineX, y - 8.0), QPointF(lineX, y + chip + 8.0));
        }
    }

    if (queueSize == 0) {
        painter.setPen(QColor(223, 248, 255, 210));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 14));
        painter.drawText(QRectF(startX, y, available, chip),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("点击右侧加入队列；拖动排序；右键撤回。"));
    } else if (queueSize > maxVisible) {
        painter.setPen(QColor(255, 245, 196));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));
        painter.drawText(QRectF(startX + maxVisible * (chip + gap) + 4.0, y, 80, chip),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("+%1").arg(queueSize - maxVisible));
    }

    painter.setPen(QColor(255, 225, 154));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
    painter.drawText(QRectF(panel.right() - 252.0, panel.top() + 28.0, 226.0, 26.0),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("已编队 %1  |  已用 %2")
                         .arg(queueSize)
                         .arg(queueCost));
}

void GameWidget::drawIntelBanner(QPainter &painter)
{
    if (m_intelBannerTimer <= 0.0
        || m_intelBannerTitle.isEmpty()
        || m_paused
        || m_infoCardActive
        || m_enemyGuideActive
        || m_towerDrillActive
        || m_tutorialStep > 0
        || (m_commandInput && m_commandInput->isVisible())) {
        return;
    }

    const double t = std::clamp(m_intelBannerTimer / 4.2, 0.0, 1.0);
    const double slide = (1.0 - std::min(1.0, t * 2.8)) * 72.0;
    const QRectF box(width() / 2.0 - 340 + slide, 142, 680, 92);
    drawGlassPanel(painter, box, m_intelBannerAccent);

    painter.setPen(QColor(m_intelBannerAccent.red(), m_intelBannerAccent.green(), m_intelBannerAccent.blue(), 230));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 17, QFont::Bold));
    painter.drawText(box.adjusted(20, 10, -20, -54), Qt::AlignLeft | Qt::AlignVCenter, m_intelBannerTitle);

    painter.setPen(QColor(223, 248, 255, 224));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13));
    painter.drawText(box.adjusted(20, 38, -20, -10), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap, m_intelBannerBody);
}

void GameWidget::showIntelBanner(const QString &title, const QString &body, const QColor &accent)
{
    m_intelBannerTitle = title;
    m_intelBannerBody = body;
    m_intelBannerAccent = accent;
    m_intelBannerTimer = 4.2;
}

void GameWidget::showInfoCard(const QString &title, const QString &body, const QColor &accent)
{
    m_infoCardTitle = title;
    m_infoCardBody = body;
    m_infoCardAccent = accent;
    m_infoCardActive = true;
    SoundManager::play(SoundCue::Dialogue);
    update();
}

void GameWidget::closeInfoCard()
{
    if (!m_infoCardActive) {
        return;
    }
    m_infoCardActive = false;
    m_infoCardTitle.clear();
    m_infoCardBody.clear();
    SoundManager::play(SoundCue::UiClick);
    queueFirstRunIntroductions();
    update();
}

void GameWidget::drawInfoCard(QPainter &painter)
{
    if (!m_infoCardActive) {
        return;
    }

    painter.fillRect(rect(), QColor(2, 6, 14, 176));
    const QRectF panel(width() / 2.0 - 440, height() / 2.0 - 225, 880, 450);
    drawGlassPanel(painter, panel, m_infoCardAccent);

    painter.setPen(QColor(m_infoCardAccent.red(), m_infoCardAccent.green(), m_infoCardAccent.blue(), 238));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 23, QFont::Bold));
    painter.drawText(panel.adjusted(30, 22, -30, -372), Qt::AlignLeft | Qt::AlignVCenter, m_infoCardTitle);

    painter.setPen(QColor(223, 248, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12));
    painter.drawText(panel.adjusted(34, 78, -34, -84), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_infoCardBody);

    const QRectF button(panel.center().x() - 126, panel.bottom() - 58, 252, 38);
    painter.setPen(QPen(QColor(255, 225, 154, 185), 1));
    painter.setBrush(QColor(16, 38, 60, 225));
    painter.drawRoundedRect(button, 8, 8);
    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 14, QFont::Bold));
    painter.drawText(button, Qt::AlignCenter, QStringLiteral("点击 / 空格键 / 回车键继续"));
}

void GameWidget::drawTutorialOverlay(QPainter &painter)
{
    if (m_tutorialStep <= 0 || m_finished) {
        return;
    }

    painter.fillRect(rect(), QColor(2, 6, 14, 138));
    const QPoint targetCell = m_tutorialStep == 7 ? m_tutorialResourceCell
                             : m_tutorialStep == 9 ? m_tutorialSlowCell
                                                   : m_tutorialCell;
    const QRectF target = cellRect(targetCell.y(), targetCell.x());
    const double pulse = 0.5 + std::sin(m_tutorialAnimTime * 5.0) * 0.5;
    if (m_tutorialStep == 3 || m_tutorialStep == 5 || m_tutorialStep == 7 || m_tutorialStep == 9) {
        painter.setPen(QPen(QColor(255, 225, 154, static_cast<int>(150 + pulse * 90)), std::max(3.0, scaledDistance(4.0))));
        painter.setBrush(QColor(255, 225, 154, 36));
        painter.drawRoundedRect(target.adjusted(-6, -6, 6, 6), 10, 10);
    }
    if (m_tutorialStep <= 2) {
        painter.setPen(QPen(QColor(80, 212, 255, 128), std::max(2.0, scaledDistance(3.0)), Qt::DashLine));
        painter.setBrush(Qt::NoBrush);
        for (const QPoint &cell : m_pathCells) {
            painter.drawRoundedRect(cellRect(cell.y(), cell.x()).adjusted(5, 5, -5, -5), 8, 8);
        }
    }

    QRectF box(width() / 2.0 - 420, height() - 226, 840, 176);
    drawGlassPanel(painter, box, QColor(255, 225, 154));
    const QString title = tutorialTitle(m_tutorialStep);
    const QString text = tutorialBody(m_tutorialStep);
    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 17, QFont::Bold));
    painter.drawText(box.adjusted(22, 10, -22, -126), Qt::AlignLeft | Qt::AlignVCenter, title);
    painter.setPen(QColor(223, 248, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 14));
    painter.drawText(box.adjusted(22, 50, -22, -58), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);

    const QRectF objectiveBox(box.left() + 22, box.bottom() - 50, box.width() - 44, 30);
    painter.setPen(QPen(QColor(255, 225, 154, 120), 1));
    painter.setBrush(QColor(5, 14, 28, 128));
    painter.drawRoundedRect(objectiveBox, 7, 7);
    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
    painter.drawText(objectiveBox, Qt::AlignCenter | Qt::TextWordWrap, tutorialFeedback(m_tutorialStep));
}

void GameWidget::drawTowerDrillOverlay(QPainter &painter)
{
    if (!m_towerDrillActive || m_towerDrillCell.x() < 0 || m_finished) {
        return;
    }

    painter.fillRect(rect(), QColor(2, 6, 14, 138));
    const double pulse = 0.5 + std::sin(m_tutorialAnimTime * 5.2) * 0.5;
    painter.setBrush(Qt::NoBrush);
    const QRectF target = cellRect(m_towerDrillCell.y(), m_towerDrillCell.x());
    painter.setPen(QPen(QColor(255, 225, 154, static_cast<int>(165 + pulse * 80)),
                        std::max(3.0, scaledDistance(4.0))));
    painter.setBrush(QColor(255, 225, 154, 42));
    painter.drawRoundedRect(target.adjusted(-7, -7, 7, 7), 11, 11);

    const QRectF panel(width() / 2.0 - 420, height() - 226, 840, 176);
    drawGlassPanel(painter, panel, QColor(255, 225, 154));

    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 17, QFont::Bold));
    painter.drawText(panel.adjusted(22, 10, -22, -126),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     m_towerDrillTitle);

    const QString objective = m_towerDrillStage == 1
                                  ? QStringLiteral("目标：鼠标点击右侧高亮按钮")
                                  : QStringLiteral("目标：左键部署到高亮稳定格");
    const QRectF objectiveBox(panel.left() + 22, panel.bottom() - 50, panel.width() - 44, 30);
    painter.setPen(QPen(QColor(255, 225, 154, 120), 1));
    painter.setBrush(QColor(5, 14, 28, 128));
    painter.drawRoundedRect(objectiveBox, 7, 7);
    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
    painter.drawText(objectiveBox,
                     Qt::AlignCenter | Qt::TextWordWrap,
                     objective);

    painter.setPen(QColor(223, 248, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 14));
    painter.drawText(panel.adjusted(22, 50, -22, -58),
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     m_towerDrillBody);
}

void GameWidget::drawEnemyGuideOverlay(QPainter &painter)
{
    if (!m_enemyGuideActive || m_finished) {
        return;
    }

    painter.fillRect(rect(), QColor(2, 6, 14, 118));
    const double pulse = 0.5 + std::sin(m_tutorialAnimTime * 5.4) * 0.5;
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(80, 212, 255, 110), std::max(2.0, scaledDistance(2.5)), Qt::DashLine));
    for (const QPoint &cell : m_pathCells) {
        painter.drawRoundedRect(cellRect(cell.y(), cell.x()).adjusted(6, 6, -6, -6), 8, 8);
    }

    const Enemy *enemy = enemyById(m_enemyGuideId);
    QPointF target = m_pathCells.empty() ? QPointF(width() / 2.0, height() / 2.0) : cellCenter(m_pathCells.front());
    if (enemy) {
        target = pathPosition(enemy->progress);
    }
    const QColor accent = enemyColor(m_enemyGuideKind);
    painter.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), static_cast<int>(175 + pulse * 70)),
                        std::max(3.0, scaledDistance(4.0))));
    painter.setBrush(QColor(accent.red(), accent.green(), accent.blue(), 34));
    painter.drawEllipse(target, scaledDistance(38.0 + pulse * 8.0), scaledDistance(38.0 + pulse * 8.0));
    painter.setPen(QPen(QColor(255, 245, 196, 210), std::max(1.5, scaledDistance(2.0))));
    painter.drawLine(target, QPointF(width() / 2.0 - 330, height() - 196));

    const double panelWidth = std::min(width() - 36.0, 660.0);
    const QRectF panel(width() / 2.0 - panelWidth / 2.0, height() - 142, panelWidth, 86);
    drawGlassPanel(painter, panel, accent);
    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 16, QFont::Bold));
    painter.drawText(panel.adjusted(24, 10, -24, -52),
                     Qt::AlignLeft | Qt::AlignTop,
                     m_enemyGuideStage == 1 ? QStringLiteral("敌情识别目标") : m_enemyGuideTitle);
    painter.setPen(QColor(223, 248, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Medium));
    const QString guideText = m_enemyGuideStage == 1
                                  ? QStringLiteral("点击高亮敌人。")
                                  : m_enemyGuideBody + QStringLiteral("  空格键继续");
    painter.drawText(panel.adjusted(24, 42, -24, -10),
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                     guideText);
}

void GameWidget::drawPrompt(QPainter &painter)
{
    if (m_promptTime <= 0.0 || m_promptTitle.isEmpty() || blocksTransientPrompt()) {
        return;
    }

    const double duration = std::max(0.1, m_promptDuration);
    const double ratio = std::clamp(m_promptTime / duration, 0.0, 1.0);
    const double fade = std::min(1.0, std::min((duration - m_promptTime) * 6.0, ratio * 6.0));
    const int alpha = static_cast<int>(std::clamp(fade, 0.0, 1.0) * 255.0);

    painter.save();
    painter.setOpacity(alpha / 255.0);

    QRectF box;
    if (m_promptLane == PromptLaneBoss) {
        const double boxWidth = std::min(width() - 36.0, 300.0);
        box = QRectF(width() - boxWidth - 28.0, 104.0, boxWidth, 58.0);
    } else if (m_promptLane == PromptLaneTop) {
        const double boxWidth = std::min(width() - 48.0, 520.0);
        box = QRectF(width() / 2.0 - boxWidth / 2.0, 112.0, boxWidth, 54.0);
    } else {
        const double boxWidth = std::min(width() - 56.0, 640.0);
        box = QRectF(width() / 2.0 - boxWidth / 2.0, height() - 78.0, boxWidth, 46.0);
    }

    drawGlassPanel(painter, box, m_promptAccent);

    if (m_promptLane == PromptLaneBoss) {
        const QRectF badge(box.left() + 12.0, box.top() + 11.0, 58.0, 36.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(m_promptAccent.red(), m_promptAccent.green(), m_promptAccent.blue(), 65));
        painter.drawRoundedRect(badge, 7, 7);
        painter.setPen(QColor(255, 245, 196));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
        painter.drawText(badge, Qt::AlignCenter, QStringLiteral("BOSS"));
        painter.setPen(QColor(255, 225, 154));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 14, QFont::Bold));
        painter.drawText(box.adjusted(82, 8, -14, -30), Qt::AlignLeft | Qt::AlignVCenter, m_promptTitle);
        painter.setPen(QColor(223, 248, 255));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Medium));
        painter.drawText(box.adjusted(82, 30, -14, -8), Qt::AlignLeft | Qt::AlignVCenter, m_promptBody);
    } else {
        painter.setPen(QColor(255, 225, 154));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
        const QString text = m_promptBody.isEmpty() ? m_promptTitle : QStringLiteral("%1：%2").arg(m_promptTitle, m_promptBody);
        painter.drawText(box.adjusted(18, 5, -18, -5), Qt::AlignCenter | Qt::TextWordWrap, text);
    }

    painter.restore();
}

void GameWidget::drawCommandPanel(QPainter &painter)
{
    if (!m_commandInput || !m_commandInput->isVisible()) {
        return;
    }

    const QRectF inputRect = QRectF(m_commandInput->geometry());
    const QRectF panel(inputRect.left() - 20,
                       inputRect.top() - 46,
                       inputRect.width() + 40,
                       inputRect.height() + 86);
    drawGlassPanel(painter, panel, QColor(80, 212, 255));
    painter.setPen(QColor(255, 245, 196));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 15, QFont::Bold));
    painter.drawText(QRectF(panel.left() + 22, panel.top() + 10, 160, 26),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("指令台"));
    painter.setPen(QColor(157, 216, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11, QFont::Medium));
    painter.drawText(QRectF(panel.left() + 120, panel.top() + 13, panel.width() - 142, 22),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("回车执行 / Esc 取消"));
    painter.setPen(QColor(223, 248, 255));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 11));
    painter.drawText(QRectF(panel.left() + 24, inputRect.bottom() + 8, panel.width() - 48, 22),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("可用：energy 加时能，clear 清场，next 下一波，win 直接胜利"));
}

void GameWidget::drawMessage(QPainter &painter)
{
    if (m_paused) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(3, 8, 18, 170));
        painter.drawRect(rect());
        const QRectF menu(width() / 2.0 - 186, height() / 2.0 - 156, 372, 332);
        drawGlassPanel(painter, menu, QColor(255, 225, 154));
        painter.setPen(QColor(223, 248, 255));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 24, QFont::Bold));
        painter.drawText(QRectF(menu.left() + 24, menu.top() + 20, menu.width() - 48, 42),
                         Qt::AlignCenter,
                         QStringLiteral("暂停"));
    }

    if (m_waitingNextWave && !m_paused) {
        const QRectF box(width() / 2.0 - 170, 132, 340, 42);
        drawGlassPanel(painter, box, QColor(255, 225, 154));
        painter.setPen(QColor(255, 225, 154));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 18, QFont::Bold));
        painter.drawText(box, Qt::AlignCenter,
                         QStringLiteral("时间线同步倒计时 %1").arg(std::ceil(m_nextWaveTimer)));
    }

    drawCommandPanel(painter);
    if (m_commandInput && m_commandInput->isVisible()) {
        return;
    }

    drawPrompt(painter);

    if (m_feedbackTime > 0.0
        && !m_feedback.isEmpty()
        && !(m_reverseMode && m_reversePlanning)
        && !blocksTransientPrompt()
        && m_promptTime <= 0.0) {
        const double boxWidth = std::min(width() - 56.0, 760.0);
        const QRectF box(width() / 2.0 - boxWidth / 2.0, height() - 88, boxWidth, 54);
        drawGlassPanel(painter, box, QColor(255, 225, 154));
        painter.setPen(QColor(255, 225, 154));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
        painter.drawText(box.adjusted(18, 6, -18, -6), Qt::AlignCenter | Qt::TextWordWrap, m_feedback);
    }
}

double GameWidget::distance(const QPointF &a, const QPointF &b)
{
    const QPointF d = a - b;
    return std::sqrt(d.x() * d.x() + d.y() * d.y());
}
