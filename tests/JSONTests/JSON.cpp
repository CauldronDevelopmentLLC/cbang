#include <cbang/util/DefaultCatch.h>

#include <cbang/json/Value.h>
#include <cbang/json/Reader.h>
#include <cbang/json/YAMLReader.h>

#include <iostream>

using namespace std;
using namespace cb::JSON;


int main(int argc, char *argv[]) {
  try {
    ValuePtr data;

    if (argc == 2 && string(argv[1]) == "--yaml") {
      YAMLReader reader(cin);
      YAMLReader::docs_t docs;

      reader.parse(docs);
      for (unsigned i = 0; i < docs.size(); i++) {
        if (i) cout << '\n';
        cout << *docs[i];
      }

    } else {
      Reader reader(cin);
      data = reader.parse();
      if (!data.isNull()) cout << *data;
    }

    return 0;

  } CBANG_CATCH_ERROR;
  return 0;
}
