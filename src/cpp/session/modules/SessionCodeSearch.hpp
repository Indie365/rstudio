/*
 * SessionCodeSearch.hpp
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

#ifndef SESSION_CODE_SEARCH_HPP
#define SESSION_CODE_SEARCH_HPP

#include <core/r_util/RSourceIndex.hpp>

namespace rscore {
   class Error;
}

namespace session {
namespace modules {
namespace code_search {

void searchSource(const std::string& term,
                  std::size_t maxResults,
                  bool prefixOnly,
                  std::vector<rscore::r_util::RSourceItem>* pItems,
                  bool* pMoreAvailable);

rscore::Error initialize();
   
} // namespace code_search
} // namespace modules
} // namespace session

#endif // SESSION_CODE_SEARCH_HPP

