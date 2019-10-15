/*
        Description: interface to git programs

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/

#include "RepositoryModel.h"
#include <RepositoryModelColumns.h>
#include <RevisionsCache.h>
#include <Revision.h>

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>

#include "lanes.h"

#include "git.h"

using namespace QGit;

RepositoryModel::RepositoryModel(QSharedPointer<RevisionsCache> revCache, QSharedPointer<Git> git, QObject *p)
   : QAbstractItemModel(p)
   , mRevCache(revCache)
   , mGit(git)
{
   mGit->setDefaultModel(this);

   mColumns.insert(RepositoryModelColumns::GRAPH, "Graph");
   mColumns.insert(RepositoryModelColumns::ID, "Id");
   mColumns.insert(RepositoryModelColumns::SHA, "Sha");
   mColumns.insert(RepositoryModelColumns::LOG, "Log");
   mColumns.insert(RepositoryModelColumns::AUTHOR, "Author");
   mColumns.insert(RepositoryModelColumns::DATE, "Date");

   lns = new Lanes();
   clear(); // after _headerInfo is set

   connect(mGit.get(), &Git::newRevsAdded, this, &RepositoryModel::on_newRevsAdded);
   connect(mGit.get(), &Git::loadCompleted, this, &RepositoryModel::on_loadCompleted);
   connect(mRevCache.get(), &RevisionsCache::signalCacheUpdated, this, &RepositoryModel::on_newRevsAdded);
}

RepositoryModel::~RepositoryModel()
{

   clear();
   delete lns;
}

int RepositoryModel::rowCount(const QModelIndex &parent) const
{

   return !parent.isValid() ? mRevCache->count() : 0;
}

bool RepositoryModel::hasChildren(const QModelIndex &parent) const
{

   return !parent.isValid();
}

void RepositoryModel::flushTail()
{
   beginResetModel();
   mRevCache->flushTail(earlyOutputCnt, earlyOutputCntBase);
   firstFreeLane = static_cast<unsigned int>(earlyOutputCntBase);
   lns->clear();
   endResetModel();
}

void RepositoryModel::clear(bool complete)
{

   if (!complete)
   {
      if (!mRevCache->isEmpty())
         flushTail();

      return;
   }

   mGit->cancelDataLoading();

   beginResetModel();

   mRevCache->clear();

   firstFreeLane = earlyOutputCntBase = 0;
   setEarlyOutputState(false);
   lns->clear();

   endResetModel();
   emit headerDataChanged(Qt::Horizontal, 0, 5);
}

void RepositoryModel::on_newRevsAdded()
{
   // do not attempt to insert 0 rows since the inclusive range would be invalid
   const auto revisionsCount = mRevCache->count();

   if (revisionsCount > 0)
   {
      beginResetModel();
      endResetModel();
   }
}

void RepositoryModel::on_loadCompleted()
{
   beginResetModel(); // force a reset to avoid artifacts in file history graph under Windows
   endResetModel();
}

QVariant RepositoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{

   if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
      return mColumns.value(static_cast<RepositoryModelColumns>(section));

   return QVariant();
}

QModelIndex RepositoryModel::index(int row, int column, const QModelIndex &) const
{
   if (row < 0 || row >= mRevCache->count())
      return QModelIndex();

   return createIndex(row, column, nullptr);
}

QModelIndex RepositoryModel::parent(const QModelIndex &) const
{

   static const QModelIndex no_parent;
   return no_parent;
}

QVariant RepositoryModel::data(const QModelIndex &index, int role) const
{

   static const QVariant no_value;

   if (!index.isValid() || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
      return no_value; // fast path, 90% of calls ends here!

   const auto r = mRevCache->getRevisionByRow(index.row());

   if (!r.isValid())
      return no_value;

   if (role == Qt::ToolTipRole)
   {
      QDateTime d;
      d.setSecsSinceEpoch(r.authorDate().toUInt());

      return QString("<p><b>Author:</b> %1</p><p><b>Date:</b> %2</p>")
          .arg(r.author().split("<").first(), d.toString(Qt::SystemLocaleShortDate));
   }

   int col = index.column();

   // calculate lanes
   if (r.lanes.count() == 0)
      mGit->setLane(r.sha());

   switch (static_cast<RepositoryModelColumns>(col))
   {
      case RepositoryModelColumns::ID:
         return no_value;
      case RepositoryModelColumns::SHA:
         return r.sha();
      case RepositoryModelColumns::LOG:
         return r.shortLog();
      case RepositoryModelColumns::AUTHOR:
         return r.author().split("<").first();
      case RepositoryModelColumns::DATE:
      {
         QDateTime dt;
         dt.setSecsSinceEpoch(r.authorDate().toUInt());
         return dt.toString("dd/MM/yyyy hh:mm");
      }
      default:
         return no_value;
   }
}
