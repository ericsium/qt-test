#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>

#include <QSqlQuery>
#include <QSqlRecord>
#include <QFileDialog>
#include <QInputDialog>
#include <QComboBox>
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlIndex>
#include <QVector>

const QList<QString> g_column_properties{"name", "type", "pk"};

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

    ui->treeWidget->setColumnCount(g_column_properties.count());
    ui->treeWidget->setHeaderLabels(g_column_properties);

    ui->treeWidget_2->header()->hide();
    ui->treeWidget_2->setRootIsDecorated(false);

    // Show/Hide columns based on TreeWidget selection
    connect(ui->treeWidget_2, &QTreeWidget::itemChanged, [&](QTreeWidgetItem *item, int col) {
        if (item->checkState(col) == Qt::Unchecked) {
            ui->tableView->hideColumn(ui->treeWidget_2->indexOfTopLevelItem(item));
        } else {
            ui->tableView->showColumn(ui->treeWidget_2->indexOfTopLevelItem(item));
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
        ui->tableView->setProperty("verticalHeaderDefaultSectionSize", newsize);
    });

    // Provide user validation feedback for SQL field and initiate queries
    connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, [&]() {
        bool valid{false};
        bool empty{false};

        // First check if SQL evaluation is enabled and return if it is not
        if (!ui->pushButton->isChecked()) {
            emit queryStatusChanged("(Active Evaluation disabled)");
            return;
        }

        QString query_text{ui->plainTextEdit->document()->toPlainText()};

        if (query_text.size() == 0) {
            valid = true;
            empty = false;
        } else {
            QSqlQuery query;
            // Basically we want to verify the 'where' clause is valid.
            if (query.exec(query_text)) {
                query.next();
                valid = true;
                empty = !query.isValid();
            }
            query.clear();
        }

        if (valid && !empty) {
            model_->setQuery(query_text);
            UpdateColumnInfo();
            emit queryStatusChanged("Valid");
        } else if (empty) {
            emit queryStatusChanged("Invalid (empty result)");
        } else {
            emit queryStatusChanged("Invalid");
        }
    });

    // Update groupbox title with query status information
    connect(this, &MainWindow::queryStatusChanged, [&](QString status) {
        ui->groupBox_2->setTitle(QString("SQL: ") + status);
    });

    // When user selects new combobox entry, update the SQL query.
    connect(ui->comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [=](int index){
        ui->plainTextEdit->setPlainText(ui->comboBox->itemData(index).toString());
    });

    // When 'active evaluation' button is checked make sure to trigger textChanged to process query
    connect(ui->pushButton, &QPushButton::clicked, [&](bool checked) {
        if (checked) emit ui->plainTextEdit->textChanged();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::UpdateColumnInfo()
{
    QTreeWidget &widget = *ui->treeWidget_2;
    QSqlRecord record = model_->record();

    // First save column_hidden attribute
    for (int index = 0; index < widget.topLevelItemCount(); ++index) {
        QTreeWidgetItem *item = widget.topLevelItem(index);
        column_hidden_[item->text(0)] = (item->checkState(0) == Qt::Unchecked);
    }
    widget.clear();

    for (int index = 0; index < record.count(); ++index) {
        QTreeWidgetItem *item = new QTreeWidgetItem;
        QString name = record.fieldName(index);
        item->setData(0, Qt::DisplayRole, name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        widget.addTopLevelItem(item);
        // Default starting state is 'checked', but honor previous state
        // we explicity add after addItem so listWidget sends out itemChanged notification
        item->setCheckState(0, column_hidden_[name] ? Qt::Unchecked : Qt::Checked);
    }
    // TODO: Remember user sized column changes
    ui->tableView->resizeColumnsToContents();

}

QVariant MainWindow::GetPragmaTableInfo(QString table, QString field, QString col)
{
    static QVariantMap map;

    if (map.empty()) {
        for (const auto & table: db.tables(QSql::AllTables)) {
            QSqlQuery query;
            query.exec(QString("PRAGMA TABLE_INFO(%1);").arg(table));
            QSqlRecord record = query.record();
            while(query.next()) {
                QString field = query.value(record.indexOf("name")).toString();
                for (auto const &col: g_column_properties) {
                    map[table + "." + field + "." + col] = query.value(col);
                }
            }
        }
    }

    QVariantMap::const_iterator it = map.find(table + "." + field + "." + col);
    if (it != map.constEnd()) return it.value();

    return QVariant();
}

void MainWindow::on_actionLoad_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open Database"),
                                                    QString(), tr("Database Files (*.db)"));

    db.setDatabaseName(fileName);

    if (db.open()) {
        qDebug() << "db open for business";
    } else {
        qDebug() << "can't open db";
        return;
    }

    // Set 'hidden' attributes for all columns that are primary keys
    // because by default people won't be interested in seeing them
    [this](QSqlDatabase &db, StringToBoolMap &col) {
        for (auto const &table: db.tables()) {
            QSqlRecord record = db.record(table);
            for (int index = 0; index < record.count(); ++index) {
                QString field = record.fieldName(index);
                if (GetPragmaTableInfo(table, field, "pk").toInt() > 0) {
                    col[record.fieldName(index)] = true;
                }
            }
        }
    }(db, column_hidden_);

    // Populate ComboBox with initial queries, either from special 'gdv_queries' table
    // Or just set 'Default' query it gdv_queries is not available
    [](QSqlDatabase &db, QComboBox &cb) {
        cb.clear();

        // Look for table that provides user selectable queries
        if (db.tables().contains("gdv_queries")) {
            QSqlQuery query("SELECT name,query FROM gdv_queries");
            while (query.next()) {
                cb.addItem(query.value(0).toString(), query.value(1).toString());
            }
        } else {
            QString table = QInputDialog::getItem(cb.parentWidget(), "Choose initial table", "Tables:", db.tables(),0,false);
            if (table.isEmpty()) table = db.tables().first();
            cb.addItem("Default", QString("SELECT * FROM ") + table);
        }
    }(db, *ui->comboBox);

    // Populate TreeView with data representing SQL tables and their fields.  Add in
    // information from SQL PRAGMA table 'table_info' to show field types and primary
    // key information.
    [this](QSqlDatabase &db, QTreeWidget &tw) {
        tw.clear();

        for (const auto &table: db.tables(QSql::AllTables)) {
            QTreeWidgetItem *parent = new QTreeWidgetItem;
            parent->setText(0, table);

            QSqlRecord record = db.record(table);
            for (int index = 0; index < record.count();  ++index) {
                QTreeWidgetItem *child = new QTreeWidgetItem;
                QString fieldName = record.fieldName(index);
                for (int col = 0; col < g_column_properties.size(); ++col) {
                    child->setText(col, GetPragmaTableInfo(table, fieldName, g_column_properties[col]).toString());
                }
                parent->addChild(child);
            }
            tw.addTopLevelItem(parent);
        }
    }(db, *ui->treeWidget);
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

    query.exec("create table gdv_queries "
               "(id integer primary key, "
               "name varchar(20), "
               "query varchar(40)) ");
    query.prepare("INSERT INTO gdv_queries (id, name, query) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(1);
    query.addBindValue("Default");
    query.addBindValue("SELECT * FROM person ORDER BY id");
    query.exec();

    query.prepare("INSERT INTO gdv_queries (id, name, query) "
                  "VALUES (?, ?, ?)");
    query.addBindValue(2);
    query.addBindValue("Alternate");
    query.addBindValue("SELECT * FROM person ORDER BY age");
    query.exec();
}
