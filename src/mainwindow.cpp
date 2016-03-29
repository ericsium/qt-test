#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#include <QSqlQuery>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    db(QSqlDatabase::addDatabase("QSQLITE")),
    model_(new QSqlTableModel(this, db))
{
    db.setDatabaseName("test.db");
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionLoad_triggered()
{
    if (db.open()) {
        qDebug() << "db open for business";
    } else {
        qDebug() << "can't open db";
    }

    for (auto table: db.tables()) {
        //QSqlQuery query;
        //query.exec(QString("drop table) " + table));
    }

    // Populate with test data
    QSqlQuery query;
    query.exec("create table person "
              "(id integer primary key, "
              "firstname varchar(20), "
              "lastname varchar(30), "
              "age integer)");

    model_->setTable("person");
    model_->select();
    model_->setHeaderData(0, Qt::Horizontal, tr("id"));
    model_->setHeaderData(1, Qt::Horizontal, tr("first"));
    model_->setHeaderData(2, Qt::Horizontal, tr("last"));
    model_->setHeaderData(3, Qt::Horizontal, tr("age"));

    ui->tableView->setModel(model_);
    ui->tableView->horizontalHeader()->setSectionsMovable(true);
    ui->tableView->show();

    {
    QSqlQuery query;
    query.prepare("INSERT INTO person (id, firstname, lastname, age) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(1001);
    query.addBindValue("Bart");
    query.addBindValue("Simpson");
    query.addBindValue("9");
    query.exec();
    }
}
