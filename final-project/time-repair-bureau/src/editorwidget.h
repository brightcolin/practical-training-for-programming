#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QPoint>
#include <QPushButton>
#include <QWidget>

#include <vector>

class EditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit EditorWidget(QWidget *parent = nullptr);
    QString customLevelPath() const;

signals:
    void playRequested(const QString &path);
    void backRequested();

public:
    enum class EditTile {
        Stable,
        Path,
        Discount,
        Anchor,
        Accelerate,
        Portal
    };

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void resetDefault();
    bool saveLevel();
    void loadLevel();
    void paintCellAt(const QPoint &pos, EditTile tile);
    void updateToolButtonStyles();
    std::vector<QPoint> routePath() const;
    std::vector<QPoint> portalCellsOnRoute(const std::vector<QPoint> &path) const;
    bool isRouteTile(EditTile tile) const;
    QPoint cellAt(const QPoint &pos) const;
    QRectF cellRect(int row, int col) const;
    QColor tileColor(EditTile tile) const;
    QString tileName(EditTile tile) const;

    int m_rows = 7;
    int m_cols = 12;
    int m_cellSize = 54;
    QPoint m_origin = QPoint(36, 120);
    EditTile m_selected = EditTile::Stable;
    std::vector<EditTile> m_tiles;
    std::vector<QPushButton *> m_buttons;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_loadButton = nullptr;
    QPushButton *m_playButton = nullptr;
    QPushButton *m_resetButton = nullptr;
    QPushButton *m_backButton = nullptr;
    QString m_message;
};

#endif
