/*
 * SessionSynctex.hpp
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

#ifndef SESSION_MODULES_TEX_SYNCTEX_HPP
#define SESSION_MODULES_TEX_SYNCTEX_HPP

#include <core/json/Json.hpp>

namespace rstudiocore {
   class Error;
   class FilePath;
}
 
namespace session {
namespace modules { 
namespace tex {
namespace synctex {

// returns an object suitable for jnsi binding back into a PdfLocation
// (or null if the search didn't succeed)
rstudiocore::Error forwardSearch(const rstudiocore::FilePath& rootDocument,
                          const rstudiocore::json::Object& sourceLocation,
                          rstudiocore::json::Value* pPdfLocation);

rstudiocore::Error initialize();

} // namespace synctex
} // namespace tex
} // namespace modules
} // namesapce session

#endif // SESSION_MODULES_TEX_SYNCTEX_HPP
