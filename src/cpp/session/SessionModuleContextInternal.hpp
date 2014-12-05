/*
 * SessionModuleContextInternal.hpp
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

#ifndef SESSION_MODULE_CONTEXT_INTERNAL_HPP
#define SESSION_MODULE_CONTEXT_INTERNAL_HPP

#include <session/SessionModuleContext.hpp>

namespace rscore {
   class Error;
   class FilePath;
   class Settings;
}

namespace r {
namespace session {
   struct RSuspendOptions;
}
}

namespace session {   
namespace module_context {
 
// initialize
rscore::Error initialize();
      
// suspend and resume

void onSuspended(const r::session::RSuspendOptions& options,
                 rscore::Settings* pPersistentState);
void onResumed(const rscore::Settings& persistentState);

// notify of backgound processing
void onBackgroundProcessing(bool isIdle);

// source diagnostics
rscore::FilePath sourceDiagnostics();

} // namespace module_context
} // namespace session

#endif // SESSION_MODULE_CONTEXT_INTERNAL_HPP

