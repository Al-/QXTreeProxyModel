/*/////////////////////////////////////////////////////////////
developed by Andreas D. Christ, software(at)quantentunnel.de
using Qt Creator and Qt SDK with non-commercial license, see http://qt.nokia.com
license: GPL or any other license compatible with Qt's license (users choice)
////////////////////////////////////////////////////////////*/



#include <QtGui/QApplication>
#include "testdialog.h"

int main(int argc, char *argv[]){
   QApplication a(argc, argv);
   TestDialog w;
   w.show();
   return a.exec();}
