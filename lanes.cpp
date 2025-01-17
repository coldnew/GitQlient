/*
        Description: history graph computation

        Author: Marco Costalba (C) 2005-2007

        Copyright: See COPYING file that comes with this distribution

*/
#include <QStringList>
#include "lanes.h"
#include <git.h>

#define IS_NODE(x) (x == (NODE) || x == (NODE_R) || x == (NODE_L))

void Lanes::init(const QString &expectedSha)
{
   clear();
   activeLane = 0;
   setBoundary(false);
   add(LaneType::BRANCH, expectedSha, activeLane);
}

void Lanes::clear()
{

   typeVec.clear();
   nextShaVec.clear();
}

void Lanes::setBoundary(bool b)
{
   // changes the state so must be called as first one

   NODE = b ? LaneType::BOUNDARY_C : LaneType::MERGE_FORK;
   NODE_R = b ? LaneType::BOUNDARY_R : LaneType::MERGE_FORK_R;
   NODE_L = b ? LaneType::BOUNDARY_L : LaneType::MERGE_FORK_L;
   boundary = b;

   if (boundary)
      typeVec[activeLane] = (LaneType::BOUNDARY);
}

bool Lanes::isFork(const QString &sha, bool &isDiscontinuity)
{

   int pos = findNextSha(sha, 0);
   isDiscontinuity = (activeLane != pos);
   if (pos == -1) // new branch case
      return false;

   return findNextSha(sha, pos + 1) != -1;
}

void Lanes::setFork(const QString &sha)
{

   int rangeStart, rangeEnd, idx;
   rangeStart = rangeEnd = idx = findNextSha(sha, 0);

   while (idx != -1)
   {
      rangeEnd = idx;
      typeVec[idx] = LaneType::TAIL;
      idx = findNextSha(sha, idx + 1);
   }
   typeVec[activeLane] = (NODE);

   auto &startT = typeVec[rangeStart];
   auto &endT = typeVec[rangeEnd];

   if (startT == NODE)
      startT = NODE_L;

   if (endT == NODE)
      endT = NODE_R;

   if (startT == LaneType::TAIL)
      startT = LaneType::TAIL_L;

   if (endT == LaneType::TAIL)
      endT = LaneType::TAIL_R;

   for (int i = rangeStart + 1; i < rangeEnd; i++)
   {

      auto &t = typeVec[i];

      if (t == LaneType::NOT_ACTIVE)
         t = (LaneType::CROSS);

      else if (t == LaneType::EMPTY)
         t = LaneType::CROSS_EMPTY;
   }
}

void Lanes::setMerge(const QStringList &parents)
{
   // setFork() must be called before setMerge()

   if (boundary)
      return; // handle as a simple active line

   auto &t = typeVec[activeLane];
   bool wasFork = (t == NODE);
   bool wasFork_L = (t == NODE_L);
   bool wasFork_R = (t == NODE_R);
   bool startJoinWasACross = false, endJoinWasACross = false;

   t = NODE;

   int rangeStart = activeLane, rangeEnd = activeLane;
   QStringList::const_iterator it(parents.constBegin());
   for (++it; it != parents.constEnd(); ++it)
   { // skip first parent

      int idx = findNextSha(*it, 0);
      if (idx != -1)
      {

         if (idx > rangeEnd)
         {

            rangeEnd = idx;
            endJoinWasACross = typeVec[idx] == (LaneType::CROSS);
         }

         if (idx < rangeStart)
         {

            rangeStart = idx;
            startJoinWasACross = typeVec[idx] == (LaneType::CROSS);
         }

         typeVec[idx] = (LaneType::JOIN);
      }
      else
         rangeEnd = add(LaneType::HEAD, *it, rangeEnd + 1);
   }
   auto &startT = typeVec[rangeStart];
   auto &endT = typeVec[rangeEnd];

   if (startT == NODE && !wasFork && !wasFork_R)
      startT = NODE_L;

   if (endT == NODE && !wasFork && !wasFork_L)
      endT = NODE_R;

   if (startT == (LaneType::JOIN) && !startJoinWasACross)
      startT = (LaneType::JOIN_L);

   if (endT == (LaneType::JOIN) && !endJoinWasACross)
      endT = (LaneType::JOIN_R);

   if (startT == (LaneType::HEAD))
      startT = (LaneType::HEAD_L);

   if (endT == (LaneType::HEAD))
      endT = (LaneType::HEAD_R);

   for (int i = rangeStart + 1; i < rangeEnd; i++)
   {

      auto &t = typeVec[i];

      if (t == LaneType::NOT_ACTIVE)
         t = LaneType::CROSS;

      else if (t == LaneType::EMPTY)
         t = LaneType::CROSS_EMPTY;

      else if (t == LaneType::TAIL_R || t == LaneType::TAIL_L)
         t = LaneType::TAIL;
   }
}

