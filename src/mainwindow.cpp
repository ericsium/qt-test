#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#include <QSqlQuery>
#include <QSqlRecord>
#include <QFileDialog>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    db(QSqlDatabase::addDatabase("QSQLITE")),
    model_(new QSqlQueryModel(this))
{
    ui->setupUi(this);

    ui->tableView->setModel(model_);
    ui->tableView->horizontalHeader()->setSectionsMovable(true);
    ui->tableView->show();

    // Show/Hide columns based on listwidget selection
    connect(ui->listWidget, &QListWidget::itemChanged, [&](QListWidgetItem *item) {
        if (item->checkState() == Qt::Unchecked) {
            ui->tableView->hideColumn(item->listWidget()->row(item));
        } else {
            ui->tableView->showColumn(item->listWidget()->row(item));
        }
    });

    // If a row is resized apply to all rows
    connect(ui->tableView->verticalHeader(), &QHeaderView::sectionResized, [&](int, int, int newsize) {
        QTableView &view = *ui->tableView;
        int count = view.verticalHeader()->count();

        for (int index = 0; index < count; ++index) {
            view.setRowHeight(index, newsize);
        }
        // Update row height default with new value
        ui->tableView->verticalHeader()->setProperty("defaultSectionSize", newsize);
    });

    // Provide user validation feedback for SQL field and handle queries
    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, [&]() {
        bool valid{false};
        bool empty{false};

        QString filter{ui->plainTextEdit->document()->toPlainText()};

        if (filter.size() == 0) {
            valid = true;
            empty = false;
        } else {
            QSqlQuery query;

            // Basically we want to verify the 'where' clause is valid.
            if (query.exec(filter + " LIMIT 1")) {
                query.next();
                valid = true;
                empty = !query.isValid();
            }
        }

        if (valid && !empty) {
            model_->setQuery(filter);
            UpdateListWidgetColumnNames(ui->listWidget);
            ui->tableView->resizeColumnsToContents();
            emit filterStatusChanged("Valid");
        } else if (empty) {
            emit filterStatusChanged("Invalid (empty result)");
        } else {
            emit filterStatusChanged("Invalid");
        }
    });

    // Update groupbox title with filter status information
    connect(this, &MainWindow::filterStatusChanged, [&](QString status) {
        ui->groupBox_2->setTitle(QString("SQL: ") + status);
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::UpdateListWidgetColumnNames(QListWidget *widget)
{
    QSqlRecord record = model_->record();

    // First save column_hidden attribute
    for (int index = 0; index < widget->count(); ++index) {
        QListWidgetItem &item = *(widget->item(index));
        column_hidden_[item.text()] = (item.checkState() == Qt::Unchecked);
    }
    widget->clear();
    for (int index = 0; index < record.count(); ++index) {
        QListWidgetItem *item = new QListWidgetItem;
        QString name = record.fieldName(index);
        item->setData(Qt::DisplayRole, name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        widget->addItem(item);
        // Default starting state is 'checked', but honor previous state
        // we explicity add after addItem so listWidget sends out itemChanged notification
        item->setCheckState(column_hidden_[name] ? Qt::Unchecked : Qt::Checked);
    }
}

void MainWindow::on_actionLoad_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Database"), QString(), tr("Database Files (*.db)"));

    db.setDatabaseName(fileName);

    if (db.open()) {
        qDebug() << "db open for business";
    } else {
        qDebug() << "can't open db";
        return;
    }

    bool ok;
    QString table = QInputDialog::getItem(this, "Choose initial table", "Tables:", db.tables(),0,false, &ok);

    ui->plainTextEdit->setPlainText(QString("SELECT * FROM ") + table);
}

void MainWindow::on_actionCreate_triggered()
{

    db.setDatabaseName("test.db");

    if (db.open()) {
        qDebug() << "db open for business";
    } else {
        qDebug() << "can't open db";
        return;
    }

    // Populate with test data
    QSqlQuery query;
    query.exec("create table person "
              "(id integer primary key, "
              "firstname varchar(20), "
              "lastname varchar(30), "
              "age integer)");

    query.prepare("INSERT INTO person (id, firstname, lastname, age) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(1001);
    query.addBindValue("Bart");
    query.addBindValue("Simpson");
    query.addBindValue("9");
    query.exec();
    query.prepare("INSERT INTO person (id, firstname, lastname, age) "
                  "VALUES (?, ?, ?, ?)");
    query.addBindValue(1002);
    query.addBindValue("Maggie");
    query.addBindValue("Simpson");
    query.addBindValue("6");
    query.exec();
}
