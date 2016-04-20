#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QtSql/QSqlDatabase>
#include <QSqlQueryModel>
#include <QSharedPointer>
#include <map>

class QListWidget;
class QFile;

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

    void on_actionResize_triggered();

signals:
    void queryStatusChanged(const QString &status);
    void rowSelectionChanged(const QVariantMap &map);
    void fileLineChanged(QString file, int line);

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QSqlQueryModel *model_;
    QString active_table_;
    StringToBoolMap column_hidden_;
    QMap<QString, QSharedPointer<QFile> > file_map_;
};

#endif // MAINWINDOW_H
