#include "mainwindow.h"

#include "editorwidget.h"
#include "gamewidget.h"
#include "levelmanager.h"
#include "progressmanager.h"
#include "soundmanager.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSlider>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace {
struct DialogueLine {
    int speaker = 0;
    QString speakerName;
    QString text;
};

struct DatabaseCardData {
    QString title;
    QString body;
    QString assetName;
    bool locked = false;
};

struct DatabaseSection {
    QString title;
    std::vector<DatabaseCardData> cards;
};

QString bossProtocolName(int levelIndex);
QString bossProtocolBrief(int levelIndex);

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

class BackgroundPage : public QWidget
{
public:
    explicit BackgroundPage(const QString &assetName, QWidget *parent = nullptr)
        : QWidget(parent)
    {
        m_background.load(assetPath(assetName));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        if (!m_background.isNull()) {
            drawCoverPixmap(painter, m_background, rect());
        } else {
            QLinearGradient fallback(rect().topLeft(), rect().bottomRight());
            fallback.setColorAt(0.0, QColor(7, 17, 31));
            fallback.setColorAt(1.0, QColor(8, 34, 54));
            painter.fillRect(rect(), fallback);
        }
        QLinearGradient veil(rect().topLeft(), rect().bottomRight());
        veil.setColorAt(0.0, QColor(3, 8, 18, 120));
        veil.setColorAt(0.52, QColor(3, 8, 18, 170));
        veil.setColorAt(1.0, QColor(3, 8, 18, 130));
        painter.fillRect(rect(), veil);
    }

private:
    QPixmap m_background;
};

class OpeningPage : public QWidget
{
public:
    explicit OpeningPage(const std::function<void()> &onFinished, QWidget *parent = nullptr)
        : QWidget(parent), m_onFinished(onFinished)
    {
        setStyleSheet(QStringLiteral(
            "QPushButton { min-width: 190px; min-height: 42px; font-size: 17px;"
            " border: 1px solid #50d4ff; border-radius: 8px; color: #dff8ff;"
            " background: #102842; }"
            "QPushButton:hover { background: #18395c; border-color: #ffe19a; color: #ffe19a; }"));
        m_skip = new QPushButton(QStringLiteral("接入终端"), this);
        connect(m_skip, &QPushButton::clicked, this, [this]() { beginFinish(); });
        QTimer::singleShot(120, this, []() {
            SoundManager::play(SoundCue::TerminalBoot);
        });
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_time += 0.016;
            if (m_finishing) {
                m_finishTime += 0.016;
                if (m_finishTime > 0.52) {
                    finish();
                    return;
                }
            } else if (m_time > 5.8) {
                m_skip->setText(QStringLiteral("进入修补局"));
            }
            update();
        });
        m_timer.start(16);
    }

protected:
    void resizeEvent(QResizeEvent *) override
    {
        m_skip->setGeometry(width() / 2 - 95, height() - 142, 190, 42);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient bg(QPointF(0, 0), QPointF(width(), height()));
        bg.setColorAt(0.0, QColor(2, 6, 14));
        bg.setColorAt(0.55, QColor(4, 16, 30));
        bg.setColorAt(1.0, QColor(5, 9, 18));
        painter.fillRect(rect(), bg);

        const double cx = width() / 2.0;
        const double cy = height() / 2.0 - 26.0;
        const double pulse = 0.5 + std::sin(m_time * 2.4) * 0.5;
        const double boot = std::clamp(m_time / 2.2, 0.0, 1.0);
        QRadialGradient glow(QPointF(cx, cy), width() * 0.42);
        glow.setColorAt(0.0, QColor(80, 212, 255, static_cast<int>(36 + pulse * 24)));
        glow.setColorAt(0.52, QColor(255, 225, 154, 16));
        glow.setColorAt(1.0, QColor(2, 6, 14, 0));
        painter.fillRect(rect(), glow);

        painter.setPen(QPen(QColor(80, 212, 255, 24), 1));
        const int gridStep = 34;
        const double drift = std::fmod(m_time * 20.0, gridStep);
        for (double x = -gridStep + drift; x < width() + gridStep; x += gridStep) {
            painter.drawLine(QPointF(x, 0), QPointF(x - width() * 0.12, height()));
        }
        for (int y = 82; y < height() - 82; y += 18) {
            painter.drawLine(QPointF(width() * 0.15, y), QPointF(width() * 0.85, y));
        }
        const double scanY = 82 + std::fmod(m_time * 92.0, std::max(1.0, height() - 164.0));
        QLinearGradient scan(QPointF(width() * 0.18, scanY), QPointF(width() * 0.82, scanY));
        scan.setColorAt(0.0, QColor(80, 212, 255, 0));
        scan.setColorAt(0.5, QColor(80, 212, 255, 145));
        scan.setColorAt(1.0, QColor(80, 212, 255, 0));
        painter.setPen(QPen(QBrush(scan), 3));
        painter.drawLine(QPointF(width() * 0.18, scanY), QPointF(width() * 0.82, scanY));

        const QRectF console(cx - 360, cy - 178, 720, 286);
        painter.setPen(QPen(QColor(80, 212, 255, static_cast<int>(45 + boot * 90)), 1));
        painter.setBrush(QColor(3, 10, 22, static_cast<int>(80 + boot * 80)));
        painter.drawRoundedRect(console, 10, 10);
        painter.setPen(QPen(QColor(255, 225, 154, 70), 1));
        painter.drawLine(console.topLeft() + QPointF(26, 18), console.topRight() + QPointF(-26, 18));
        painter.drawLine(console.bottomLeft() + QPointF(26, -18), console.bottomRight() + QPointF(-26, -18));

        painter.setBrush(Qt::NoBrush);
        for (int i = 0; i < 5; ++i) {
            const double r = 72 + i * 40 + std::sin(m_time * 1.5 + i) * 5.0;
            const double start = std::fmod(m_time * (22 + i * 7) + i * 40, 360.0);
            painter.setPen(QPen(QColor(80, 212, 255, 42 + i * 16), 1.4));
            painter.drawArc(QRectF(cx - r, cy - r * 0.38, r * 2, r * 0.76),
                            static_cast<int>(start * 16),
                            static_cast<int>((115 + i * 16) * 16));
            painter.setPen(QPen(QColor(255, 225, 154, 22 + i * 11), 1.1));
            painter.drawEllipse(QPointF(cx, cy), r * 0.62, r * 0.24);
        }

        const QPointF nodeCenter(cx, cy + 70);
        painter.setPen(QPen(QColor(80, 212, 255, 90), 2, Qt::DashLine));
        painter.drawLine(QPointF(cx - 230, nodeCenter.y()), QPointF(cx + 230, nodeCenter.y()));
        for (int i = 0; i < 6; ++i) {
            const double t = i / 5.0;
            const QPointF node(cx - 230 + 460 * t, nodeCenter.y() + std::sin(m_time * 2.1 + i) * 5);
            const bool active = boot > t * 0.85;
            const QColor color = active ? QColor(255, 225, 154) : QColor(80, 212, 255);
            painter.setPen(QPen(color, active ? 2 : 1));
            painter.setBrush(QColor(color.red(), color.green(), color.blue(), active ? 54 : 22));
            painter.drawEllipse(node, active ? 8 + pulse * 2 : 6, active ? 8 + pulse * 2 : 6);
        }

        const double titleAlpha = std::clamp((m_time - 0.45) / 0.9, 0.0, 1.0);
        painter.setPen(QColor(255, 225, 154, static_cast<int>(245 * titleAlpha)));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 58, QFont::Bold));
        const double jitter = m_time < 1.45 ? std::sin(m_time * 55.0) * (1.0 - titleAlpha) * 8.0 : 0.0;
        painter.drawText(QRectF(jitter, cy - 86, width(), 82), Qt::AlignCenter, QStringLiteral("时间修补局"));

        painter.setPen(QColor(157, 216, 255, static_cast<int>(220 * std::clamp((m_time - 1.05) / 0.8, 0.0, 1.0))));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 18, QFont::Bold));
        painter.drawText(QRectF(0, cy + 4, width(), 32), Qt::AlignCenter,
                         QStringLiteral("MAIN TIMELINE REPAIR PROTOCOL"));

        const QStringList lines = {
            QStringLiteral("身份通道：见习修补员 / 接入席位确认"),
            QStringLiteral("主时间线：6 条异常协议锁定"),
            QStringLiteral("通讯队列：玄衡 / 小昼 / 刻度 待命"),
            QStringLiteral("建议流程：开始新任务会先进入序幕")
        };
        const QRectF statusPanel(cx - 370, height() - 284, 740, 86);
        painter.setPen(QPen(QColor(80, 212, 255, 84), 1));
        painter.setBrush(QColor(3, 10, 22, 138));
        painter.drawRoundedRect(statusPanel, 8, 8);
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 13, QFont::Bold));
        for (int i = 0; i < lines.size(); ++i) {
            const double a = std::clamp((m_time - 1.8 - i * 0.35) / 0.35, 0.0, 1.0);
            painter.setPen(QColor(223, 248, 255, static_cast<int>(210 * a)));
            const QRectF lineRect(statusPanel.left() + (i % 2) * statusPanel.width() / 2.0,
                                  statusPanel.top() + 16 + (i / 2) * 34,
                                  statusPanel.width() / 2.0,
                                  28);
            painter.drawText(lineRect.adjusted(16, 0, -16, 0),
                             Qt::AlignCenter | Qt::TextWordWrap,
                             lines[i]);
        }

        if (std::fmod(m_time, 2.4) < 0.12) {
            painter.fillRect(QRectF(cx - 290, cy - 44, 580, 3), QColor(255, 225, 154, 70));
            painter.fillRect(QRectF(cx - 220, cy - 20, 440, 2), QColor(80, 212, 255, 60));
        }

        if (m_finishing) {
            const double connect = std::clamp(m_finishTime / 0.52, 0.0, 1.0);
            painter.fillRect(rect(), QColor(80, 212, 255, static_cast<int>(28 * connect)));
            painter.setPen(QPen(QColor(255, 225, 154, static_cast<int>(210 * connect)), 2));
            painter.drawLine(QPointF(cx - 300 + 300 * connect, cy + 92),
                             QPointF(cx + 300 - 300 * connect, cy + 92));
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 15, QFont::Bold));
            painter.drawText(QRectF(0, cy + 112, width(), 32),
                             Qt::AlignCenter,
                             QStringLiteral("接入确认中"));
        }
    }

private:
    void beginFinish()
    {
        if (m_finishing) {
            return;
        }
        m_finishing = true;
        m_finishTime = 0.0;
        m_skip->setEnabled(false);
        m_skip->setText(QStringLiteral("接入中"));
        SoundManager::play(SoundCue::MissionStart);
    }

    void finish()
    {
        m_timer.stop();
        if (m_onFinished) {
            m_onFinished();
        }
    }

    QTimer m_timer;
    QPushButton *m_skip = nullptr;
    std::function<void()> m_onFinished;
    double m_time = 0.0;
    double m_finishTime = 0.0;
    bool m_finishing = false;
};

class ResultPage : public BackgroundPage
{
public:
    ResultPage(const QString &assetName, bool victory, QWidget *parent = nullptr)
        : BackgroundPage(assetName, parent), m_victory(victory)
    {
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_time += 0.016;
            update();
        });
        m_timer.start(16);
    }

    void restartAnimation()
    {
        restartAnimation(m_victory ? SoundCue::Victory : SoundCue::Failure);
    }

    void restartAnimation(SoundCue cue)
    {
        m_time = 0.0;
        SoundManager::play(cue);
        update();
    }

    void setRatingText(const QString &rating)
    {
        m_ratingText = rating;
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        BackgroundPage::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QColor accent = m_victory ? QColor(255, 225, 154) : QColor(255, 90, 105);
        const double pulse = 0.5 + std::sin(m_time * 3.2) * 0.5;
        QRadialGradient glow(QPointF(width() * 0.5, height() * 0.35), width() * 0.6);
        glow.setColorAt(0.0, QColor(accent.red(), accent.green(), accent.blue(), m_victory ? 55 : 72));
        glow.setColorAt(1.0, QColor(accent.red(), accent.green(), accent.blue(), 0));
        painter.fillRect(rect(), glow);

        painter.setPen(QPen(QColor(accent.red(), accent.green(), accent.blue(), m_victory ? 80 : 120), 2));
        for (int i = 0; i < 4; ++i) {
            const double r = std::fmod(m_time * 95.0 + i * 120.0, width() * 0.55);
            painter.drawEllipse(QPointF(width() / 2.0, height() / 2.0), r, r * 0.42);
        }

        if (!m_victory) {
            painter.fillRect(rect(), QColor(70, 0, 18, static_cast<int>(28 + 18 * pulse)));
        }

        if (!m_ratingText.isEmpty() && m_time > 0.9) {
            const double t = std::clamp((m_time - 0.9) / 0.8, 0.0, 1.0);
            painter.setPen(QColor(accent.red(), accent.green(), accent.blue(), static_cast<int>(210 * t)));
            const int size = m_ratingText.size() > 1 ? static_cast<int>(44 + 8 * t) : static_cast<int>(86 + 18 * t);
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), size, QFont::Bold));
            painter.drawText(QRectF(width() - 300, 86, 240, 132), Qt::AlignCenter, m_ratingText);
            painter.setPen(QPen(QColor(255, 255, 255, static_cast<int>(90 * (1.0 - t))), 4));
            painter.drawEllipse(QPointF(width() - 180, 150), 70 + 70 * t, 40 + 40 * t);
        }
    }

private:
    QTimer m_timer;
    bool m_victory = true;
    double m_time = 0.0;
    QString m_ratingText;
};

class LevelSelectPage : public BackgroundPage
{
public:
    explicit LevelSelectPage(QWidget *parent = nullptr)
        : BackgroundPage(QStringLiteral("bg-main.png"), parent)
    {
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_time += 0.016;
            update();
        });
        m_timer.start(16);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        BackgroundPage::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(80, 212, 255, 34), 1));
        const double scanX = std::fmod(m_time * 130.0, std::max(1, width()));
        painter.drawLine(QPointF(scanX, 0), QPointF(scanX - 180, height()));
        painter.setBrush(Qt::NoBrush);
        for (int i = 0; i < 6; ++i) {
            const double x = width() * (0.12 + i * 0.145);
            const double y = height() * (0.78 + std::sin(m_time * 1.4 + i) * 0.025);
            painter.setPen(QPen(QColor(255, 225, 154, 42), 1.5));
            painter.drawEllipse(QPointF(x, y), 30 + i * 3, 10 + i * 1.5);
            if (i > 0) {
                painter.drawLine(QPointF(width() * (0.12 + (i - 1) * 0.145), y - 8),
                                 QPointF(x, y + 8));
            }
        }
    }

private:
    QTimer m_timer;
    double m_time = 0.0;
};

class StartMenuPage : public BackgroundPage
{
public:
    explicit StartMenuPage(QWidget *parent = nullptr)
        : BackgroundPage(QStringLiteral("bg-main.png"), parent)
    {
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_time += 0.016;
            update();
        });
        m_timer.start(16);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        BackgroundPage::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const double pulse = 0.5 + std::sin(m_time * 2.2) * 0.5;
        QRadialGradient commandGlow(QPointF(width() * 0.5, height() * 0.44), width() * 0.58);
        commandGlow.setColorAt(0.0, QColor(80, 212, 255, static_cast<int>(36 + pulse * 24)));
        commandGlow.setColorAt(0.62, QColor(255, 225, 154, 18));
        commandGlow.setColorAt(1.0, QColor(2, 6, 14, 0));
        painter.fillRect(rect(), commandGlow);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(80, 212, 255, 34), 1));
        const int gridStep = 48;
        const double offset = std::fmod(m_time * 18.0, gridStep);
        for (double x = -gridStep + offset; x < width() + gridStep; x += gridStep) {
            painter.drawLine(QPointF(x, 0), QPointF(x - width() * 0.18, height()));
        }
        for (int y = 86; y < height() - 76; y += 42) {
            painter.drawLine(QPointF(0, y), QPointF(width(), y));
        }

        const QRectF topPanel(width() / 2.0 - 390, 42, 780, 48);
        painter.setPen(QPen(QColor(80, 212, 255, 86), 1));
        painter.setBrush(QColor(5, 14, 28, 118));
        painter.drawRoundedRect(topPanel, 8, 8);
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 12, QFont::Bold));
        painter.setPen(QColor(223, 248, 255, 210));
        painter.drawText(topPanel.adjusted(20, 0, -20, 0),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         QStringLiteral("MAINLINE COMMAND // REPAIR BUREAU"));
        painter.setPen(QColor(255, 225, 154, 220));
        painter.drawText(topPanel.adjusted(20, 0, -20, 0),
                         Qt::AlignVCenter | Qt::AlignRight,
                         QStringLiteral("READY"));

        const double centerY = height() - 116.0;
        const double left = width() * 0.22;
        const double right = width() * 0.78;
        painter.setPen(QPen(QColor(80, 212, 255, 78), 2, Qt::DashLine));
        painter.drawLine(QPointF(left, centerY), QPointF(right, centerY));
        for (int i = 0; i < 6; ++i) {
            const double t = i / 5.0;
            const QPointF node(left + (right - left) * t, centerY + std::sin(m_time * 1.4 + i) * 5.0);
            const QColor nodeColor = i == 5 ? QColor(255, 90, 105) : QColor(255, 225, 154);
            painter.setPen(QPen(nodeColor, 2));
            painter.setBrush(QColor(nodeColor.red(), nodeColor.green(), nodeColor.blue(), static_cast<int>(40 + pulse * 35)));
            painter.drawEllipse(node, 8 + (i == 5 ? pulse * 4 : 0), 8 + (i == 5 ? pulse * 4 : 0));
            painter.setPen(QColor(223, 248, 255, 150));
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9, QFont::Bold));
            painter.drawText(QRectF(node.x() - 28, node.y() + 12, 56, 18),
                             Qt::AlignCenter,
                             QStringLiteral("%1").arg(i + 1, 2, 10, QLatin1Char('0')));
        }
    }

