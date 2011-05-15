/*///////////////////////////////////////////////////////////////////////////////////////////////////
  copy from http://developer.qt.nokia.com/wiki/QSqlRelationalDelegate_subclass_that_works_with_QSqlRelationalTableModel
  retrieved 2011-04-03
  license Creative Commons Attribution-ShareAlike 2.5 Generic

      QSqlRelationalDelegate subclass that works with QSqlRelationalTableModel

      When you are using a QSqlRelationalTableModel in combination with QSortFilterProxyModel, you loose the automatic combobox that is displayed in a QTableView.
      This is a subclass of QSqlRelationalDelegate, that works
////////////////////////////////////////////////////////////*/



#ifndef MYSQLRELATIONALDELEGATE_H
#define MYSQLRELATIONALDELEGATE_H

#include <QSqlRelationalDelegate>

class mySqlRelationalDelegate : public QSqlRelationalDelegate
{
    Q_OBJECT
public:
    explicit mySqlRelationalDelegate(QObject *parent = 0);

    QWidget *createEditor(QWidget *aParent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;

signals:

public slots:

};

#endif // MYSQLRELATIONALDELEGATE_H
