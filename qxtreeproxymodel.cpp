#include "qxtreeproxymodel.h"
#include <QAbstractTableModel>
#include <QSqlRelationalTableModel>
#include <QSqlTableModel>
#include <qdebug.h>
#include <boost/cast.hpp>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QMimeData>
#include <QApplication>
#include <QTimer>
#include <limits>
#include <QFont>


/*!
  \class QXTreeProxyModel
  \brief QXTreeProxyModel is a proxy model that takes as input a table model and outputs a tree model

  License: LGPL
  This software is written using the non-commercial LGLP version of Qt. There are no additional restrictions
  to the use of this software than those that are mandated by the underlying Qt license. See http://qt.nokia.com

  QXTreeProxyModel inherits QAbstractProxyModel.

  The underlying sourceModel must satisfy the following criteria
  - needs to have an id column and a parent column (and optional more columns)
  - id column needs to be unique, in sqlite PRIMARY KEY
  - id column and parent column need to be integers (or strings convertible to integer)
  - id column must not be 0 (nor "0" or anything else that would be converted to zero)
  - parent column may be empty (which is equivalent to zero) or refer to a valid parent, i.e., have a number that is the id of another record
  - records must not be circularly connected through their id and parent columns

  Errors attributable to a database not fulfilling the above criteria may be ignored by QXTreeProxyModel, some lead to an exception,
  some to an Q_ASSERT failure (in debug mode); and unluckily some might lead to a crash (please report bugs!). The basic idea is
  that errors in the database structure lead to Q_ASSERT failures as these can be prevented by the programmer; errors
  due to content of the database should lead to exceptions as these errors could be introduced by the end user, thus, the
  programmer should get a chance to catch them.

  The source model, the id column index and the parent column index need to be set (see setter functions for detailed
  requirements). The tree is built such that the value (e.g., 123) in the parent field of row A determines
  which is the parent row of that row A: it is the row with that id (i.e., 123). '0' as parentCol value defines
  the first level rows (i.e., children of the invisible root item).

  The proxy model supports drag and drop, insertion and deletion of rows, insertion (always appends)and
  deletions of columns (limited to columns with field index greater than idCol and than parentCol).

  Uncommitted deletions of records are displayed using striked out font (re-implement data() if this is not desired).

  Due to limitations in QSqlRelationalDelegate, a derived version of that class needs to be used in conjunction with
  QXTreeProxyModel (see mysqlrelationaldelegate.cpp and mysqlrelationaldelegate.h,
  based on http://developer.qt.nokia.com/wiki/QSqlRelationalDelegate_subclass_that_works_with_QSqlRelationalTableModel)
  */

const char treeproxymime[] = "application/x-qxtreeproxymodeldatalist";

/*!
  \brief constructor

  The parameter parent is forwarded to QAbstractProxyModel from which this class is derived.
*/
QXTreeProxyModel::QXTreeProxyModel(QObject *parent) : QAbstractProxyModel(parent), lastInsertedId(0), idColumn(-1), parentColumn(-1) {
   }

/*!
  \brief destructor

  Frees resources (none needed to be freed from QXTreeproxyModel itself, possibly from inherited classes)
*/
QXTreeProxyModel::~QXTreeProxyModel(){}

// getters and setters

/*!
  \property QXTreeProxyModel::idCol
  \brief index of column that holds unique key for each record (i.e., each row)

  This property holds the index of column (i.e., database field) with the unique key. This property
  must be set for the proxy model to work. And it must refer to a column that holds a unique key
  for every record. This key must be an integer (qint32, to be accurate) and it must not be 0. For
  SQLite databases this field must be the PRIMARY KEY, optionally AUTOINCREMENT
  There are several possibilities how to ensure that newly added records get a unique value assigned.
  Which is used depends primarily on the input source model (sql or not) and on the EditStrategy.
  - for sql models: connect to signal primeInsert and provide a suitable unique key
  - for sql models: use AUTOINCREMENT (does NOT work for OnManualSubmit strategy: the unique key is only
    generated once the record is submitted, but new child records would need to knoe that key prior to that)
  - use default values, see setDefaultValues: this works only for single inserts and for single row drag and drop
  - the source model could provide unique keys in the insertRows function (needs a custom made derived model)
  - let QXTreeProxyModel handle it: this leads to incrementing values; does NOT work with AUTOINCREMENT of database
*/
/*!
  \fn int QXTreeProxyModel::idCol() const
  \brief getter function

  \sa setDefaultValues(QList<QVariant>) idCol
*/
int QXTreeProxyModel::idCol() const {
   return idColumn;}

/*!
  \brief setter function

  \sa idCol
*/
bool QXTreeProxyModel::setIdCol(unsigned int col){
   int iCol(boost::numeric_cast<int>(col));
   idColumn = iCol;
   return true;}

/*!
  \property QXTreeProxyModel::parentCol
  \brief index of column that refers to parent of each record (i.e., each row)
*/

/*!
  \brief getter function

  \sa parentCol
*/
int QXTreeProxyModel::parentCol() const {
   return parentColumn;}

/*!
  \brief setter function

  \sa parentCol
*/
bool QXTreeProxyModel::setParentCol(unsigned int col){
   int pCol(boost::numeric_cast<int>(col));
   parentColumn = pCol;
   return true;}