private:
    QTimer m_timer;
    double m_time = 0.0;
};

class MissionBriefingPage : public BackgroundPage
{
public:
    explicit MissionBriefingPage(QWidget *parent = nullptr)
        : BackgroundPage(QStringLiteral("bg-archive.png"), parent)
    {
        setStyleSheet(QStringLiteral(
            "QWidget { background: transparent; color: #f5d98b; }"
            "QLabel#title { font-size: 44px; font-weight: 700; color: #ffe19a; }"
            "QLabel#subtitle { font-size: 18px; color: #9ed8ff; }"
            "QPushButton { min-width: 240px; min-height: 44px; font-size: 18px;"
            "  border: 1px solid #50d4ff; border-radius: 8px; color: #dff8ff;"
            "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #173754, stop:1 #0c2038); }"
            "QPushButton:hover { background: #1f4667; border-color: #ffe19a; color: #ffe19a; }"));
        auto *outer = new QVBoxLayout(this);
        outer->setContentsMargins(76, 46, 76, 42);
        outer->setAlignment(Qt::AlignCenter);

        auto *content = new QWidget(this);
        content->setMaximumWidth(920);
        content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto *layout = new QVBoxLayout(content);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(14);

        m_title = new QLabel(this);
        m_title->setObjectName(QStringLiteral("title"));
        m_title->setAlignment(Qt::AlignCenter);
        m_subtitle = new QLabel(this);
        m_subtitle->setObjectName(QStringLiteral("subtitle"));
        m_subtitle->setAlignment(Qt::AlignCenter);

        m_body = new QLabel(this);
        m_body->setWordWrap(true);
        m_body->setTextFormat(Qt::RichText);
        m_body->setMinimumWidth(640);
        m_body->setMaximumWidth(860);
        m_body->setStyleSheet(QStringLiteral(
            "QLabel { color: #dff8ff; font-size: 18px; line-height: 132%;"
            " background: rgba(5, 14, 28, 178); border: 1px solid rgba(80, 212, 255, 110);"
            " border-radius: 8px; padding: 22px 28px; }"));

        m_startButton = new QPushButton(QStringLiteral("开始修补"), this);
        connect(m_startButton, &QPushButton::clicked, this, [this]() { finish(); });
        connect(&m_timer, &QTimer::timeout, this, [this]() {
            m_time += 0.016;
            update();
        });
        m_timer.start(16);

        layout->addWidget(m_title, 0, Qt::AlignCenter);
        layout->addWidget(m_subtitle, 0, Qt::AlignCenter);
        layout->addSpacing(24);
        layout->addWidget(m_body, 0, Qt::AlignCenter);
        layout->addSpacing(14);
        layout->addWidget(m_startButton, 0, Qt::AlignCenter);
        outer->addWidget(content, 0, Qt::AlignCenter);
    }

