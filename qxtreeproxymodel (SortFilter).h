#ifndef QXTREEPROXYMODEL_H
#define QSORTFILTERPROXYMODEL_H

#include <QAbstractProxyModel>
#include <QVector>

class QXTreeProxyModel : public QAbstractProxyModel{
   Q_OBJECT
public:
   QXTreeProxyModel(QObject* parent = 0);
   ~QXTreeProxyModel();
   void setSourceModel(QAbstractItemModel *sourceModel);
   QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
   QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
   QItemSelection mapSelectionToSource(const QItemSelection &proxySelection) const;
   QItemSelection mapSelectionFromSource(const QItemSelection &sourceSelection) const;

public slots:
    void invalidate();

public:
   inline QObject *parent() const { return QObject::parent(); }
   QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
   QModelIndex parent(const QModelIndex &child) const;

   int rowCount(const QModelIndex &parent = QModelIndex()) const;
   int columnCount(const QModelIndex &parent = QModelIndex()) const;
   bool hasChildren(const QModelIndex &parent = QModelIndex()) const;

   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
   bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

   QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
   bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole);

   QMimeData *mimeData(const QModelIndexList &indexes) const;
   bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

   bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
   bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex());
   bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
   bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex());
   void fetchMore(const QModelIndex &parent);
   bool canFetchMore(const QModelIndex &parent) const;
   Qt::ItemFlags flags(const QModelIndex &index) const;
   QModelIndex buddy(const QModelIndex &index) const;
   QModelIndexList match(const QModelIndex &start, int role,
                          const QVariant &value, int hits = 1,
                          Qt::MatchFlags flags =
                          Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;
   QSize span(const QModelIndex &index) const;

   QStringList mimeTypes() const;
   Qt::DropActions supportedDropActions() const;
private:
   Q_DISABLE_COPY(QXTreeProxyModel)
private slots:
   void _q_sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right);
   void _q_sourceHeaderDataChanged(Qt::Orientation orientation, int start, int end);
   void _q_sourceAboutToBeReset();
   void _q_sourceReset();
   void _q_sourceLayoutAboutToBeChanged();
   void _q_sourceLayoutChanged();
   void _q_sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
   void _q_sourceRowsInserted(const QModelIndex &source_parent, int start, int end);
   void _q_sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
   void _q_sourceRowsRemoved(const QModelIndex &source_parent, int start, int end);
   void _q_sourceColumnsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
   void _q_sourceColumnsInserted(const QModelIndex &source_parent, int start, int end);
   void _q_sourceColumnsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
   void _q_sourceColumnsRemoved(const QModelIndex &source_parent, int start, int end);
public:
    struct Mapping {
        QVector<int> source_rows;
        QVector<int> source_columns;
        QVector<int> proxy_rows;
        QVector<int> proxy_columns;
        QVector<QModelIndex> mapped_children;
        QHash<QModelIndex, Mapping *>::const_iterator map_iter;
    };

    mutable QHash<QModelIndex, Mapping*> source_index_mapping;
   void clear_mapping();
   typedef QList<QPair<QModelIndex, QPersistentModelIndex> > QModelIndexPairList;
   QModelIndexPairList store_persistent_indexes();
   QModelIndexPairList saved_persistent_indexes;
   void update_persistent_indexes(const QModelIndexPairList &source_indexes);
   QModelIndex proxy_to_source(const QModelIndex &proxy_index) const;
   QModelIndex source_to_proxy(const QModelIndex &source_index) const;
};


#endif // QSORTFILTERPROXYMODEL_H
