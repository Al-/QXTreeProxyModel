/*
developed by Andreas D. Christ, software(at)quantentunnel.de
using Qt Creator and Qt SDK with non-commercial license, see http://qt.nokia.com
license: GPL or any other license compatible with Qt's license (users choice)
*/

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>

#include "testdialog.h"
#include "qxtreeproxymodel.h"

TestDialog::TestDialog(QWidget *parent) : QDialog(parent){
   setupUi(this);
   QSqlDatabase db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"));
   db.setDatabaseName(QLatin1String(":memory:"));
   Q_ASSERT(db.open());
   QSqlQuery sqlQuery(db);
   Q_ASSERT(sqlQuery.exec(QLatin1String("CREATE TABLE Table1 (ID INTEGER PRIMARY KEY, Parent INTEGER NOT NULL, Content, Details);")));
   Q_ASSERT(sqlQuery.exec(QLatin1String("INSERT INTO Table1 VALUES (1, 0, 'first item', 'Details for first item');")));
   Q_ASSERT(sqlQuery.exec(QLatin1String("INSERT INTO Table1 VALUES (2, 1, 'second item', 'Details for second item');")));
   Q_ASSERT(sqlQuery.exec(QLatin1String("INSERT INTO Table1 VALUES (3, 1, 'third item', 'Details for 3rd item');")));
   Q_ASSERT(sqlQuery.exec(QLatin1String("INSERT INTO Table1 VALUES (4, 0, 'fourth item', 'Details for 4th item');")));



   QSqlTableModel* tableModel = new QSqlTableModel(this, db);
   tableModel->setTable(QLatin1String("Table1"));
   tableModel->select();
   tableView->setModel(tableModel);
   tableView->resizeColumnsToContents();

   QXTreeProxyModel* treeModel = new QXTreeProxyModel(this);
   treeModel->setSourceModel(tableModel);
   treeView->setModel(treeModel);
}