    void setMission(int index, const std::function<void()> &onFinished)
    {
        m_levelIndex = std::clamp(index, 0, LevelManager::builtInLevelCount() - 1);
        m_onFinished = onFinished;
        m_time = 0.0;
        m_title->setText(QStringLiteral("%1 任务投影").arg(LevelManager::levelTitle(m_levelIndex)));
        m_subtitle->setText(QStringLiteral("%1 · %2 · %3")
                                .arg(chapterForLevel(m_levelIndex),
                                     LevelManager::levelDifficulty(m_levelIndex),
                                     threatForLevel(m_levelIndex)));
        m_body->setText(QStringLiteral(
                            "<table width='100%' cellspacing='0' cellpadding='7'>"
                            "<tr><td width='74' valign='top'><b style='color:#ffe19a;'>威胁</b></td><td>%1</td></tr>"
                            "<tr><td valign='top'><b style='color:#ffe19a;'>授权</b></td><td>%2</td></tr>"
                            "<tr><td valign='top'><b style='color:#ffe19a;'>策略</b></td><td>%3</td></tr>"
                            "<tr><td valign='top'><b style='color:#ffe19a;'>目标</b></td><td><span style='color:#ffe19a;'>守住核心，封存 %4。</span></td></tr>"
                            "</table>")
                            .arg(shortThreatForLevel(m_levelIndex),
                                 shortAuthorizationForLevel(m_levelIndex),
                                 shortRecommendationForLevel(m_levelIndex),
                                 bossProtocolName(m_levelIndex)));
        SoundManager::play(SoundCue::MissionStart);
        update();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        BackgroundPage::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QRectF radar = m_body
                                  ? QRectF(m_body->geometry()).adjusted(-1, -1, 1, 1)
                                  : QRectF(width() / 2.0 - 430, height() / 2.0 - 118, 860, 236);
        painter.setPen(QPen(QColor(80, 212, 255, 72), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(radar, 10, 10);
        const double sweep = std::fmod(m_time * 240.0, radar.width());
        painter.setPen(QPen(QColor(255, 225, 154, 125), 3));
        painter.drawLine(QPointF(radar.left() + sweep, radar.top()),
                         QPointF(radar.left() + sweep - 110, radar.bottom()));
        painter.setPen(QPen(QColor(80, 212, 255, 42), 1));
        for (int i = 0; i < 9; ++i) {
            const double x = radar.left() + 42 + i * ((radar.width() - 84.0) / 8.0);
            const double y = radar.center().y() + std::sin(m_time * 1.8 + i) * 52;
            painter.drawEllipse(QPointF(x, y), 7, 7);
            if (i > 0) {
                const double px = radar.left() + 42 + (i - 1) * ((radar.width() - 84.0) / 8.0);
                const double py = radar.center().y() + std::sin(m_time * 1.8 + i - 1) * 52;
                painter.drawLine(QPointF(px, py), QPointF(x, y));
            }
        }
    }

private:
    static QString chapterForLevel(int index)
    {
        static const std::array<QString, 6> texts = {
            QStringLiteral("第一章：接入训练线"),
            QStringLiteral("第二章：城市交通线"),
            QStringLiteral("第三章：雾港跃迁线"),
            QStringLiteral("第四章：档案逆流线"),
            QStringLiteral("第五章：深时供能线"),
            QStringLiteral("终章：终末封存线")
        };
        return texts[static_cast<size_t>(std::clamp(index, 0, 5))];
    }

    static QString threatForLevel(int index)
    {
        static const std::array<QString, 6> texts = {
            QStringLiteral("威胁等级 I"),
            QStringLiteral("威胁等级 II"),
            QStringLiteral("威胁等级 III"),
            QStringLiteral("威胁等级 IV"),
            QStringLiteral("威胁等级 V"),
            QStringLiteral("威胁等级 Ω")
        };
        return texts[static_cast<size_t>(std::clamp(index, 0, 5))];
    }

    static QString chapterBriefForLevel(int index)
    {
        static const std::array<QString, 6> texts = {
            QStringLiteral("训练线被真实异常入侵，校准守门者正在测试你的第一条防线。"),
            QStringLiteral("异常开始攻击公共交通节点，疾行站台长把城市节奏推向失控。"),
            QStringLiteral("传送裂隙被人为配对，雾港折跃核暴露出幕后协议的铺路能力。"),
            QStringLiteral("档案记录被反向改写，逆流编目者开始利用未来失败制造当前压力。"),
            QStringLiteral("能源井仍在供能，深井护盾体逼迫修补行动转入资源运营。"),
            QStringLiteral("所有协议在失败时间线汇合，纪元主宰压迫完整防线与资源判断。")
        };
        return texts[static_cast<size_t>(std::clamp(index, 0, 5))];
    }

    static QString shortThreatForLevel(int index)
    {
        static const std::array<QString, 6> texts = {
            QStringLiteral("基础残影与弱化 Boss，检验拐角火力、升级和冻结时机。"),
            QStringLiteral("高速敌人与密集站台压缩反应时间。"),
            QStringLiteral("传送裂隙会改写路线压力，Boss 会短距折跃。"),
            QStringLiteral("分裂与抗性单位叠加，防线需要拖延和范围清理。"),
            QStringLiteral("高护盾敌人逼迫你把经济及时换成火力。"),
            QStringLiteral("多阶段 Boss 与混合波次同时压测全装置协同。")
        };
        return texts[static_cast<size_t>(std::clamp(index, 0, 5))];
    }

    static QString shortRecommendationForLevel(int index)
    {
        static const std::array<QString, 6> texts = {
            QStringLiteral("炮台守拐角，凝滞棱镜放 Boss 前，冻结留给最后压力。"),
            QStringLiteral("震荡器处理密集波，出口前补减速。"),
            QStringLiteral("切割器守长直线，传送出口留第二层火力。"),
            QStringLiteral("屏障拖住关键点，范围塔覆盖分裂死亡点。"),
            QStringLiteral("经济塔早放但别贪，Boss 前必须完成输出升级。"),
            QStringLiteral("穿透、范围、减速、屏障都要覆盖，冻结留给终末阶段。")
        };
        return texts[static_cast<size_t>(std::clamp(index, 0, 5))];
    }

    static QString shortAuthorizationForLevel(int index)
    {
        static const std::array<QString, 6> texts = {
            QStringLiteral("指针炮台、凝滞棱镜、时能汲取仪。"),
            QStringLiteral("新增悖论震荡器。"),
            QStringLiteral("新增因果切割器。"),
            QStringLiteral("新增锚点屏障。"),
            QStringLiteral("全装置开放，重点练习资源运营。"),
            QStringLiteral("全装置开放，终局协议汇合。")
        };
        return texts[static_cast<size_t>(std::clamp(index, 0, 5))];
    }

    static QString recommendationForLevel(int index)
    {
        static const std::array<QString, 6> texts = {
            QStringLiteral("指针炮台 + 凝滞棱镜，先学会覆盖拐角、升级和在 Boss 进弯时冻结。"),
            QStringLiteral("悖论震荡器 + 凝滞棱镜，用范围伤害处理密集波，并在 Boss 前铺出口减速。"),
            QStringLiteral("因果切割器 + 悖论震荡器，入口和传送出口都要覆盖，避免 Boss 折跃后直冲核心。"),
            QStringLiteral("锚点屏障 + 范围伤害 + 减速交叉火力，处理分裂、抗性和关底拖延。"),
            QStringLiteral("时能汲取仪提前经营，同时保证核心前线火力足够击破护盾。"),
            QStringLiteral("全装置协同，保留冻结技能应对纪元主宰和终局混合波。")
        };
        return texts[static_cast<size_t>(std::clamp(index, 0, 5))];
    }

    static QString authorizationForLevel(int index)
    {
        static const std::array<QString, 6> texts = {
            QStringLiteral("初始授权：指针炮台、凝滞棱镜、时能汲取仪。"),
            QStringLiteral("新增授权：悖论震荡器，开始处理密集敌群。"),
            QStringLiteral("新增授权：因果切割器，用于长直线和传送出口穿透。"),
            QStringLiteral("新增授权：锚点屏障，可在路径上阻挡非端点敌人。"),
            QStringLiteral("全装置开放：重点练习资源运营与组合防线。"),
            QStringLiteral("全装置开放：终局协议汇合所有地形、敌人与装置组合。")
        };
        return texts[static_cast<size_t>(std::clamp(index, 0, 5))];
    }

    void finish()
    {
        SoundManager::play(SoundCue::UiClick);
        if (m_onFinished) {
            m_onFinished();
        }
    }

    QTimer m_timer;
    QLabel *m_title = nullptr;
    QLabel *m_subtitle = nullptr;
    QLabel *m_body = nullptr;
    QPushButton *m_startButton = nullptr;
    std::function<void()> m_onFinished;
    int m_levelIndex = 0;
    double m_time = 0.0;
};

class DialoguePage : public BackgroundPage
{
public:
    explicit DialoguePage(QWidget *parent = nullptr)
        : BackgroundPage(QStringLiteral("bg-archive.png"), parent)
    {
        m_operatorSheet.load(assetPath(QStringLiteral("sheet-operators.png")));
        setStyleSheet(QStringLiteral(
            "QPushButton { min-width: 170px; min-height: 42px; font-size: 17px;"
            " border: 1px solid #50d4ff; border-radius: 8px; color: #dff8ff;"
            " background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #173754, stop:1 #0c2038); }"
            "QPushButton:hover { background: #1f4667; border-color: #ffe19a; color: #ffe19a; }"));

        auto *outer = new QVBoxLayout(this);
        outer->setContentsMargins(58, 42, 58, 42);
        outer->setSpacing(16);

        m_title = new QLabel(this);
        m_title->setObjectName(QStringLiteral("title"));
        m_title->setAlignment(Qt::AlignCenter);
        m_title->setStyleSheet(QStringLiteral("QLabel#title { font-size: 36px; font-weight: 700; color: #ffe19a; }"));

        auto *stage = new QHBoxLayout();
        stage->setSpacing(24);

        m_portraitFrame = new QFrame(this);
        m_portraitFrame->setStyleSheet(QStringLiteral(
            "QFrame { background: rgba(4, 12, 24, 178); border: 1px solid rgba(255, 225, 154, 150); border-radius: 8px; }"));
        auto *portraitLayout = new QVBoxLayout(m_portraitFrame);
        portraitLayout->setContentsMargins(14, 14, 14, 14);
        m_portrait = new QLabel(m_portraitFrame);
        m_portrait->setMinimumSize(260, 330);
        m_portrait->setAlignment(Qt::AlignCenter);
        m_portrait->setScaledContents(false);
        portraitLayout->addWidget(m_portrait);

        auto *dialogueFrame = new QFrame(this);
        dialogueFrame->setStyleSheet(QStringLiteral(
            "QFrame { background: rgba(5, 14, 28, 190); border: 1px solid rgba(80, 212, 255, 120); border-radius: 8px; }"));
        auto *dialogueLayout = new QVBoxLayout(dialogueFrame);
        dialogueLayout->setContentsMargins(24, 22, 24, 22);
        dialogueLayout->setSpacing(14);

        m_speaker = new QLabel(dialogueFrame);
        m_speaker->setStyleSheet(QStringLiteral("QLabel { color: #ffe19a; font-size: 20px; font-weight: 700; }"));
        m_text = new QLabel(dialogueFrame);
        m_text->setWordWrap(true);
        m_text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        m_text->setStyleSheet(QStringLiteral("QLabel { color: #eaf8ff; font-size: 20px; line-height: 150%; }"));
        m_progress = new QLabel(dialogueFrame);
        m_progress->setAlignment(Qt::AlignRight);
        m_progress->setStyleSheet(QStringLiteral("QLabel { color: #9ed8ff; font-size: 14px; }"));

        dialogueLayout->addWidget(m_speaker);
        dialogueLayout->addWidget(m_text, 1);
        dialogueLayout->addWidget(m_progress);

        stage->addWidget(m_portraitFrame, 0);
        stage->addWidget(dialogueFrame, 1);

        auto *buttons = new QHBoxLayout();
        buttons->setSpacing(12);
        buttons->addStretch();
        m_skipButton = new QPushButton(QStringLiteral("跳过通讯"), this);
        m_nextButton = new QPushButton(QStringLiteral("下一句"), this);
        buttons->addWidget(m_skipButton);
        buttons->addWidget(m_nextButton);

        outer->addWidget(m_title);
        outer->addLayout(stage, 1);
        outer->addLayout(buttons);

        connect(m_nextButton, &QPushButton::clicked, this, [this]() { advance(); });
        connect(m_skipButton, &QPushButton::clicked, this, [this]() { finish(); });
        connect(&m_typeTimer, &QTimer::timeout, this, [this]() { revealNextCharacter(); });
        connect(&m_animTimer, &QTimer::timeout, this, [this]() {
            m_animTime += 0.033;
            const int glow = static_cast<int>(135 + 90 * (0.5 + std::sin(m_animTime * 4.2) * 0.5));
            m_portraitFrame->setStyleSheet(QStringLiteral(
                "QFrame { background: rgba(4, 12, 24, 178); border: 1px solid rgba(255, 225, 154, %1); border-radius: 8px; }")
                                               .arg(glow));
            update();
        });
        m_animTimer.start(33);
    }

    void setDialogue(const QString &title,
                     const std::vector<DialogueLine> &lines,
                     const QString &finishText,
                     const std::function<void()> &onFinished)
    {
        m_title->setText(title);
        m_lines = lines;
        m_finishText = finishText;
        m_onFinished = onFinished;
        m_index = 0;
        m_animTime = 0.0;
        showCurrentLine();
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        BackgroundPage::paintEvent(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(QColor(80, 212, 255, 24), 1));
        const int offset = static_cast<int>(std::fmod(m_animTime * 42.0, 10.0));
        for (int y = offset; y < height(); y += 10) {
            painter.drawLine(0, y, width(), y);
        }
        painter.setPen(QPen(QColor(255, 225, 154, 60), 2));
        const double sweep = std::fmod(m_animTime * 230.0, std::max(1, width()));
        painter.drawLine(QPointF(sweep, 0), QPointF(sweep - 180, height()));
    }

private:
    void advance()
    {
        SoundManager::play(SoundCue::UiClick);
        if (m_typeTimer.isActive()) {
            m_typeTimer.stop();
            m_visibleChars = m_fullText.size();
            m_text->setText(m_fullText);
            updateButtonText();
            return;
        }
        if (m_index + 1 >= static_cast<int>(m_lines.size())) {
            finish();
            return;
        }
        ++m_index;
        showCurrentLine();
    }

    void finish()
    {
        m_typeTimer.stop();
        SoundManager::play(SoundCue::MissionStart);
        if (m_onFinished) {
            m_onFinished();
        }
    }

    void showCurrentLine()
    {
        if (m_lines.empty()) {
            m_speaker->setText(QStringLiteral("系统"));
            m_text->setText(QStringLiteral("通讯已建立。"));
            m_progress->clear();
            m_nextButton->setText(m_finishText);
            return;
        }

        const DialogueLine &line = m_lines[static_cast<size_t>(m_index)];
        m_speaker->setText(line.speakerName);
        m_fullText = line.text;
        m_visibleChars = 0;
        m_text->clear();
        m_progress->setText(QStringLiteral("%1 / %2").arg(m_index + 1).arg(m_lines.size()));
        m_nextButton->setText(QStringLiteral("接收中"));
        m_typeTimer.start(24);
        SoundManager::play(SoundCue::Dialogue);

        if (!m_operatorSheet.isNull()) {
            const int columns = 3;
            const int sourceW = m_operatorSheet.width() / columns;
            const int sourceH = m_operatorSheet.height();
            const int col = std::clamp(line.speaker, 0, columns - 1);
            QPixmap portrait = m_operatorSheet.copy(col * sourceW, 0, sourceW, sourceH);
            m_portrait->setPixmap(portrait.scaled(m_portrait->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            m_portrait->setText(line.speakerName.left(2));
        }
    }

    void revealNextCharacter()
    {
        if (m_visibleChars >= m_fullText.size()) {
            m_typeTimer.stop();
            updateButtonText();
            return;
        }
        m_visibleChars += 1;
        m_text->setText(m_fullText.left(m_visibleChars));
        if (m_visibleChars % 5 == 0) {
            SoundManager::play(SoundCue::Dialogue);
        }
    }

    void updateButtonText()
    {
        m_nextButton->setText(m_index + 1 >= static_cast<int>(m_lines.size()) ? m_finishText : QStringLiteral("下一句"));
    }

    QLabel *m_title = nullptr;
    QFrame *m_portraitFrame = nullptr;
    QLabel *m_portrait = nullptr;
    QLabel *m_speaker = nullptr;
    QLabel *m_text = nullptr;
    QLabel *m_progress = nullptr;
    QPushButton *m_nextButton = nullptr;
    QPushButton *m_skipButton = nullptr;
    QPixmap m_operatorSheet;
    std::vector<DialogueLine> m_lines;
    std::function<void()> m_onFinished;
    QString m_finishText;
    QString m_fullText;
    QTimer m_typeTimer;
    QTimer m_animTimer;
    int m_index = 0;
    int m_visibleChars = 0;
    double m_animTime = 0.0;
};

QString pageStyle()
{
    return QStringLiteral(
        "QWidget { background: transparent; color: #f5d98b; }"
        "QLabel#title { font-size: 48px; font-weight: 700; color: #ffe19a; }"
        "QLabel#subtitle { font-size: 18px; color: #9ed8ff; }"
        "QPushButton { min-width: 240px; min-height: 44px; font-size: 18px; font-weight: 650;"
        "  border: 1px solid #50d4ff; border-radius: 8px; color: #dff8ff;"
        "  padding: 6px 20px;"
        "  background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #173754, stop:1 #0c2038); }"
        "QPushButton:hover { background: #1f4667; border-color: #ffe19a; color: #ffe19a; }"
        "QPushButton:pressed { background: #0b1b2d; border-color: #9ed8ff; }"
        "QPushButton:disabled { color: rgba(190, 208, 220, 120); border-color: rgba(126, 148, 165, 85); background: rgba(8, 18, 30, 160); }"
        "QCheckBox { color: #dff8ff; font-size: 18px; spacing: 10px; }"
        "QCheckBox::indicator { width: 20px; height: 20px; border: 1px solid #50d4ff; border-radius: 4px; background: #07111f; }"
        "QCheckBox::indicator:checked { background: #ffe19a; border-color: #ffe19a; }");
}

QString progressSummaryText()
{
    const int highest = std::clamp(ProgressManager::highestUnlockedLevel(), 0, LevelManager::builtInLevelCount() - 1);
    const int last = std::clamp(ProgressManager::lastLevelIndex(), 0, LevelManager::builtInLevelCount() - 1);
    return QStringLiteral("当前任务：%1/6  %2    最高授权：%3/6\n继续修补回到最近任务；开始新任务会从序幕重新接入。")
        .arg(last + 1)
        .arg(LevelManager::levelTitle(last))
        .arg(highest + 1);
}

bool confirmClearProgress(QWidget *parent)
{
    QDialog dialog(parent);
    dialog.setModal(true);
    dialog.setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog.setAttribute(Qt::WA_TranslucentBackground, true);
    dialog.setStyleSheet(pageStyle()
                         + QStringLiteral(
                             "QDialog { background: transparent; }"
                             "QFrame#confirmPanel { background: rgba(5, 14, 28, 232); border: 1px solid rgba(255, 225, 154, 150); border-radius: 8px; }"
                             "QLabel#confirmTitle { color: #ffe19a; font-size: 24px; font-weight: 700; border: 0; background: transparent; }"
                             "QLabel#confirmBody { color: #dff8ff; font-size: 15px; border: 0; background: transparent; }"
                             "QPushButton#confirmSecondary, QPushButton#confirmDanger { min-width: 144px; min-height: 38px; font-size: 15px; }"
                             "QPushButton#confirmDanger { color: #ffe19a; border-color: #ffe19a; background: rgba(91, 45, 30, 225); }"
                             "QPushButton#confirmDanger:hover { background: rgba(133, 67, 42, 235); }"));

    auto *outer = new QVBoxLayout(&dialog);
    outer->setContentsMargins(0, 0, 0, 0);
    auto *panel = new QFrame(&dialog);
    panel->setObjectName(QStringLiteral("confirmPanel"));
    auto *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("清空本机进度"), panel);
    title->setObjectName(QStringLiteral("confirmTitle"));
    title->setAlignment(Qt::AlignCenter);
    auto *body = new QLabel(QStringLiteral("将清空关卡进度、序幕状态和图鉴解锁记录。\n音量与教学开关会保留。"), panel);
    body->setObjectName(QStringLiteral("confirmBody"));
    body->setAlignment(Qt::AlignCenter);
    body->setWordWrap(true);

    auto *buttons = new QHBoxLayout();
    buttons->setContentsMargins(0, 0, 0, 0);
    buttons->setSpacing(14);
    auto *cancel = new QPushButton(QStringLiteral("取消"), panel);
    cancel->setObjectName(QStringLiteral("confirmSecondary"));
    auto *confirm = new QPushButton(QStringLiteral("清空进度"), panel);
    confirm->setObjectName(QStringLiteral("confirmDanger"));
    QObject::connect(cancel, &QPushButton::clicked, &dialog, [&dialog]() {
        SoundManager::play(SoundCue::UiClick);
        dialog.reject();
    });
    QObject::connect(confirm, &QPushButton::clicked, &dialog, [&dialog]() {
        SoundManager::play(SoundCue::UiClick);
        dialog.accept();
    });
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(confirm);
    buttons->addStretch();

    layout->addWidget(title);
    layout->addWidget(body);
    layout->addLayout(buttons);
    outer->addWidget(panel);
    dialog.resize(520, 210);
    return dialog.exec() == QDialog::Accepted;
}

QString minutesSeconds(double seconds)
{
    const int total = std::max(0, static_cast<int>(std::round(seconds)));
    return QStringLiteral("%1:%2")
        .arg(total / 60, 2, 10, QLatin1Char('0'))
        .arg(total % 60, 2, 10, QLatin1Char('0'));
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

QString bossProtocolBrief(int levelIndex)
{
    static const std::array<QString, 6> texts = {
        QStringLiteral("弱化 Boss，会在护盾被击破后重启一次护盾，教学冻结时机。"),
        QStringLiteral("高速 Boss，会周期性冲刺，检验出口前减速和站台范围火力。"),
        QStringLiteral("折跃 Boss，低血量时向前短距跃迁并补盾，要求传送出口有第二层防线。"),
        QStringLiteral("复制 Boss，血量下降时召唤高速、抗性和分裂单位，检验屏障与范围清理。"),
        QStringLiteral("护盾 Boss，会周期性回充护盾，逼迫玩家把经济转化成升级火力。"),
        QStringLiteral("多阶段 Boss，血量下降时补盾并召唤混合单位，是最终协议汇合。")
    };
    return texts[static_cast<size_t>(std::clamp(levelIndex, 0, 5))];
}

[[maybe_unused]] std::vector<DialogueLine> prologueDialogue()
{
    return {
        {0, QStringLiteral("局长 玄衡"), QStringLiteral("2079 年 6 月 30 日，城市主时间线出现连续断裂。正式修补员全部外勤，总部只剩一名见习生在线。")},
        {1, QStringLiteral("导航员 小昼"), QStringLiteral("也就是你。别紧张，我会在频道里带路。我们不需要把时间打碎，只要在异常体抵达核心前，把秩序接回去。")},
        {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("基础规则：异常沿高亮路径移动；稳定格可以部署装置；右键升级；F 键释放时序冻结；空格暂停。")},
        {1, QStringLiteral("导航员 小昼"), QStringLiteral("第一次玩建议先进入 01 钟楼回响。它会教你建塔、升级、冻结和资源格，后面的车站、雾港、档案馆会逐步加压。")},
        {0, QStringLiteral("局长 玄衡"), QStringLiteral("别把这当练习。训练线已经失控，下一次警报会从真实城市传来。修补员，接入任务。")}
    };
}

[[maybe_unused]] std::vector<DialogueLine> introDialogueForLevel(int levelIndex)
{
    const QString title = LevelManager::levelTitle(levelIndex);
    switch (std::clamp(levelIndex, 0, LevelManager::builtInLevelCount() - 1)) {
    case 0:
        return {
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("%1 失控。训练场的安全锁被异常绕过，你现在临时接管核心防线。").arg(title)},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("先看路径：异常只会沿亮起的因果轨道前进。把指针炮台放在能覆盖拐角的位置，火力会更稳。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("初始授权：指针炮台、凝滞棱镜、时能汲取仪。教学会暂停讲解路线、资源、敌人属性、升级和冻结。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("如果漏怪，别慌。观察它在哪里突破，下次把减速或屏障放在核心前的关键格。")}
        };
    case 1:
        return {
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("车站不是事故。异常开始选择人流节点，说明它有目的。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("站台被切成两段，疾行错帧会借加速格冲线。本关新增悖论震荡器，用范围伤害处理密集波次。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("建议：入口用稳定输出，出口前放减速。敌人密度上升时，用震荡器覆盖拐角，而不是盲目铺满。")}
        };
    case 2:
        return {
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("雾港传送裂隙的两端时间戳完全一致。结论：它们被人为配对。")},
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("有人在替异常铺路。别追问是谁，先让这条路断掉。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("入口和出口都要有火力。本关授权因果切割器，传送后的长直线正适合用穿透光束扫过去。")}
        };
    case 3:
        return {
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("档案馆记录被改写。死亡名单里出现了还没进入战场的敌人。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("分裂悖影会在被击破后生成子单位，抗性回声会降低受到的伤害。单一路径输出不足以稳定清场。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("本关授权锚点屏障。把火力做成交叉面：屏障拖住关键点，减速留住第一批，范围塔处理分裂。")}
        };
    case 4:
        return {
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("深时能源井还在供能，但污染会抢走节奏。我们要边修边养经济。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("时能汲取仪越早部署，收益越高；但它没有攻击能力，需要放在相对安全的位置。")},
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("别为了攒资源丢核心。能源只是手段，防线才是答案。")}
        };
    default:
        return {
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("终末纪元已打开。所有异常协议在这里汇合，这是修补局最后一道门。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("检测到 Boss、分裂、抗性、传送和加速协议并行。推荐建立多层防线，而非依赖单个高等级装置。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("你已经学完全部工具了。稳住入口、守住出口、留一手冻结，最后把今天抢回来。")}
        };
    }
}

std::vector<DialogueLine> polishedPrologueDialogue()
{
    return {
        {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("主时间线失去连续性。城市时钟在同一秒重复 117 次，交通、港口、档案馆和能源井同时出现回流。")},
        {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("异常体正在沿可计算路径向核心推进。若核心被抵达，当前城市日会被覆盖为失败时间线。")},
        {0, QStringLiteral("局长 玄衡"), QStringLiteral("我是玄衡，时间修补局现任局长。正式修补员全部外勤，总部只剩一个还能接入战术席位的人：你。")},
        {1, QStringLiteral("导航员 小昼"), QStringLiteral("我是小昼，你的导航员。别被“拯救时间线”吓住，我们先做小事：看路线，放装置，升级关键点，必要时冻结全场。")},
        {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("我是刻度，负责监测异常协议。基础规则：异常体沿高亮轨道前进；稳定格可部署装置；右键升级；F 键释放时序冻结。")},
        {0, QStringLiteral("局长 玄衡"), QStringLiteral("六条异常协议已经命名：守门者、站台长、折跃核、编目者、护盾体，以及最后的纪元主宰。每一条都会在关底现身。")},
        {1, QStringLiteral("导航员 小昼"), QStringLiteral("第一条训练线已经被入侵，但它仍保留教学系统。我会暂停战场，一步步带你完成第一次部署。")},
        {0, QStringLiteral("局长 玄衡"), QStringLiteral("从现在起，这不是练习。见习修补员，接入钟楼回响，把今天从失败结局里抢回来。")}
    };
}

std::vector<DialogueLine> polishedIntroDialogueForLevel(int levelIndex)
{
    switch (std::clamp(levelIndex, 0, LevelManager::builtInLevelCount() - 1)) {
    case 0:
        return {
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("钟楼回响原本只是训练线。现在安全锁被异常撬开，第一批残影已经沿着教学轨道回流。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("本关将以暂停引导接管战场：输出塔、经济塔、减速塔、升级和时序冻结会逐项实操。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("关底协议：校准守门者。它是弱化 Boss，但带有护盾和警报演出，用来确认你理解冻结时机。")},
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("别急着证明自己。先活下来，再学会复盘。修补员的第一课，是把一次失败变成下一次判断。")}
        };
    case 1:
        return {
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("断裂车站不是随机事故。异常选择了人流节点，说明它已经懂得哪里最容易让城市失序。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("站台广播一直倒放同一句话：别上车。高速残影会借加速格压线，本关授权悖论震荡器处理密集波。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("关底协议：疾行站台长。它会把车站的加速压力集中到最后一波，出口前减速必须提前成型。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("战术建议：入口建立稳定输出，出口前铺减速；震荡器覆盖拥堵点，收益高于平均铺开。")}
        };
    case 2:
        return {
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("雾港传送裂隙成对出现。入口与出口的时间戳完全一致，判定为被外部协议手动配对。")},
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("有人在替异常铺路。先别追问名字，把它留下的路切断。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("本关授权因果切割器。传送出口后的长直线很适合穿透光束，火力必须从入口延伸到出口。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("关底协议：雾港折跃核。它会把传送出口变成第二个入口，不要把所有资源都压在起点。")}
        };
    case 3:
        return {
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("逆流档案馆的死亡名单被提前写好。记录里出现了还没进入战场的敌人，像有人在预支我们的失败。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("分叉时间体死亡后会生成子单位，抗滞异常体会削弱控制效果。单点输出不足以稳定清场。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("本关授权锚点屏障。把屏障放在路线上拖住第一批，范围塔处理分裂，减速塔守住漏网单位。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("关底协议：逆流编目者。它会把分裂和抗性压力集中写入最终波，屏障不是装饰，是买时间。")}
        };
    case 4:
        return {
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("深时能源井还在供能，但污染正在偷走节奏。这里不是单纯守线，我们要边修边养经济。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("时能汲取仪越早部署收益越高，但它没有攻击能力。资源只是手段，防线才是答案。")},
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("关底协议：深井护盾体。它会检验你是否把经济及时换成升级火力。保留冻结，别把资源攒成遗憾。")}
        };
    default:
        return {
            {0, QStringLiteral("局长 玄衡"), QStringLiteral("终末纪元已经打开。所有异常协议在这里汇合，像一条从未来倒灌回来的失败时间线。")},
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("关底协议：纪元主宰。检测到 Boss、分裂、抗性、传送和加速协议并行。建议建立多层阵线，而不是依赖单个高等级装置。")},
            {1, QStringLiteral("导航员 小昼"), QStringLiteral("你已经学完全部工具了。稳住入口，守住出口，留一手冻结。最后，把今天抢回来。")}
        };
    }
}

QString failureAdviceForSummary(const GameSummary &summary)
{
    if (summary.towersBuilt <= 2) {
        return QStringLiteral("防线密度偏低。下一次先在拐角和核心前各放一座输出塔，再考虑补经济。");
    }
    if (summary.energy >= 220 && summary.wavesCleared < summary.totalWaves) {
        return QStringLiteral("时能还有结余，说明资源没有及时转化成火力。下一次优先升级已有塔，别让时能躺在仓库里。");
    }
    if (summary.wavesCleared <= std::max(1, summary.totalWaves / 3)) {
        return QStringLiteral("前期防线成型太慢。开局先用低费塔覆盖路径拐角，再把减速放在出口前。");
    }
    switch (std::clamp(summary.levelIndex, 0, LevelManager::builtInLevelCount() - 1)) {
    case 1:
        return QStringLiteral("断裂车站的加速格会突然压线。把凝滞棱镜和锚点屏障前移到加速带之后。");
    case 2:
        return QStringLiteral("雾港的传送出口不能空着。入口守不住所有异常，出口附近需要范围塔补第二层火力。");
    case 3:
        return QStringLiteral("档案馆的分裂敌人会扩大压力。范围伤害要覆盖死亡点，减速要留住子单位。");
    case 4:
        return QStringLiteral("能源井不能只攒经济。时能汲取仪要早放，但核心前线至少保留两层攻击火力。");
    case 5:
        return QStringLiteral("终末纪元需要多层阵线。Boss 出现前保留时序冻结，并让穿透、范围和减速同时覆盖主路。");
    default:
        return QStringLiteral("核心前最后三格需要减速、屏障或范围火力。失败点越靠后，越说明出口防线要加厚。");
    }
}

QString performanceReviewForSummary(const GameSummary &summary, bool victory)
{
    if (!victory) {
        return failureAdviceForSummary(summary);
    }
    if (summary.totalWaves > 0 && summary.wavesCleared >= summary.totalWaves
        && summary.energy >= 260 && summary.elapsedSeconds <= 180.0) {
        return QStringLiteral("节奏优秀：用时和时能结余都处于高位，说明火力覆盖与升级时机都比较成熟。");
    }
    if (summary.energy >= 260) {
        return QStringLiteral("资源结余偏高：可以更早升级核心输出塔，把安全优势转化成更短通关时间。");
    }
    if (summary.energy < 90 && summary.towersBuilt >= 12) {
        return QStringLiteral("资源使用很积极，但防线略偏铺开。下一次可少建一两座低级塔，集中培养关键拐角。");
    }
    if (summary.towersBuilt <= 4 && summary.levelIndex >= 2) {
        return QStringLiteral("精简通关：塔数很少，说明关键点判断准确；后续高压关建议提前补一层减速或范围火力。");
    }
    if (summary.elapsedSeconds > 240.0) {
        return QStringLiteral("通关稳定但清场偏慢：可以把因果切割器放在长直线，或提前升级悖论震荡器清密集波。");
    }
    return QStringLiteral("防线表现稳定：资源、清场和部署数量比较均衡，可以尝试追求更高剩余时能或更短用时。");
}

QString failureDirectorLine(const GameSummary &summary)
{
    if (summary.totalWaves > 0 && summary.wavesCleared >= summary.totalWaves - 1) {
        return QStringLiteral("你已经接近封存。别重做整条防线，只修最后一次崩口。");
    }
    if (summary.towersBuilt <= 2) {
        return QStringLiteral("这不是能力问题，是部署太少。重开，先把第一层阵线立起来。");
    }
    return QStringLiteral("失败不是终点，是修补局的第二份教材。重开时只改关键弱点，别推翻全部判断。");
}

std::vector<DialogueLine> outroDialogueForSummary(const GameSummary &summary, bool victory)
{
    if (victory) {
        return {
            {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("异常波形归零。%1 的核心频率已经回到稳定区间。").arg(summary.levelName)},
            {1, QStringLiteral("导航员 小昼"), LevelManager::levelVictoryText(summary.levelIndex)},
            {0, QStringLiteral("局长 玄衡"), LevelManager::levelNextHook(summary.levelIndex)}
        };
    }

    return {
        {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("核心完整性跌破阈值。当前记录：完成波次 %1/%2，部署装置 %3，剩余时能 %4。")
                                            .arg(summary.wavesCleared)
                                            .arg(summary.totalWaves)
                                            .arg(summary.towersBuilt)
                                            .arg(summary.energy)},
        {1, QStringLiteral("导航员 小昼"), failureAdviceForSummary(summary)},
        {0, QStringLiteral("局长 玄衡"), failureDirectorLine(summary)}
    };
}

QFrame *makeStatCard(const QString &title, const QString &objectName, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background: rgba(5, 14, 28, 185); border: 1px solid rgba(80, 212, 255, 120); border-radius: 8px; }"
        "QLabel#statTitle { color: #9ed8ff; font-size: 13px; }"
        "QLabel#statValue { color: #ffe19a; font-size: 22px; font-weight: 700; }"));
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(4);
    auto *titleLabel = new QLabel(title, card);
    titleLabel->setObjectName(QStringLiteral("statTitle"));
    titleLabel->setAlignment(Qt::AlignCenter);
    auto *value = new QLabel(QStringLiteral("--"), card);
    value->setObjectName(objectName);
    value->setProperty("class", QStringLiteral("statValue"));
    value->setStyleSheet(QStringLiteral("QLabel { color: #ffe19a; font-size: 22px; font-weight: 700; }"));
    value->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    layout->addWidget(value);
    return card;
}

QLabel *makeTextBlock(const QString &text, QWidget *parent, int fontSize = 16)
{
    auto *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setStyleSheet(QStringLiteral(
                             "QLabel { color: #dff8ff; font-size: %1px; line-height: 145%;"
                             " background: rgba(5, 14, 28, 150); border: 1px solid rgba(80, 212, 255, 90);"
                             " border-radius: 8px; padding: 14px; }")
                             .arg(fontSize));
    return label;
}

[[maybe_unused]] QWidget *createArchivePage(MainWindow *owner,
                           const QString &titleText,
                           const QString &subtitleText,
                           const QStringList &sections,
                           const std::function<void()> &onBack)
{
    auto *page = new BackgroundPage(QStringLiteral("bg-archive.png"), owner);
    page->setStyleSheet(pageStyle());

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(70, 42, 70, 42);
    outer->setSpacing(14);

    auto *title = new QLabel(titleText, page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(subtitleText, page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    auto *scroll = new QScrollArea(page);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setMaximumWidth(1320);
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scroll->setStyleSheet(QStringLiteral("QScrollArea { background: transparent; border: 0; }"));

    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(12);
    contentLayout->setContentsMargins(10, 10, 10, 10);
    for (const QString &section : sections) {
        contentLayout->addWidget(makeTextBlock(section, content));
    }
    contentLayout->addStretch();
    scroll->setWidget(content);

    auto *back = new QPushButton(QStringLiteral("返回"), page);
    QObject::connect(back, &QPushButton::clicked, owner, onBack);

    outer->addWidget(title);
    outer->addWidget(subtitle);
    outer->addWidget(scroll, 1, Qt::AlignHCenter);
    outer->addWidget(back, 0, Qt::AlignCenter);
    return page;
}

QPixmap databaseIconPixmap(const QString &assetName, bool locked)
{
    if (assetName.isEmpty()) {
        return {};
    }
    QString fileName = assetName;
    int operatorIndex = -1;
    const int marker = assetName.indexOf(QStringLiteral("#op"));
    if (marker >= 0) {
        fileName = assetName.left(marker);
        bool ok = false;
        operatorIndex = assetName.mid(marker + 3).toInt(&ok);
        if (!ok) {
            operatorIndex = -1;
        }
    }

    QPixmap pixmap(assetPath(fileName));
    if (pixmap.isNull()) {
        return {};
    }
    if (operatorIndex >= 0) {
        const int columns = 3;
        const int sourceW = pixmap.width() / columns;
        const int x = std::clamp(operatorIndex, 0, columns - 1) * sourceW;
        pixmap = pixmap.copy(x, 0, sourceW, pixmap.height());
    }
    QPixmap scaled = pixmap.scaled(76, 76, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (!locked) {
        return scaled;
    }

    QPixmap dimmed(scaled.size());
    dimmed.fill(Qt::transparent);
    QPainter painter(&dimmed);
    painter.setOpacity(0.42);
    painter.drawPixmap(0, 0, scaled);
    painter.setOpacity(1.0);
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(dimmed.rect(), QColor(8, 16, 27, 150));
    return dimmed;
}

QFrame *makeDatabaseCard(const DatabaseCardData &data, QWidget *parent)
{
    auto *card = new QFrame(parent);
    card->setMinimumSize(data.assetName.isEmpty() ? QSize(360, 118) : QSize(390, 132));
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    card->setStyleSheet(QStringLiteral(
        "QFrame { background: rgba(5, 14, 28, 172); border: 1px solid rgba(80, 212, 255, 105); border-radius: 8px; }"
        "QLabel#dbTitle { color: #ffe19a; font-size: 17px; font-weight: 700; border: 0; background: transparent; }"
        "QLabel#dbBody { color: #dff8ff; font-size: 13px; border: 0; background: transparent; }"
        "QLabel#dbIcon { background: rgba(255, 225, 154, 18); border: 1px solid rgba(255, 225, 154, 70); border-radius: 8px; }"));
    auto *layout = new QHBoxLayout(card);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(14);
    if (!data.assetName.isEmpty()) {
        auto *iconLabel = new QLabel(card);
        iconLabel->setObjectName(QStringLiteral("dbIcon"));
        iconLabel->setFixedSize(76, 76);
        iconLabel->setAlignment(Qt::AlignCenter);
        const QPixmap icon = databaseIconPixmap(data.assetName, data.locked);
        if (!icon.isNull()) {
            iconLabel->setPixmap(icon);
        } else {
            iconLabel->setText(QStringLiteral("?"));
        }
        layout->addWidget(iconLabel, 0, Qt::AlignTop);
    }

    auto *textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(6);
    auto *titleLabel = new QLabel(data.title, card);
    titleLabel->setObjectName(QStringLiteral("dbTitle"));
    auto *bodyLabel = new QLabel(data.body, card);
    bodyLabel->setObjectName(QStringLiteral("dbBody"));
    bodyLabel->setWordWrap(true);
    bodyLabel->setMinimumWidth(250);
    textLayout->addWidget(titleLabel);
    textLayout->addWidget(bodyLabel, 1);
    layout->addLayout(textLayout, 1);
    return card;
}

QWidget *makeDatabaseSections(const std::vector<DatabaseSection> &sections, QWidget *parent)
{
    auto *host = new QWidget(parent);
    host->setMaximumWidth(1120);
    host->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *layout = new QVBoxLayout(host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *tabs = new QWidget(host);
    tabs->setStyleSheet(QStringLiteral(
        "QPushButton { min-width: 104px; min-height: 34px; font-size: 14px; padding: 4px 12px;"
        " border: 1px solid rgba(80, 212, 255, 185); border-radius: 8px; color: #dff8ff;"
        " background: rgba(10, 39, 66, 198); }"
        "QPushButton:checked { color: #06111f; background: #ffe19a; border-color: #ffe19a; }"
        "QPushButton:hover { color: #ffe19a; border-color: #ffe19a; }"
        "QPushButton:checked:hover { color: #06111f; }"));
    auto *tabLayout = new QHBoxLayout(tabs);
    tabLayout->setContentsMargins(0, 0, 0, 0);
    tabLayout->setSpacing(10);
    tabLayout->addStretch();

    auto *stack = new QStackedWidget(host);
    stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    std::vector<QPushButton *> buttons;
    buttons.reserve(sections.size());
    for (int sectionIndex = 0; sectionIndex < static_cast<int>(sections.size()); ++sectionIndex) {
        const DatabaseSection &section = sections[static_cast<size_t>(sectionIndex)];
        auto *tab = new QPushButton(section.title, tabs);
        tab->setCheckable(true);
        tabLayout->addWidget(tab);
        buttons.push_back(tab);

        auto *page = new QWidget(stack);
        page->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto *grid = new QGridLayout(page);
        grid->setContentsMargins(0, 0, 0, 0);
        grid->setHorizontalSpacing(14);
        grid->setVerticalSpacing(14);
        grid->setColumnStretch(0, 1);
        grid->setColumnStretch(1, 1);

        const int visibleCards = std::min(6, static_cast<int>(section.cards.size()));
        for (int i = 0; i < visibleCards; ++i) {
            grid->addWidget(makeDatabaseCard(section.cards[static_cast<size_t>(i)], page),
                            i / 2,
                            i % 2);
        }
        for (int i = visibleCards; i < 6; ++i) {
            auto *spacer = new QWidget(page);
            spacer->setMinimumSize(390, 132);
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            grid->addWidget(spacer, i / 2, i % 2);
        }
        stack->addWidget(page);
    }
    tabLayout->addStretch();
    for (int sectionIndex = 0; sectionIndex < static_cast<int>(buttons.size()); ++sectionIndex) {
        QObject::connect(buttons[static_cast<size_t>(sectionIndex)], &QPushButton::clicked, stack, [stack, buttons, sectionIndex]() {
            stack->setCurrentIndex(sectionIndex);
            for (int i = 0; i < static_cast<int>(buttons.size()); ++i) {
                buttons[static_cast<size_t>(i)]->setChecked(i == sectionIndex);
            }
            SoundManager::play(SoundCue::UiClick);
        });
    }
    if (!buttons.empty()) {
        buttons.front()->setChecked(true);
    }

    if (sections.size() > 1) {
        layout->addWidget(tabs, 0, Qt::AlignHCenter);
    }
    layout->addWidget(stack);
    return host;
}

[[maybe_unused]] QWidget *createDatabasePage(MainWindow *owner,
                            const QString &titleText,
                            const QString &subtitleText,
                            const std::vector<std::pair<QString, QString>> &cards,
                            const std::function<void()> &onBack)
{
    auto *page = new BackgroundPage(QStringLiteral("bg-archive.png"), owner);
    page->setStyleSheet(pageStyle());

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(64, 40, 64, 40);
    outer->setSpacing(14);

    auto *title = new QLabel(titleText, page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(subtitleText, page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    std::vector<DatabaseCardData> cardData;
    cardData.reserve(cards.size());
    for (const auto &entry : cards) {
        cardData.push_back(DatabaseCardData{entry.first, entry.second, QString(), false});
    }
    std::vector<DatabaseSection> sections;
    if (cardData.size() <= 6) {
        sections.push_back(DatabaseSection{QStringLiteral("总览"), cardData});
    } else {
        const std::array<QString, 3> names = {QStringLiteral("入门"), QStringLiteral("战斗"), QStringLiteral("资料")};
        for (int start = 0; start < static_cast<int>(cardData.size()); start += 3) {
            std::vector<DatabaseCardData> group;
            const int end = std::min(start + 3, static_cast<int>(cardData.size()));
            for (int i = start; i < end; ++i) {
                group.push_back(cardData[static_cast<size_t>(i)]);
            }
            sections.push_back(DatabaseSection{names[static_cast<size_t>(std::min(start / 3, 2))], group});
        }
    }
    auto *sectionsView = makeDatabaseSections(sections, page);

    auto *back = new QPushButton(QStringLiteral("返回档案与图鉴"), page);
    QObject::connect(back, &QPushButton::clicked, owner, onBack);

    outer->addStretch();
    outer->addWidget(title);
    outer->addWidget(subtitle);
    outer->addWidget(sectionsView, 0, Qt::AlignHCenter);
    outer->addWidget(back, 0, Qt::AlignCenter);
    outer->addStretch();
    return page;
}

[[maybe_unused]] QWidget *createDatabasePage(MainWindow *owner,
                            const QString &titleText,
                            const QString &subtitleText,
                            const std::vector<DatabaseCardData> &cards,
                            const std::function<void()> &onBack)
{
    auto *page = new BackgroundPage(QStringLiteral("bg-archive.png"), owner);
    page->setStyleSheet(pageStyle());

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(64, 40, 64, 40);
    outer->setSpacing(14);

    auto *title = new QLabel(titleText, page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(subtitleText, page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    auto *sectionsView = makeDatabaseSections({DatabaseSection{QStringLiteral("总览"), cards}}, page);

    auto *back = new QPushButton(QStringLiteral("返回档案与图鉴"), page);
    QObject::connect(back, &QPushButton::clicked, owner, onBack);

    outer->addStretch();
    outer->addWidget(title);
    outer->addWidget(subtitle);
    outer->addWidget(sectionsView, 0, Qt::AlignHCenter);
    outer->addWidget(back, 0, Qt::AlignCenter);
    outer->addStretch();
    return page;
}

QWidget *createDatabasePage(MainWindow *owner,
                            const QString &titleText,
                            const QString &subtitleText,
                            const std::vector<DatabaseSection> &sections,
                            const std::function<void()> &onBack)
{
    auto *page = new BackgroundPage(QStringLiteral("bg-archive.png"), owner);
    page->setStyleSheet(pageStyle());

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(64, 40, 64, 40);
    outer->setSpacing(14);

    auto *title = new QLabel(titleText, page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(subtitleText, page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    auto *sectionsView = makeDatabaseSections(sections, page);

    auto *back = new QPushButton(QStringLiteral("返回档案与图鉴"), page);
    QObject::connect(back, &QPushButton::clicked, owner, onBack);

    outer->addStretch();
    outer->addWidget(title);
    outer->addWidget(subtitle);
    outer->addWidget(sectionsView, 0, Qt::AlignHCenter);
    outer->addWidget(back, 0, Qt::AlignCenter);
    outer->addStretch();
    return page;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    SoundManager::reloadSettings();
    setWindowTitle(QStringLiteral("时间修补局"));
    resize(1120, 760);
    setMinimumSize(980, 680);

    m_stack = new QStackedWidget(this);
    setCentralWidget(m_stack);

    m_openingPage = new OpeningPage([this]() { showOpeningPrologue(); }, this);
    m_startPage = createStartPage();
    m_levelSelectPage = createLevelSelectPage();
    m_storyPage = createStoryPage();
    m_helpPage = createHelpPage();
    m_settingsPage = createSettingsPage();
    m_aboutPage = createAboutPage();
    m_codexPage = createCodexPage();
    m_reverseSelectPage = createReverseSelectPage();
    m_dialoguePage = createDialoguePage();
    m_missionBriefingPage = createMissionBriefingPage();
    m_game = new GameWidget(this);
    m_editor = new EditorWidget(this);
    m_failureAnnouncePage = createFailureAnnouncePage();
    m_victoryPage = createResultPage(QStringLiteral("时间线修复完成"),
                                     QStringLiteral("悖论污染已清除，时间核心恢复稳定。"),
                                     true);
    m_failurePage = createResultPage(QStringLiteral("时间核心崩坏"),
                                     QStringLiteral("异常体突破防线，当前时间线需要重新修补。"),
                                     false);

    m_stack->addWidget(m_openingPage);
    m_stack->addWidget(m_startPage);
    m_stack->addWidget(m_levelSelectPage);
    m_stack->addWidget(m_storyPage);
    m_stack->addWidget(m_helpPage);
    m_stack->addWidget(m_settingsPage);
    m_stack->addWidget(m_aboutPage);
    m_stack->addWidget(m_codexPage);
    m_stack->addWidget(m_reverseSelectPage);
    m_stack->addWidget(m_dialoguePage);
    m_stack->addWidget(m_missionBriefingPage);
    m_stack->addWidget(m_game);
    m_stack->addWidget(m_editor);
    m_stack->addWidget(m_failureAnnouncePage);
    m_stack->addWidget(m_victoryPage);
    m_stack->addWidget(m_failurePage);

    connect(m_game, &GameWidget::victory, this, &MainWindow::showVictoryPage);
    connect(m_game, &GameWidget::failure, this, &MainWindow::showFailurePage);
    connect(m_game, &GameWidget::backToMenu, this, [this]() {
        if (m_reverseSession) {
            showReverseSelectPage();
        } else {
            showLevelSelectPage();
        }
    });
    connect(m_game, &GameWidget::settingsRequested, this, [this]() {
        m_settingsReturnToGame = true;
        showSettingsPage();
    });
    connect(m_editor, &EditorWidget::backRequested, this, [this]() {
        if (m_archiveReturnMode) {
            showStoryPage();
        } else {
            showLevelSelectPage();
        }
    });
    connect(m_editor, &EditorWidget::playRequested, this, &MainWindow::playCustomLevel);

    m_stack->setCurrentWidget(m_openingPage);
}

QWidget *MainWindow::createStartPage()
{
    auto *page = new BackgroundPage(QStringLiteral("bg-main.png"), this);
    page->setStyleSheet(pageStyle()
                        + QStringLiteral(
                            "QLabel#menuHint { color: #dff8ff; font-size: 16px;"
                            " background: rgba(5, 14, 28, 160); border: 1px solid rgba(80, 212, 255, 92);"
                            " border-radius: 8px; padding: 14px 20px; }"
                            "QLabel#progressPill { color: #ffe19a; font-size: 15px; font-weight: 700;"
                            " background: rgba(5, 14, 28, 185); border: 1px solid rgba(255, 225, 154, 105);"
                            " border-radius: 8px; padding: 10px 16px; }"));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(60, 38, 60, 38);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel(QStringLiteral("时间修补局"), page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel(QStringLiteral("破碎时间线防御协议"), page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    const int highest = std::clamp(ProgressManager::highestUnlockedLevel(), 0, LevelManager::builtInLevelCount() - 1);
    const int last = std::clamp(ProgressManager::lastLevelIndex(), 0, LevelManager::builtInLevelCount() - 1);
    auto *progressPill = new QLabel(QStringLiteral("主线 %1/6    当前：%2    音效 %3")
                                        .arg(highest + 1)
                                        .arg(LevelManager::levelTitle(last))
                                        .arg(ProgressManager::soundEnabled() ? QStringLiteral("开启") : QStringLiteral("关闭")),
                                    page);
    progressPill->setObjectName(QStringLiteral("progressPill"));
    progressPill->setAlignment(Qt::AlignCenter);
    progressPill->setMaximumWidth(680);

    auto *continueButton = new QPushButton(QStringLiteral("继续修补"), page);
    connect(continueButton, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        continueMainStory();
    });

    auto *newTask = new QPushButton(QStringLiteral("开始新任务"), page);
    connect(newTask, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        startNewStory();
    });

    auto *story = new QPushButton(QStringLiteral("档案与图鉴"), page);
    connect(story, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        showStoryPage();
    });

    auto *settings = new QPushButton(QStringLiteral("选项"), page);
    connect(settings, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        showSettingsPage();
    });

    auto *about = new QPushButton(QStringLiteral("关于"), page);
    connect(about, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        showAboutPage();
    });

    auto *quit = new QPushButton(QStringLiteral("退出"), page);
    connect(quit, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        close();
    });

    auto *buttons = new QVBoxLayout();
    buttons->setSpacing(10);
    buttons->setAlignment(Qt::AlignCenter);
    auto *buttonGrid = new QGridLayout();
    buttonGrid->setHorizontalSpacing(14);
    buttonGrid->setVerticalSpacing(12);
    buttonGrid->addWidget(continueButton, 0, 0);
    buttonGrid->addWidget(newTask, 0, 1);
    buttonGrid->addWidget(story, 1, 0);
    buttonGrid->addWidget(settings, 1, 1);
    buttonGrid->addWidget(about, 2, 0);
    buttonGrid->addWidget(quit, 2, 1);
    buttons->addLayout(buttonGrid);

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(10);
    layout->addWidget(progressPill, 0, Qt::AlignCenter);
    layout->addSpacing(18);
    layout->addLayout(buttons);
    layout->addStretch();

    return page;
}

QWidget *MainWindow::createLevelSelectPage()
{
    auto *page = new LevelSelectPage(this);
    page->setStyleSheet(pageStyle()
                        + QStringLiteral(
                            "QWidget#levelBoard { background: rgba(3, 10, 22, 82); border: 1px solid rgba(80, 212, 255, 70); border-radius: 8px; }"
                            "QFrame#levelCard { background: rgba(5, 14, 28, 184); border: 1px solid rgba(80, 212, 255, 105); border-radius: 8px; }"
                            "QFrame#levelCardLocked { background: rgba(4, 8, 15, 156); border: 1px solid rgba(150, 170, 185, 72); border-radius: 8px; }"
                            "QLabel#levelTitle { color: #ffe19a; font-size: 20px; font-weight: 750; border: 0; background: transparent; }"
                            "QLabel#levelTags { color: #9ed8ff; font-size: 13px; border: 0; background: transparent; }"
                            "QLabel#levelMeta { color: #ffe19a; font-size: 13px; font-weight: 650; border: 0; background: transparent; }"
                            "QLabel#levelHint { color: #dff8ff; font-size: 13px; border: 0; background: transparent; }"
                            "QLabel#levelState { color: #08121f; font-size: 12px; font-weight: 700;"
                            " background: rgba(255, 225, 154, 210); border: 0; border-radius: 8px; padding: 3px 8px; }"
                            "QPushButton#levelEnter { min-width: 112px; min-height: 32px; font-size: 14px; padding: 4px 14px; }"));

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(56, 30, 56, 30);
    outer->setAlignment(Qt::AlignCenter);

    auto *content = new QWidget(page);
    content->setMaximumWidth(1180);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("选择受损时间线"), page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel(m_acceptanceMode
                                    ? QStringLiteral("自由演练已开启：全部时间线可直接进入。")
                                    : QStringLiteral("主线任务按通关进度逐步解锁。未开放的时间线会保持锁定。"),
                                page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    auto *editor = new QPushButton(QStringLiteral("关卡编辑器"), page);
    auto *back = new QPushButton(m_archiveReturnMode ? QStringLiteral("返回档案与图鉴")
                                                     : QStringLiteral("返回开始界面"),
                                 page);
    const int highestUnlocked = std::clamp(ProgressManager::highestUnlockedLevel(), 0, LevelManager::builtInLevelCount() - 1);

    connect(editor, &QPushButton::clicked, this, &MainWindow::showEditorPage);
    connect(back, &QPushButton::clicked, this, [this]() {
        if (m_archiveReturnMode) {
            showStoryPage();
        } else {
            showStartPage();
        }
    });

    layout->addWidget(title);
    layout->addWidget(subtitle);

    const std::array<QString, 6> threats = {
        QStringLiteral("威胁 I"),
        QStringLiteral("威胁 II"),
        QStringLiteral("威胁 III"),
        QStringLiteral("威胁 IV"),
        QStringLiteral("威胁 V"),
        QStringLiteral("威胁 Ω")
    };
    const std::array<QString, 6> unlocks = {
        QStringLiteral("炮台 / 减速 / 经济"),
        QStringLiteral("授权：悖论震荡器"),
        QStringLiteral("授权：因果切割器"),
        QStringLiteral("授权：锚点屏障"),
        QStringLiteral("资源运营 / Boss 应急"),
        QStringLiteral("全系统综合封存")
    };
    const std::array<QString, 6> shortBriefs = {
        QStringLiteral("基础训练线，适合熟悉部署节奏。"),
        QStringLiteral("密集波压迫，学习范围清场。"),
        QStringLiteral("传送干扰，守住长直线出口。"),
        QStringLiteral("分裂与逆流，拖住关键路段。"),
        QStringLiteral("资源压力提升，提前规划经济。"),
        QStringLiteral("最终协议汇合，综合封存核心。")
    };

    auto *board = new QWidget(page);
    board->setObjectName(QStringLiteral("levelBoard"));
    board->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *grid = new QGridLayout(board);
    grid->setContentsMargins(18, 16, 18, 16);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(14);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    for (int row = 0; row < 3; ++row) {
        grid->setRowStretch(row, 1);
    }

    for (int i = 0; i < LevelManager::builtInLevelCount(); ++i) {
        const bool unlocked = m_acceptanceMode || i <= highestUnlocked;
        auto *card = new QFrame(board);
        card->setObjectName(unlocked ? QStringLiteral("levelCard") : QStringLiteral("levelCardLocked"));
        card->setMinimumSize(300, 126);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(18, 13, 18, 13);
        cardLayout->setSpacing(5);

        auto *name = new QLabel(QStringLiteral("%1  %2")
                                    .arg(i + 1, 2, 10, QLatin1Char('0'))
                                    .arg(LevelManager::levelTitle(i)),
                                card);
        name->setObjectName(QStringLiteral("levelTitle"));

        auto *state = new QLabel(unlocked
                                     ? (i == highestUnlocked ? QStringLiteral("当前授权") : QStringLiteral("已归档"))
                                     : QStringLiteral("锁定"),
                                 card);
        state->setObjectName(QStringLiteral("levelState"));
        state->setAlignment(Qt::AlignCenter);
        if (!unlocked) {
            state->setStyleSheet(QStringLiteral(
                "QLabel#levelState { color: #9aa8b7; font-size: 12px; font-weight: 700;"
                " background: rgba(42, 52, 64, 210); border: 0; border-radius: 8px; padding: 3px 8px; }"));
        } else if (i < highestUnlocked) {
            state->setStyleSheet(QStringLiteral(
                "QLabel#levelState { color: #06111f; font-size: 12px; font-weight: 700;"
                " background: rgba(126, 220, 255, 205); border: 0; border-radius: 8px; padding: 3px 8px; }"));
        }

        auto *titleRow = new QHBoxLayout();
        titleRow->setContentsMargins(0, 0, 0, 0);
        titleRow->setSpacing(8);
        titleRow->addWidget(name, 1);
        titleRow->addWidget(state, 0, Qt::AlignRight | Qt::AlignVCenter);

        auto *tags = new QLabel(QStringLiteral("%1 · %2")
                                    .arg(LevelManager::levelDifficulty(i),
                                         threats[static_cast<size_t>(i)]),
                                card);
        tags->setObjectName(QStringLiteral("levelTags"));

        auto *unlock = new QLabel(unlocked ? unlocks[static_cast<size_t>(i)] : QStringLiteral("完成上一线后开放"), card);
        unlock->setObjectName(QStringLiteral("levelMeta"));

        auto *brief = new QLabel(unlocked ? shortBriefs[static_cast<size_t>(i)] : QStringLiteral("自由演练可临时查看全部关卡。"), card);
        brief->setObjectName(QStringLiteral("levelHint"));
        brief->setWordWrap(true);

        auto *button = new QPushButton(unlocked ? QStringLiteral("进入通讯") : QStringLiteral("异常锁定"), card);
        button->setObjectName(QStringLiteral("levelEnter"));
        button->setEnabled(unlocked);
        button->setToolTip(LevelManager::levelBriefing(i));
        connect(button, &QPushButton::clicked, this, [this, i]() { startLevel(i); });

        cardLayout->addLayout(titleRow);
        cardLayout->addWidget(tags);
        cardLayout->addWidget(unlock);
        cardLayout->addWidget(brief);
        cardLayout->addStretch();
        cardLayout->addWidget(button, 0, Qt::AlignRight);

        grid->addWidget(card, i / 2, i % 2);
    }

    auto *buttons = new QWidget(page);
    auto *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(14);
    buttonLayout->addStretch();
    buttonLayout->addWidget(editor);
    buttonLayout->addWidget(back);
    buttonLayout->addStretch();

    layout->addWidget(board, 0, Qt::AlignHCenter);
    layout->addWidget(buttons);
    outer->addWidget(content, 0, Qt::AlignCenter);

    return page;
}

QWidget *MainWindow::createStoryPage()
{
    const std::vector<std::pair<QString, QString>> notes = {
        {QStringLiteral("主线档案"), QStringLiteral("六条时间线等待修复，继续任务会回到最近进度。")},
        {QStringLiteral("接入流程"), QStringLiteral("新任务先播放序幕，再进入第一关通讯。")},
        {QStringLiteral("作战原则"), QStringLiteral("战斗只给短提示，完整资料集中在图鉴。")}
    };

    auto *page = new BackgroundPage(QStringLiteral("bg-archive.png"), this);
    page->setStyleSheet(pageStyle()
                        + QStringLiteral(
                            "QFrame#storyNote { background: rgba(5, 14, 28, 156); border: 1px solid rgba(80, 212, 255, 82); border-radius: 8px; }"
                            "QLabel#storyNoteTitle { color: #ffe19a; font-size: 17px; font-weight: 700; border: 0; background: transparent; }"
                            "QLabel#storyNoteBody { color: #dff8ff; font-size: 15px; border: 0; background: transparent; }"));

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(64, 36, 64, 36);
    outer->setAlignment(Qt::AlignCenter);

    auto *content = new QWidget(page);
    content->setMaximumWidth(1060);
    content->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(12);

    auto *title = new QLabel(QStringLiteral("档案与图鉴"), page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(QStringLiteral("主线记录、作战手册与时间图鉴"), page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    auto *actions = new QWidget(content);
    actions->setMaximumWidth(780);
    actions->setStyleSheet(QStringLiteral(
        "QPushButton { min-height: 50px; font-size: 17px; line-height: 120%; padding: 8px 14px; }"));
    auto *actionGrid = new QGridLayout(actions);
    actionGrid->setContentsMargins(0, 0, 0, 0);
    actionGrid->setHorizontalSpacing(12);
    actionGrid->setVerticalSpacing(12);

    auto *levels = new QPushButton(QStringLiteral("关卡档案\n选择受损时间线"), actions);
    connect(levels, &QPushButton::clicked, this, [this]() {
        m_archiveReturnMode = true;
        m_acceptanceMode = false;
        showLevelSelectPage();
    });
    auto *help = new QPushButton(QStringLiteral("游戏说明\n操作与地形速查"), actions);
    connect(help, &QPushButton::clicked, this, [this]() {
        m_archiveReturnMode = true;
        showHelpPage();
    });
    auto *codex = new QPushButton(QStringLiteral("时间图鉴\n装置、敌人与 Boss"), actions);
    connect(codex, &QPushButton::clicked, this, [this]() {
        m_archiveReturnMode = true;
        showCodexPage();
    });
    auto *editor = new QPushButton(QStringLiteral("关卡编辑器\n自定义路线演练"), actions);
    connect(editor, &QPushButton::clicked, this, [this]() {
        m_archiveReturnMode = true;
        showEditorPage();
    });
    auto *acceptance = new QPushButton(QStringLiteral("自由演练\n开放全部时间线"), actions);
    connect(acceptance, &QPushButton::clicked, this, [this]() {
        m_archiveReturnMode = true;
        m_acceptanceMode = true;
        showLevelSelectPage();
    });
    auto *reverse = new QPushButton(QStringLiteral("逆向推演\n3 条异常协议可用"), actions);
    connect(reverse, &QPushButton::clicked, this, [this]() {
        m_archiveReturnMode = true;
        showReverseSelectPage();
    });

    actionGrid->addWidget(levels, 0, 0);
    actionGrid->addWidget(codex, 0, 1);
    actionGrid->addWidget(help, 1, 0);
    actionGrid->addWidget(editor, 1, 1);
    actionGrid->addWidget(acceptance, 2, 0);
    actionGrid->addWidget(reverse, 2, 1);

    auto *noteHost = new QWidget(content);
    noteHost->setMaximumWidth(1060);
    noteHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    auto *noteGrid = new QGridLayout(noteHost);
    noteGrid->setContentsMargins(0, 0, 0, 0);
    noteGrid->setHorizontalSpacing(12);
    noteGrid->setVerticalSpacing(12);
    for (int i = 0; i < static_cast<int>(notes.size()); ++i) {
        auto *card = new QFrame(noteHost);
        card->setObjectName(QStringLiteral("storyNote"));
        card->setMinimumSize(300, 92);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 12, 16, 12);
        cardLayout->setSpacing(6);
        auto *noteTitle = new QLabel(notes[static_cast<size_t>(i)].first, card);
        noteTitle->setObjectName(QStringLiteral("storyNoteTitle"));
        auto *noteBody = new QLabel(notes[static_cast<size_t>(i)].second, card);
        noteBody->setObjectName(QStringLiteral("storyNoteBody"));
        noteBody->setMinimumWidth(230);
        noteBody->setWordWrap(true);
        cardLayout->addWidget(noteTitle);
        cardLayout->addWidget(noteBody);
        noteGrid->addWidget(card, 0, i);
        noteGrid->setColumnStretch(i, 1);
    }

    auto *back = new QPushButton(QStringLiteral("返回开始界面"), content);
    connect(back, &QPushButton::clicked, this, &MainWindow::showStartPage);

    contentLayout->addWidget(title);
    contentLayout->addWidget(subtitle);
    contentLayout->addWidget(actions, 0, Qt::AlignHCenter);
    contentLayout->addWidget(noteHost, 0, Qt::AlignHCenter);
    contentLayout->addSpacing(4);
    contentLayout->addWidget(back, 0, Qt::AlignCenter);
    outer->addWidget(content, 0, Qt::AlignCenter);
    return page;
}

QWidget *MainWindow::createReverseSelectPage()
{
    struct ReverseCard {
        QString title;
        QString badge;
        QString difficulty;
        QString map;
        QString defense;
        QString plan;
        QString button;
        QColor accent;
    };
    const std::array<ReverseCard, 3> cards = {{
        {QStringLiteral("逆向训练线"),
         QStringLiteral("训练"),
         QStringLiteral("难度 I  宽松预算"),
         QStringLiteral("地图：钟楼回响，路线清晰，适合熟悉编队。"),
         QStringLiteral("系统：轻防守，前两波容错更高。"),
         QStringLiteral("推荐：数量压测 + 速度突防"),
         QStringLiteral("开始训练"),
         QColor(80, 212, 255)},
        {QStringLiteral("裂隙压测线"),
         QStringLiteral("标准"),
         QStringLiteral("难度 III  中等预算"),
         QStringLiteral("地图：逆流档案馆，需要处理折线与火力空窗。"),
         QStringLiteral("系统：会补减速与范围塔，并开始升级。"),
         QStringLiteral("推荐：高血消耗 + 反控制"),
         QStringLiteral("开始压测"),
         QColor(178, 108, 255)},
        {QStringLiteral("终局破防线"),
         QStringLiteral("挑战"),
         QStringLiteral("难度 Ω  紧凑预算"),
         QStringLiteral("地图：终末纪元，路线更长但系统火力完整。"),
         QStringLiteral("系统：高塔位、高升级，第六波组织 Boss 破核。"),
         QStringLiteral("推荐：分裂扰乱 + Boss 压核心"),
         QStringLiteral("开始破防"),
         QColor(255, 190, 92)}
    }};

    auto *page = new BackgroundPage(QStringLiteral("bg-archive.png"), this);
    page->setStyleSheet(pageStyle()
                        + QStringLiteral(
                            "QFrame#reverseCard { background: rgba(5, 14, 28, 178); border: 1px solid rgba(80, 212, 255, 86); border-radius: 8px; }"
                            "QFrame#reverseCard:hover { border-color: #ffe19a; background: rgba(9, 27, 46, 214); }"
                            "QLabel#reverseTitle { color: #ffe19a; font-size: 25px; font-weight: 850; border: 0; background: transparent; }"
                            "QLabel#reverseBadge { color: #06111e; font-size: 13px; font-weight: 850; border: 0; border-radius: 8px; padding: 4px 10px; background: #ffe19a; }"
                            "QLabel#reverseMeta { color: #9ed8ff; font-size: 14px; font-weight: 700; border: 0; background: transparent; }"
                            "QLabel#reverseBody { color: #dff8ff; font-size: 15px; line-height: 130%; border: 0; background: transparent; }"
                            "QLabel#reverseTag { color: #ffe19a; font-size: 14px; font-weight: 750; border: 1px solid rgba(255, 225, 154, 86); border-radius: 8px; padding: 7px 10px; background: rgba(36, 29, 14, 126); }"
                            "QLabel#reverseTip { color: #dff8ff; font-size: 14px; border: 1px solid rgba(80, 212, 255, 72); border-radius: 8px; padding: 9px 12px; background: rgba(5, 14, 28, 142); }"
                            "QPushButton#reverseEnter { min-width: 176px; min-height: 40px; font-size: 16px; }"));

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(58, 44, 58, 42);
    outer->setSpacing(16);
    outer->setAlignment(Qt::AlignCenter);

    auto *content = new QWidget(page);
    content->setMaximumWidth(1160);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("逆向推演"), content);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(QStringLiteral("选择异常协议战场：玩家投放敌人，系统自动建塔防守"), content);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    auto *gridHost = new QWidget(content);
    auto *grid = new QGridLayout(gridHost);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setHorizontalSpacing(16);
    grid->setVerticalSpacing(16);

    for (int i = 0; i < static_cast<int>(cards.size()); ++i) {
        const auto &data = cards[static_cast<size_t>(i)];
        auto *card = new QFrame(gridHost);
        card->setObjectName(QStringLiteral("reverseCard"));
        card->setMinimumSize(330, 312);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(22, 20, 22, 18);
        cardLayout->setSpacing(9);

        auto *header = new QWidget(card);
        auto *headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setSpacing(10);

        auto *cardTitle = new QLabel(data.title, card);
        cardTitle->setObjectName(QStringLiteral("reverseTitle"));
        auto *badge = new QLabel(data.badge, card);
        badge->setObjectName(QStringLiteral("reverseBadge"));
        badge->setAlignment(Qt::AlignCenter);
        badge->setStyleSheet(QStringLiteral("QLabel#reverseBadge { background: rgba(%1, %2, %3, 220); color: #06111e; }")
                                 .arg(data.accent.red())
                                 .arg(data.accent.green())
                                 .arg(data.accent.blue()));
        headerLayout->addWidget(cardTitle, 1);
        headerLayout->addWidget(badge, 0, Qt::AlignRight | Qt::AlignVCenter);

        auto *difficulty = new QLabel(data.difficulty, card);
        difficulty->setObjectName(QStringLiteral("reverseMeta"));
        auto *map = new QLabel(data.map, card);
        map->setObjectName(QStringLiteral("reverseBody"));
        map->setWordWrap(true);
        auto *defense = new QLabel(data.defense, card);
        defense->setObjectName(QStringLiteral("reverseBody"));
        defense->setWordWrap(true);
        auto *tag = new QLabel(data.plan, card);
        tag->setObjectName(QStringLiteral("reverseTag"));
        tag->setWordWrap(true);
        auto *enter = new QPushButton(data.button, card);
        enter->setObjectName(QStringLiteral("reverseEnter"));
        enter->setStyleSheet(QStringLiteral(
                                 "QPushButton#reverseEnter { min-width: 176px; min-height: 40px; font-size: 16px; font-weight: 750;"
                                 " color: #dff8ff; border: 1px solid rgba(%1, %2, %3, 210); border-radius: 8px;"
                                 " background: rgba(15, 39, 64, 224); padding: 6px 20px; }"
                                 "QPushButton#reverseEnter:hover { color: #ffe19a; border-color: #ffe19a; background: rgba(31, 70, 103, 235); }")
                                 .arg(data.accent.red())
                                 .arg(data.accent.green())
                                 .arg(data.accent.blue()));
        connect(enter, &QPushButton::clicked, this, [this, i]() {
            SoundManager::play(SoundCue::UiClick);
            launchReverseMode(i);
        });

        cardLayout->addWidget(header);
        cardLayout->addWidget(difficulty);
        cardLayout->addSpacing(8);
        cardLayout->addWidget(map);
        cardLayout->addWidget(defense);
        cardLayout->addSpacing(6);
        cardLayout->addWidget(tag);
        cardLayout->addStretch();
        cardLayout->addWidget(enter, 0, Qt::AlignRight);

        grid->addWidget(card, 0, i);
        grid->setColumnStretch(i, 1);
    }

    auto *tips = new QWidget(content);
    auto *tipsLayout = new QHBoxLayout(tips);
    tipsLayout->setContentsMargins(0, 0, 0, 0);
    tipsLayout->setSpacing(12);
    const std::array<QString, 3> tipTexts = {
        QStringLiteral("6 波制：每波先编队，再开波"),
        QStringLiteral("胜利条件：异常突破核心"),
        QStringLiteral("资料职责：完整参数仍在时间图鉴")
    };
    for (const QString &tipText : tipTexts) {
        auto *tip = new QLabel(tipText, tips);
        tip->setObjectName(QStringLiteral("reverseTip"));
        tip->setAlignment(Qt::AlignCenter);
        tip->setWordWrap(true);
        tip->setMinimumHeight(46);
        tipsLayout->addWidget(tip, 1);
    }

    auto *back = new QPushButton(QStringLiteral("返回档案与图鉴"), content);
    connect(back, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        showStoryPage();
    });

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(gridHost);
    layout->addWidget(tips);
    layout->addWidget(back, 0, Qt::AlignCenter);
    outer->addWidget(content, 0, Qt::AlignCenter);
    return page;
}

QWidget *MainWindow::createHelpPage()
{
    const std::vector<std::pair<QString, QString>> cards = {
        {QStringLiteral("第一次接入"), QStringLiteral("点击“开始新任务”会先看序幕，再进入任务投影。第 1 关只教学核心操作，其余参数可随时查图鉴。")},
        {QStringLiteral("基础操作"), QStringLiteral("左键部署，右键升级，数字 1-6 切换已授权装置，空格暂停，Ctrl+R 回溯当前关。")},
        {QStringLiteral("装置授权"), QStringLiteral("第 1 关开放指针炮台、凝滞棱镜和时能汲取仪；第 2/3/4 关逐步开放震荡器、切割器和屏障。")},
        {QStringLiteral("章节推进"), QStringLiteral("六关从教学、节奏、传送、分裂抗性、资源运营一路推进到终局协议汇合。每关只给一句核心策略。")},
        {QStringLiteral("主动技能"), QStringLiteral("按 F 释放时序冻结，可短时间大幅减速全场异常体，适合 Boss 或漏怪前线。")},
        {QStringLiteral("指令台"), QStringLiteral("/ 或回车键打开命令框。energy、clear、next、win 可用于自由演练。")},
        {QStringLiteral("地形数据库"), QStringLiteral("路径不可普通建塔；补给格降费；锚点格适合屏障；加速格会让敌人更快；传送格会折跃敌人。")},
        {QStringLiteral("图鉴职责"), QStringLiteral("战斗内只显示短提示；装置参数、敌人属性、Boss 机制和挑战目标集中放在“时间图鉴”。")},
        {QStringLiteral("战术建议"), QStringLiteral("前期先覆盖拐角，中期补减速和范围，后期用资源塔、切割器和屏障稳定核心前线。")}
    };
    std::vector<DatabaseCardData> cardData;
    cardData.reserve(cards.size());
    for (const auto &entry : cards) {
        cardData.push_back(DatabaseCardData{entry.first, entry.second, QString(), false});
    }
    auto helpSlice = [&cardData](int start, int count) {
        std::vector<DatabaseCardData> result;
        const int end = std::min(start + count, static_cast<int>(cardData.size()));
        for (int i = start; i < end; ++i) {
            result.push_back(cardData[static_cast<size_t>(i)]);
        }
        return result;
    };
    const std::vector<DatabaseSection> sections = {
        {QStringLiteral("入门"), helpSlice(0, 3)},
        {QStringLiteral("战斗"), helpSlice(3, 3)},
        {QStringLiteral("资料"), helpSlice(6, 3)}
    };
    return createDatabasePage(this,
                              QStringLiteral("游戏说明"),
                              QStringLiteral("操作、地形与战术提示数据库"),
                              sections,
                              [this]() { showStoryPage(); });
}

QWidget *MainWindow::createAboutPage()
{
    auto *page = new BackgroundPage(QStringLiteral("bg-archive.png"), this);
    page->setStyleSheet(pageStyle()
                        + QStringLiteral(
                            "QFrame#aboutPanel { background: rgba(5, 14, 28, 184); border: 1px solid rgba(80, 212, 255, 108); border-radius: 8px; }"
                            "QLabel#aboutLead { color: #dff8ff; font-size: 18px; line-height: 135%; border: 0; background: transparent; }"
                            "QLabel#aboutItemTitle { color: #ffe19a; font-size: 16px; font-weight: 750; border: 0; background: transparent; }"
                            "QLabel#aboutItemBody { color: #dff8ff; font-size: 15px; line-height: 130%; border: 0; background: transparent; }"));

    auto *outer = new QVBoxLayout(page);
    outer->setContentsMargins(64, 48, 64, 48);
    outer->setAlignment(Qt::AlignCenter);

    auto *content = new QWidget(page);
    content->setMaximumWidth(900);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(16);

    auto *title = new QLabel(QStringLiteral("关于时间修补局"), content);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel(QStringLiteral("作品信息、技术与素材说明"), content);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    auto *panel = new QFrame(content);
    panel->setObjectName(QStringLiteral("aboutPanel"));
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(32, 28, 32, 28);
    panelLayout->setSpacing(16);

    auto *lead = new QLabel(QStringLiteral("《时间修补局》是一款以时间线防御为主题的策略塔防作品。玩家作为见习修补员，在六条受损时间线中部署装置、处理异常，并可在逆向推演中切换攻防立场。"), panel);
    lead->setObjectName(QStringLiteral("aboutLead"));
    lead->setWordWrap(true);
    panelLayout->addWidget(lead);

    const std::array<std::pair<QString, QString>, 4> items = {{
        {QStringLiteral("作者"), QStringLiteral("brightcolin")},
        {QStringLiteral("开发时间"), QStringLiteral("2026 年 7 月")},
        {QStringLiteral("技术栈"), QStringLiteral("C++ / Qt Widgets，本地存档、关卡编辑器、图鉴、音效与多模式玩法。")},
        {QStringLiteral("素材与音效"), QStringLiteral("美术素材包含 AI 生成与二次整理；音效与音乐包含 Pixabay 来源素材。")}
    }};
    for (const auto &item : items) {
        auto *row = new QWidget(panel);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        rowLayout->setSpacing(18);
        auto *itemTitle = new QLabel(item.first, row);
        itemTitle->setObjectName(QStringLiteral("aboutItemTitle"));
        itemTitle->setFixedWidth(96);
        auto *itemBody = new QLabel(item.second, row);
        itemBody->setObjectName(QStringLiteral("aboutItemBody"));
        itemBody->setWordWrap(true);
        rowLayout->addWidget(itemTitle, 0, Qt::AlignTop);
        rowLayout->addWidget(itemBody, 1);
        panelLayout->addWidget(row);
    }

    auto *back = new QPushButton(QStringLiteral("返回开始界面"), content);
    connect(back, &QPushButton::clicked, this, &MainWindow::showStartPage);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(panel);
    layout->addWidget(back, 0, Qt::AlignCenter);
    outer->addWidget(content, 0, Qt::AlignCenter);
    return page;
}

QWidget *MainWindow::createSettingsPage()
{
    auto *page = new BackgroundPage(QStringLiteral("bg-archive.png"), this);
    page->setStyleSheet(pageStyle()
                        + QStringLiteral(
                            "QFrame#settingsPanel { background: rgba(5, 14, 28, 186); border: 1px solid rgba(80, 212, 255, 118); border-radius: 8px; }"
                            "QFrame#settingRow { background: rgba(4, 12, 24, 126); border: 1px solid rgba(80, 212, 255, 64); border-radius: 8px; }"
                            "QLabel#settingName { color: #ffe19a; font-size: 18px; font-weight: 700; border: 0; background: transparent; }"
                            "QLabel#settingHint { color: #dff8ff; font-size: 13px; border: 0; background: transparent; }"
                            "QLabel#settingValue { color: #9decff; font-size: 15px; font-weight: 700; border: 0; background: transparent; }"
                            "QPushButton#minorButton { min-width: 118px; min-height: 32px; font-size: 14px; }"
                            "QCheckBox { color: #dff8ff; font-size: 15px; spacing: 10px; }"));

    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(92, 42, 92, 42);
    layout->setSpacing(12);
    layout->setAlignment(Qt::AlignCenter);

    auto *title = new QLabel(QStringLiteral("选项"), page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    auto *subtitle = new QLabel(QStringLiteral("声音、提示与本机进度"), page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);

    auto *panel = new QFrame(page);
    panel->setObjectName(QStringLiteral("settingsPanel"));
    panel->setMaximumWidth(860);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(22, 18, 22, 18);
    panelLayout->setSpacing(10);
    const int settingActionWidth = 156;

    auto makeRow = [panel](const QString &name, const QString &hint) {
        auto *row = new QFrame(panel);
        row->setObjectName(QStringLiteral("settingRow"));
        auto *rowLayout = new QVBoxLayout(row);
        rowLayout->setContentsMargins(18, 12, 18, 12);
        rowLayout->setSpacing(8);

        auto *textLayout = new QHBoxLayout();
        textLayout->setContentsMargins(0, 0, 0, 0);
        textLayout->setSpacing(10);
        auto *nameLabel = new QLabel(name, row);
        nameLabel->setObjectName(QStringLiteral("settingName"));
        auto *hintLabel = new QLabel(hint, row);
        hintLabel->setObjectName(QStringLiteral("settingHint"));
        hintLabel->setWordWrap(true);
        textLayout->addWidget(nameLabel);
        textLayout->addWidget(hintLabel, 1);
        rowLayout->addLayout(textLayout);
        return std::pair<QFrame *, QVBoxLayout *>(row, rowLayout);
    };

    auto [soundRow, soundLayout] = makeRow(QStringLiteral("音效"),
                                           QStringLiteral("按钮、部署、攻击与胜负反馈"));
    auto *soundCheck = new QCheckBox(QStringLiteral("开启"), soundRow);
    soundCheck->setFixedWidth(82);
    soundCheck->setChecked(ProgressManager::soundEnabled());
    auto *soundValue = new QLabel(QStringLiteral("%1%").arg(ProgressManager::soundVolume()), soundRow);
    soundValue->setObjectName(QStringLiteral("settingValue"));
    soundValue->setMinimumWidth(54);
    soundValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *soundVolumeSlider = new QSlider(Qt::Horizontal, soundRow);
    soundVolumeSlider->setRange(0, 100);
    soundVolumeSlider->setValue(ProgressManager::soundVolume());
    soundVolumeSlider->setMinimumWidth(220);
    connect(soundCheck, &QCheckBox::toggled, this, [soundCheck](bool checked) {
        SoundManager::setSoundEnabled(checked);
        soundCheck->setText(checked ? QStringLiteral("开启") : QStringLiteral("关闭"));
        if (checked) {
            SoundManager::play(SoundCue::UiClick);
        }
    });
    soundCheck->setText(ProgressManager::soundEnabled() ? QStringLiteral("开启") : QStringLiteral("关闭"));
    connect(soundVolumeSlider, &QSlider::valueChanged, this, [soundValue](int value) {
        SoundManager::setSoundVolume(value);
        soundValue->setText(QStringLiteral("%1%").arg(value));
    });
    connect(soundVolumeSlider, &QSlider::sliderReleased, this, []() {
        SoundManager::play(SoundCue::UiClick);
    });
    auto *testSound = new QPushButton(QStringLiteral("试听"), soundRow);
    testSound->setObjectName(QStringLiteral("minorButton"));
    testSound->setFixedWidth(settingActionWidth);
    connect(testSound, &QPushButton::clicked, this, []() {
        SoundManager::play(SoundCue::MissionStart);
    });
    auto *soundControls = new QHBoxLayout();
    soundControls->setContentsMargins(0, 0, 0, 0);
    soundControls->setSpacing(14);
    soundControls->addWidget(soundCheck);
    soundControls->addWidget(soundVolumeSlider, 1);
    soundControls->addWidget(soundValue);
    soundControls->addWidget(testSound);
    soundLayout->addLayout(soundControls);

    auto [musicRow, musicLayout] = makeRow(QStringLiteral("音乐"),
                                           QStringLiteral("接入界面、菜单、剧情与战斗背景音乐"));
    auto *musicCheck = new QCheckBox(QStringLiteral("开启"), musicRow);
    musicCheck->setFixedWidth(82);
    musicCheck->setChecked(ProgressManager::ambienceEnabled());
    musicCheck->setText(ProgressManager::ambienceEnabled() ? QStringLiteral("开启") : QStringLiteral("关闭"));
    auto *musicValue = new QLabel(QStringLiteral("%1%").arg(ProgressManager::musicVolume()), musicRow);
    musicValue->setObjectName(QStringLiteral("settingValue"));
    musicValue->setMinimumWidth(54);
    musicValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    auto *musicVolumeSlider = new QSlider(Qt::Horizontal, musicRow);
    musicVolumeSlider->setRange(0, 100);
    musicVolumeSlider->setValue(ProgressManager::musicVolume());
    musicVolumeSlider->setMinimumWidth(220);
    connect(musicCheck, &QCheckBox::toggled, this, [this, musicCheck](bool checked) {
        SoundManager::setMusicEnabled(checked);
        musicCheck->setText(checked ? QStringLiteral("开启") : QStringLiteral("关闭"));
        if (!checked) {
            SoundManager::stopMusic();
            return;
        }
        if (m_settingsReturnToGame) {
            SoundManager::resumeMusic();
        } else {
            SoundManager::startMusic(MusicCue::Menu);
        }
        SoundManager::refreshVolumes();
    });
    connect(musicVolumeSlider, &QSlider::valueChanged, this, [musicValue](int value) {
        SoundManager::setMusicVolume(value);
        musicValue->setText(QStringLiteral("%1%").arg(value));
    });
    auto *musicControls = new QHBoxLayout();
    musicControls->setContentsMargins(0, 0, 0, 0);
    musicControls->setSpacing(14);
    musicControls->addWidget(musicCheck);
    musicControls->addWidget(musicVolumeSlider, 1);
    musicControls->addWidget(musicValue);
    auto *musicActionSlot = new QWidget(musicRow);
    musicActionSlot->setFixedWidth(settingActionWidth);
    musicControls->addWidget(musicActionSlot);
    musicLayout->addLayout(musicControls);

    auto [tutorialRow, tutorialLayout] = makeRow(QStringLiteral("教学提示"),
                                                 QStringLiteral("控制首次提示是否主动暂停"));
    auto *tutorialCheck = new QCheckBox(QStringLiteral("开启"), tutorialRow);
    tutorialCheck->setFixedWidth(82);
    tutorialCheck->setChecked(ProgressManager::tutorialHintsEnabled());
    tutorialCheck->setText(ProgressManager::tutorialHintsEnabled() ? QStringLiteral("开启") : QStringLiteral("关闭"));
    connect(tutorialCheck, &QCheckBox::toggled, this, [tutorialCheck](bool checked) {
        ProgressManager::setTutorialHintsEnabled(checked);
        tutorialCheck->setText(checked ? QStringLiteral("开启") : QStringLiteral("关闭"));
        if (checked) {
            SoundManager::play(SoundCue::UiClick);
        }
    });
    auto *tutorialState = new QLabel(QStringLiteral("关闭后不再主动暂停；完整说明仍在图鉴中"), tutorialRow);
    tutorialState->setObjectName(QStringLiteral("settingHint"));
    auto *tutorialControls = new QHBoxLayout();
    tutorialControls->setContentsMargins(0, 0, 0, 0);
    tutorialControls->setSpacing(14);
    tutorialControls->addWidget(tutorialCheck);
    tutorialControls->addWidget(tutorialState, 1);
    auto *tutorialActionSlot = new QWidget(tutorialRow);
    tutorialActionSlot->setFixedWidth(settingActionWidth);
    tutorialControls->addWidget(tutorialActionSlot);
    tutorialLayout->addLayout(tutorialControls);

    auto [saveRow, saveLayout] = makeRow(QStringLiteral("存档"),
                                         QStringLiteral("本机记录关卡进度、序幕状态与图鉴解锁"));
    auto *progress = new QLabel(progressSummaryText(), saveRow);
    progress->setObjectName(QStringLiteral("settingHint"));
    progress->setWordWrap(true);
    auto *resetProgress = new QPushButton(QStringLiteral("清空进度"), saveRow);
    resetProgress->setObjectName(QStringLiteral("minorButton"));
    resetProgress->setFixedWidth(settingActionWidth);
    connect(resetProgress, &QPushButton::clicked, this, [this]() {
        if (!confirmClearProgress(this)) {
            return;
        }
        ProgressManager::resetProgressKeepSettings();
        SoundManager::play(SoundCue::Failure);
        showSettingsPage();
    });
    auto *saveControls = new QHBoxLayout();
    saveControls->setContentsMargins(0, 0, 0, 0);
    saveControls->setSpacing(14);
    auto *saveLeftSlot = new QWidget(saveRow);
    saveLeftSlot->setFixedWidth(82);
    saveControls->addWidget(saveLeftSlot);
    saveControls->addWidget(progress, 1);
    saveControls->addWidget(resetProgress);
    saveLayout->addLayout(saveControls);

    auto *defaultButton = new QPushButton(QStringLiteral("推荐默认"), panel);
    defaultButton->setObjectName(QStringLiteral("minorButton"));
    defaultButton->setFixedWidth(settingActionWidth);
    connect(defaultButton, &QPushButton::clicked, this, [this]() {
        SoundManager::setSoundEnabled(true);
        SoundManager::setMusicEnabled(true);
        SoundManager::setSoundVolume(82);
        SoundManager::setMusicVolume(46);
        ProgressManager::setTutorialHintsEnabled(true);
        if (m_settingsReturnToGame) {
            SoundManager::resumeMusic();
        } else {
            SoundManager::startMusic(MusicCue::Menu);
        }
        SoundManager::play(SoundCue::CodexUnlock);
        showSettingsPage();
    });

    panelLayout->addWidget(soundRow);
    panelLayout->addWidget(musicRow);
    panelLayout->addWidget(tutorialRow);
    panelLayout->addWidget(saveRow);
    panelLayout->addWidget(defaultButton, 0, Qt::AlignRight);

    auto *back = new QPushButton(m_settingsReturnToGame ? QStringLiteral("返回游戏") : QStringLiteral("返回开始界面"), page);
    connect(back, &QPushButton::clicked, this, [this]() {
        SoundManager::play(SoundCue::UiClick);
        if (m_settingsReturnToGame) {
            m_settingsReturnToGame = false;
            SoundManager::resumeMusic();
            SoundManager::refreshVolumes();
            m_stack->setCurrentWidget(m_game);
            m_game->setRunning(true);
            m_game->setFocus();
            return;
        }
        showStartPage();
    });

    layout->addStretch();
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(panel);
    layout->addWidget(back, 0, Qt::AlignCenter);
    layout->addStretch();
    return page;
}

QWidget *MainWindow::createCodexPage()
{
    const int highest = std::clamp(ProgressManager::highestUnlockedLevel(), 0, LevelManager::builtInLevelCount() - 1);
    auto towerText = [highest](int unlockLevel, const QString &text) {
        return highest >= unlockLevel ? text : QStringLiteral("资料锁定：该装置将在第 %1 关授权后显示完整参数与战术说明。").arg(unlockLevel + 1);
    };
    auto enemyText = [](int enemyIndex, const QString &text) {
        return ProgressManager::tutorialFlag(QStringLiteral("enemyIntroductions"), enemyIndex)
                   ? text
                   : QStringLiteral("资料锁定：首次遭遇该异常体后，敌情卡会写入图鉴。");
    };
    auto towerLocked = [highest](int unlockLevel) {
        return highest < unlockLevel;
    };
    auto enemyLocked = [](int enemyIndex) {
        return !ProgressManager::tutorialFlag(QStringLiteral("enemyIntroductions"), enemyIndex);
    };
    auto bossText = [highest](int levelIndex) {
        return highest >= levelIndex
                   ? bossProtocolBrief(levelIndex)
                   : QStringLiteral("资料锁定：通过第 %1 关后写入该关关底协议。").arg(levelIndex + 1);
    };
    auto bossLocked = [highest](int levelIndex) {
        return highest < levelIndex;
    };
    const std::vector<DatabaseCardData> cards = {
        {QStringLiteral("时间修补局"), QStringLiteral("负责维护城市主时间线的应急机构。本次危机中，正式修补员全部外勤，玩家以见习身份接入战术席位。"), QStringLiteral("bg-archive.png"), false},
        {QStringLiteral("局长 玄衡"), QStringLiteral("修补局现任局长，负责判断主线危机优先级。语气冷静，但会在关键节点给出复盘和压力判断。"), QStringLiteral("sheet-operators.png#op0"), false},
        {QStringLiteral("导航员 小昼"), QStringLiteral("玩家的战术导航员，把复杂局面拆成可执行目标。新手引导、关卡提示和失败安抚都由她承担。"), QStringLiteral("sheet-operators.png#op1"), false},
        {QStringLiteral("监测 AI：刻度"), QStringLiteral("负责记录敌情、装置授权和异常协议。它的判断逐渐从规则说明转向对幕后协议的追踪。"), QStringLiteral("sheet-operators.png#op2"), false},
        {QStringLiteral("指针炮台"), towerText(0, QStringLiteral("低费稳定单体输出，适合开局、拐角和出口补火力。面对密集波次时需要升级或配合范围塔。")), QStringLiteral("tower-shooter.png"), towerLocked(0)},
        {QStringLiteral("凝滞棱镜"), towerText(0, QStringLiteral("低伤害控制塔，能显著减速。适合高速敌人、加速格出口和 Boss 前缓冲区。")), QStringLiteral("tower-slow.png"), towerLocked(0)},
        {QStringLiteral("时能汲取仪"), towerText(0, QStringLiteral("经济装置，周期性生产少量时能。越早部署收益越高，但产能较慢且没有攻击能力。")), QStringLiteral("tower-resource.png"), towerLocked(0)},
        {QStringLiteral("悖论震荡器"), towerText(1, QStringLiteral("第 2 关授权。范围伤害并附带时蚀持续伤害，适合密集路线和分裂敌人。")), QStringLiteral("tower-splash.png"), towerLocked(1)},
        {QStringLiteral("因果切割器"), towerText(2, QStringLiteral("第 3 关授权。沿直线穿透多个目标，长直线路径和传送出口收益最高。")), QStringLiteral("tower-laser.png"), towerLocked(2)},
        {QStringLiteral("锚点屏障"), towerText(3, QStringLiteral("第 4 关授权。部署在路径上阻挡敌人，为火力争取时间，但会被攻击摧毁。")), QStringLiteral("tower-wall.png"), towerLocked(3)},
        {QStringLiteral("裂隙残影"), enemyText(0, QStringLiteral("基础异常体，生命和速度均衡。用指针炮台覆盖拐角即可稳定处理。")), QStringLiteral("enemy-normal-v2.png"), enemyLocked(0)},
        {QStringLiteral("加速残影"), enemyText(1, QStringLiteral("高速单位，生命较低但会快速压线。凝滞棱镜和出口前火力最有效。")), QStringLiteral("enemy-fast-v2.png"), enemyLocked(1)},
        {QStringLiteral("固化悖论"), enemyText(2, QStringLiteral("高生命低速度单位。优先升级已有输出，提高单位时间伤害。")), QStringLiteral("enemy-armored-v2.png"), enemyLocked(2)},
        {QStringLiteral("抗滞异常体"), enemyText(3, QStringLiteral("降低受到的伤害和减速时长。需要多层持续火力覆盖。")), QStringLiteral("enemy-resistant-v2.png"), enemyLocked(3)},
        {QStringLiteral("分叉时间体"), enemyText(4, QStringLiteral("死亡后分裂成高速子单位。适合用范围火力和减速提前处理。")), QStringLiteral("enemy-splitter-v2.png"), enemyLocked(4)},
        {QStringLiteral("纪元崩坏体"), enemyText(5, QStringLiteral("Boss 单位，血量和护盾高，出现时触发警报，需要冻结和全线集火。")), QStringLiteral("enemy-boss-v2.png"), enemyLocked(5)},
        {QStringLiteral("Boss：校准守门者"), bossText(0), QStringLiteral("enemy-boss-v2.png"), bossLocked(0)},
        {QStringLiteral("Boss：疾行站台长"), bossText(1), QStringLiteral("enemy-boss-v2.png"), bossLocked(1)},
        {QStringLiteral("Boss：雾港折跃核"), bossText(2), QStringLiteral("enemy-boss-v2.png"), bossLocked(2)},
        {QStringLiteral("Boss：逆流编目者"), bossText(3), QStringLiteral("enemy-boss-v2.png"), bossLocked(3)},
        {QStringLiteral("Boss：深井护盾体"), bossText(4), QStringLiteral("enemy-boss-v2.png"), bossLocked(4)},
        {QStringLiteral("Boss：纪元主宰"), bossText(5), QStringLiteral("enemy-boss-v2.png"), bossLocked(5)},
        {QStringLiteral("扩展内容"), QStringLiteral("6 关主线、关卡编辑器、指令台、进度保存、AI 美术、原创音效、演出动画和自动检查脚本。"), QString(), false}
    };
    auto codexSlice = [&cards](int start, int count) {
        std::vector<DatabaseCardData> result;
        const int end = std::min(start + count, static_cast<int>(cards.size()));
        for (int i = start; i < end; ++i) {
            result.push_back(cards[static_cast<size_t>(i)]);
        }
        return result;
    };
    const std::vector<DatabaseCardData> terrainCards = {
        {QStringLiteral("稳定格"), QStringLiteral("标准部署格，可放置大多数修补装置。守住拐角和出口最稳定。"), QStringLiteral("tile-stable.png"), false},
        {QStringLiteral("高亮轨道"), QStringLiteral("异常行进路线，普通塔不能放在线路上，屏障可用于临时封堵。"), QStringLiteral("tile-path.png"), false},
        {QStringLiteral("补给格"), QStringLiteral("部署费用降低，适合提前布置经济装置或关键控制塔。"), QStringLiteral("tile-discount.png"), false},
        {QStringLiteral("锚点格"), QStringLiteral("适合放置屏障和守线装置，能把敌群拖入火力覆盖区。"), QStringLiteral("tile-anchor.png"), false},
        {QStringLiteral("加速格"), QStringLiteral("异常经过时速度提升，需要提前使用减速和范围火力处理。"), QStringLiteral("tile-accelerate.png"), false},
        {QStringLiteral("传送门"), QStringLiteral("异常会折跃到另一处路径点，出口前应布置高压火力。"), QStringLiteral("tile-portal.png"), false}
    };
    const std::vector<DatabaseSection> sections = {
        {QStringLiteral("角色"), codexSlice(0, 4)},
        {QStringLiteral("装置"), codexSlice(4, 6)},
        {QStringLiteral("异常"), codexSlice(10, 6)},
        {QStringLiteral("Boss"), codexSlice(16, 6)},
        {QStringLiteral("地形"), terrainCards},
        {QStringLiteral("系统"), codexSlice(22, 1)}
    };
    return createDatabasePage(this,
                              QStringLiteral("时间图鉴"),
                              QStringLiteral("装置、敌人与挑战目标速查数据库"),
                              sections,
                              [this]() { showStoryPage(); });
}
QWidget *MainWindow::createDialoguePage()
{
    return new DialoguePage(this);
}

QWidget *MainWindow::createMissionBriefingPage()
{
    return new MissionBriefingPage(this);
}

QWidget *MainWindow::createResultPage(const QString &titleText, const QString &subtitleText, bool victory)
{
    auto *page = new ResultPage(victory ? QStringLiteral("bg-victory.png") : QStringLiteral("bg-failure.png"), victory, this);
    page->setStyleSheet(QStringLiteral(
        "QWidget { background: transparent; }"
        "QLabel#title { font-size: 42px; font-weight: 700; color: %1; }"
        "QLabel#subtitle { font-size: 18px; color: #c7d8e8; }"
        "QPushButton { min-width: 240px; min-height: 42px; font-size: 17px;"
        "  border: 1px solid #50d4ff; border-radius: 8px; color: #dff8ff;"
        "  background: #102842; }"
        "QPushButton:hover { background: #18395c; border-color: #ffe19a; color: #ffe19a; }")
                            .arg(victory ? QStringLiteral("#ffe19a") : QStringLiteral("#ff7c7c")));

    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(76, 50, 76, 46);
    layout->setSpacing(18);

    auto *title = new QLabel(titleText, page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);

    auto *subtitle = new QLabel(subtitleText, page);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);
    subtitle->setMaximumWidth(820);
    subtitle->setStyleSheet(QStringLiteral(
        "QLabel#subtitle { color: #dff8ff; font-size: 18px;"
        " background: rgba(5, 14, 28, 165); border: 1px solid rgba(255, 225, 154, 95);"
        " border-radius: 8px; padding: 14px; }"));

    auto *statPanel = new QWidget(page);
    statPanel->setMaximumWidth(760);
    auto *statGrid = new QGridLayout(statPanel);
    statGrid->setContentsMargins(0, 0, 0, 0);
    statGrid->setHorizontalSpacing(12);
    statGrid->setVerticalSpacing(12);
    statGrid->addWidget(makeStatCard(QStringLiteral("用时"), QStringLiteral("stat_time"), statPanel), 0, 0);
    statGrid->addWidget(makeStatCard(QStringLiteral("剩余时能"), QStringLiteral("stat_energy"), statPanel), 0, 1);
    statGrid->addWidget(makeStatCard(QStringLiteral("击败异常"), QStringLiteral("stat_kills"), statPanel), 0, 2);
    statGrid->addWidget(makeStatCard(QStringLiteral("部署装置"), QStringLiteral("stat_towers"), statPanel), 1, 0);
    statGrid->addWidget(makeStatCard(QStringLiteral("完成波次"), QStringLiteral("stat_waves"), statPanel), 1, 1);
    statGrid->addWidget(makeStatCard(QStringLiteral("评级"), QStringLiteral("stat_rating"), statPanel), 1, 2);

    auto *again = new QPushButton(QStringLiteral("重新开始本关"), page);
    again->setObjectName(QStringLiteral("result_again"));
    connect(again, &QPushButton::clicked, this, [this]() {
        if (m_reverseSession) {
            launchReverseMode(m_game->reverseScenarioIndex());
        } else {
            startLevel(m_game->levelIndex());
        }
    });

    auto *next = new QPushButton(QStringLiteral("进入下一条时间线"), page);
    next->setObjectName(QStringLiteral("result_next"));
    connect(next, &QPushButton::clicked, this, &MainWindow::startNextLevel);
    next->setVisible(victory);

    auto *menu = new QPushButton(QStringLiteral("返回档案与图鉴"), page);
    menu->setObjectName(QStringLiteral("result_menu"));
    connect(menu, &QPushButton::clicked, this, [this]() {
        if (m_reverseSession) {
            showReverseSelectPage();
        } else {
            showLevelSelectPage();
        }
    });

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(statPanel, 0, Qt::AlignCenter);
    layout->addSpacing(22);
    layout->addWidget(again, 0, Qt::AlignCenter);
    layout->addWidget(next, 0, Qt::AlignCenter);
    layout->addWidget(menu, 0, Qt::AlignCenter);

    return page;
}

QWidget *MainWindow::createFailureAnnouncePage()
{
    auto *page = new ResultPage(QStringLiteral("bg-failure.png"), false, this);
    page->setStyleSheet(pageStyle());

    auto *layout = new QVBoxLayout(page);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(76, 60, 76, 60);
    layout->setSpacing(18);

    auto *title = new QLabel(QStringLiteral("时间核心崩坏"), page);
    title->setObjectName(QStringLiteral("title"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral("QLabel#title { font-size: 54px; font-weight: 800; color: #ff7c7c; }"));

    auto *subtitle = new QLabel(QStringLiteral("异常体已抵达核心，当前时间线封存失败。"), page);
    subtitle->setObjectName(QStringLiteral("failure_level"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);
    subtitle->setMaximumWidth(820);
    subtitle->setStyleSheet(QStringLiteral(
        "QLabel { color: #ffd6dc; font-size: 20px;"
        " background: rgba(70, 0, 18, 150); border: 1px solid rgba(255, 90, 105, 140);"
        " border-radius: 8px; padding: 16px; }"));

    auto *hint = new QLabel(QStringLiteral("修补局将启动回溯通讯，分析崩口并给出下一次调整建议。"), page);
    hint->setAlignment(Qt::AlignCenter);
    hint->setWordWrap(true);
    hint->setMaximumWidth(760);
    hint->setStyleSheet(QStringLiteral("QLabel { color: #dff8ff; font-size: 17px; }"));

    auto *button = new QPushButton(QStringLiteral("查看回溯通讯"), page);
    connect(button, &QPushButton::clicked, this, [this]() { showOutroDialogue(false); });

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(hint);
    layout->addSpacing(28);
    layout->addWidget(button, 0, Qt::AlignCenter);
    return page;
}

void MainWindow::refreshResultPage(bool victory)
{
    QWidget *page = victory ? m_victoryPage : m_failurePage;
    const GameSummary summary = m_game->summary();
    if (auto *resultPage = dynamic_cast<ResultPage *>(page)) {
        const SoundCue cue = summary.reverseMode
                                 ? (victory ? SoundCue::BossStinger : SoundCue::ErrorDeny)
                                 : (victory ? SoundCue::Victory : SoundCue::Failure);
        resultPage->restartAnimation(cue);
    }
    auto *title = page->findChild<QLabel *>(QStringLiteral("title"));
    auto *subtitle = page->findChild<QLabel *>(QStringLiteral("subtitle"));
    if (auto *again = page->findChild<QPushButton *>(QStringLiteral("result_again"))) {
        again->setText(summary.reverseMode ? QStringLiteral("重新推演") : QStringLiteral("重新开始本关"));
    }
    if (auto *next = page->findChild<QPushButton *>(QStringLiteral("result_next"))) {
        next->setVisible(victory && !summary.reverseMode);
    }
    if (auto *menu = page->findChild<QPushButton *>(QStringLiteral("result_menu"))) {
        menu->setText(summary.reverseMode ? QStringLiteral("返回逆向选择") : QStringLiteral("返回关卡选择"));
    }
    if (title) {
        if (summary.reverseMode) {
            title->setText(victory
                               ? QStringLiteral("%1 突破").arg(summary.levelName)
                               : QStringLiteral("%1 受阻").arg(summary.levelName));
        } else {
            title->setText(victory ? QStringLiteral("时间线修复完成") : QStringLiteral("时间核心崩坏"));
        }
    }
    if (subtitle) {
        if (summary.reverseMode) {
            subtitle->setText(victory
                                  ? QStringLiteral("异常协议突破系统防线，核心已被抵达。")
                                  : QStringLiteral("系统防守 AI 守住了本轮异常投放。"));
        } else {
            const QString review = performanceReviewForSummary(summary, victory);
            subtitle->setText(victory
                                  ? QStringLiteral("%1\n%2\n\n作战评价：%3")
                                        .arg(LevelManager::levelVictoryText(summary.levelIndex),
                                             LevelManager::levelNextHook(summary.levelIndex),
                                             review)
                                  : QStringLiteral("异常体突破防线，当前时间线需要回溯。\n作战评价：%1").arg(review));
        }
    }
    const QString rating = summary.reverseMode
                               ? (victory ? QStringLiteral("突破") : QStringLiteral("受阻"))
                               : (victory
                                      ? (summary.energy >= 260 && summary.elapsedSeconds <= 180.0 ? QStringLiteral("S")
                                         : summary.energy >= 160 ? QStringLiteral("A")
                                                                 : QStringLiteral("B"))
                                      : QStringLiteral("待修复"));
    if (auto *resultPage = dynamic_cast<ResultPage *>(page)) {
        resultPage->setRatingText(rating);
    }
    const std::vector<std::pair<QString, QString>> values = {
        {QStringLiteral("stat_time"), minutesSeconds(summary.elapsedSeconds)},
        {QStringLiteral("stat_energy"), QString::number(summary.energy)},
        {QStringLiteral("stat_kills"), QString::number(summary.enemiesDefeated)},
        {QStringLiteral("stat_towers"), QString::number(summary.towersBuilt)},
        {QStringLiteral("stat_waves"), QStringLiteral("%1/%2").arg(summary.wavesCleared).arg(summary.totalWaves)},
        {QStringLiteral("stat_rating"), rating}
    };
    for (const auto &entry : values) {
        if (auto *label = page->findChild<QLabel *>(entry.first)) {
            label->setText(entry.second);
        }
    }
}

void MainWindow::startLevel(int index)
{
    if (std::clamp(index, 0, LevelManager::builtInLevelCount() - 1) == 0) {
        showMissionBriefing(0);
        return;
    }
    showIntroDialogue(index);
}

void MainWindow::continueMainStory()
{
    m_acceptanceMode = false;
    if (!ProgressManager::prologueSeen()) {
        showPrologueThenStart();
        return;
    }
    const int target = std::clamp(ProgressManager::lastLevelIndex(), 0, LevelManager::builtInLevelCount() - 1);
    startLevel(std::min(target, ProgressManager::highestUnlockedLevel()));
}

void MainWindow::startNewStory()
{
    m_acceptanceMode = false;
    ProgressManager::saveLastLevelIndex(0);
    showPrologueThenStart();
}

void MainWindow::showOpeningPrologue()
{
    showStartPage();
}

void MainWindow::showPrologueThenStart()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Story);
    auto *dialogue = static_cast<DialoguePage *>(m_dialoguePage);
    dialogue->setDialogue(QStringLiteral("序幕通讯：接入修补局"),
                          polishedPrologueDialogue(),
                          QStringLiteral("进入任务投影"),
                          [this]() {
                              ProgressManager::markPrologueSeen();
                              showMissionBriefing(0);
                          });
    m_stack->setCurrentWidget(m_dialoguePage);
}

void MainWindow::showRecruitBriefingThenStart()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Story);
    auto *dialogue = static_cast<DialoguePage *>(m_dialoguePage);
    dialogue->setDialogue(QStringLiteral("见习修补员入职卡"),
                          {
                              {1, QStringLiteral("导航员 小昼"), QStringLiteral("入职确认：你不是在一口气记住所有东西。第一关会暂停教学，带你看路线、资源、敌人属性、建塔、升级和冻结。")},
                              {2, QStringLiteral("监测 AI：刻度"), QStringLiteral("核心循环：观察路径，部署装置，升级关键点，用冻结处理失控波次。每种新装置和新敌人首次出现时，只会说明一次。")},
                              {0, QStringLiteral("局长 玄衡"), QStringLiteral("允许犯错，但不允许不复盘。失败后先确认核心崩坏，再读取回溯通讯。现在，进入训练线。")}
                          },
                          QStringLiteral("进入第一关"),
                          [this]() { startLevel(0); });
    m_stack->setCurrentWidget(m_dialoguePage);
}

void MainWindow::launchLevel(int index)
{
    m_reverseSession = false;
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Battle);
    ProgressManager::saveLastLevelIndex(index);
    m_game->setLevelIndex(index);
    m_stack->setCurrentWidget(m_game);
    m_game->setRunning(true);
    m_game->setFocus();
}

void MainWindow::launchReverseMode(int scenarioIndex)
{
    m_reverseSession = true;
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Battle);
    m_game->startReverseMode(scenarioIndex);
    m_stack->setCurrentWidget(m_game);
    m_game->setRunning(true);
    m_game->setFocus();
}

void MainWindow::showMissionBriefing(int index)
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Story);
    m_pendingLevelIndex = std::clamp(index, 0, LevelManager::builtInLevelCount() - 1);
    auto *missionPage = static_cast<MissionBriefingPage *>(m_missionBriefingPage);
    missionPage->setMission(m_pendingLevelIndex, [this]() { launchLevel(m_pendingLevelIndex); });
    m_stack->setCurrentWidget(m_missionBriefingPage);
}

void MainWindow::showPrologueDialogue()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Story);
    auto *dialogue = static_cast<DialoguePage *>(m_dialoguePage);
    dialogue->setDialogue(QStringLiteral("序章：接入修补局"),
                          polishedPrologueDialogue(),
                          QStringLiteral("返回档案与图鉴"),
                          [this]() {
                              ProgressManager::markPrologueSeen();
                              showStoryPage();
                          });
    m_stack->setCurrentWidget(m_dialoguePage);
}

