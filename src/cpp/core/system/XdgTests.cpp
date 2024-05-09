/*
 * XdgTests.cpp
 *
 * Copyright (C) 2022 by Posit Software, PBC
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

#include <tests/TestThat.hpp>

#include <boost/filesystem.hpp>

#include <core/Log.hpp>
#include <core/system/System.hpp>
#include <core/system/Xdg.hpp>

#include <core/system/Environment.hpp>


namespace rstudio {
namespace core {
namespace system {
namespace xdg {
namespace tests {

namespace {

boost::optional<std::string> s_defaultUser("default");
boost::optional<FilePath>    s_defaultHome("/tmp/default");

} // end anonymous namespace

#ifndef _WIN32

test_context("XDG")
{
   test_that("XDG directories resolve as expected")
   {
      FilePath homePath(core::system::getenv("HOME"));
      
      expect_equal(userConfigDir(), homePath.completeChildPath(".config/rstudio"));
      expect_equal(userConfigDir(s_defaultUser), homePath.completeChildPath(".config/rstudio"));
      expect_equal(userConfigDir(s_defaultUser, s_defaultHome), FilePath("/tmp/default/.config/rstudio"));
      
      expect_equal(userDataDir(), homePath.completeChildPath(".local/share/rstudio"));
      expect_equal(userDataDir(s_defaultUser), homePath.completeChildPath(".local/share/rstudio"));
      expect_equal(userDataDir(s_defaultUser, s_defaultHome), FilePath("/tmp/default/.local/share/rstudio"));
 
      expect_equal(userCacheDir(), homePath.completeChildPath(".cache/rstudio"));
      expect_equal(userCacheDir(s_defaultUser), homePath.completeChildPath(".cache/rstudio"));
      expect_equal(userCacheDir(s_defaultUser, s_defaultHome), FilePath("/tmp/default/.cache/rstudio"));
   }
   
   test_that("XDG overrides work as expected")
   {
      {
         EnvironmentScope scope("RSTUDIO_CONFIG_HOME", "/tmp/rstudio/config");
         expect_equal(userConfigDir(), FilePath("/tmp/rstudio/config"));
         expect_equal(userConfigDir(s_defaultUser), FilePath("/tmp/rstudio/config"));
         expect_equal(userConfigDir(s_defaultUser, s_defaultHome), FilePath("/tmp/rstudio/config"));
      }
      
      {
         EnvironmentScope scope("RSTUDIO_DATA_HOME", "/tmp/rstudio/data");
         expect_equal(userDataDir(), FilePath("/tmp/rstudio/data"));
         expect_equal(userDataDir(s_defaultUser), FilePath("/tmp/rstudio/data"));
         expect_equal(userDataDir(s_defaultUser, s_defaultHome), FilePath("/tmp/rstudio/data"));
      }
      
      {
         EnvironmentScope scope("RSTUDIO_CACHE_HOME", "/tmp/rstudio/cache");
         expect_equal(userCacheDir(), FilePath("/tmp/rstudio/cache"));
         expect_equal(userCacheDir(s_defaultUser), FilePath("/tmp/rstudio/cache"));
         expect_equal(userCacheDir(s_defaultUser, s_defaultHome), FilePath("/tmp/rstudio/cache"));
      }
   }
   
   test_that("RStudio uses existing rstudio directory in XDG_CONFIG_DIRS")
   {
      // set up XDG directories in tempdir
      char templateString[] = "/tmp/rstudio-XXXXXX";
      char* testDir = ::mkdtemp(templateString);
      std::string xdgConfigA = fmt::format("{}/xdg-a", testDir);
      std::string xdgConfigB = fmt::format("{}/xdg-b", testDir);
      std::string xdgConfigDirs = fmt::format("{}:{}", xdgConfigA, xdgConfigB);
      
      {
         EnvironmentScope scope("XDG_CONFIG_DIRS", xdgConfigDirs.c_str());
         // None of the XDG_CONFIG_DIRS exist; use fallback default
         expect_equal(systemConfigDir(), FilePath("/etc/rstudio"));
         
         // An rstudio directory exists in the XDG_CONFIG_DIRS path; use it
         CHECK(boost::filesystem::create_directories(xdgConfigB + "/rstudio"));
         CHECK(systemConfigDir() == FilePath(xdgConfigB + "/rstudio"));
         
         // An rstudio directory exists in the XDG_CONFIG_DIRS path; use it
         CHECK(boost::filesystem::create_directories(xdgConfigA + "/rstudio"));
         CHECK(systemConfigDir() == FilePath(xdgConfigA + "/rstudio"));
      }
      
      // clean up
      boost::filesystem::remove_all(testDir);
   }
}

#endif

} // end namespace tests
} // end namespace xdg
} // end namespace system
} // end namespace core
} // end namespace rstudio
