#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class QEvent;

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