void MainWindow::showIntroDialogue(int index)
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Story);
    m_pendingLevelIndex = std::clamp(index, 0, LevelManager::builtInLevelCount() - 1);
    auto *dialogue = static_cast<DialoguePage *>(m_dialoguePage);
    dialogue->setDialogue(QStringLiteral("%1 任务通讯").arg(LevelManager::levelTitle(m_pendingLevelIndex)),
                          polishedIntroDialogueForLevel(m_pendingLevelIndex),
                          QStringLiteral("进入任务投影"),
                          [this]() { showMissionBriefing(m_pendingLevelIndex); });
    m_stack->setCurrentWidget(m_dialoguePage);
}

void MainWindow::showOutroDialogue(bool victory)
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Story);
    const GameSummary summary = m_game->summary();
    auto *dialogue = static_cast<DialoguePage *>(m_dialoguePage);
    dialogue->setDialogue(victory ? QStringLiteral("修补完成通讯") : QStringLiteral("回溯通讯"),
                          outroDialogueForSummary(summary, victory),
                          QStringLiteral("查看结算"),
                          [this, victory]() {
                              refreshResultPage(victory);
                              m_stack->setCurrentWidget(victory ? m_victoryPage : m_failurePage);
                          });
    m_stack->setCurrentWidget(m_dialoguePage);
}

