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

#include <cbang/String.h>
#include <cbang/openssl/Digest.h>
#include <cbang/Catch.h>

using namespace std;
using namespace cb;


int main(int argc, char *argv[]) {
  // Test data from:
  // http://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-query-string-auth.html

  try {
    string key = "wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY";
    key = Digest::signHMAC("AWS4" + key, "20130524", "sha256");
    key = Digest::signHMAC(key, "us-east-1", "sha256");
    key = Digest::signHMAC(key, "s3", "sha256");
    key = Digest::signHMAC(key, "aws4_request", "sha256");

    string data =
      "AWS4-HMAC-SHA256\n"
      "20130524T000000Z\n"
      "20130524/us-east-1/s3/aws4_request\n"
      "3bfa292879f6447bbcda7001decf97f4a54dc650c8942174ae0a9121cf58ad04";

    string sig = Digest::signHMAC(key, data, "sha256");

    cout << "sig=" << String::hexEncode(sig) << endl;

    return 0;
  } CATCH_ERROR;

  return 1;
}
