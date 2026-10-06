#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QSignalMapper>

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private slots:
    void AppendDigit(int digit);
    void ClearDisplay();

private:
    QLineEdit *display;
    QSignalMapper *signal_mapper;

};
#endif // WIDGET_H