void MainWindow::showStartPage()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Menu);
    m_acceptanceMode = false;
    m_archiveReturnMode = false;
    m_settingsReturnToGame = false;
    m_reverseSession = false;
    if (m_startPage) {
        m_stack->removeWidget(m_startPage);
        m_startPage->deleteLater();
    }
    m_startPage = createStartPage();
    m_stack->insertWidget(1, m_startPage);
    m_stack->setCurrentWidget(m_startPage);
}

void MainWindow::showLevelSelectPage()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Menu);
    m_settingsReturnToGame = false;
    m_reverseSession = false;
    if (m_levelSelectPage) {
        m_stack->removeWidget(m_levelSelectPage);
        m_levelSelectPage->deleteLater();
    }
    m_levelSelectPage = createLevelSelectPage();
    m_stack->addWidget(m_levelSelectPage);
    m_stack->setCurrentWidget(m_levelSelectPage);
}

void MainWindow::showStoryPage()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Menu);
    m_settingsReturnToGame = false;
    m_stack->setCurrentWidget(m_storyPage);
}

void MainWindow::showHelpPage()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Menu);
    m_stack->setCurrentWidget(m_helpPage);
}

void MainWindow::showAboutPage()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Menu);
    m_settingsReturnToGame = false;
    m_stack->setCurrentWidget(m_aboutPage);
}

