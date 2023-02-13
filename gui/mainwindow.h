#ifndef GUI_MAINWINDOW_H
#define GUI_MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QtGui>

namespace Gui {

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QDeclarativeView *ui;
};

} // namespace Gui

#endif // GUI_MAINWINDOW_H
