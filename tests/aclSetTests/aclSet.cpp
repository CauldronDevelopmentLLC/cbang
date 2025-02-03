/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
                Copyright (c) 2003-2021, Cauldron Development LLC
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#include <cbang/Exception.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/util/ACLSet.h>

#include <exception>
#include <iostream>
#include <iomanip>

using namespace cb;
using namespace std;


void usage(const char *name) {
  cout
    << "Usage: " << name << " [OPTIONS]\n\n"
    << "OPTIONS:\n"
    << "\t--help                           Print this help screen and exit.\n"
    << "\t--stdin                          Read ACL set from standard input.\n"
    << "\t--read <file>                    Read ACL set from a file.\n"
    << "\t--add-user <user>                Add a user.\n"
    << "\t--del-user <user>                Delete a user.\n"
    << "\t--add-group <group>              Add a group.\n"
    << "\t--del-group <group>              Delete a group.\n"
    << "\t--group-add-user <group> <user>  Add user to group.\n"
    << "\t--group-del-user <group> <user>  Delete user from group.\n"
    << "\t--add-acl <path>                 Add an ACL.\n"
    << "\t--del-acl <path>                 Delete an ACL.\n"
    << "\t--acl-add-user <path> <user>     Add user to ACL.\n"
    << "\t--acl-del-user <path> <user>     Delete user from ACL.\n"
    << "\t--acl-add-group <path> <group>   Add group to ACL.\n"
    << "\t--acl-del-group <path> <group>   Delete group from ACL.\n"
    << "\t--allow <path> <user>            Return 0 if user is allowed to \n"
    << "\t                                 access path, 1 otherwise.\n"
    << "\t--show-allow <path> <user>       Print to screen if user is allowed\n"
    << "\t                                 to access path.\n"
    << endl;
}


int main(int argc, char *argv[]) {
  try {
    ACLSet aclSet;

    for (int i = 1; i < argc; i++) {
      string arg = argv[i];

      if (arg == "--help") {
        usage(argv[0]);
        return 0;

      } else if (arg == "--stdin") {
        cin >> aclSet;

      } else if (arg == "--read" && i < argc - 1) {
        *SystemUtilities::iopen(argv[++i]) >> aclSet;

      } else if (arg == "--add-user" && i < argc - 1) {
        aclSet.addUser(argv[++i]);

      } else if (arg == "--del-user" && i < argc - 1) {
        aclSet.delUser(argv[++i]);

      } else if (arg == "--add-group" && i < argc - 1) {
        aclSet.addGroup(argv[++i]);

      } else if (arg == "--del-group" && i < argc - 1) {
        aclSet.delGroup(argv[++i]);

      } else if (arg == "--group-add-user" && i < argc - 2) {
        aclSet.groupAddUser(argv[i + 1], argv[i + 2]);
        i += 2;

      } else if (arg == "--group-del-user" && i < argc - 2) {
        aclSet.groupDelUser(argv[i + 1], argv[i + 2]);
        i += 2;

      } else if (arg == "--add-acl" && i < argc - 1) {
        aclSet.addACL(argv[++i]);

      } else if (arg == "--del-acl" && i < argc - 1) {
        aclSet.delACL(argv[++i]);

      } else if (arg == "--acl-add-user" && i < argc - 2) {
        aclSet.aclAddUser(argv[i + 1], argv[i + 2]);
        i += 2;

      } else if (arg == "--acl-del-user" && i < argc - 2) {
        aclSet.aclDelUser(argv[i + 1], argv[i + 2]);
        i += 2;

      } else if (arg == "--acl-add-group" && i < argc - 2) {
        aclSet.aclAddGroup(argv[i + 1], argv[i + 2]);
        i += 2;

      } else if (arg == "--acl-del-group" && i < argc - 2) {
        aclSet.aclDelGroup(argv[i + 1], argv[i + 2]);
        i += 2;

      } else if (arg == "--allow" && i < argc - 2) {
        return aclSet.allow(argv[i + 1], argv[i + 2]) ? 0 : 1;

      } else if (arg == "--show-allow" && i < argc - 2) {
        cout << "allow(" << argv[i + 1] << ", " << argv[i + 2] << ")="
             << (aclSet.allow(argv[i + 1], argv[i + 2]) ? "true" : "false")
             << '\n';
        i += 2;

      } else {
        usage(argv[0]);
        THROWS("Invalid arg '" << arg << "'");
      }
    }

    cout << aclSet << flush;

    return 0;

  } catch (const Exception &e) {
    cerr << "Exception: " << e << endl;

  } catch (const std::exception &e) {
    cerr << "std::exception: " << e.what() << endl;
  }

  return 1;
}

