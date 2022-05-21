#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qgraphicsscene.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QGraphicsScene m_gfx_scene;
};
#endif // MAINWINDOW_H