void Lanes::setInitial()
{

   auto &t = typeVec[activeLane];
   if (!IS_NODE(t) && t != LaneType::APPLIED)
      t = boundary ? LaneType::BOUNDARY : LaneType::INITIAL;
}

void Lanes::setApplied()
{

   // applied patches are not merges, nor forks
   typeVec[activeLane] = (LaneType::APPLIED); // TODO test with boundaries
}

void Lanes::changeActiveLane(const QString &sha)
{

   auto &t = typeVec[activeLane];
   if (t == LaneType::INITIAL || isBoundary(t))
      t = LaneType::EMPTY;
   else
      t = LaneType::NOT_ACTIVE;

   int idx = findNextSha(sha, 0); // find first sha
   if (idx != -1)
      typeVec[idx] = (LaneType::ACTIVE); // called before setBoundary()
   else
      idx = add(LaneType::BRANCH, sha, activeLane); // new branch

   activeLane = idx;
}

void Lanes::afterMerge()
{

   if (boundary)
      return; // will be reset by changeActiveLane()

   for (int i = 0; i < typeVec.count(); i++)
   {

      auto &t = typeVec[i];

      if (isHead(t) || isJoin(t) || t == (LaneType::CROSS))
         t = LaneType::NOT_ACTIVE;

      else if (t == LaneType::CROSS_EMPTY)
         t = LaneType::EMPTY;

      else if (IS_NODE(t))
         t = LaneType::ACTIVE;
   }
}

void Lanes::afterFork()
{

   for (int i = 0; i < typeVec.count(); i++)
   {

      auto &t = typeVec[i];

      if (t == LaneType::CROSS)
         t = LaneType::NOT_ACTIVE;

      else if (isTail(t) || t == LaneType::CROSS_EMPTY)
         t = LaneType::EMPTY;

      if (!boundary && IS_NODE(t))
         t = LaneType::ACTIVE; // boundary will be reset by changeActiveLane()
   }
   while (typeVec.last() == LaneType::EMPTY)
   {
      typeVec.pop_back();
      nextShaVec.pop_back();
   }
}

bool Lanes::isBranch()
{

   return (typeVec[activeLane] == (LaneType::BRANCH));
}

void Lanes::afterBranch()
{

   typeVec[activeLane] = (LaneType::ACTIVE); // TODO test with boundaries
}

void Lanes::afterApplied()
{

   typeVec[activeLane] = (LaneType::ACTIVE); // TODO test with boundaries
}

void Lanes::nextParent(const QString &sha)
{

   nextShaVec[activeLane] = (boundary ? "" : sha);
}

int Lanes::findNextSha(const QString &next, int pos)
{

   for (int i = pos; i < nextShaVec.count(); i++)
      if (nextShaVec[i] == next)
         return i;
   return -1;
}

int Lanes::findType(const LaneType type, int pos)
{

   for (int i = pos; i < typeVec.count(); i++)
      if (typeVec[i] == (type))
         return i;
   return -1;
}

int Lanes::add(const LaneType type, const QString &next, int pos)
{

   // first check empty lanes starting from pos
   if (pos < (int)typeVec.count())
   {
      pos = findType(LaneType::EMPTY, pos);
      if (pos != -1)
      {
         typeVec[pos] = (type);
         nextShaVec[pos] = next;
         return pos;
      }
   }
   // if all lanes are occupied add a new lane
   typeVec.append((type));
   nextShaVec.append(next);
   return typeVec.count() - 1;
}
