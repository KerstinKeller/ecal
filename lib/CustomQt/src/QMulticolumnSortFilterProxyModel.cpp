/* ========================= eCAL LICENSE =================================
 *
 * Copyright (C) 2016 - 2019 Continental Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ========================= eCAL LICENSE =================================
*/

#include "CustomQt/QMulticolumnSortFilterProxyModel.h"

#include <numeric>
#include <QRegularExpression>

QMulticolumnSortFilterProxyModel::QMulticolumnSortFilterProxyModel(QObject* parent)
  : QStableSortFilterProxyModel(parent)
  , always_sorted_column_          (-1)
  , always_sorted_force_sort_order_(false)
  , always_sorted_sort_order_      (Qt::SortOrder::AscendingOrder)
{}

////////////////////////////////////////////
// Filtering
////////////////////////////////////////////

QMulticolumnSortFilterProxyModel::~QMulticolumnSortFilterProxyModel()
{}

void QMulticolumnSortFilterProxyModel::setFilterKeyColumns(const QVector<int>& columns)
{
  if (!columns.empty())
  {
    QSortFilterProxyModel::setFilterKeyColumn(columns[0]);
  }
  else
  {
    QSortFilterProxyModel::setFilterKeyColumn(-1);
  }
  filter_columns_ = columns;
  invalidateFilter();
}

QVector<int> QMulticolumnSortFilterProxyModel::filterKeyColumns() const
{
  return filter_columns_;
}

bool QMulticolumnSortFilterProxyModel::filterDirectAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
  // Qt 5 uses the deprecated QRegExp by default when setting a FilterFixedString. The QRegularExpression is then empty
  //      QRegularExpression didn't even exist in Qt 5.11 and earlier
  // Qt 6 sets the QRegularExpression (QRegExp does not exist anymore) when setting a FilterFixedString.

#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
  // For Qt5.11 there only exists the RegExp, so we need to check the QRegExp

  QRegExp const filter_regexp = filterRegExp();

  for (const int column : filter_columns_)
  {
    const QModelIndex index = sourceModel()->index(source_row, column, source_parent);
    if (index.isValid())
    {
      const QString data = sourceModel()->data(index, filterRole()).toString();
      if (data.contains(filter_regexp))
      {
        return true;
      }
    }
  }
  return false;

#elif QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  // For Qt5.12 - 5.15 (i.e. pre-Qt6) we need to check the QRegExp and the QRegularExpression
  QRegExp const filter_regexp = filterRegExp();

  if (!filter_regexp.isEmpty())
  {
    // Use QRegExp
    for (const int column : filter_columns_)
    {
      const QModelIndex index = sourceModel()->index(source_row, column, source_parent);
      if (index.isValid())
      {
        const QString data = sourceModel()->data(index, filterRole()).toString();
        if (data.contains(filter_regexp))
        {
          return true;
        }
      }
    }
    return false;
  }
  else
  {
    // Use QRegularExpression, as QRegExp is empty
    QRegularExpression const filter_regularexpression = filterRegularExpression();

    for (const int column : filter_columns_)
    {
      const QModelIndex index = sourceModel()->index(source_row, column, source_parent);
      if (index.isValid())
      {
        const QString data = sourceModel()->data(index, filterRole()).toString();
        if (data.contains(filter_regularexpression))
        {
          return true;
        }
      }
    }
    return false;
  }
#else
  // For Qt6 we only need to check the QRegularExpression
  QRegularExpression const filter_regularexpression = filterRegularExpression();

  for (const int column : filter_columns_)
  {
    const QModelIndex index = sourceModel()->index(source_row, column, source_parent);
    if (index.isValid())
    {
      const QString data = sourceModel()->data(index, filterRole()).toString();
      if (data.contains(filter_regularexpression))
      {
        return true;
      }
    }
  }
  return false;
#endif
}

////////////////////////////////////////////
// Sorting
////////////////////////////////////////////

bool QMulticolumnSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
  if (always_sorted_column_ >= 0)
  {
    QVariant const left_data   = sourceModel()->data(sourceModel()->index(left.row(), always_sorted_column_, left.parent()), sortRole());
    QVariant const right_data  = sourceModel()->data(sourceModel()->index(right.row(), always_sorted_column_, right.parent()), sortRole());

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning (disable : 4996)
#elif __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    if (left_data != right_data)
    {
      if (!always_sorted_force_sort_order_)
      {
        // Don't fake the sort order, we have to follow the user-set one

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        return QPartialOrdering::Less == QVariant::compare(left_data, right_data);
#else // Pre Qt 6.0
        // Qt Deprecation note:
        // 
        // QVariant::operator< is deprecated since Qt 5.15, mainly because
        // QVariants may contain datatypes that are not comparable at all. The
        // developer is now supposed to unpack the QVariant and compare the
        // native values. In a SortFilterProxyModel we cannot do that, as we
        // don't know what this model will be used for and what the type of the
        // QVariants will be.
        // Thus, we would have to implement a best guess approach anyways. As I
        // doubt that I will hack a custom comparator that is better than the
        // deprecated QVariant one, we will just keep using it.
        // 
        // The operators still exist in Qt 6.0 beta.
        return (left_data < right_data);
#endif // QT_VERSION
      }
      else
      {
        // We want to ignore the user-set sort order, so we fake the less-than method
        if (sortOrder() == always_sorted_sort_order_)
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          return QPartialOrdering::Less == QVariant::compare(left_data, right_data);
#else // Pre Qt 6.0
          return (left_data < right_data); // Qt 5.15 Deprecation note: see above
#endif // QT_VERSION
        }
        else
        {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
          return QPartialOrdering::Greater == QVariant::compare(left_data, right_data);
#else // Pre Qt 6.0
          return (left_data > right_data); // Qt 5.15 Deprecation note: see above
#endif // QT_VERSION
        }
      }
    }
#ifdef _MSC_VER
  #pragma warning(pop)
#elif __GNUC__
  #pragma GCC diagnostic pop
#endif
  }

  return QStableSortFilterProxyModel::lessThan(left, right);
}


void QMulticolumnSortFilterProxyModel::setAlwaysSortedColumn(int column)
{
  always_sorted_force_sort_order_ = false;
  always_sorted_column_           = column;

  invalidate();
}

void QMulticolumnSortFilterProxyModel::setAlwaysSortedColumn(int column, Qt::SortOrder forced_sort_order)
{
  always_sorted_force_sort_order_ = true;
  always_sorted_sort_order_       = forced_sort_order;
  always_sorted_column_           = column;

  invalidate();
}

int QMulticolumnSortFilterProxyModel::alwaysSortedColumn() const
{
  return always_sorted_column_;
}