void MainWindow::showSettingsPage()
{
    m_game->setRunning(false);
    if (m_settingsReturnToGame) {
        SoundManager::refreshVolumes();
    } else {
        SoundManager::startMusic(MusicCue::Menu);
    }
    if (m_settingsPage) {
        m_stack->removeWidget(m_settingsPage);
        m_settingsPage->deleteLater();
    }
    m_settingsPage = createSettingsPage();
    m_stack->addWidget(m_settingsPage);
    m_stack->setCurrentWidget(m_settingsPage);
}

void MainWindow::showCodexPage()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Menu);
    if (m_codexPage) {
        m_stack->removeWidget(m_codexPage);
        m_codexPage->deleteLater();
    }
    m_codexPage = createCodexPage();
    m_stack->addWidget(m_codexPage);
    m_stack->setCurrentWidget(m_codexPage);
}

void MainWindow::showReverseSelectPage()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Menu);
    m_archiveReturnMode = true;
    m_reverseSession = false;
    if (m_reverseSelectPage) {
        m_stack->removeWidget(m_reverseSelectPage);
        m_reverseSelectPage->deleteLater();
    }
    m_reverseSelectPage = createReverseSelectPage();
    m_stack->addWidget(m_reverseSelectPage);
    m_stack->setCurrentWidget(m_reverseSelectPage);
}

