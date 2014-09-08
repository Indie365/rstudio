/*
 * ServerAuthHandler.hpp
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

#ifndef SERVER_AUTH_HANDLER_HPP
#define SERVER_AUTH_HANDLER_HPP

#include <string>

#include <boost/function.hpp>

#include <core/http/UriHandler.hpp>
#include <core/http/AsyncUriHandler.hpp>

#include <server/auth/ServerSecureUriHandler.hpp>

namespace server {
namespace auth {
namespace handler {

// uri constants
extern const char * const kSignIn;
extern const char * const kSignOut;
extern const char * const kRefreshCredentialsAndContinue;

// functions which can be called on the handler directly
std::string getUserIdentifier(const rstudiocore::http::Request& request);

std::string userIdentifierToLocalUsername(const std::string& userIdentifier);

bool mainPageFilter(const rstudiocore::http::Request& request,
                    rstudiocore::http::Response* pResponse);

void signInThenContinue(const rstudiocore::http::Request& request,
                        rstudiocore::http::Response* pResponse);

// Special uri handler which attempts to refresh the user's
// credentials then continues on to the originally requested
// URI (or to a special override URI if specified). if the
// auth back-end doesn't support this behavior then it should
// redirect to the sign-in page
void refreshCredentialsThenContinue(
      boost::shared_ptr<rstudiocore::http::AsyncConnection> pConnection);


// functions which must be provided by an auth handler
struct Handler
{
   boost::function<std::string(const rstudiocore::http::Request&)> getUserIdentifier;
   boost::function<std::string(const std::string&)>
                                                userIdentifierToLocalUsername;
   rstudiocore::http::UriFilterFunction mainPageFilter;
   rstudiocore::http::UriHandlerFunction signInThenContinue;
   rstudiocore::http::AsyncUriHandlerFunction refreshCredentialsThenContinue;
   rstudiocore::http::AsyncUriHandlerFunction updateCredentials;
   rstudiocore::http::UriHandlerFunction signIn;
   rstudiocore::http::UriHandlerFunction signOut;

   boost::function<void(const rstudiocore::http::Request&,
                        const std::string&,
                        bool,
                        rstudiocore::http::Response*)> setSignInCookies;
};

// register the auth handler
void registerHandler(const Handler& handler);

// is there a handler already registered?
bool isRegistered();

// set sign in cookies
bool canSetSignInCookies();
void setSignInCookies(const rstudiocore::http::Request& request,
                      const std::string& username,
                      bool persist,
                      rstudiocore::http::Response* pResponse);

// sign out
void signOut(const rstudiocore::http::Request& request,
             rstudiocore::http::Response* pResponse);

} // namespace handler
} // namespace auth
} // namespace server

#endif // SERVER_AUTH_HANDLER_HPP


