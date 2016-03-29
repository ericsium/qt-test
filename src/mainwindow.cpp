#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#include <QSqlQuery>
#include <QSqlRecord>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    db(QSqlDatabase::addDatabase("QSQLITE")),
    model_(new QSqlTableModel(this, db))
{
    db.setDatabaseName("test.db");
    ui->setupUi(this);
   // connect(_model, QSqlDatabase::)
    connect(ui->listWidget, &QListWidget::itemChanged, [&](QListWidgetItem *item) {
        if (item->checkState() == Qt::Checked) {
            ui->tableView->showColumn(item->listWidget()->row(item));
        } else {
            ui->tableView->hideColumn(item->listWidget()->row(item));
        }
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::InitListWidgetWithColumnNames(QListWidget *widget)
{
    QSqlRecord record = model_->record();
    widget->clear();
    for (int index = 0; index < record.count(); ++index) {
        QListWidgetItem *item = new QListWidgetItem;
        item->setData(Qt::DisplayRole, record.fieldName(index));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable); // set checkable flag
        item->setCheckState(Qt::Checked); // AND initialize check state
        widget->addItem(item);
    }
}

void MainWindow::SelectTable(QString table)
{
    model_->setTable(table);

    QSqlRecord record = model_->record();
    for (int index = 0; index < record.count(); ++index) {
        model_->setHeaderData(index, Qt::Horizontal, record.fieldName(index));
    }

    model_->select();
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

    SelectTable("person");

    ui->tableView->setModel(model_);
    ui->tableView->horizontalHeader()->setSectionsMovable(true);
    ui->tableView->resizeColumnsToContents();
    InitListWidgetWithColumnNames(ui->listWidget);

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
