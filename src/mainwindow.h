#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtSql/QSqlDatabase>
#include <QSqlTableModel>

class QListWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
   void InitListWidgetWithColumnNames(QListWidget *widget);
   void SelectTable(QString table);

private slots:
    void on_actionLoad_triggered();

signals:
    void filterStatusChanged(QString status);

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QSqlTableModel *model_;
    QString active_table_;
};

#endif // MAINWINDOW_H
