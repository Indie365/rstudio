/*
 * SessionAutomation.cpp
 *
 * Copyright (C) 2024 by Posit Software, PBC
 *
 * Unless you have received this program directly from Posit Software pursuant
 * to the terms of a commercial license agreement with Posit Software, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include "SessionAutomation.hpp"

#include <boost/bind.hpp>

#include <shared_core/Error.hpp>

#include <core/Exec.hpp>

#include <r/RExec.hpp>

#include <session/SessionModuleContext.hpp>

using namespace rstudio;
using namespace rstudio::core;
using namespace boost::placeholders;

namespace rstudio {
namespace session {
namespace modules {
namespace automation {

namespace {

} // end anonymous namespace

Error run()
{
   SEXP resultSEXP = R_NilValue;
   r::sexp::Protect protect;
   return r::exec::evaluateString(".rs.automation.run()", &resultSEXP, &protect);
}

Error initialize()
{
   using namespace module_context;
   
   ExecBlock initBlock;
   initBlock.addFunctions()
      (boost::bind(sourceModuleRFile, "SessionAutomation.R"))
      (boost::bind(sourceModuleRFile, "SessionAutomationClient.R"))
      (boost::bind(sourceModuleRFile, "SessionAutomationRemote.R"))
      (boost::bind(sourceModuleRFile, "SessionAutomationTargets.R"));
   
   Error error = initBlock.execute();
   if (error)
      LOG_ERROR(error);
   
   return Success();
   
}

} // end namespace automation
} // end namespace modules
} // end namespace session
} // end namespace rstudio
