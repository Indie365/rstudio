/*
 * SessionRCompletions.cpp
 *
 * Copyright (C) 2014 by RStudio, Inc.
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

#include "SessionRCompletions.hpp"

#include <core/Exec.hpp>

#include <r/RSexp.hpp>
#include <r/RInternal.hpp>
#include <r/RExec.hpp>
#include <r/RJson.hpp>
#include <r/RRoutines.hpp>
#include <r/ROptions.hpp>
#include <r/session/RClientState.hpp>
#include <r/session/RSessionUtils.hpp>

#include <core/system/FileScanner.hpp>

#include <core/r_util/RProjectFile.hpp>
#include <core/r_util/RSourceIndex.hpp>
#include <core/r_util/RPackageInfo.hpp>
#include <core/r_util/RSourceIndex.hpp>

#include <session/projects/SessionProjects.hpp>
#include <session/SessionModuleContext.hpp>

#include "SessionCodeSearch.hpp"

using namespace rstudio::core;

namespace rstudio {
namespace session {
namespace modules {
namespace r_completions {

namespace {

char ends(char begins) {
   switch(begins) {
   case '(': return ')';
   case '[': return ']';
   case '{': return '}';
   }
   return '\0';
}

bool isBinaryOp(char character)
{
   return character == '~' ||
         character == '!' ||
         character == '@' ||
         character == '$' ||
         character == '%' ||
         character == '^' ||
         character == '&' ||
         character == '*' ||
         character == '-' ||
         character == '+' ||
         character == '*' ||
         character == '/' ||
         character == '=' ||
         character == '|' ||
         character == '<' ||
         character == '>' ||
         character == '?';

}

} // end anonymous namespace

std::string finishExpression(const std::string& expression)
{
   std::string result = expression;

   // If the last character of the expression is a binary op, then we
   // place a '.' after it
   int n = expression.length();
   if (isBinaryOp(expression[n - 1]))
      result.append(".");

   std::vector<char> terminators;

   char top = '\0';
   terminators.push_back(top);

   bool in_string = false;
   bool in_escape = false;

   for (int i = 0; i < n; i++) {

      char cur = expression[i];

      if (in_string) {

         if (in_escape) {
            in_escape = false;
            continue;
         }

         if (cur == '\\') {
            in_escape = true;
            continue;
         }

         if (cur != top) {
            continue;
         }

         in_string = false;
         terminators.pop_back();
         top = terminators.back();
         continue;

      }

      if (cur == top) {
         terminators.pop_back();
         top = terminators.back();
      } else if (cur == '(' || cur == '{' || cur == '[') {
         char end = ends(cur);
         top = end;
         terminators.push_back(top);
      } else if (cur == '"' || cur == '`' || cur == '\'') {
         top = cur;
         in_string = true;
         terminators.push_back(top);
      }

   }

   // append to the output
   for (std::size_t i = terminators.size() - 1; i > 0; --i)
      result.push_back(terminators[i]);

   return result;
}

namespace {

SEXP rs_finishExpression(SEXP stringSEXP)
{
   r::sexp::Protect rProtect;
   int n = r::sexp::length(stringSEXP);

   std::vector<std::string> output;
   output.reserve(n);
   for (int i = 0; i < n; ++i)
      output.push_back(finishExpression(CHAR(STRING_ELT(stringSEXP, i))));

   return r::sexp::create(output, &rProtect);
}

struct SourceIndexCompletions {
   std::vector<std::string> completions;
   std::vector<bool> isFunction;
   bool moreAvailable;
};

SourceIndexCompletions getSourceIndexCompletions(const std::string& token)
{
   // get functions from the source index
   std::vector<core::r_util::RSourceItem> items;
   bool moreAvailable = false;

   // TODO: wire up 'moreAvailable'
   modules::code_search::searchSource(token,
                                      1E3,
                                      true,
                                      &items,
                                      &moreAvailable);

   SourceIndexCompletions srcCompletions;
   BOOST_FOREACH(const core::r_util::RSourceItem& item, items)
   {
      if (item.braceLevel() == 0)
      {
         srcCompletions.completions.push_back(item.name());
         srcCompletions.isFunction.push_back(item.isFunction() || item.isMethod());
      }
   }

   srcCompletions.moreAvailable = moreAvailable;
   return srcCompletions;
}

SEXP rs_getSourceIndexCompletions(SEXP tokenSEXP)
{
   r::sexp::Protect protect;
   std::string token = r::sexp::asString(tokenSEXP);
   SourceIndexCompletions srcCompletions = getSourceIndexCompletions(token);

   std::vector<std::string> names;
   names.push_back("completions");
   names.push_back("isFunction");
   names.push_back("moreAvailable");

   SEXP resultSEXP = r::sexp::createList(names, &protect);
   r::sexp::setNamedListElement(resultSEXP, "completions", srcCompletions.completions);
   r::sexp::setNamedListElement(resultSEXP, "isFunction", srcCompletions.isFunction);
   r::sexp::setNamedListElement(resultSEXP, "moreAvailable", srcCompletions.moreAvailable);

   return resultSEXP;
}

bool subsequenceFilter(const FileInfo& fileInfo,
                       const std::string& pattern,
                       int parentPathLength,
                       int maxCount,
                       std::vector<std::string>* pPaths,
                       int* pCount,
                       bool* pMoreAvailable)
{
   if (*pCount >= maxCount)
   {
      *pMoreAvailable = true;
      return false;
   }
   
   bool isSubsequence = string_utils::isSubsequence(
            fileInfo.absolutePath().substr(parentPathLength + 2),
            pattern,
            true);
   
   if (isSubsequence)
   {
      ++*pCount;
      pPaths->push_back(fileInfo.absolutePath());
   }

   // Always add subdirectories
   if (fileInfo.isDirectory())
   {
      return true;
   }
   
   return false;
}

SEXP rs_scanFiles(SEXP pathSEXP,
                  SEXP patternSEXP,
                  SEXP maxCountSEXP)
{
   std::string path = r::sexp::asString(pathSEXP);
   std::string pattern = r::sexp::asString(patternSEXP);
   int maxCount = r::sexp::asInteger(maxCountSEXP);

   FilePath filePath(path);
   FileInfo fileInfo(filePath);
   tree<FileInfo> tree;

   core::system::FileScannerOptions options;
   options.recursive = true;
   options.yield = true;

   // Use a subsequence filter, and bail after too many files
   std::vector<std::string> paths;
   
   int count = 0;
   bool moreAvailable = false;
   options.filter = boost::bind(subsequenceFilter,
                                _1,
                                pattern,
                                path.length(),
                                maxCount,
                                &paths,
                                &count,
                                &moreAvailable);

   Error error = scanFiles(fileInfo, options, &tree);
   if (error)
      return R_NilValue;

   r::sexp::Protect protect;
   r::sexp::ListBuilder builder(&protect);

   builder.add("paths", paths);
   builder.add("more_available", moreAvailable);

   return builder;
}

SEXP rs_isSubsequence(SEXP stringsSEXP, SEXP querySEXP)
{
   std::vector<std::string> strings;
   if (!r::sexp::fillVectorString(stringsSEXP, &strings))
      return R_NilValue;

   std::string query = r::sexp::asString(querySEXP);

   std::vector<bool> result(strings.size());

   std::transform(strings.begin(),
                  strings.end(),
                  result.begin(),
                  boost::bind(
                     string_utils::isSubsequence,
                     _1,
                     query));

   r::sexp::Protect protect;
   return r::sexp::create(result, &protect);

}

SEXP rs_getActiveFrame(SEXP depthSEXP)
{
   int depth = r::sexp::asInteger(depthSEXP);
   RCNTXT* context = r::getGlobalContext();
   for (int i = 0; i < depth; ++i)
   {
      context = context->nextcontext;
      if (context == NULL)
         return R_NilValue;
   }
   return context->cloenv;
}

} // end anonymous namespace

Error initialize() {

   r::routines::registerCallMethod(
            "rs_finishExpression",
            (DL_FUNC) r_completions::rs_finishExpression,
            1);

   r::routines::registerCallMethod(
            "rs_getSourceIndexCompletions",
            (DL_FUNC) r_completions::rs_getSourceIndexCompletions,
            1);

   r::routines::registerCallMethod(
            "rs_scanFiles",
            (DL_FUNC) rs_scanFiles,
            4);

   r::routines::registerCallMethod(
            "rs_isSubsequence",
            (DL_FUNC) rs_isSubsequence,
            2);

   r::routines::registerCallMethod(
            "rs_getActiveFrame",
            (DL_FUNC) rs_getActiveFrame,
            1);
   
   using boost::bind;
   using namespace module_context;
   ExecBlock initBlock;
   initBlock.addFunctions()
         (bind(sourceModuleRFile, "SessionRCompletions.R"));
   return initBlock.execute();
}

} // namespace r_completions
} // namespace modules
} // namespace session
} // namespace rstudio
