#include "qxtreeproxymodel.h"
#include <QAbstractTableModel>

#include <qdebug.h>


QXTreeProxyModel::QXTreeProxyModel(QObject *parent) : QSortFilterProxyModel(parent), d_idCol(0), d_parentCol(1) {}

QXTreeProxyModel::~QXTreeProxyModel(){}

bool QXTreeProxyModel::setIdCol(int col){
   if (col == d_parentCol) return false;
   d_idCol = col;
   return true;}

bool QXTreeProxyModel::setParentCol(int col){
   if (col == d_idCol) return false;
   d_parentCol = col;
   return true;}

void QXTreeProxyModel::setSourceModel(QAbstractItemModel *sourceModel){
   Q_ASSERT(qobject_cast<QAbstractTableModel*>(sourceModel));
   QSortFilterProxyModel::setSourceModel(sourceModel);}

QModelIndex QXTreeProxyModel::mapToSource(const QModelIndex &proxyIndex) const{
   int sourceRow(proxyIndex.internalId());
//   QModelIndex sourceIndex(sourceModel()->index(sourceRow, proxyIndex.column(), QModelIndex()));
   QModelIndex sourceIndex(QSortFilterProxyModel::mapToSource(proxyIndex));
   return sourceIndex;}

QModelIndex QXTreeProxyModel::mapFromSource(const QModelIndex &sourceIndex) const{
//   QModelIndex proxyIndex(index(sourceIndex.row(), sourceIndex.column(), QModelIndex()));
   QModelIndex proxyIndex(QSortFilterProxyModel::mapFromSource(sourceIndex));
   return proxyIndex;}

QModelIndex QXTreeProxyModel::index(int row, int column, const QModelIndex &parent) const{
//   return createIndex(row, column, 0);
   QModelIndex newIndex(QSortFilterProxyModel::index(row, column, parent));
   qDebug()<< newIndex.row() << newIndex.column() << newIndex.internalId() << newIndex.internalPointer();
   return newIndex;
}

bool QXTreeProxyModel::hasChildren(const QModelIndex &parent) const{
   // for improved performance: not count children
   return (rowCount(parent) > 0);}

int QXTreeProxyModel::rowCount(const QModelIndex &parent) const{
   return QSortFilterProxyModel::rowCount(parent);}

QModelIndex QXTreeProxyModel::parent(const QModelIndex &child) const{
   return QSortFilterProxyModel::parent(child);}

