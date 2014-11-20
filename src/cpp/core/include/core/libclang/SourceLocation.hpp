/*
 * SourceLocation.hpp
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef CORE_LIBCLANG_SOURCE_LOCATION_HPP
#define CORE_LIBCLANG_SOURCE_LOCATION_HPP

#include <string>
#include <iosfwd>

#include "clang-c/Index.h"

namespace rscore {
namespace libclang {

class SourceLocation
{
public:
   SourceLocation();

   explicit SourceLocation(CXSourceLocation location)
      : location_(location)
   {
   }

   // source location objects are managed internal to clang so instances
   // of this type can be freely copied

   bool empty() const;

   bool isFromMainFile() const;

   bool isInSystemHeader() const;

   void getExpansionLocation(std::string* pFile,
                             unsigned* pLine,
                             unsigned* pColumn,
                             unsigned* pOffset = NULL) const;

   void printExpansionLocation(std::ostream& ostr);

   void getSpellingLocation(std::string* pFile,
                            unsigned* pLine,
                            unsigned* pColumn,
                            unsigned* pOffset = NULL) const;

   void printSpellingLocation(std::ostream& ostr);

   bool operator==(const SourceLocation& other) const ;
   bool operator!=(const SourceLocation& other) const ;

private:
   CXSourceLocation location_;
};


} // namespace libclang
} // namespace rscore

#endif // CORE_LIBCLANG_SOURCE_LOCATION_HPP
