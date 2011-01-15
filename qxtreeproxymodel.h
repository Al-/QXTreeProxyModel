#ifndef QXTREEPROXYMODEL_H
#define QXTREEPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QVector>

class QXTreeProxyModel : public QSortFilterProxyModel{
   Q_OBJECT
public:
   QXTreeProxyModel(QObject* parent = 0);
   ~QXTreeProxyModel();
   int idCol() const {return d_idCol;}
   int parentCol() const {return d_parentCol;}
   QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
   QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
   QModelIndex index(int row, int column, const QModelIndex& parent) const;
   bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
   int rowCount(const QModelIndex &parent = QModelIndex()) const;
   QModelIndex parent(const QModelIndex &child) const;
   void setSourceModel(QAbstractItemModel* sourceModel);
   /*   QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
   int columnCount(const QModelIndex &parent = QModelIndex()) const;
   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
   bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
   QMimeData *mimeData(const QModelIndexList &indexes) const;
   bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
   bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
   bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex());
   bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
   bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex());
   void fetchMore(const QModelIndex &parent);
   bool canFetchMore(const QModelIndex &parent) const;
   Qt::ItemFlags flags(const QModelIndex &index) const;
//   QModelIndex buddy(const QModelIndex &index) const;
//   QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;
//   QSize span(const QModelIndex &index) const;
//   QStringList mimeTypes() const;
//   Qt::DropActions supportedDropActions() const;
   */
public slots:
   bool setParentCol(int col);
   bool setIdCol(int col);
private:
   Q_DISABLE_COPY(QXTreeProxyModel)
   int d_idCol;
   int d_parentCol;
};


#endif // QSORTFILTERPROXYMODEL_H
