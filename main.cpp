/*
        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include <QApplication>

#include <GitQlient.h>
#include <QLogger.h>

using namespace QLogger;

int main(int argc, char *argv[])
{
   QApplication app(argc, argv);
   app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

   const auto manager = QLoggerManager::getInstance();
   manager->addDestination("GitQlient.log", "UI", LogLevel::Debug);

   QLog_Info("UI", "Starting GitQlient...");

   const auto mainWin = new GitQlient();
   mainWin->show();

   QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

   const auto ret = app.exec();

   QLog_Info("UI", "Stopping GitQlient...");

   return ret;
}
