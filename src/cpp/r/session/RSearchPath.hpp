/*
 * RSearchPath.hpp
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

#ifndef R_SESSION_SEARCH_PATH_HPP
#define R_SESSION_SEARCH_PATH_HPP

namespace rstudiocore {
   class Error;
   class FilePath;
}

namespace r {
namespace session {
namespace search_path {

rstudiocore::Error save(const rstudiocore::FilePath& statePath);
rstudiocore::Error saveGlobalEnvironment(const rstudiocore::FilePath& statePath);
rstudiocore::Error restore(const rstudiocore::FilePath& statePath);
   
} // namespace search_path
} // namespace session
} // namespace r

#endif // R_SESSION_SEARCH_PATH_HPP 

