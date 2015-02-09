/*
 * Macros.hpp
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

#ifndef CORE_MACROS_HPP
#define CORE_MACROS_HPP

// re-define this in implementation files for labelled debugging
#ifndef RSTUDIO_DEBUG_LABEL
# define RSTUDIO_DEBUG_LABEL "rstudio"
#endif

#ifndef RSTUDIO_ENABLE_DEBUG_MACROS

# define RSTUDIO_DEBUG(x) do {} while (0)
# define RSTUDIO_DEBUG_LINE(x) do {} while (0)
# define RSTUDIO_DEBUG_BLOCK(x) if (false)

#else

# define RSTUDIO_DEBUG(x)                                                      \
   do                                                                          \
   {                                                                           \
      std::cerr << "(" << RSTUDIO_DEBUG_LABEL << "): "                         \
                << x << std::endl;                                             \
   } while (0)

#define RSTUDIO_DEBUG_LINE(x)                                                  \
   do                                                                          \
   {                                                                           \
      std::string file = std::string(__FILE__);                                \
      std::string shortFile = file.substr(file.rfind("/") + 1);                \
      std::cerr << "(" << RSTUDIO_DEBUG_LABEL << ")"                           \
                << "[" << shortFile << ":" << __LINE__ << "]: " << std::endl   \
                << x << std::endl;                                             \
   } while (0)

# define RSTUDIO_DEBUG_BLOCK if (true)

#endif

#ifndef DEBUG
# define DEBUG RSTUDIO_DEBUG
#endif

#ifndef DEBUG_LINE
# define DEBUG_LINE RSTUDIO_DEBUG_LINE
#endif

#ifndef DEBUG_BLOCK
# define DEBUG_BLOCK RSTUDIO_DEBUG_BLOCK
#endif

#endif
