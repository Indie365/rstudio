/*
 * PosixSystem.cpp
 *
 * Copyright (C) 2022 by Posit Software, PBC
 *
 * Unless you have received this program directly from Posit Software pursuant to the terms of a commercial license agreement
 * with Posit, then this program is licensed to you under the following terms:
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <shared_core/system/PosixSystem.hpp>
#include <csignal>
#include <grp.h>
#include <ifaddrs.h>
#include <memory.h>
#include <netdb.h>
#include <pwd.h>

#ifdef __linux__
#include <sys/prctl.h>
#endif

#include <shared_core/Error.hpp>
#include <shared_core/system/User.hpp>

namespace rstudio {
namespace core {
namespace system {
namespace posix {

namespace {

bool s_privInited = false;
std::string s_privUserName;
uid_t s_privUid;
gid_t s_privGid;


// Cach the root uids/gids to keep restorePrivileges from having to call ::getpwuid. restorePriv is used by
// rserver-pam's onAfterFork callback, where it can use a vendor api (e.g. centrify) that uses locking that
// will fail in a process that has forked while holding a lock that then appears held in the child
//
// Must be called from a single-threaded context like process startup code.
Error initSystemPriv(uid_t in_uid)
{
   if (s_privInited)
   {
       if (in_uid == s_privUid)
          return Success();
   }

   struct passwd* pPrivPasswd = ::getpwuid(in_uid);
   if (pPrivPasswd == nullptr)
   {
      return systemError(errno, ERROR_LOCATION);
   }

   s_privUid = in_uid;
   s_privGid = pPrivPasswd->pw_gid;
   s_privUserName = std::string(pPrivPasswd->pw_name);
   s_privInited = true;

   return Success();
}

Error restorePrivilegesImpl(uid_t in_uid)
{
   // Reset error state.
   errno = 0;

   // Change the effective user to root.
   if (::seteuid(in_uid) < 0)
      return systemError(errno, ERROR_LOCATION);
   // Verify
   if (::geteuid() != 0)
      return systemError(EACCES, ERROR_LOCATION);

   Error error = initSystemPriv(in_uid);
   if (error)
      return error;

   // Supplemental groups
   if (::initgroups(s_privUserName.c_str(), s_privGid) < 0)
      return systemError(errno, ERROR_LOCATION);

   // Set effective group
   if (::setegid(s_privGid) < 0)
      return systemError(errno, ERROR_LOCATION);
   // Verify
   if (::getegid() != s_privGid)
      return systemError(EACCES, ERROR_LOCATION);

   return Success();

}

} // anonymous namespace

Error enableCoreDumps()
{
#ifdef __linux__
   int res = ::prctl(PR_SET_DUMPABLE, 1);
   if (res == -1)
      return systemError(errno, ERROR_LOCATION);
#endif

   return Success();
}

std::string getEnvironmentVariable(const std::string& in_name)
{
   char* value = ::getenv(in_name.c_str());
   if (value)
      return std::string(value);

   return std::string();
}

Error getIpAddresses(std::vector<IpAddress>& out_addresses, bool in_includeIPv6)
{
    // get addrs
    struct ifaddrs* pAddrs;
    if (::getifaddrs(&pAddrs) == -1)
        return systemError(errno, ERROR_LOCATION);

    // iterate through the linked list
    for (struct ifaddrs* pAddr = pAddrs; pAddr != nullptr; pAddr = pAddr->ifa_next)
    {
        if (pAddr->ifa_addr == nullptr)
            continue;

        // filter out non-ip addresses
        sa_family_t family = pAddr->ifa_addr->sa_family;
        bool filterAddr = in_includeIPv6 ? (family != AF_INET && family != AF_INET6) : (family != AF_INET);
        if (filterAddr)
            continue;

        char host[NI_MAXHOST];
        if (::getnameinfo(pAddr->ifa_addr,
                          (family == AF_INET) ? sizeof(struct sockaddr_in) :
                          sizeof(struct sockaddr_in6),
                          host, NI_MAXHOST,
                          nullptr, 0, NI_NUMERICHOST) != 0)
        {
            log::logError(systemError(errno, ERROR_LOCATION));
            continue;
        }

        struct IpAddress addr;
        addr.Name = pAddr->ifa_name;
        addr.Address = host;
        out_addresses.push_back(addr);
    }

    // free them and return success
    ::freeifaddrs(pAddrs);
    return Success();
}

Error ignoreSignal(int in_signal)
{
   struct sigaction sa;
   ::memset(&sa, 0, sizeof(sa));

   sa.sa_handler = SIG_IGN;
   int result = ::sigaction(in_signal, &sa, nullptr);
   if (result != 0)
   {
      Error error = systemError(result, ERROR_LOCATION);
      error.addProperty("signal", in_signal);
      return error;
   }

   return Success();
}

bool realUserIsRoot()
{
   return ::getuid() == 0;
}

Error restoreRoot()
{
   return restorePrivilegesImpl(0);
}

// privilege manipulation for systems that support setresuid/getresuid
#if defined(HAVE_SETRESUID)

Error restorePrivileges()
{
   // reset error state
   errno = 0;

   // set user
   uid_t ruid, euid, suid;
   if (::getresuid(&ruid, &euid, &suid) < 0)
      return systemError(errno, ERROR_LOCATION);
   if (::setresuid(-1, suid, -1) < 0)
      return systemError(errno, ERROR_LOCATION);
   // verify
   if (::geteuid() != suid)
      return systemError(EACCES, ERROR_LOCATION);

   Error error = initSystemPriv(suid);
   if (error)
      return error;

   // supplemental groups
   if (::initgroups(s_privUserName.c_str(), s_privGid) < 0)
      return systemError(errno, ERROR_LOCATION);

   // set group
   gid_t rgid, egid, sgid;
   if (::getresgid(&rgid, &egid, &sgid) < 0)
      return systemError(errno, ERROR_LOCATION);
   if (::setresgid(-1, sgid, -1) < 0)
      return systemError(errno, ERROR_LOCATION);
   // verify
   if (::getegid() != sgid)
      return systemError(EACCES, ERROR_LOCATION);

   // success
   return Success();
}

Error temporarilyDropPrivileges(const User& in_user, const boost::optional<GidType>& in_group)
{
   // clear error state
   errno = 0;

   // the starting uid
   uid_t privUid = ::geteuid();

   Error error = initSystemPriv(privUid);
   if (error)
      return error;

   GidType targetGID = in_group.or_else(in_user.getGroupId());

   // init supplemental group list
   // NOTE: if porting to CYGWIN may need to call getgroups/setgroups
   // after initgroups -- more research required to confirm
   // NOTE: We are intentionally specifying the user's primary group here
   // regardless of whether an alternate group is provided. This so all of
   // the user's groups are maintained for the new process. Initializing
   // with the alternate group results in the process running with only a single
   // group.
   if (::initgroups(in_user.getUsername().c_str(), in_user.getGroupId()) < 0)
      return systemError(errno, ERROR_LOCATION);

   // set group and verify
   if (::setresgid(-1, targetGID, ::getegid()) < 0)
      return systemError(errno, ERROR_LOCATION);
   if (::getegid() != targetGID)
      return systemError(EACCES, ERROR_LOCATION);

   // set user and verify
   if (::setresuid(-1, in_user.getUserId(), ::geteuid()) < 0)
      return systemError(errno, ERROR_LOCATION);
   if (::geteuid() != in_user.getUserId())
      return systemError(EACCES, ERROR_LOCATION);

   // success
   return Success();
}

// privilege manipulation for systems that don't support setresuid/getresuid
#else

Error restorePrivileges()
{
   return restorePrivilegesImpl(s_privUid);
}

Error temporarilyDropPrivileges(const User& in_user, const boost::optional<GidType>& in_group)
{
   // clear error state
   errno = 0;

   GidType targetGID = in_group.value_or(in_user.getGroupId());

   // init supplemental group list
   // NOTE: We are intentionally specifying the user's primary group here
   // regardless of whether an alternate group is provided. This so all of
   // the user's groups are maintained for the new process. Initializing
   // with the alternate group results in the process running with only a single
   // group.
   if (::initgroups(in_user.getUsername().c_str(), in_user.getGroupId()) < 0)
      return systemError(errno, ERROR_LOCATION);

   // set group

   // save old EGUID
   gid_t oldEGUID = ::getegid();

   // copy EGUID to SGID
   if (::setregid(::getgid(), oldEGUID) < 0)
      return systemError(errno, ERROR_LOCATION);

   // set new EGID
   if (::setegid(targetGID) < 0)
      return systemError(errno, ERROR_LOCATION);

   // verify
   if (::getegid() != targetGID)
      return systemError(EACCES, ERROR_LOCATION);


   // set user

   // save old EUID
   uid_t oldEUID = ::geteuid();

   // copy EUID to SUID
   if (::setreuid(::getuid(), oldEUID) < 0)
      return systemError(errno, ERROR_LOCATION);

   // set new EUID
   if (::seteuid(in_user.getUserId()) < 0)
      return systemError(errno, ERROR_LOCATION);

   // verify
   if (::geteuid() != in_user.getUserId())
      return systemError(EACCES, ERROR_LOCATION);

   // save privileged user id and gid info
   initSystemPriv(s_privUid);

   // success
   return Success();
}

#endif

} // namespace posix
} // namespace system
} // namespace core
} // namespace rstudio
