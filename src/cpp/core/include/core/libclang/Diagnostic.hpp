/*
 * Diagnostic.hpp
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

#ifndef CORE_LIBCLANG_DIAGNOSTIC_HPP
#define CORE_LIBCLANG_DIAGNOSTIC_HPP

#include <boost/shared_ptr.hpp>

#include "clang-c/Index.h"

#include "SourceLocation.hpp"

namespace rscore {
namespace libclang {

class Diagnostic
{
public:
   explicit Diagnostic(CXDiagnostic diagnostic)
      : pDiagnostic_(new CXDiagnostic(diagnostic))
   {
   }

   ~Diagnostic();

   std::string format() const;
   std::string format(unsigned options) const;

   CXDiagnosticSeverity getSeverity() const;
   SourceLocation getLocation() const;
   std::string getSpelling() const;

private:
   CXDiagnostic diagnostic() const { return *pDiagnostic_; }

private:
   boost::shared_ptr<CXDiagnostic> pDiagnostic_;
};

} // namespace libclang
} // namespace rscore

#endif // CORE_LIBCLANG_DIAGNOSTIC_HPP