/*!
  \brief setter function for sourceModel (reimplemented)

  Any QAbstractItemModel derived model is acceptable, as long as it provides a flat table-like
  structure (i.e., the parent of each item is an invalid QModelIndex). The class was tested using
  QSqlTableModel, QSqlRelationalTableModel and QStandardItemModel.
*/
void QXTreeProxyModel::setSourceModel(QAbstractItemModel* newSourceModel){
   emit beginResetModel();
   if (sourceModel()) {
      bool ok;
      ok = disconnect(sourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)), this, SLOT(sourceHeaderDataChanged(Qt::Orientation,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(sourceColumnsAboutToBeInserted(QModelIndex,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(columnsInserted(QModelIndex,int,int)), this, SLOT(sourceColumnsInserted(QModelIndex,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(columnsRemoved(QModelIndex,int,int)), this, SLOT(sourceColumnsRemoved(QModelIndex,int,int)));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(layoutAboutToBeChanged()), this, SLOT(sourceLayoutAboutToBeChanged()));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(layoutChanged()), this, SLOT(sourceLayoutChanged()));
      Q_ASSERT(ok);
      ok = disconnect(sourceModel(), SIGNAL(modelReset()), this, SLOT(_q_sourceReset()));
      Q_ASSERT(ok);}
   QAbstractProxyModel::setSourceModel(newSourceModel);
   qDebug() << "model addresses: QXTreeProxyModel =" << this << ", sourceModel =" << QAbstractProxyModel::sourceModel();
   bool ok = connect(sourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(headerDataChanged(Qt::Orientation,int,int)), this, SLOT(sourceHeaderDataChanged(Qt::Orientation,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(rowsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(sourceRowsAboutToBeInserted(QModelIndex,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)), this, SLOT(sourceColumnsAboutToBeInserted(QModelIndex,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(columnsInserted(QModelIndex,int,int)), this, SLOT(sourceColumnsInserted(QModelIndex,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(sourceRowsAboutToBeRemoved(QModelIndex,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(sourceColumnsAboutToBeRemoved(QModelIndex,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(columnsRemoved(QModelIndex,int,int)), this, SLOT(sourceColumnsRemoved(QModelIndex,int,int)));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(layoutAboutToBeChanged()), this, SLOT(sourceLayoutAboutToBeChanged()));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(layoutChanged()), this, SLOT(sourceLayoutChanged()));
   Q_ASSERT(ok);
   ok = connect(sourceModel(), SIGNAL(modelReset()), this, SLOT(sourceReset()));
   Q_ASSERT(ok);
   //reset();
   emit endResetModel();}

// reimplemented virtual functions (basic set)
/*!
  \brief reimplemented function
*/
QModelIndex QXTreeProxyModel::mapToSource(const QModelIndex& proxyIndex) const{
   Q_ASSERT(sourceModel());
   if (!proxyIndex.isValid()) return QModelIndex();
   qint32 recordId = getId(proxyIndex);
   Q_ASSERT(recordId != 0);
   QModelIndex idx = sourceindexFromId(recordId);
   bool ok(true);
   Q_ASSERT(sourceModel()->data(idx, Qt::DisplayRole).toInt(&ok) == recordId && ok);
   QModelIndex sourceIndex = idx.sibling(idx.row(), proxyIndex.column());
   // qDebug() << "mapToSource" << proxyIndex << "finds" << sourceIndex << "with id" << sourceModel()->data(sourceModel()->sibling(sourceIndex.row(), idCol(), sourceIndex), Qt::DisplayRole).toInt();
   return sourceIndex;}

/*!
  \brief reimplemented function
*/
QModelIndex QXTreeProxyModel::mapFromSource(const QModelIndex& sourceIndex) const{
   Q_ASSERT(sourceModel());
   Q_ASSERT(sourceIndex.isValid());
   // qDebug() << "mapFromSource" << sourceIndex;
   QModelIndex proxyIndex;
   if (sourceIndex.row() < 0) exit(99); //proxyIndex = QModelIndex();
   else {
      QModelIndex idx = sourceModel()->index(sourceIndex.row(), idCol());
      Q_ASSERT(idx.isValid());
      QVariant recordIdVariant = sourceModel()->data(idx, Qt::DisplayRole);
      idx = sourceModel()->index(sourceIndex.row(), parentCol());
      Q_ASSERT(idx.isValid());
      if (/*recordParentVariant.toInt() != std::numeric_limits<qint32>::min() && */ recordIdVariant.isValid()){
         bool ok;
         qint32 recordId = recordIdVariant.toInt(&ok);
         if (!ok) throw EXDatabase(QLatin1String("no int value in id column"), 0);
         Q_ASSERT_X(ok, "sourceModel()->data conversion to int", qPrintable(sourceModel()->data(idx, Qt::DisplayRole).toString()));
         Q_ASSERT(recordId != 0);
         QVariant parentIdVariant = sourceModel()->data(sourceModel()->index(sourceIndex.row(), parentCol()), Qt::DisplayRole);
         qint32 parentId;
         if (parentIdVariant.isNull()) parentId = 0;     // not yet set??
         else parentId = parentIdVariant.toInt(&ok);
         Q_ASSERT_X(ok, QString::number(recordId).toLocal8Bit(), sourceModel()->data(sourceModel()->index(sourceIndex.row(), parentCol()), Qt::DisplayRole).toByteArray());
         if (!ok) throw EXDatabase(QLatin1String("no int value in parent column"), recordId);
         int rowNumber = rowFromId(recordId, parentId);
         // find parent index of proxy index
         QModelIndex parentIndex;
         if (parentId == 0) parentIndex = QModelIndex();
         else {     // need to recursively find parent index, as not yet 0
            // qDebug() << "   entering recursion to find index with id" << parentId;
            QModelIndex idx = sourceindexFromId(parentId);
//            if (d_idFilterModel->rowCount() == 0) parentIndex = QModelIndex();    // id of requested parent not found -> assign to root
//            else {
            QModelIndex sourceParentIndex = idx.sibling(idx.row(), 0);      // parent is always first column
            parentIndex = mapFromSource(sourceParentIndex);
            Q_ASSERT_X(getId(parentIndex) == parentId, "wrong parentIndex found", qPrintable(QString::number(parentId)));}
         proxyIndex = index(rowNumber, sourceIndex.column(), parentIndex);}
      else proxyIndex = QModelIndex();}    // "none of my business" as id field of source index row is empty; most likely record not yet fully constructed
   return proxyIndex;}

/*!
  \brief reimplemented function

  This function \bold only re-implements the role == Qt::FontRole to strike out uncommitted record deletions.
  To change this behaviour re-implement this function for role == Qt::FontRole and return
  QAbstractProxyModel::data(proxyIndex, role) for all other roles.
*/
QVariant QXTreeProxyModel::data(const QModelIndex& proxyIndex, int role) const{
   QVariant result = QAbstractProxyModel::data(proxyIndex, role);
   if (role == Qt::FontRole){ // draw deleted (but not yet submitted) rows strike-through
      QModelIndex sourceIndex = mapToSource(proxyIndex);
      if (isSourceDeleted(sourceIndex)){
         QFont myFont;
         // qDebug() << proxyIndex << "is deleted";
         if (result.isNull()) myFont = QFont();    // redundant
         else {
            myFont = result.value<QFont>();
            Q_ASSERT(myFont != QFont());}
         myFont.setStrikeOut(true);
         result = myFont;}}
   return result;}

/*!
  \brief reimplemented function
*/
QModelIndex QXTreeProxyModel::index(int row, int column, const QModelIndex& parent) const{
   // qDebug() << "index for" << row << column << parent;
   Q_ASSERT(row >= 0);
   Q_ASSERT(column >= 0);
   Q_ASSERT((parent.model() == NULL && parent.row() == -1 && parent.column() == -1) ||
            (parent.model() != NULL && parent.row() >= 0 && parent.column() >= 0 && qobject_cast<const QXTreeProxyModel*>(parent.model()) != NULL));
   // if (row < 0) return QModelIndex();
   // if (column < 0) return QModelIndex();
   if (parent.column() != 0 && parent.isValid()) return QModelIndex();
   QModelIndexList idxList = sourcechildrenFromId(getId(parent));
   // qDebug() << "   parent filter" << d_parentFilterModel->filterRegExp().pattern() << "returns" << d_parentFilterModel->rowCount() << "rows";
   // qDebug() << idxList;
   Q_ASSERT_X(idxList.count() > row, "too few children found",
              qPrintable(QString(QLatin1String("expected >%1, found %2 rows when filtering %3 for %4 in column %5"))
                         .arg(row).arg(idxList.count()).arg(sourceModel()->objectName()).arg(getId(parent)).arg(parentCol())));
   QModelIndex sourceParentIndex(idxList.at(row));
   Q_ASSERT(sourceParentIndex.isValid());
   QModelIndex sourceIdIndex = sourceParentIndex.sibling(sourceParentIndex.row(), idCol());
   Q_ASSERT(sourceIdIndex.isValid());
   bool ok;
   int recordId = sourceModel()->data(sourceIdIndex, Qt::DisplayRole).toInt(&ok);
   Q_ASSERT_X(ok, qPrintable(QString(QLatin1String("id in sourceModel with row = %1, col = %2")).arg(row).arg(idCol())),
                  qPrintable(sourceModel()->data(idxList.at(row).sibling(row, idCol()), Qt::DisplayRole).toString()));
   Q_ASSERT(recordId != 0);
   QModelIndex newIndex = createIndex(row, column, recordId);
   // qDebug() << "index for" << row << column << parent << "is" << newIndex;
   return newIndex;}

/*!
  \brief reimplemented function
*/
bool QXTreeProxyModel::hasChildren(const QModelIndex &parent) const{
   // to improve performance: do not count children
   // qDebug() << "hasChildren" << parent;
   return (rowCount(parent) > 0);}

/*!
  \brief reimplemented function
*/
int QXTreeProxyModel::rowCount(const QModelIndex& parent) const{
   // qDebug() << "rowCount" << parent;
   int rows;
   if (parent.isValid() && parent.column() != 0) rows = 0;   // AQP: only first column is parent in tree model
//   if (parent.column() != 0) rows = 0;    first asks for child count of root item, i.e., children of an invalid model index
   else {
      QModelIndexList idxList = sourcechildrenFromId(getId(parent));
      rows = idxList.count();}
   // qDebug() << "   rowCount for " << parent << "with id" << getId(parent) << "returns" << rows;
   return rows;}

/*!
  \brief reimplemented function
*/
int QXTreeProxyModel::columnCount(const QModelIndex& parent) const {
   Q_UNUSED(parent);
   if (!sourceModel()) return 0;
   /* quote from documentation 4.7.1
      Note: When implementing a table based model, columnCount() should return 0 when the parent is valid.*/
   // although in theory possible, all columnCounts need to be identical, as per note above
   //QModelIndex sourceIndex = mapToSource(parent);
   int n = sourceModel()->columnCount(/*sourceIndex*/ QModelIndex());
   return n;}

/*!
  \brief reimplemented function
*/
QModelIndex QXTreeProxyModel::parent(const QModelIndex& child) const{
   Q_ASSERT(sourceModel());
   Q_ASSERT(child.isValid());
   // qDebug() << "parent() for parameter" << child;
   qint32 childId = getId(child);
   // if(child.row() == -1 && child.column() == -1 && child.internalId() == 0 && child.model() == NULL) return QModelIndex();
   Q_ASSERT_X(childId != 0, "getId returned 0 for index",
              qPrintable(QString(QLatin1String("row %1, column %2, internalId %3, model address %4"))
                                       .arg(child.row()).arg(child.column()).arg(child.internalId()).arg((qlonglong)(void*)child.model())));
   QModelIndex idx = sourceindexFromId(childId);
   bool ok;
   qint32 parentId = sourceModel()->data(idx.sibling(idx.row(), parentCol()), Qt::DisplayRole).toInt(&ok);
   // if (!ok) parentId = 0;
   Q_ASSERT_X(ok, "illegal parent", sourceModel()->data(idx, Qt::DisplayRole).toByteArray());
   if (!ok) {
      EXDatabase exception;
      exception.msg = QLatin1String("illegal entry in parent field");
      exception.id = getId(child);
      throw exception;}
   if (parentId == 0) return QModelIndex();
   idx = sourceindexFromId(parentId);
   QModelIndex sourceIndex = idx.sibling(idx.row(), 0); //AQP: all rows are child of parent's 1st column
   Q_ASSERT_X(sourceModel()->data(sourceModel()->index(sourceIndex.row(), idCol())).toInt(&ok) == parentId && ok, "id at source",
                           qPrintable(sourceModel()->data(sourceModel()->index(sourceIndex.row(), idCol())).toString()));
   QModelIndex proxyIndex = mapFromSource(sourceIndex);
   // qDebug() << "   parent() for parameter" << child << "with id" << getId(child) << "is" << sourceIndex << "," << proxyIndex << "and has id" << parentId;
   Q_ASSERT_X(proxyIndex.internalId() == parentId, qPrintable(QString::number(parentId)), qPrintable(QString::number(proxyIndex.internalId())));
   return proxyIndex;}

/*!
  \brief reimplemented function
*/
Qt::ItemFlags QXTreeProxyModel::flags(const QModelIndex& index) const {
   Q_ASSERT(sourceModel());
   Qt::ItemFlags result = QAbstractProxyModel::flags(index);
   // qDebug() << "preset flags for" << index << "=" << result;
   if (index.isValid()) result |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
   if (index.column() == 0) result |= Qt::ItemIsDragEnabled;
   Qt::ItemFlags sourceFlags = sourceModel()->flags(mapToSource(index));
   if (sourceFlags.testFlag(Qt::ItemIsEditable) && index.column() != -1) result |= Qt::ItemIsEditable;
   // qDebug() << "   source flags for" << index << "=" << result;
   // if (index.column() == idCol() || index.column() == parentCol()) result &= ~Qt::ItemIsEditable;
   result |= Qt::ItemIsDropEnabled;  // even invalid index = empty space, accepts dropped items
   // qDebug() << "   final flags for" << index << "=" << result;
   // BUG: only_toplevelitems_are_selectable when QTreeView has rowselection behaviour
   return result;}

// Drag and drop functionality
/*!
  \brief reimplemented function
*/
QStringList QXTreeProxyModel::mimeTypes() const{
   QStringList validMimeTypes;
   validMimeTypes << QLatin1String(treeproxymime);
   return validMimeTypes;}

/*!
  \brief reimplemented function
*/
QMimeData* QXTreeProxyModel::mimeData(const QModelIndexList &indexes) const{
   Q_ASSERT(!indexes.isEmpty());
   Q_ASSERT(indexes.at(0).isValid());
   // qDebug() << "indexlist count" << indexes.count();
   // foreach(QModelIndex idx, indexes) qDebug() << "   " << idx << getId(idx) << "of parent" << parent(idx);
   qint32 firstParentId = getId(parent(indexes.at(0)));
   QSet<qint32> idList;    // QSet to eliminate duplicates
   foreach(QModelIndex idx, indexes) if (getId(parent(idx)) == firstParentId) idList << getId(idx);   //list of all rows, excluding child rows
   // qDebug() << "idList count" << idList.count();
   // foreach(qint32 id, idList) qDebug() << "   encoded" << id;
   QMimeData *mimeData = new QMimeData();
   QByteArray encodedData;
   QDataStream stream(&encodedData, QIODevice::WriteOnly);
   foreach(qint32 id, idList) stream << id;
   // qDebug() << "encoded data" << encodedData.toHex();
   mimeData->setData(QLatin1String(treeproxymime), encodedData);
   return mimeData;}

/*!
  \brief reimplemented function
*/
bool QXTreeProxyModel::dropMimeData(const QMimeData *mimedata, Qt::DropAction action, int row, int column, const QModelIndex &newParent){
   Q_UNUSED(column);
   Q_UNUSED(row);
   if (action == Qt::IgnoreAction) return true;
   if (action != Qt::CopyAction && action != Qt::MoveAction) return false;
   if (isSourceDeleted(mapToSource(newParent))) return false;
   // qDebug() << "dropping data" << (action == Qt::CopyAction ? "Qt::CopyAction" : "Qt::MoveAction");
   if (!mimedata->hasFormat(QLatin1String(treeproxymime))) return false;
   QByteArray encodedData = mimedata->data(QLatin1String(treeproxymime));
   // qDebug() << "data to decode" << encodedData.toHex();
   QDataStream stream(&encodedData, QIODevice::ReadOnly);
   QList<qint32> idList;
   while (!stream.atEnd()){
      qint32 id;
      stream >> id;
      idList.append(id);}
   qSort(idList);
   // qDebug() << "   rows to drop:" << idList.count();
   bool ok(true);
   // check that not moved or copied to own child; do this prior to start moving to avoid that some rows are already moved
   for(QList<qint32>::iterator iter= idList.begin(); ok && iter != idList.end(); ++iter){
      QModelIndex sourceIndex = sourceindexFromId(*iter);
      if (isSourceDeleted(sourceIndex)) *iter = 0;      //id 0 for deleted rows
      QModelIndex proxyIndex = mapFromSource(sourceIndex);
      Q_ASSERT(proxyIndex.isValid());
      QModelIndex parentLine(newParent);           // do not drop on own child
      while (ok && parentLine.isValid()){
         ok = (parentLine != proxyIndex);
         parentLine = parent(parentLine);}}
   if (!ok) return false;
   foreach(qint32 id, idList) if (id != 0 && ok) {
      switch (action){
      case Qt::MoveAction:
         ok = moveBranch(id, getId(newParent));
         Q_ASSERT(ok);
         break;
      case Qt::CopyAction:
        ok = copyBranch(id, getId(newParent));
        Q_ASSERT(ok);
         break;
      default:
         Q_ASSERT(false);}}
   return ok;}

/*!
  \brief reimplemented function
*/
Qt::DropActions QXTreeProxyModel::supportedDropActions() const{
   return Qt::CopyAction | Qt::MoveAction;}

bool QXTreeProxyModel::moveBranch(qint32 id, qint32 newParent){
   // no need to move child nodes, as these remain attached to moved item
   QModelIndex idx = sourceindexFromId(id);
   // qDebug() << "moveBranch" << id << "to replace parent by" << newParent;
   // no row move in source model, thus no need to call beginMoveRows(); modification of value in parent column will be communicated by source model
   if (!sourceModel()->setData(idx.sibling(idx.row(), parentCol()), newParent, Qt::EditRole)) return false;   // read-only sourceModel
   return true;}

bool QXTreeProxyModel::copyBranch(qint32 id, qint32 newParent){
   // qDebug() << "copyBranch" << id << "to parent" << newParent;
   if (!insertRow(0, QModelIndex())) return false;       // read-only sourceModel
   QModelIndex sourceIndex = sourceindexFromId(id);
   if (isSourceDeleted(sourceIndex)) return true;  //row is deleted but not yet submitted; needed for recurisve calls
   qint32 newId(lastInsertedId);
   QList<QVariant> dataToCopy;
   for (int c(0); c < sourceModel()->columnCount(QModelIndex()); ++c){
      dataToCopy << sourceModel()->data(sourceIndex.sibling(sourceIndex.row(), c), Qt::EditRole);
      if (dataToCopy.at(c).isValid()){
         if (QSqlRelationalTableModel* relationalModel = qobject_cast<QSqlRelationalTableModel*>(sourceModel())){
            QSqlRelation relation = relationalModel->relation(c);
            if (relation.isValid()){
               QSqlTableModel* relatedTable = relationalModel->relationModel(c);
               int displayField = relatedTable->fieldIndex(relation.displayColumn());
               int indexField = relatedTable->fieldIndex(relation.indexColumn());
               QModelIndexList idxs = relatedTable->match(relatedTable->index(0, displayField), Qt::DisplayRole, dataToCopy.at(c));
               if (idxs.count() == 1){
                  QModelIndex idx = idxs.at(0);
                  Q_ASSERT(idx.isValid());
                  dataToCopy[c] =  relatedTable->data(relatedTable->index(idx.row(), indexField), Qt::DisplayRole);}
               else if (idxs.count() == 0);  // assume that the value is a forgein key that occurs ij not yet commited records
               else Q_ASSERT(false);}}}}
   // qDebug() << dataToCopy;
   Q_ASSERT_X(dataToCopy.at(idCol()).toInt() == id, (dataToCopy.at(idCol()).toString() + QLatin1String(" and ") + QString::number(id)).toLocal8Bit(), "should be identical");
   sourceIndex = sourceindexFromId(newId);
   bool ok(true);
   for (int c(0); c < sourceModel()->columnCount(QModelIndex()); ++c){
      if (c == idCol()) Q_ASSERT(sourceModel()->data(sourceIndex.sibling(sourceIndex.row(), c), Qt::DisplayRole).toInt(&ok) == newId);
      else if (c == parentCol()) ok = sourceModel()->setData(sourceIndex.sibling(sourceIndex.row(), c), newParent, Qt::EditRole);
      else ok = sourceModel()->setData(sourceIndex.sibling(sourceIndex.row(), c), dataToCopy.at(c), Qt::EditRole);
      Q_ASSERT(ok);}
   QModelIndexList childIndices = sourcechildrenFromId(id);
   QSet<qint32> childIds;
   foreach (QModelIndex idx, childIndices) {
      qint32 childId(sourceModel()->data(idx, Qt::DisplayRole).toInt(&ok));
      Q_ASSERT(ok && childId != 0);
      childIds.insert(childId);}
   if (childIndices.count() != childIds.count()){
      EXDatabase exception;
      exception.msg = QLatin1String("children have duplicate id");
      exception.id = id;}
   if (childIds.count() > 0) for (QSet<qint32>::const_iterator iter = childIds.constBegin(); iter != childIds.constEnd() && ok; ++iter){
      ok = copyBranch(*iter, newId);
      Q_ASSERT(ok);}
   return ok;}

// structure maipulation
/*!
  \brief reimplemented function
*/
bool QXTreeProxyModel::removeRows(int row, int count, const QModelIndex& parent){
   Q_ASSERT(sourceModel());
   if (count == 0) return true;
   bool ok;
   QList<qint32> ids;
   for (int i(0); i < count; ++i) {
      QModelIndex proxyIndex = index(row + i, 0, parent);
      qint32 recordId = getId(proxyIndex);
      Q_ASSERT(recordId != 0);
      ids.append(recordId);}
   Q_ASSERT(ids.count() > 0 && ids.count() <= count);
   // qDebug() << "remove" << ids.count() << "sibling rows:" << ids;
//   beginRemoveRows(parent, row, row + count - 1);
   foreach(qint32 recordId, ids){
      QModelIndex sourceIndex = sourceindexFromId(recordId);
      // qDebug() << "removing item" << recordId << "with column(0) =" << sourceModel()->data(sourceIndex.sibling(sourceIndex.row(), 0));
      ok = sourceModel()->removeRow(sourceIndex.row(), QModelIndex());
      if (!ok) {
         EXDatabase exception;
         exception.msg = QLatin1String("can not remove record");
         exception.id = recordId;}
      removeChildRows(recordId);}
//   endRemoveRows();
   return ok;}

/*!
  \brief reimplemented function
*/
bool QXTreeProxyModel::insertRows(int row, int count, const QModelIndex& parent){
   Q_UNUSED(row);
   if (!sourceModel()) return false;
   if (isSourceDeleted(mapToSource(parent))) return false;
   // qDebug() << "insert" << count << "rows" << "to parent" << parent;
   if (count == 0) return true;
   qint32 parentId = getId(parent);
   for (int i(0); i < count; ++i){
      bool ok = sourceModel()->insertRow(0, QModelIndex());
      if (!ok) return false;  // likely a read-only sourceModel
      QModelIndex idx = sourceModel()->index(0, idCol());
      Q_ASSERT(idx.isValid());
      Q_ASSERT(ok);
      for (int c(0); c < sourceModel()->columnCount(QModelIndex()); ++c){
#ifndef QT_NO_DEBUG_OUTPUT
         if (QSqlRelationalTableModel * relationalModel = qobject_cast<QSqlRelationalTableModel*>(sourceModel())){
            QSqlRelation relation = relationalModel->relation(c);
            if (relation.isValid()) Q_ASSERT(d_defaultValues.value(c).isValid());}  // need to fill relation column, otherwise insertRows() fails
#endif
         idx = sourceModel()->index(0, c);
         Q_ASSERT(idx.isValid());
         // qDebug() << "freshly inserted field" << idx << sourceModel()->data(idx, Qt::DisplayRole);
         if (c == idCol()){            // set id column
            if (sourceModel()->data(idx, Qt::DisplayRole).toInt() != 0);       // already filled by primeInsert or derived sourceModel class or ...
            else if (d_defaultValues.value(c).isValid()) ok = sourceModel()->setData(idx, d_defaultValues.at(c), Qt::EditRole);  // use provided value
            else ok = sourceModel()->setData(idx, nextFreeId(), Qt::EditRole);}                                                  // use homebrewn autoincrement
         else if (c == parentCol()) ok = sourceModel()->setData(idx, std::numeric_limits<qint32>::min(), Qt::EditRole); // temporary marker to identify row
         else if (d_defaultValues.value(c).isValid()) ok = sourceModel()->setData(idx, d_defaultValues.at(c), Qt::EditRole);
         // qDebug() << "   modified to" << idx << sourceModel()->data(idx, Qt::DisplayRole);
         Q_ASSERT(ok);}
      ok = (sourceModel()->data(sourceModel()->index(0, idCol())).toInt() != 0);    // needed to judge result of submit(); which in turn might change id)
      ok = (sourceModel()->submit() || ok);
      Q_ASSERT_X(ok, "submit() failed and no valid id was set previously", "is the edit strategy erroneously OnManualSubmit combined with autoincrement at database level?");
      // qDebug() << "sourceModel has" << sourceModel()->rowCount() << "rows and" << sourceModel()->columnCount() << "columns;";
      /* assert for row count incorrectly fails if un-submitted row deletions are in the source model
      Q_ASSERT_X(sourceModel()->rowCount() == oldRowCount + 1, "row inserted?",
                 QString(QLatin1String("old row count %1, new %2")).arg(oldRowCount).arg(sourceModel()->rowCount()).toLocal8Bit()); */
      QModelIndexList idxList = sourcechildrenFromId(std::numeric_limits<qint32>::min());
      Q_ASSERT_X(idxList.count() == 1, "row count for tag in parent column should be 1", QString::number(idxList.count()).toLatin1());
      lastInsertedId = sourceModel()->data(idxList.at(0), Qt::DisplayRole).toInt(&ok);
      Q_ASSERT_X(ok, "lastInsertedId", sourceModel()->data(idxList.at(0), Qt::DisplayRole).toByteArray());
      Q_ASSERT(lastInsertedId != 0);
      QModelIndex filterIndex = idxList.at(0).sibling(idxList.at(0).row(), parentCol());
      // qDebug() << "lastInserted" << lastInsertedId << "old parent" << sourceModel()->data(filterIndex, Qt::DisplayRole) << "new parent" << parentId;
      ok = sourceModel()->setData(filterIndex, QVariant(parentId), Qt::EditRole);
      // qDebug() << "   set parent" << sourceModel()->data(filterIndex, Qt::DisplayRole);
      Q_ASSERT(ok);}
   return true;}

/*!
  \brief reimplemented function

  Columns are always appended, i.e., parameters column and parent are ignored.
  Returns 'false' if no column(s) could be appended; is the underlying model able to append rows?
*/
bool QXTreeProxyModel::insertColumns(int column, int count, const QModelIndex& parent){
   Q_UNUSED(parent);
   Q_UNUSED(column);
   // qDebug() <<"insert" << count << "column(s) after last column";
   if (!sourceModel()) return false;
   bool ok = sourceModel()->insertColumns(sourceModel()->columnCount(QModelIndex()), count);
   Q_ASSERT(ok);
   return ok;}

/*!
  \brief reimplemented function

  Only columns to the right of both idCol and parentCol can be removed. An attempt to remove
  other columns returns 'false'. Same if the underlying model does not support removal of columns.
*/
bool QXTreeProxyModel::removeColumns(int column, int count, const QModelIndex& parent){
   Q_UNUSED(parent);
   if (!sourceModel()) return false;
   if (column <= idCol() || column <= parentCol()) return false;
   bool ok = sourceModel()->removeColumns(column, count);
   Q_ASSERT(ok);
   return ok;}

// private helper functions
qint32 QXTreeProxyModel::getId(const QModelIndex& idx) const{
   Q_ASSERT(idx.model() == NULL || idx.model() == this);
   //qDebug() << "getId: " << idx;
   qint32 id;
   if (idx.isValid()) { //idx.row() >= 0){
      Q_ASSERT_X(idx.internalId() != 0, "getId:", qPrintable(QString(QLatin1String("%1 %2 %3")).arg(idx.row()).arg(idx.column()).arg(idx.internalId())));
      id = idx.internalId();}
   else {
       //qDebug() << "   invalid index";
      Q_ASSERT_X(idx.internalId() == 0, "getId:", qPrintable(QString(QLatin1String("%1 %2 %3")).arg(idx.row()).arg(idx.column()).arg(idx.internalId())));
      id = 0;}
   //qDebug() << "   id is" << id;
   return id;}

void QXTreeProxyModel::removeChildRows(qint32 parentId){
   Q_ASSERT(sourceModel());
   QModelIndexList childIndices = sourcechildrenFromId(parentId);
   foreach (QModelIndex childIndex, childIndices){
      if (!isSourceDeleted(childIndex)){
         // store id of this child to later remove it and its children
         bool ok;
         qint32 childId = sourceModel()->data(childIndex, Qt::DisplayRole).toInt(&ok);
         Q_ASSERT(ok);
         Q_ASSERT(childId != 0);
         ok = sourceModel()->removeRow(childIndex.row(), QModelIndex());
         Q_ASSERT(ok); // if the model supported to remove the parent, then it must also be able to remove the child rows
         removeChildRows(childId);}}}

int QXTreeProxyModel::rowFromId(qint32 recordId, qint32 parentId) const {
   // qDebug() << "find rowFromId where recordId is" << recordId << "with parentId" << parentId;
   QModelIndexList childIndices = sourcechildrenFromId(parentId);
   //d_parentFilterModel->sort(idCol(), Qt::AscendingOrder);
   int rowNumber(0);
   bool ok;
   while (rowNumber < childIndices.count() && sourceModel()->data(childIndices.at(rowNumber)).toInt(&ok) != recordId && ok) ++ rowNumber;
   Q_ASSERT(ok);
   if (rowNumber == childIndices.count()) {
      EXDatabase exception;
      exception.msg = QLatin1String("row from id not found");
      exception.id = recordId;}
   // qDebug() << "found rowFromId" << rowNumber;
   return rowNumber;}

QModelIndex QXTreeProxyModel::sourceindexFromId(qint32 id) const {
   // qDebug() << "sourceindexFromId" << id;
   QModelIndexList idxList = sourceModel()->match(sourceModel()->index(0, idCol()), Qt::DisplayRole, id, -1, Qt::MatchExactly);
   Q_ASSERT_X(!idxList.isEmpty(), "key not found", QString::number(id).toLocal8Bit());
   Q_ASSERT_X(idxList.at(0).isValid(), "index for key is not valid", QString::number(id).toLocal8Bit());
   if (idxList.count() > 1){
      EXDatabase exception;
      exception.id = id;
      exception.msg = QLatin1String("duplicate key found");
      throw exception;}
   return idxList.at(0);}

QModelIndexList QXTreeProxyModel::sourcechildrenFromId(qint32 id) const {
   // qDebug() << "sourcechildrenFromId looks for" << id << "in column" << parentCol();
   QModelIndexList parentidxList = sourceModel()->match(sourceModel()->index(0, parentCol()), Qt::DisplayRole, id, -1, Qt::MatchExactly);
   // qDebug() << "   sourcechildrenFromId for" << id << "found" << parentidxList.count() << "child indices in parent column";
   QModelIndexList idxList;
   foreach (QModelIndex idx, parentidxList) idxList.append(idx.sibling(idx.row(), idCol()));
   // qDebug() << "   sourcechildrenFromId for" << id << "found" << idxList.count() << "child indices in id column";
   return idxList;}

bool QXTreeProxyModel::isSourceDeleted(QModelIndex sourceIndex) const {
   QString rowHeader = sourceModel()->headerData(sourceIndex.row(), Qt::Vertical, Qt::DisplayRole).toString();
   return (rowHeader == QLatin1String("!"));}

qint32 QXTreeProxyModel::nextFreeId() const {
   static qint32 lastId(45);
   // qDebug() << "nextFreeId after" << lastId;
   bool idExisting(true);
   while (idExisting && ++lastId < std::numeric_limits<qint32>::max()){
      QModelIndexList idxList = sourceModel()->match(sourceModel()->index(0, idCol(), QModelIndex()), Qt::DisplayRole, lastId, 1, Qt::MatchExactly);
      idExisting = !idxList.isEmpty();}
   // qDebug() << "   is" << lastId;
   if (idExisting) return 0;
   else return lastId;}

//private slots, needed to forward signals

void QXTreeProxyModel::sourceDataChanged(const QModelIndex &source_top_left, const QModelIndex &source_bottom_right){
   // qDebug() << "sourceDataChanged"  << source_top_left << source_top_left.model()->data(source_top_left, Qt::DisplayRole);
   Q_ASSERT(sourceModel());
   Q_ASSERT(source_top_left.isValid());
   Q_ASSERT(source_bottom_right.isValid());
   if (source_top_left.column() <= boost::numeric_cast<int>(idCol()) && source_bottom_right.column() >= boost::numeric_cast<int>(idCol())){
      emit beginResetModel();
      emit endResetModel();}
   else if (source_top_left.column() <= boost::numeric_cast<int>(parentCol()) && source_bottom_right.column() >= boost::numeric_cast<int>(parentCol())){
      emit beginResetModel();
      emit endResetModel();}
   else for (int r(source_top_left.row()); r <= source_bottom_right.row(); ++r)
   for (int c(source_top_left.column()); c <= source_bottom_right.column(); ++c) {
      QModelIndex proxyIndex = mapFromSource(sourceModel()->index(r, c));
      // qDebug() << "   maps to" << proxyIndex;
      if (!proxyIndex.isValid()) return;     // incomplete record with missing id value; safely ignore as not used in QXTreeModel
      else emit dataChanged(proxyIndex, proxyIndex);}}


void QXTreeProxyModel::sourceHeaderDataChanged(Qt::Orientation orientation, int start, int end){
   emit headerDataChanged(orientation, start, end);}

void QXTreeProxyModel::sourceReset(){
   reset();}

void QXTreeProxyModel::sourceLayoutAboutToBeChanged(){
   // qDebug() << "sourceLayoutAboutToBeChanged";
   emit layoutAboutToBeChanged();}

void QXTreeProxyModel::sourceLayoutChanged(){
   // qDebug() << "sourceLayoutChanged";
   emit layoutChanged();}

void QXTreeProxyModel::sourceRowsAboutToBeInserted(const QModelIndex &source_parent, int start, int end){
   // qDebug() << "sourceRowsAboutToBeInserted:" << source_parent << "from start" << start << "to end" << end;
   Q_UNUSED(source_parent);
   Q_UNUSED(start);
   Q_UNUSED(end);
   emit beginResetModel();}

void QXTreeProxyModel::sourceRowsInserted(const QModelIndex &source_parent, int start, int end){
   // qDebug() << "sourceRowsInserted:" << source_parent << "from start" << start << "to end" << end;
   Q_UNUSED(source_parent);
   Q_ASSERT(source_parent == QModelIndex());
   Q_UNUSED(start);
   Q_UNUSED(end);
   emit endResetModel();
   // qDebug() << "emit endResetModel completed; now all rows will be removed and re-added";
   Q_ASSERT(sourceModel()->hasIndex(start, idCol(), source_parent));}

void QXTreeProxyModel::sourceRowsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end){
   // qDebug() << "sourceRowsAboutToBeRemoved: " << source_parent << "from start" << start << "to end" << end;
   Q_UNUSED(source_parent);
   Q_UNUSED(start);
   Q_UNUSED(end);
   emit beginResetModel();}

void QXTreeProxyModel::sourceRowsRemoved(const QModelIndex &source_parent, int start, int end){
   // qDebug() << "sourceRowsRemoved: " << source_parent << "from start" << start << "to end" << end;
   Q_UNUSED(source_parent);
   Q_UNUSED(start);
   Q_UNUSED(end);
   emit endResetModel();}

void QXTreeProxyModel::sourceColumnsAboutToBeInserted(const QModelIndex &source_parent, int start, int end){
   // qDebug() << "sourceColumnsAboutToBeInserted" << source_parent << start << end;
   Q_UNUSED(source_parent);
   Q_UNUSED(start);
   Q_UNUSED(end);
   Q_ASSERT_X(start > idCol() && start > parentCol(), "sourceColumnsAboutToBeInserted",
              "illegal to insert columns in front of parent column or in front of id column");
   emit beginInsertColumns(QModelIndex(), start, end);}

void QXTreeProxyModel::sourceColumnsInserted(const QModelIndex &source_parent, int start, int end){
   // qDebug() << "sourceColumnsInserted" << source_parent << start << end << "   where idCol" << idCol() <<"and parentCol" << parentCol();
   Q_UNUSED(source_parent);
   Q_UNUSED(start);
   Q_UNUSED(end);
   int columnsAdded = end - start + 1;
   Q_ASSERT(columnsAdded > 0);
   if (parentCol() < idCol()){   // order matters: to ensure that never idCol == parentCol (assert violation in setter)
      if (idCol() >= start) {
         bool ok = setIdCol(idCol() + columnsAdded);
         Q_ASSERT(ok);}
      if (parentCol() >= start) {
         bool ok = setParentCol(parentCol() + columnsAdded);
         Q_ASSERT(ok);}}
   else {
      if (parentCol() >= start){
         bool ok = setParentCol(parentCol() + columnsAdded);
         Q_ASSERT(ok);}
      if (idCol() >= start) {
         bool ok = setIdCol(idCol() + columnsAdded);
         Q_ASSERT(ok);}}
   emit endInsertColumns();} // now associated treeViews will update

void QXTreeProxyModel::sourceColumnsAboutToBeRemoved(const QModelIndex &source_parent, int start, int end){
   Q_UNUSED(source_parent);
   Q_UNUSED(start);
   Q_UNUSED(end);
   Q_ASSERT_X(start > idCol() && start > parentCol(), "sourceColumnsAboutToBeInserted",
              "illegal to insert columns in front of parent column or in front of id column");
   emit beginRemoveColumns(QModelIndex(), start, end);} //beginResetModel();}

void QXTreeProxyModel::sourceColumnsRemoved(const QModelIndex &source_parent, int start, int end){
   Q_UNUSED(source_parent);
   Q_UNUSED(start);
   Q_UNUSED(end);
   emit endRemoveColumns();} //endResetModel();}
