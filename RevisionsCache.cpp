#include "RevisionsCache.h"

#include <common.h>

RevisionsCache::RevisionsCache(QSharedPointer<Git> git, QObject *parent)
   : QObject(parent)
   , mGit(git)
{
   revs.reserve(QGit::MAX_DICT_SIZE);
}

Revision RevisionsCache::getRevision(const QString &sha)
{
   return revs.value(sha);
}

Revision RevisionsCache::getRevisionByRow(int row) const
{
   const auto validRow = row >= 0 && row < revOrder.count();
   return validRow ? getRevisionBySha(revOrder.at(row)) : Revision();
}

Revision RevisionsCache::getRevisionBySha(const QString &sha) const
{
   return !sha.isEmpty() ? revs.value(sha) : Revision();
}

void RevisionsCache::insertRevision(const QString &sha, const Revision &rev)
{
   revs.insert(sha, rev);
   revOrder.append(sha);
}

int RevisionsCache::row(const QString &sha) const
{
   return !sha.isEmpty() ? revs.value(sha).orderIdx : -1;
}

void RevisionsCache::flushTail(int earlyOutputCnt, int earlyOutputCntBase)
{
   if (earlyOutputCnt >= 0 && earlyOutputCnt < count())
   {
      for (int i = earlyOutputCntBase; i < revOrder.count(); ++i)
         revs[revOrder[i]].lanes.clear();

      auto cnt = count() - earlyOutputCnt + 1;
      while (cnt > 0)
      {
         const auto sha = revOrder.last();
         revs.remove(sha);
         revOrder.pop_back();
         --cnt;
      }
   }
}

void RevisionsCache::clear()
{
   revs.clear();
   revOrder.clear();
}
