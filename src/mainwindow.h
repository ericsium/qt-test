#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtSql/QSqlDatabase>
#include <QSqlQueryModel>
#include <map>

class QListWidget;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    typedef std::map<QString, bool> StringToBoolMap;
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
   void UpdateColumnInfo();
   QVariant GetPragmaTableInfo(QString table, QString field, QString col);

private slots:
    void on_actionLoad_triggered();
    void on_actionCreate_triggered();

signals:
    void queryStatusChanged(QString status);

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QSqlQueryModel *model_;
    QString active_table_;
    StringToBoolMap column_hidden_;
};

#endif // MAINWINDOW_H
