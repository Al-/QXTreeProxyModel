QXTreeProxyModel is documented in Doxygen-style. An extract is included here.

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
