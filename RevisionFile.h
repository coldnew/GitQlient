#pragma once

#include <QByteArray>
#include <QVector>

class RevisionFile
{
   friend class Cache; // to directly load status
   friend class Git;

   // Status information is splitted in a flags vector and in a string
   // vector in 'status' are stored flags according to the info returned
   // by 'git diff-tree' without -C option.
   // In case of a working directory file an IN_INDEX flag is or-ed togheter in
   // case file is present in git index.
   // If file is renamed or copied an entry in 'extStatus' stores the
   // value returned by 'git diff-tree -C' plus source and destination
   // files info.
   // When status of all the files is 'modified' then onlyModified is
   // set, this let us to do some optimization in this common case
   bool onlyModified;
   QVector<int> status;
   QVector<QString> extStatus;

   // prevent implicit C++ compiler defaults
   RevisionFile(const RevisionFile &);
   RevisionFile &operator=(const RevisionFile &);

public:
   enum StatusFlag
   {
      MODIFIED = 1,
      DELETED = 2,
      NEW = 4,
      RENAMED = 8,
      COPIED = 16,
      UNKNOWN = 32,
      IN_INDEX = 64,
      ANY = 127
   };

   RevisionFile()
      : onlyModified(true)
   {
   }

   /* This QByteArray keeps indices in some dir and names vectors,
    * defined outside RevisionFile. Paths are splitted in dir and file
    * name, first all the dirs are listed then the file names to
    * achieve a better compression when saved to disk.
    * A single QByteArray is used instead of two vectors because it's
    * much faster to load from disk when using a QDataStream
    */
   QByteArray pathsIdx;

   int dirAt(int idx) const { return ((const int *)pathsIdx.constData())[idx]; }
   int nameAt(int idx) const { return ((const int *)pathsIdx.constData())[count() + idx]; }

   QVector<int> mergeParent;

   // helper functions
   int count() const { return pathsIdx.size() / (sizeof(int) * 2); }
   bool statusCmp(int idx, StatusFlag sf) const
   {

      return ((onlyModified ? MODIFIED : status.at(static_cast<int>(idx))) & sf);
   }
   const QString extendedStatus(int idx) const
   {
      /*
         rf.extStatus has size equal to position of latest copied/renamed file,
         that could be lower then count(), so we have to explicitly check for
         an out of bound condition.
      */
      return (!extStatus.isEmpty() && idx < extStatus.count() ? extStatus.at(idx) : "");
   }
   const RevisionFile &operator>>(QDataStream &) const;
   RevisionFile &operator<<(QDataStream &);
};
