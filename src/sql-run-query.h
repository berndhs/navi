#ifndef DELIBERATE_SQL_RUN_QUERY_H
#define DELIBERATE_SQL_RUN_QUERY_H

/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2010, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QVariant>
#include <QString>
#include <QVector>
#include <QList>
#include <QtSql>

#include <new>

class QSqlQuery;
class QSqlDatabase;

namespace deliberate
{

class SqlRunner;
class SqlRunDatabase;

class SqlRunQuery
{
public:

  bool            isFinished ();
  bool            isActive ();
  bool            next ();
  bool            first ();
  int             rowsAffected ();

  void            bindValue (int column, const QVariant & value, 
                       QSql::ParamType paramType = QSql::In );
  int             rowCount ();
  QVariant        value (int column);
  QVariant        value (int row, int column);
  bool            exec ();
  bool            exec (const QString & query);
  bool            prepare (const QString & query);

  QString         executedQuery () const;
  SqlRunner      *runner();
  SqlRunDatabase *database();
  void            deleteLater ();

  void * operator new (size_t size);
  void   operator delete (void * p);

  static int LiveDynamic () { return liveDynamic; }

private:

  SqlRunQuery (SqlRunner * r, QSqlDatabase * sdb, int dbh, int qh);

  ~SqlRunQuery ();
  void clear ();

  bool              finished;
  bool              execStarted;
  int               dbHandle;
  int               queryHandle;
  SqlRunner        *dbRunner;
  QSqlQuery        *sqlQuery;

  QList <QMap<int, QVariant> >  result;
  int               currentRow;
  int               numRows;
  int               rowsChanged;
  QString           didQuery;

  friend class SqlRunner;
  friend class SqlRunDatabase;
    
  static int liveDynamic;

};

} // namespace

#endif
