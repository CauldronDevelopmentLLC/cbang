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

#include <cbang/config.h>

#ifdef HAVE_LEVELDB

#include "LevelDB.h"

#include <cbang/Exception.h>

#include <leveldb/db.h>
#include <leveldb/slice.h>
#include <leveldb/comparator.h>
#include <leveldb/iterator.h>
#include <leveldb/write_batch.h>

#include <cstring>

#undef CBANG_EXCEPTION
#define CBANG_EXCEPTION LevelDBError

using namespace cb;
using namespace std;

namespace {
  const string nsLast =
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
}


string LevelDBNS::nsKey(const string &key) const {return name + key;}


string LevelDBNS::stripKey(const string &key) const {
  if (key.length() < name.length())
    THROW("Invalid key '" << key << "' for namespace '" << name << "'");
  return key.substr(name.length());
}


LevelDB::Snapshot::~Snapshot() {db->ReleaseSnapshot(snapshot);}


int LevelDB::Comparator::operator()(const string &, const string &) const {
  THROW("Must implement one of the compare functions");
}


bool LevelDB::Iterator::valid() const {
  if (!it->Valid()) return false;
  auto &ns = db->getNS();
  if (it->key().size() < ns.size()) return false;
  return memcmp(it->key().data(), ns.data(), ns.size()) == 0;
}


void LevelDB::Iterator::first() {
  if (db->hasNS()) it->Seek(db->getNS());
  else it->SeekToFirst();
}


void LevelDB::Iterator::last() {
  if (db->hasNS()) {
    seek(nsLast);
    if (it->Valid()) it->Prev();
    else it->SeekToLast(); // This is the last NS or DB is empty

  } else it->SeekToLast();
}


void LevelDB::Iterator::seek(const string &key) {it->Seek(db->nsKey(key));}


void LevelDB::Iterator::next() {
  if (!valid()) THROW("Cannot call next() on invalid iterator");
  it->Next();
}


void LevelDB::Iterator::prev() {
  if (!valid()) THROW("Cannot call prev() on invalid iterator");
  it->Prev();
}


string LevelDB::Iterator::key() const {
  if (!valid()) THROW("Cannot call key() on invalid iterator");
  return db->stripKey(it->key().ToString());
}


string LevelDB::Iterator::value() const {
  if (!valid()) THROW("Cannot call value() on invalid iterator");
  return it->value().ToString();
}


LevelDB::Iterator &LevelDB::Iterator::operator=(const Iterator &it) {
  db = it.db;
  this->it = it.it;
  return *this;
}


bool LevelDB::Iterator::operator==(const Iterator &it) const {
  if (!valid() && !it.valid()) return true;
  if (!valid() || !it.valid()) return false;
  return this->it->key().ToString() == it.it->key().ToString();
}


LevelDB::Batch::Batch(
  LevelDB &db, const SmartPointer<leveldb::WriteBatch> &batch,
  const string &name) : LevelDBNS(name), db(db), batch(batch) {}


LevelDB::Batch::~Batch() {}


LevelDB::Batch LevelDB::Batch::ns(const std::string &name) {
  return Batch(db, batch, getNS() + name);
}


void LevelDB::Batch::clear() {batch->Clear();}


void LevelDB::Batch::set(const string &key, const string &value) {
  batch->Put(nsKey(key), value);
}


void LevelDB::Batch::erase(const string &key) {batch->Delete(nsKey(key));}


void LevelDB::Batch::eraseAll(int options) {
  Iterator it = db.iterator(options);
  for (it.first(); it.valid(); it++) erase(it.key());
}


void LevelDB::Batch::commit(int options) {db.commit(*batch, options);}


namespace {
  class LevelDBComparator : public leveldb::Comparator {
  public:
    SmartPointer<LevelDB::Comparator> comparator;

    LevelDBComparator(const SmartPointer<LevelDB::Comparator> &comparator) :
      comparator(comparator) {}

    int Compare(
      const leveldb::Slice &a, const leveldb::Slice &b) const override {
      return (*comparator)(a.data(), a.size(), b.data(), b.size());
    }

    const char *Name() const override {return comparator->getName().c_str();}
    void FindShortestSeparator(
      std::string *, const leveldb::Slice &) const override {}
    void FindShortSuccessor(std::string *) const override {}
  };
}


LevelDB::LevelDB(const SmartPointer<Comparator> &comparator) :
  comparator(comparator.isNull() ? 0 : new LevelDBComparator(comparator)) {}


LevelDB::LevelDB(const string &name,
                 const SmartPointer<Comparator> &comparator) :
  LevelDBNS(name),
  comparator(comparator.isNull() ? 0 : new LevelDBComparator(comparator)) {}


