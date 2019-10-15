#pragma once

/****************************************************************************************
 ** GitQlient is an application to manage and operate one or several Git repositories. With
 ** GitQlient you will be able to add commits, branches and manage all the options Git provides.
 ** Copyright (C) 2019  Francesc Martinez
 **
 ** LinkedIn: www.linkedin.com/in/cescmm/
 ** Web: www.francescmm.com
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License as published by the Free Software Foundation; either
 ** version 3 of the License, or (at your option) any later version.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 ** Lesser General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public
 ** License along with this library; if not, write to the Free Software
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 ***************************************************************************************/

#include <Revision.h>

#include <QObject>
#include <QHash>
#include <QSharedPointer>

class Git;

class RevisionsCache : public QObject
{
   Q_OBJECT

signals:
   void signalCacheUpdated();

public:
   explicit RevisionsCache(QSharedPointer<Git> git, QObject *parent = nullptr);

   void clear();

   /* Revs*/
   Revision getRevision(const QString &sha);
   Revision getRevisionByRow(int row) const;
   Revision getRevisionBySha(const QString &sha) const;
   bool contains(const QString &sha) { return revs.contains(sha); }
   void insertRevision(const QString &sha, const Revision &rev);
   int row(const QString &sha) const;

   /* RevsOrder */
   int count() const { return revOrder.count(); }
   bool isEmpty() const { return revOrder.isEmpty(); }
   void flushTail(int earlyOutputCnt, int earlyOutputCntBase);
   QString &createRevisionSha(int index) { return revOrder[index]; }
   QString getRevisionSha(int index) const { return revOrder.at(index); }
   int revOrderCount() const { return revOrder.count(); }

private:
   QSharedPointer<Git> mGit;
   QHash<QString, Revision> revs;
   QVector<QString> revOrder;
};
