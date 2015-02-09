/*
 * SessionAsyncRProcess.cpp
 *
 * Copyright (C) 2009-14 by RStudio, Inc.
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

#include <session/SessionConsoleProcess.hpp>
#include <session/SessionModuleContext.hpp>

#include <core/system/Environment.hpp>
#include <core/system/Process.hpp>

#include <session/SessionAsyncRProcess.hpp>

namespace rstudio {
namespace session {
namespace async_r {

AsyncRProcess::AsyncRProcess():
   isRunning_(false),
   terminationRequested_(false)
{
}

void AsyncRProcess::start(const char* rCommand, 
                          const core::FilePath& workingDir, 
                          AsyncRProcessOptions rOptions,
                          const std::vector<core::FilePath>& rSourceFiles)
{
   // R binary
   core::FilePath rProgramPath;
   core::Error error = module_context::rScriptPath(&rProgramPath);
   if (error)
   {
      LOG_ERROR(error);
      onCompleted(EXIT_FAILURE);
      return;
   }

   // args
   std::vector<std::string> args;
   args.push_back("--slave");
   if (rOptions & R_PROCESS_VANILLA)
      args.push_back("--vanilla");
   args.push_back("-e");

   bool needsQuote = false;

   // On Windows, we turn the vector of strings into a single
   // string to send over the command line, so we must ensure
   // that the arguments following '-e' are quoted, so that
   // they are all interpretted as a single argument (rather
   // than multiple arguments) to '-e'.

#ifdef _WIN32
   needsQuote = strlen(rCommand) > 0 && rCommand[0] != '"';
#endif

   std::stringstream command;
   if (needsQuote)
      command << "\"";

   if (rSourceFiles.size())
   {
      // Use shims for the main RStudio functions
      command << "options(error = traceback); ";

      command << ".rs.Env <- attach(NULL, name = 'tools:rstudio'); ";

      command << ".rs.addFunction <- function(name, FN, attrs = list()) {"
              << "   fullName = paste('.rs.', name, sep=''); "
              << "   for (attrib in names(attrs)) "
              << "     attr(FN, attrib) <- attrs[[attrib]];"
              << "   assign(fullName, FN, .rs.Env); "
              << "   environment(.rs.Env[[fullName]]) <- .rs.Env; "
              << "};";

      // similarly for .rs.addJsonRpcHandler
      command << ".rs.addJsonRpcHandler <- function(name, FN) {"
              << "   .rs.addFunction(paste('rpc.', name, sep = ''), FN, TRUE);"
              << "};";

      // add in the r source files requested
      for (std::vector<core::FilePath>::const_iterator it = rSourceFiles.begin();
           it != rSourceFiles.end();
           ++it)
         command << "source('" << it->absolutePath() << "');";
      
      command << rCommand;
   }
   else
   {
      command << rCommand;
   }

   if (needsQuote)
      command << "\"";

   args.push_back(command.str());

   // options
   core::system::ProcessOptions options;
   options.terminateChildren = true;
   if (rOptions & R_PROCESS_REDIRECTSTDERR)
      options.redirectStdErrToStdOut = true;

   // if a working directory was specified, use it
   if (!workingDir.empty())
   {
      options.workingDir = workingDir;
   }

   // forward R_LIBS so the child process has access to the same libraries
   // we do
   core::system::Options childEnv;
   core::system::environment(&childEnv);
   std::string libPaths = module_context::libPathsString();
   if (!libPaths.empty())
   {
      core::system::setenv(&childEnv, "R_LIBS", libPaths);
      options.environment = childEnv;
   }

   core::system::ProcessCallbacks cb;
   using namespace module_context;
   cb.onContinue = boost::bind(&AsyncRProcess::onContinue,
                               AsyncRProcess::shared_from_this());
   cb.onStdout = boost::bind(&AsyncRProcess::onStdout,
                             AsyncRProcess::shared_from_this(),
                             _2);
   cb.onStderr = boost::bind(&AsyncRProcess::onStderr,
                             AsyncRProcess::shared_from_this(),
                             _2);
   cb.onExit =  boost::bind(&AsyncRProcess::onProcessCompleted,
                             AsyncRProcess::shared_from_this(),
                             _1);
   error = module_context::processSupervisor().runProgram(
            rProgramPath.absolutePath(),
            args,
            options,
            cb);
   if (error)
   {
      LOG_ERROR(error);
      onCompleted(EXIT_FAILURE);
}
else
{
   isRunning_ = true;
}
}

void AsyncRProcess::onStdout(const std::string& output)
{
   // no-op stub for optional implementation by derived classees
}

void AsyncRProcess::onStderr(const std::string& output)
{
   // no-op stub for optional implementation by derived classees
}

bool AsyncRProcess::onContinue()
{
   return !terminationRequested_;
}

void AsyncRProcess::onProcessCompleted(int exitStatus)
{
   markCompleted();
   onCompleted(exitStatus);
}

bool AsyncRProcess::isRunning()
{
   return isRunning_;
}

void AsyncRProcess::terminate()
{
   terminationRequested_ = true;
}

void AsyncRProcess::markCompleted() 
{
   isRunning_ = false;
}

AsyncRProcess::~AsyncRProcess()
{
}

} // namespace async_r
} // namespace session
} // namespace rstudio

