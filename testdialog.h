/*/////////////////////////////////////////////////////////////
developed by Andreas D. Christ, software(at)quantentunnel.de
using Qt Creator and Qt SDK with non-commercial license, see http://qt.nokia.com
license: GPL or any other license compatible with Qt's license (users choice)
////////////////////////////////////////////////////////////*/



#ifndef TESTDIALOG_H
#define TESTDIALOG_H

#include "ui_testdialog.h"
#include <QDialog>

class TestDialog : public QDialog, private Ui::TestDialog {
   Q_OBJECT
public:
   TestDialog(QWidget *parent = 0);};

#endif // TESTDIALOG_H