void MainWindow::showEditorPage()
{
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Menu);
    m_stack->setCurrentWidget(m_editor);
}

void MainWindow::playCustomLevel(const QString &path)
{
    m_reverseSession = false;
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Battle);
    m_game->loadCustomLevel(path);
    m_stack->setCurrentWidget(m_game);
    m_game->setRunning(true);
    m_game->setFocus();
}

void MainWindow::showVictoryPage()
{
    if (m_game->summary().reverseMode) {
        m_game->setRunning(false);
        SoundManager::startMusic(MusicCue::Menu);
        refreshResultPage(true);
        m_stack->setCurrentWidget(m_victoryPage);
        return;
    }
    showOutroDialogue(true);
}

void MainWindow::showFailurePage()
{
    if (m_game->summary().reverseMode) {
        m_game->setRunning(false);
        SoundManager::startMusic(MusicCue::Menu);
        refreshResultPage(false);
        m_stack->setCurrentWidget(m_failurePage);
        return;
    }
    m_game->setRunning(false);
    SoundManager::startMusic(MusicCue::Story);
    const GameSummary summary = m_game->summary();
    if (auto *label = m_failureAnnouncePage->findChild<QLabel *>(QStringLiteral("failure_level"))) {
        label->setText(QStringLiteral("%1 防线被突破。\n当前完成波次：%2/%3，部署装置：%4。")
                           .arg(summary.levelName)
                           .arg(summary.wavesCleared)
                           .arg(summary.totalWaves)
                           .arg(summary.towersBuilt));
    }
    if (auto *page = dynamic_cast<ResultPage *>(m_failureAnnouncePage)) {
        page->restartAnimation();
    }
    m_stack->setCurrentWidget(m_failureAnnouncePage);
}

void MainWindow::startNextLevel()
{
    const int next = m_game->levelIndex() + 1;
    if (next >= LevelManager::builtInLevelCount()) {
        showLevelSelectPage();
        return;
    }
    startLevel(next);
}
