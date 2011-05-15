#ifndef QXTREEPROXYMODEL_H
#define QXTREEPROXYMODEL_H

class QSortFilterProxyModel;
#include <QAbstractProxyModel>
#include <QVector>

class QXTreeProxyModel : public QAbstractProxyModel{
   Q_OBJECT
   Q_PROPERTY(int idCol READ idCol WRITE setIdCol)
   Q_PROPERTY(int parentCol READ parentCol WRITE setParentCol)
   struct EXDatabase{
      EXDatabase(QLatin1String _msg = QLatin1String(""), qint32 _id = 0): msg(_msg), id(_id){};
      QString msg;
      qint32 id;};
public:
   QXTreeProxyModel(QObject* parent = 0);
   ~QXTreeProxyModel();
   int idCol() const;
   bool setIdCol(unsigned int col);
   int parentCol() const;
   bool setParentCol(unsigned int col);
   /*!
     \brief defines default values for newly added records (i.e., rows)

     The provided parameter is a QList of values to be used as default values in newly
     added records. Item 0 is placed into record field 0, item 1 in field 1 and so on. If the list
     is too short, then remaining fields remain uninitialized, equally those items that contain
     an invalid QVariant (i.e., QVariant()). A too long list does not harm, extra list items are
     silently ignored.

     If using QSqlRelationalTableModel as undrlying model:
     all foreign key fields must be initialized to a valid entry (that is present in the related
     table; this is a requirement steming from QSqlRelationalTableModel.

     If the item corresponding to the id field is not an invalid QVariant, then the provided value
     will be used to initialize the id field; as is the cas for all other field. This has implications:
     only 1 row may be inserted per call to insertRows(), and between calls to insertRows() a new id
     value needs to be set by setDefaultValue. That only 1 row may be inserted at a time has further
     consequences: drag and drop functionality is limited. Thus, this is not a recommended method to
     provide unique keys for most applications; consequently, the item corresponding to the id field
     will most often conatin QVarient().
   */
   void setDefaultValues(QList<QVariant> newDefaultValues) {d_defaultValues = newDefaultValues;}
   /* lazy model not needed, as lazyness is inherited from underlying source model
   void fetchMore(const QModelIndex &parent);
   bool canFetchMore(const QModelIndex &parent) const; */
   QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
   QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
   QModelIndex index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const;
   bool hasChildren(const QModelIndex& parent = QModelIndex()) const;
   int rowCount(const QModelIndex& parent = QModelIndex()) const;
   int columnCount(const QModelIndex& parent = QModelIndex()) const;
   QModelIndex parent(const QModelIndex& child) const;
   void setSourceModel(QAbstractItemModel* sourceModel);
   bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex());
   bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex());
   bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
   bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
   /* inherited data access functionality is sufficient
   bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole); inherited functionality is sufficient */
   // drag and drop support
   Qt::ItemFlags flags(const QModelIndex &index) const;
   QStringList mimeTypes() const;
   QMimeData *mimeData(const QModelIndexList &indexes) const;
   bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
   Qt::DropActions supportedDropActions() const;
   /* other inherited virtual functions, for which I see no need to re-implement
   QModelIndex buddy(const QModelIndex &index) const;
   QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith|Qt::MatchWrap)) const;
   QSize span(const QModelIndex &index) const;
   public slots: bool submit();
   public slots: void revert(); */
private:
   Q_DISABLE_COPY(QXTreeProxyModel)
   qint32 lastInsertedId;
   int idColumn;
   int parentColumn;
   QModelIndex sourceindexFromId(qint32 id) const;
   QModelIndexList sourcechildrenFromId(qint32 id) const;
   qint32 getId(const QModelIndex& idx) const;
   void removeChildRows(qint32 parentId);
   int rowFromId(qint32 recordId, qint32 parentId) const;
   bool moveBranch(qint32 id, qint32 newParent);
   bool copyBranch(qint32 id, qint32 newParent);
   QList<QVariant> d_defaultValues;
   bool isSourceDeleted(QModelIndex sourceIndex) const;
   qint32 nextFreeId() const;
private slots:
   void sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right);
   void sourceHeaderDataChanged(Qt::Orientation orientation, int start, int end);
   void sourceReset();
   void sourceLayoutAboutToBeChanged();
   void sourceLayoutChanged();
   void sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
   void sourceRowsInserted(const QModelIndex &source_parent, int start, int end);
   void sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
   void sourceRowsRemoved(const QModelIndex &source_parent, int start, int end);
   void sourceColumnsAboutToBeInserted(const QModelIndex &source_parent, int start, int end);
   void sourceColumnsInserted(const QModelIndex &source_parent, int start, int end);
   void sourceColumnsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end);
   void sourceColumnsRemoved(const QModelIndex &source_parent, int start, int end);
};


#endif // QSORTFILTERPROXYMODEL_H
