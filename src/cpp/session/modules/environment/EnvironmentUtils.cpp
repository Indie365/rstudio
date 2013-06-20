/*
 * EnvironmentUtils.cpp
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

#include "EnvironmentUtils.hpp"

#include <r/RExec.hpp>

using namespace core;

namespace session {
namespace modules {
namespace environment {
namespace {

json::Value classOfVar(SEXP var)
{
   std::string value;
   Error error = r::exec::RFunction(".rs.getSingleClass",
                                    var).call(&value);
   if (error)
   {
      LOG_ERROR(error);
      return json::Value(); // return null
   }
   else
   {
      return value;
   }
}

json::Value valueOfVar(SEXP var)
{
   std::string value;
   Error error = r::exec::RFunction(".rs.valueAsString",
                                    var).call(&value);
   if (error)
   {
      LOG_ERROR(error);
      return json::Value(); // return null
   }
   else
   {
      return value;
   }
}

json::Value descriptionOfVar(SEXP var)
{
   std::string value;
   Error error = r::exec::RFunction(
            isUnevaluatedPromise(var) ||
            r::sexp::isLanguage(var) ?
                  ".rs.languageDescription" :
                  ".rs.valueDescription",
               var).call(&value);
   if (error)
   {
      LOG_ERROR(error);
      return json::Value(); // return null
   }
   else
   {
      return value;
   }
}

json::Array contentsOfVar(SEXP var)
{
   std::vector<std::string> value;
   Error error = r::exec::RFunction(".rs.valueContents", var).call(&value);
   if (error)
   {
      LOG_ERROR(error);
      return json::Array(); // return null
   }
   else
   {
      return json::toJsonArray(value);
   }
}

} // anonymous namespace

// a variable is an unevaluated promise if its promise value is still unbound
bool isUnevaluatedPromise (SEXP var)
{
   return (TYPEOF(var) == PROMSXP) && (PRVALUE(var) == R_UnboundValue);
}

json::Object varToJson(const r::sexp::Variable& var)
{
   json::Object varJson;
   varJson["name"] = var.first;
   SEXP varSEXP = var.second;

   // is this a type of object for which we can get something that looks like
   // a value? if so, get the value appropriate to the object's class.
   if ((varSEXP != R_UnboundValue) &&
       (varSEXP != R_MissingArg) &&
       !r::sexp::isLanguage(varSEXP) &&
       !isUnevaluatedPromise(varSEXP))
   {
      json::Value varClass = classOfVar(varSEXP);
      varJson["type"] = varClass;
      varJson["value"] = valueOfVar(varSEXP);
      varJson["description"] = descriptionOfVar(varSEXP);
      varJson["length"] = r::sexp::length(varSEXP);
      if (varClass == "data.frame" ||
          varClass == "data.table" ||
          varClass == "list" ||
          varClass == "cast_df" ||
          Rf_isS4(varSEXP))
      {
         varJson["contents"] = contentsOfVar(varSEXP);
      }
      else
      {
         varJson["contents"] = json::Array();
      }
   }
   // this is not a type of object for which we can get a value; describe
   // what we can and stub out the rest.
   else
   {
      if (r::sexp::isLanguage((varSEXP)))
      {
         varJson["type"] = std::string("language");
      }
      else if (isUnevaluatedPromise(varSEXP))
      {
         varJson["type"] = std::string("promise");
      }
      else
      {
         varJson["type"] = std::string("unknown");
      }
      varJson["value"] = (isUnevaluatedPromise(varSEXP) ||
                          r::sexp::isLanguage(varSEXP) ||
                          varSEXP == R_MissingArg) ?
               descriptionOfVar(varSEXP) :
               std::string("<unknown>");
      varJson["description"] = std::string("");
      varJson["contents"] = json::Array();
      varJson["length"] = 0;
   }
   return varJson;
}

} // namespace environment
} // namespace modules
} // namespace session
