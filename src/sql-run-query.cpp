
#include "sql-run-query.h"

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

#include "sql-runner.h"
#include <QSqlDatabase>
#include <QSqlQuery>


namespace deliberate
{
int                  SqlRunQuery::liveDynamic (0);

SqlRunQuery::SqlRunQuery (SqlRunner * r, QSqlDatabase * sdb,
                           int dbh, int qh)
 :finished (false),
  execStarted (false),
  dbHandle (dbh),
  queryHandle (qh),
  dbRunner (r),
  sqlQuery (0),
  rowsChanged (-1)
{
  if (sdb) {
    sqlQuery = new QSqlQuery (*sdb);
  }
}

SqlRunQuery::~SqlRunQuery ()
{
  if (sqlQuery) {
    delete sqlQuery;
    sqlQuery = 0;
  }
}

void*
SqlRunQuery::operator new (size_t size)
{
  void * pQ (0);
  pQ = malloc (size);
  liveDynamic++;
  return pQ;
}

void
SqlRunQuery::operator delete (void * p)
{
  SqlRunQuery * pQ = static_cast <SqlRunQuery*> (p);
  free (pQ);
  liveDynamic--;
}

void
SqlRunQuery::clear ()
{
  result.clear ();
}

bool
SqlRunQuery::isFinished ()
{
  return finished;
}

bool
SqlRunQuery::isActive ()
{
  return execStarted && !finished;
}

int
SqlRunQuery::rowsAffected ()
{
  return rowsChanged;
}

bool
SqlRunQuery::next ()
{
  if (currentRow < numRows) {
    currentRow++;
  }
  return (0 <= currentRow && currentRow < numRows);
}

bool
SqlRunQuery::first ()
{
  currentRow = 0;
  return numRows > 0;
}

QVariant
SqlRunQuery::value (int column)
{
  return value (currentRow, column);
}

QVariant
SqlRunQuery::value (int row, int column)
{
  if (0 <= row && row < result.count()) {
    if (result.at(row).contains (column)) {
      return result.at(row)[column];
    }
  }
  return QVariant();
}

void
SqlRunQuery::bindValue (int column, 
                        const QVariant & value,
                        QSql::ParamType paramType)
{
  if (dbRunner) {
    dbRunner->RequestBind (queryHandle, column, value, paramType);
  }
}

int
SqlRunQuery::rowCount ()
{
  if (finished) {
    return result.count();
  } else {
    return 0;
  }
}

bool
SqlRunQuery::exec ()
{
  finished = false;
  execStarted = false;
  if (dbRunner) {
    dbRunner->RequestExec (queryHandle);
    return true;
  } else {
    return false;
  }
}

bool
SqlRunQuery::exec (const QString & query)
{
  finished = false;
  execStarted = false;
  if (dbRunner) {
    dbRunner->RequestExec (queryHandle, query);
    return true;
  } else {
    return false;
  }
}

bool
SqlRunQuery::prepare (const QString & query)
{
  if (dbRunner) {
    dbRunner->RequestPrepare (queryHandle, query);
    return true;
  } else {
    return false;
  }
}

QString
SqlRunQuery::executedQuery () const
{
  if (finished) {
    return didQuery;
  } else {
    return QString();
  }
}

void
SqlRunQuery::deleteLater ()
{
  if (dbRunner) {
    dbRunner->RequestDelete (queryHandle);
  }
}

SqlRunner*
SqlRunQuery::runner ()
{
  return dbRunner;
}

SqlRunDatabase *
SqlRunQuery::database ()
{
  if (dbRunner) {
    return dbRunner->database (dbHandle);
  } else {
    return 0;
  }
}

} // namespace
