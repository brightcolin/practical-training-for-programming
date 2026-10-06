#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H

#include <QElapsedTimer>
#include <QEvent>
#include <QColor>
#include <QLineEdit>
#include <QPointF>
#include <QPushButton>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "gametypes.h"

#include <memory>
#include <array>
#include <vector>

class GameWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GameWidget(QWidget *parent = nullptr);
    void resetGame();
    void setLevelIndex(int index);
    void startReverseMode(int scenarioIndex = 0);
    void loadCustomLevel(const QString &path);
    void setRunning(bool running);
    int levelIndex() const { return m_levelIndex; }
    int reverseScenarioIndex() const { return m_reverseScenarioIndex; }
    GameSummary summary() const { return m_summary; }
    bool reverseMode() const { return m_reverseMode; }

signals:
    void victory();
    void failure();
    void backToMenu();
    void settingsRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    enum PromptLane {
        PromptLaneBottom = 0,
        PromptLaneTop = 1,
        PromptLaneBoss = 2
    };

    struct PromptMessage {
        QString title;
        QString body;
        QColor accent;
        double duration = 2.0;
        PromptLane lane = PromptLaneBottom;
    };

    struct VisualPulse {
        QPointF center;
        QColor color;
        double age = 0.0;
        double duration = 0.9;
        double maxRadius = 90.0;
    };

    void setupTowerButtons();
    void updateTowerButtonStates();
    void updateReverseButtonStates();
    void syncTutorialFeedback();
    void skipTutorial();
    void queueFirstRunIntroductions();
    void queuePrompt(const QString &title,
                     const QString &body,
                     const QColor &accent,
                     double duration = 2.0,
                     PromptLane lane = PromptLaneBottom);
    void clearPrompts();
    void updatePrompt(double dt);
    bool blocksTransientPrompt() const;
    bool startTowerDrillIfNeeded();
    void beginTowerDrill(TowerKind kind);
    void completeTowerDrill();
    void closeEnemyGuide();
    QPoint recommendedDrillCell(TowerKind kind) const;
    void loadVisualAssets();
    void loadTowerConfig();
    void saveProgress(bool cleared);
    void openCommandInput();
    void closeCommandInput();
    void executeCommandInput();
    void executeCheatCommand(const QString &command);
    void updateControlButtonGeometry();
    void rebuildPathMetrics(double oldLength = 0.0);
    double mapScale() const;
    double scaledDistance(double designPixels) const;
    double scaledTowerRange(const Tower &tower) const;

    void updateGame();
    void updateEnemies(double dt);
    void updateTowers(double dt);
    void updateProjectiles(double dt);
    void updateWaveSpawner(double dt);
    void updateBossMechanic(Enemy &enemy, double &speed, double dt);
    void cleanupDeadEnemies();
    void cleanupDestroyedTowers();
    void checkEndConditions();
    void configureReverseMode();
    void prepareReversePlanningWave();
    void startReverseWave();
    void autoDeployReverseDefense();
    int reverseScenarioLevelIndex() const;
    QString reverseScenarioName() const;
    int reverseInitialDefenseEnergy() const;
    int reverseWaveDefenseEnergy(int waveIndex) const;
    bool tryAutoBuildTower(TowerKind kind, double preferredProgress);
    bool tryAutoUpgradeTower();
    EnemyKind reverseEnemyKindForButton(int index) const;
    bool reverseEnemyUnlocked(EnemyKind kind) const;
    int reverseEnemyWaveLimit(EnemyKind kind) const;
    int reverseEnemyCost(EnemyKind kind) const;
    int reverseWaveBudget(int waveIndex) const;
    QString reverseEnemyButtonText(EnemyKind kind, int count) const;
    QString reverseEnemyHint(EnemyKind kind) const;
    QRectF reverseQueuePanelRect() const;
    int reverseQueueIndexAt(const QPoint &pos, bool insertion) const;
    bool removeReverseQueuedEnemyAt(const QPoint &pos);

    QPointF cellCenter(const QPoint &cell) const;
    QRectF cellRect(int row, int col) const;
    QRect towerPanelRect() const;
    QPoint cellAt(const QPoint &pos) const;
    QPoint pathCellForProgress(double progress) const;
    QPointF pathPosition(double progress) const;
    double progressAtPathCell(const QPoint &cell) const;
    double pathLength() const;
    bool isPathCell(const QPoint &cell) const;
    bool isEndpointCell(const QPoint &cell) const;
    bool hasTowerAt(const QPoint &cell) const;
    bool isTowerUnlocked(TowerKind kind) const;
    bool canBuildAt(const QPoint &cell, TowerKind kind) const;
    int buildCost(TowerKind kind, const QPoint &cell) const;
    int upgradeCost(const Tower &tower) const;
    int towerIndexAt(const QPoint &cell);
    int blockingWallIndex(const QPointF &enemyPos);
    Enemy *enemyById(int id) const;
    Enemy *nearestEnemy(const QPointF &from, double range) const;
    std::vector<Enemy *> enemiesInRange(const QPointF &from, double range) const;

    Tower makeTower(TowerKind kind, const QPoint &cell) const;
    void upgradeTower(int index);
    void activateFreezeSkill();
    std::unique_ptr<Enemy> makeEnemy(EnemyKind kind);
    void spawnEnemyAt(EnemyKind kind, double progress, double hpScale = 1.0, double speedScale = 1.0);
    void applyDamage(Enemy &enemy, double damage);
    void applySlow(Enemy &enemy, double seconds, double factor);
    void applyBurn(Enemy &enemy, double seconds, double dps);

    QString towerName(TowerKind kind) const;
    QString towerShortName(TowerKind kind) const;
    QString towerUnlockHint(TowerKind kind) const;
    QString towerIntroText(TowerKind kind) const;
    QString towerButtonTooltip(TowerKind kind) const;
    QString enemyName(EnemyKind kind) const;
    QString enemyIntelText(EnemyKind kind) const;
    QColor towerColor(TowerKind kind) const;
    QColor enemyColor(EnemyKind kind) const;
    QColor tileColor(TileKind kind) const;

    void drawBackground(QPainter &painter);
    void renderBackground(QPainter &painter);
    void drawHud(QPainter &painter);
    void drawTowerButtonHoverCard(QPainter &painter);
    void drawMap(QPainter &painter);
    void drawTowers(QPainter &painter);
    void drawEnemies(QPainter &painter);
    void drawProjectiles(QPainter &painter);
    void drawMessage(QPainter &painter);
    void drawPrompt(QPainter &painter);
    void drawCommandPanel(QPainter &painter);
    void drawCinematicEffects(QPainter &painter);
    void drawDeploymentIntro(QPainter &painter);
    void drawTutorialOverlay(QPainter &painter);
    void drawTowerDrillOverlay(QPainter &painter);
    void drawEnemyGuideOverlay(QPainter &painter);
    void drawReversePlanningOverlay(QPainter &painter);
    void drawIntelBanner(QPainter &painter);
    void showIntelBanner(const QString &title, const QString &body, const QColor &accent);
    void showInfoCard(const QString &title, const QString &body, const QColor &accent);
    void closeInfoCard();
    void drawInfoCard(QPainter &painter);
    void rebuildScaledVisualCache();

    static double distance(const QPointF &a, const QPointF &b);

    QTimer m_timer;
    QElapsedTimer m_clock;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_restartButton = nullptr;
    QPushButton *m_menuButton = nullptr;
    QPushButton *m_settingsButton = nullptr;
    QPushButton *m_skipTutorialButton = nullptr;
    QPushButton *m_reverseLaunchButton = nullptr;
    QLineEdit *m_commandInput = nullptr;
    std::vector<QPushButton *> m_towerButtons;
    QPixmap m_backgroundImage;
    QPixmap m_cachedBackground;
    QSize m_cachedBackgroundSize;
    std::array<QPixmap, 6> m_towerIcons;
    std::array<QPixmap, 6> m_enemyIcons;
    std::array<QPixmap, 6> m_tileIcons;
    std::array<QPixmap, 6> m_scaledTowerIcons;
    std::array<QPixmap, 6> m_scaledEnemyIcons;
    std::array<QPixmap, 6> m_scaledBossEnemyIcons;
    std::array<QPixmap, 6> m_scaledTileIcons;
    int m_cachedCellSize = 0;

    std::vector<TileKind> m_tiles;
    std::vector<QPoint> m_pathCells;
    std::vector<double> m_pathSegmentLengths;
    std::vector<std::unique_ptr<Enemy>> m_enemies;
    std::vector<std::unique_ptr<Enemy>> m_pendingEnemies;
    std::vector<Tower> m_towers;
    std::vector<Projectile> m_projectiles;
    std::vector<std::vector<WaveEntry>> m_waves;
    std::vector<std::vector<WaveEntry>> m_reversePlans;

    int m_rows = 7;
    int m_cols = 12;
    int m_cellSize = 58;
    QPoint m_origin = QPoint(32, 124);
    QPoint m_hoverCell = QPoint(-1, -1);
    int m_hoveredTowerButtonIndex = -1;
    QPoint m_portalFrom = QPoint(5, 1);
    QPoint m_portalTo = QPoint(8, 3);

    TowerKind m_selectedTower = TowerKind::Shooter;
    int m_nextEnemyId = 1;
    int m_currentWave = 0;
    int m_currentEntry = 0;
    int m_entrySpawned = 0;
    double m_spawnTimer = 0.0;
    bool m_waitingNextWave = false;
    double m_nextWaveTimer = 0.0;
    bool m_reverseMode = false;
    bool m_reversePlanning = false;
    int m_reverseRiftPoints = 0;
    int m_reverseSpentThisWave = 0;
    int m_reverseScenarioIndex = 0;
    int m_reverseDragIndex = -1;
    int m_reverseDragHoverIndex = -1;

    int m_energy = 240;
    int m_initialEnergy = 240;
    int m_levelIndex = 0;
    QString m_levelName = QStringLiteral("钟楼回响");
    QString m_customLevelPath;
    std::array<TowerSpec, 6> m_towerSpecs;
    double m_energyTick = 0.0;
    double m_elapsedSeconds = 0.0;
    double m_freezeSkillCooldown = 0.0;
    double m_deploymentIntroTimer = 0.0;
    double m_freezePulseTimer = 0.0;
    double m_bossAlertTimer = 0.0;
    double m_waveBannerTimer = 0.0;
    QString m_waveBannerText;
    int m_tutorialStep = 0;
    QPoint m_tutorialCell = QPoint(2, 2);
    QPoint m_tutorialResourceCell = QPoint(1, 2);
    QPoint m_tutorialSlowCell = QPoint(3, 2);
    double m_tutorialAnimTime = 0.0;
    bool m_towerDrillActive = false;
    TowerKind m_towerDrillKind = TowerKind::Shooter;
    int m_towerDrillStage = 0;
    QPoint m_towerDrillCell = QPoint(-1, -1);
    QString m_towerDrillTitle;
    QString m_towerDrillBody;
    bool m_enemyGuideActive = false;
    EnemyKind m_enemyGuideKind = EnemyKind::Normal;
    int m_enemyGuideId = -1;
    int m_enemyGuideStage = 1;
    QString m_enemyGuideTitle;
    QString m_enemyGuideBody;
    std::array<bool, 6> m_enemyIntelShown = {};
    std::array<bool, 6> m_towerIntroShownThisRun = {};
    std::vector<PromptMessage> m_promptQueue;
    QString m_promptTitle;
    QString m_promptBody;
    QColor m_promptAccent = QColor(255, 225, 154);
    double m_promptTime = 0.0;
    double m_promptDuration = 0.0;
    PromptLane m_promptLane = PromptLaneBottom;
    double m_intelBannerTimer = 0.0;
    QString m_intelBannerTitle;
    QString m_intelBannerBody;
    QColor m_intelBannerAccent = QColor(255, 225, 154);
    bool m_infoCardActive = false;
    QString m_infoCardTitle;
    QString m_infoCardBody;
    QColor m_infoCardAccent = QColor(255, 225, 154);
    std::vector<VisualPulse> m_visualPulses;
    bool m_paused = false;
    bool m_finished = false;
    bool m_running = false;
    QString m_feedback;
    double m_feedbackTime = 0.0;
    int m_towersBuilt = 0;
    int m_enemiesDefeated = 0;
    GameSummary m_summary;
};

#endif