LevelDB::LevelDB(const string &name,
  const SmartPointer<leveldb::Comparator> &comparator,
  const SmartPointer<leveldb::DB> &db, const SmartPointer<Snapshot> &snapshot) :
  LevelDBNS(name), comparator(comparator), db(db), _snapshot(snapshot) {}


LevelDB::~LevelDB() {}


LevelDB LevelDB::ns(const string &name) {
  if (db.isNull()) THROW("DB not open");
  return LevelDB(getNS() + name, comparator, db, _snapshot);
}


LevelDB LevelDB::snapshot() {
  if (db.isNull()) THROW("DB not open");
  return LevelDB(getNS(), comparator, db, new Snapshot(db, db->GetSnapshot()));
}


void LevelDB::open(const string &path, int options) {
  leveldb::Options opts = getOptions(options);
  if (!comparator.isNull()) opts.comparator = comparator.get();

  leveldb::DB *db;
  leveldb::Status status = leveldb::DB::Open(opts, path, &db);
  if (!status.ok())
    THROW("Failed to open DB '" << path << "': " << status.ToString());

  this->db = db;
}


void LevelDB::close() {db.release();}


int LevelDB::compare(const string &keyA, const string &keyB) const {
  if (comparator.isSet()) return comparator->Compare(keyA, keyB);
  return keyA.compare(keyB);
}


bool LevelDB::has(const string &key, int options) const {
  string value;
  leveldb::Status s = db->Get(getReadOptions(options), nsKey(key), &value);
  if (s.IsNotFound()) return false;
  check(s, key);
  return true;
}


string LevelDB::get(const string &key, int options) const {
  string value;
  leveldb::Status s = db->Get(getReadOptions(options), nsKey(key), &value);
  check(s, key);
  return value;
}


string LevelDB::get(const string &key, const string &defaultValue,
                    int options) const {
  string value;
  leveldb::Status s = db->Get(getReadOptions(options), nsKey(key), &value);
  if (s.IsNotFound()) return defaultValue;
  check(s, key);
  return value;
}


void LevelDB::set(const string &key, const string &value, int options) {
  check(db->Put(getWriteOptions(options), nsKey(key), value), key);
}


void LevelDB::erase(const string &key, int options) {
  check(db->Delete(getWriteOptions(options), nsKey(key)), key);
}


void LevelDB::eraseAll(int options) {
  for (auto it = first(options); it.valid(); it++)
    erase(it.key(), options);
}


LevelDB::Iterator LevelDB::iterator(int options) const {
  return Iterator(*this, db->NewIterator(getReadOptions(options)));
}


LevelDB::Iterator LevelDB::first(int options) const {
  auto it = iterator(options);
  it.first();
  return it;
}


LevelDB::Iterator LevelDB::last(int options) const {
  auto it = iterator(options);
  it.last();
  return it;
}


LevelDB::Batch LevelDB::batch() {
  return Batch(*this, new leveldb::WriteBatch, getNS());
}


void LevelDB::commit(leveldb::WriteBatch &batch, int options) {
  check(db->Write(getWriteOptions(options), &batch));
}


string LevelDB::getProperty(const string &name) {
  string value;
  if (!db->GetProperty(name, &value))
    THROW("Invalid property '" << name << "'");
  return value;
}


void LevelDB::compact(const string &begin, const string &end) {
  SmartPointer<leveldb::Slice> beginSlice =
    begin.empty() ? 0 : new leveldb::Slice(begin);
  SmartPointer<leveldb::Slice> endSlice =
    end.empty() ? 0 : new leveldb::Slice(begin);

  db->CompactRange(beginSlice.get(), endSlice.get());
}


leveldb::Options LevelDB::getOptions(int options) const {
  // TODO Support other options

  leveldb::Options opts;

  opts.create_if_missing = options & CREATE_IF_MISSING;
  opts.error_if_exists   = options & ERROR_IF_EXISTS;
  opts.paranoid_checks   = options & PARANOID_CHECKS;
  if (options & NO_COMPRESSION) opts.compression = leveldb::kNoCompression;

  return opts;
}


leveldb::ReadOptions LevelDB::getReadOptions(int options) const {
  leveldb::ReadOptions opts;

  if (_snapshot.isSet()) opts.snapshot = _snapshot->get();
  opts.verify_checksums = options & VERIFY_CHECKSUMS;
  opts.fill_cache       = options & FILL_CACHE;

  return opts;
}


leveldb::WriteOptions LevelDB::getWriteOptions(int options) const {
  return {.sync = bool(options & SYNC)};
}


void LevelDB::check(const leveldb::Status &s, const std::string &key) const {
  if (s.ok()) return;
  if (s.IsNotFound()) THROW("DB ERROR: Key '" << nsKey(key) << "' not found");
  THROW("DB ERROR: " << s.ToString());
}

#endif // HAVE_LEVELDB
