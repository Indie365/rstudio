/*
 * TranslationUnit.cpp
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

#include <core/libclang/TranslationUnit.hpp>

#include <core/FilePath.hpp>

#include <core/libclang/Utils.hpp>
#include <core/libclang/LibClang.hpp>
#include <core/libclang/UnsavedFiles.hpp>

namespace rscore {
namespace libclang {

namespace  {

std::string formatBytes(double value)
{
   int mb = static_cast<int>(value / 1024 / 1024);
   if (mb > 1024)
   {
      double gb = (double)mb / 1024.0;
      boost::format fmt("%1.1f gb");
      return boost::str(fmt % gb);
   }
   else if (mb > 1)
   {
      boost::format fmt("%1% mb");
      return boost::str(fmt % mb);
   }
   else
   {
      boost::format fmt("%1% kb");
      return boost::str(fmt % static_cast<int>(value / 1024));
   }
}

} // anonymous namespace

std::string TranslationUnit::getSpelling() const
{
   return toStdString(clang().getTranslationUnitSpelling(tu_));
}

bool TranslationUnit::includesFile(const std::string& filename) const
{
   return clang().getFile(tu_, filename.c_str()) != NULL;
}

unsigned TranslationUnit::getNumDiagnostics() const
{
   return clang().getNumDiagnostics(tu_);
}

Diagnostic TranslationUnit::getDiagnostic(unsigned index) const
{
   return Diagnostic(clang().getDiagnostic(tu_, index));
}

Cursor TranslationUnit::getCursor(const std::string& filename,
                                  unsigned line,
                                  unsigned column)
{
   // get the file
   CXFile file = clang().getFile(tu_, filename.c_str());
   if (file == NULL)
      return Cursor();

   // get the source location
   CXSourceLocation sourceLoc = clang().getLocation(tu_, file, line, column);

   // get the cursor
   CXCursor cursor = clang().getCursor(tu_, sourceLoc);
   if (clang().equalCursors(cursor, clang().getNullCursor()))
      return Cursor();

   // return it
   return Cursor(cursor);
}

CodeCompleteResults TranslationUnit::codeCompleteAt(const std::string& filename,
                                                    unsigned line,
                                                    unsigned column)
{
   CXCodeCompleteResults* pResults = clang().codeCompleteAt(
                                 tu_,
                                 filename.c_str(),
                                 line,
                                 column,
                                 pUnsavedFiles_->unsavedFilesArray(),
                                 pUnsavedFiles_->numUnsavedFiles(),
                                 clang().defaultCodeCompleteOptions());

   if (pResults != NULL)
   {
      clang().sortCodeCompletionResults(pResults->Results,
                                        pResults->NumResults);

      return CodeCompleteResults(pResults);
   }
   else
   {
      return CodeCompleteResults();
   }
}

void TranslationUnit::printResourceUsage(std::ostream& ostr, bool detailed)
{
   CXTUResourceUsage usage = clang().getCXTUResourceUsage(tu_);

   unsigned long totalBytes = 0;
   for (unsigned i = 0; i < usage.numEntries; i++)
   {
      CXTUResourceUsageEntry entry = usage.entries[i];

      if (detailed)
      {
         ostr << clang().getTUResourceUsageName(entry.kind) << ": "
              << formatBytes(entry.amount) << std::endl;
      }

      if (entry.kind >= CXTUResourceUsage_MEMORY_IN_BYTES_BEGIN &&
          entry.kind <= CXTUResourceUsage_MEMORY_IN_BYTES_END)
      {
         totalBytes += entry.amount;
      }
   }
   ostr << "TOTAL MEMORY: " << formatBytes(totalBytes)
        << " (" << FilePath(getSpelling()).filename() << ")" << std::endl;

   clang().disposeCXTUResourceUsage(usage);
}


} // namespace libclang
} // namespace rscore


