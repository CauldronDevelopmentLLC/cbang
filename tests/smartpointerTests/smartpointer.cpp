// test_smartpointer.cpp
// Comprehensive tests for SmartPointer / RefCounter
// Returns 0 if all selected tests pass, 1 otherwise.
// Usage:
//   ./test_smartpointer            -- run all tests
//   ./test_smartpointer <name>     -- run one named test
//   ./test_smartpointer --list     -- list all test names

#include <cbang/SmartPointer.h>

#include <atomic>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace cb;

// ---------------------------------------------------------------------------
// Test framework
// ---------------------------------------------------------------------------
static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg)                                                  \
  do {                                                                    \
    if (cond) { ++g_pass; }                                               \
    else {                                                                \
      ++g_fail;                                                           \
      std::cerr << "  FAIL [line " << __LINE__ << "]: " << msg << "\n";  \
    }                                                                     \
  } while(0)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
struct Tracker {
  static std::atomic<int> live;
  static std::atomic<int> total;
  int id;
  Tracker()                : id(++total) { ++live; }
  Tracker(const Tracker &) : id(++total) { ++live; }
  ~Tracker() { --live; }
  static void reset() { live = 0; total = 0; }
};
std::atomic<int> Tracker::live{0};
std::atomic<int> Tracker::total{0};

struct TrackerRC : public RefCounted {
  static std::atomic<int> live;
  static std::atomic<int> total;
  int id;
  TrackerRC()  : id(++total) { ++live; }
  ~TrackerRC() { --live; }
  static void reset() { live = 0; total = 0; }
};
std::atomic<int> TrackerRC::live{0};
std::atomic<int> TrackerRC::total{0};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

bool test_basic_lifetime() {
  Tracker::reset();
  { SmartPointer<Tracker> p = new Tracker;
    CHECK(Tracker::live == 1,   "one live after construction");
    CHECK(p.getRefCount() == 1, "ref count == 1");
    CHECK(p.isSet(),            "isSet"); }
  CHECK(Tracker::live == 0, "destroyed after scope exit");
  return true;
}

bool test_copy_construction() {
  Tracker::reset();
  { SmartPointer<Tracker> a = new Tracker;
    SmartPointer<Tracker> b = a;
    CHECK(a.getRefCount() == 2, "refcount 2 after copy");
    CHECK(Tracker::live == 1,   "still one object"); }
  CHECK(Tracker::live == 0, "destroyed after both out of scope");
  return true;
}

bool test_assignment() {
  Tracker::reset();
  { SmartPointer<Tracker> a = new Tracker;
    SmartPointer<Tracker> b;
    b = a;
    CHECK(b.getRefCount() == 2, "refcount 2 after assign");
    a.release();
    CHECK(b.getRefCount() == 1, "refcount 1 after release of a");
    CHECK(Tracker::live == 1,   "object still alive"); }
  CHECK(Tracker::live == 0, "object destroyed");
  return true;
}

bool test_self_assignment() {
  Tracker::reset();
  { SmartPointer<Tracker> a = new Tracker;
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"
#endif
    a = a;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
    CHECK(a.getRefCount() == 1, "refcount unchanged after self-assign");
    CHECK(Tracker::live == 1,   "object still alive"); }
  CHECK(Tracker::live == 0, "object destroyed");
  return true;
}

bool test_null() {
  SmartPointer<Tracker> p;
  CHECK(p.isNull(),           "default is null");
  CHECK(!p.isSet(),           "!isSet");
  CHECK(p.get() == nullptr,   "get() == nullptr");
  CHECK(p.getRefCount() == 0, "refcount == 0");
  bool threw = false;
  try { *p; } catch (...) { threw = true; }
  CHECK(threw, "dereference null throws");
  return true;
}

bool test_release() {
  Tracker::reset();
  SmartPointer<Tracker> a = new Tracker;
  SmartPointer<Tracker> b = a;
  a.release();
  CHECK(Tracker::live == 1,   "still alive after releasing one");
  CHECK(a.isNull(),           "a is null after release");
  CHECK(b.getRefCount() == 1, "b refcount == 1");
  b.release();
  CHECK(Tracker::live == 0, "destroyed after releasing both");
  return true;
}

bool test_adopt() {
  Tracker::reset();
  SmartPointer<Tracker> p = new Tracker;
  Tracker *raw = p.adopt();
  CHECK(raw != nullptr,     "adopt returns non-null");
  CHECK(p.isNull(),         "pointer null after adopt");
  CHECK(Tracker::live == 1, "object still alive after adopt");
  delete raw;
  CHECK(Tracker::live == 0, "object destroyed after manual delete");
  return true;
}

bool test_adopt_multiple_refs() {
  SmartPointer<Tracker> a = new Tracker;
  SmartPointer<Tracker> b = a;
  bool threw = false;
  try { a.adopt(); } catch (...) { threw = true; }
  CHECK(threw, "adopt with multiple refs throws");
  return true;
}

bool test_weak_basic() {
  Tracker::reset();
  { SmartPointer<Tracker>       strong = new Tracker;
    SmartPointer<Tracker>::Weak weak   = strong;
    CHECK(strong.getRefCount(false) == 1, "strong count == 1");
    CHECK(strong.getRefCount(true)  == 1, "weak count == 1");
    CHECK(weak.isSet(), "weak isSet while strong alive"); }
  CHECK(Tracker::live == 0, "object destroyed when strong gone");
  return true;
}

bool test_weak_after_strong_destroyed() {
  Tracker::reset();
  SmartPointer<Tracker>::Weak weak;
  { SmartPointer<Tracker> strong = new Tracker;
    weak = strong;
    CHECK(weak.isSet(), "weak valid while strong alive"); }
  CHECK(!weak.isSet(),     "weak invalid after strong destroyed");
  CHECK(Tracker::live == 0, "object destroyed");
  return true;
}

bool test_weak_promote_expired() {
  Tracker::reset();
  SmartPointer<Tracker>::Weak weak;
  { SmartPointer<Tracker> strong = new Tracker;
    weak = strong; }
  SmartPointer<Tracker> promoted = weak;
  CHECK(promoted.isNull(), "promotion of expired weak yields null");
  CHECK(Tracker::live == 0, "no object resurrected");
  return true;
}

bool test_weak_promote_alive() {
  Tracker::reset();
  { SmartPointer<Tracker>       strong = new Tracker;
    SmartPointer<Tracker>::Weak weak   = strong;
    SmartPointer<Tracker>       promoted(weak);
    CHECK(promoted.isSet(),               "promoted pointer is set");
    CHECK(strong.getRefCount(false) == 2, "strong count == 2 after promotion");
    CHECK(Tracker::live == 1,             "still one object"); }
  CHECK(Tracker::live == 0, "object destroyed after all gone");
  return true;
}

bool test_weak_unmanaged_throws() {
  bool threw = false;
  try {
    Tracker t;
    SmartPointer<Tracker>::Weak w = &t;
  } catch (...) { threw = true; }
  CHECK(threw, "weak ptr to unmanaged object throws");
  return true;
}

bool test_phony() {
  Tracker::reset();
  Tracker t;
  { SmartPointer<Tracker>::Phony p(&t);
    CHECK(p.isSet(),   "phony isSet");
    CHECK(p.isPhony(), "isPhony() true"); }
  CHECK(Tracker::live == 1, "phony did not free object");
  return true;
}

bool test_array() {
  { SmartPointer<int>::Array p(new int[10]);
    CHECK(p.isSet(), "array pointer isSet");
    p[0] = 42;
    CHECK(p[0] == 42, "array index access works"); }
  CHECK(true, "array pointer cleaned up without crash");
  return true;
}

bool test_malloc() {
  { int *raw = static_cast<int *>(malloc(sizeof(int)));
    *raw = 99;
    SmartPointer<int>::Malloc p(raw);
    CHECK(p.isSet(), "malloc pointer isSet");
    CHECK(*p == 99,  "malloc pointer dereference"); }
  CHECK(true, "malloc pointer cleaned up without crash");
  return true;
}

bool test_refcounted_copy() {
  TrackerRC::reset();
  { SmartPointer<TrackerRC> a = new TrackerRC;
    SmartPointer<TrackerRC> b = a;
    SmartPointer<TrackerRC> c = a;
    CHECK(a.getRefCount() == 3, "refcount 3 after two copies");
    CHECK(TrackerRC::live == 1, "one object");
    b.release();
    CHECK(a.getRefCount() == 2, "refcount 2 after releasing b"); }
  CHECK(TrackerRC::live == 0, "RefCounted object destroyed");
  return true;
}

bool test_refcounted_reuse_counter() {
  TrackerRC::reset();
  TrackerRC *raw = new TrackerRC;
  SmartPointer<TrackerRC> a(raw);
  SmartPointer<TrackerRC> b(raw); // reuses counter stored in RefCounted base
  CHECK(a.getRefCount() == 2, "refcount 2 from same raw ptr");
  CHECK(TrackerRC::live == 1, "one object");
  a.release();
  CHECK(TrackerRC::live == 1, "still alive after releasing a");
  b.release();
  CHECK(TrackerRC::live == 0, "destroyed after releasing b");
  return true;
}

bool test_cast() {
  struct Base    { virtual ~Base() {} virtual int val() { return 1; } };
  struct Derived : Base { int val() override { return 2; } };
  SmartPointer<Base> b = new Derived;
  SmartPointer<Derived> d = b.cast<Derived>();
  CHECK(d.isSet(),     "cast succeeded");
  CHECK(d->val() == 2, "cast result correct");
  SmartPointer<Base> b2 = new Base;
  bool threw = false;
  try { b2.cast<Derived>(); } catch (...) { threw = true; }
  CHECK(threw, "invalid cast throws");
  return true;
}

bool test_is_instance() {
  struct Base    { virtual ~Base() {} };
  struct Derived : Base {};
  SmartPointer<Base> b = new Derived;
  CHECK( b.isInstance<Derived>(),  "isInstance<Derived> true for Derived");
  CHECK( b.isInstance<Base>(),     "isInstance<Base> true for Derived");
  SmartPointer<Base> b2 = new Base;
  CHECK(!b2.isInstance<Derived>(), "isInstance<Derived> false for Base");
  return true;
}

bool test_comparisons() {
  SmartPointer<Tracker> a = new Tracker;
  SmartPointer<Tracker> b = a;
  SmartPointer<Tracker> c = new Tracker;
  CHECK( (a == b),  "a == b (same object)");
  CHECK( (a != c),  "a != c (different objects)");
  CHECK(!(a == c),  "!(a == c)");
  Tracker *raw = a.get();
  CHECK( (a == raw), "SP == raw ptr");
  CHECK(!(a != raw), "!(SP != raw ptr)");
  return true;
}

bool test_bool_operators() {
  SmartPointer<Tracker> p;
  CHECK(!p, "!null is true");
  p = new Tracker;
  CHECK(!(!p), "!non-null is false");
  return true;
}

bool test_getrefcount_weak() {
  Tracker::reset();
  SmartPointer<Tracker>       strong = new Tracker;
  SmartPointer<Tracker>::Weak w1     = strong;
  SmartPointer<Tracker>::Weak w2     = strong;
  CHECK(strong.getRefCount(false) == 1, "strong count == 1");
  CHECK(strong.getRefCount(true)  == 2, "weak count == 2");
  w1.release();
  CHECK(strong.getRefCount(true) == 1, "weak count == 1 after release");
  return true;
}

bool test_counter_lifetime() {
  Tracker::reset();
  SmartPointer<Tracker>::Weak weak;
  { SmartPointer<Tracker> strong = new Tracker;
    weak = strong;
    CHECK(Tracker::live == 1, "object alive"); }
  CHECK(Tracker::live == 0, "object destroyed");
  CHECK(!weak.isSet(),      "weak invalid");
  weak.release();
  CHECK(weak.isNull(), "weak null after release");
  return true;
}

bool test_multiple_weak_cleanup() {
  Tracker::reset();
  SmartPointer<Tracker>::Weak w1, w2, w3;
  { SmartPointer<Tracker> strong = new Tracker;
    w1 = w2 = w3 = strong;
    CHECK(strong.getRefCount(true) == 3, "3 weak refs"); }
  CHECK(Tracker::live == 0, "object destroyed");
  w1.release(); w2.release(); w3.release();
  CHECK(true, "counter cleaned up after last weak release");
  return true;
}

bool test_threaded_strong() {
  Tracker::reset();
  SmartPointer<Tracker> root = new Tracker;
  const int N = 8, ITERS = 10000;
  std::vector<std::thread> threads;
  for (int i = 0; i < N; ++i)
    threads.emplace_back([&root]() {
      for (int j = 0; j < ITERS; ++j) {
        SmartPointer<Tracker> local = root; (void)local;
      }
    });
  for (auto &t : threads) t.join();
  CHECK(root.getRefCount() == 1, "refcount back to 1 after threads done");
  CHECK(Tracker::live == 1,      "object still alive");
  root.release();
  CHECK(Tracker::live == 0, "object destroyed");
  return true;
}

bool test_threaded_weak_promotion() {
  Tracker::reset();
  SmartPointer<Tracker>       root     = new Tracker;
  SmartPointer<Tracker>::Weak weakRoot = root;
  const int N = 8, ITERS = 5000;
  std::atomic<int> promoted{0};
  std::vector<std::thread> threads;
  for (int i = 0; i < N; ++i)
    threads.emplace_back([&]() {
      for (int j = 0; j < ITERS; ++j) {
        SmartPointer<Tracker> local = weakRoot;
        if (local.isSet()) ++promoted;
      }
    });
  for (auto &t : threads) t.join();
  CHECK(promoted == N * ITERS, "all promotions succeeded (root was alive)");
  CHECK(root.getRefCount() == 1, "refcount back to 1");
  return true;
}

bool test_threaded_weak_race() {
  Tracker::reset();
  const int ITERS = 10000;
  for (int iter = 0; iter < ITERS; ++iter) {
    SmartPointer<Tracker>       strong = new Tracker;
    SmartPointer<Tracker>::Weak weak   = strong;
    std::thread destroyer([&strong]() { strong.release(); });
    std::thread promoter([&weak]() {
      SmartPointer<Tracker> local = weak; (void)local;
    });
    destroyer.join();
    promoter.join();
    weak.release();
  }
  CHECK(Tracker::live == 0, "no leaked objects after race test");
  return true;
}

bool test_base_conversion() {
  struct Base    { virtual ~Base() {} virtual int val() { return 1; } };
  struct Derived : Base { int val() override { return 2; } };
  SmartPointer<Derived> d = new Derived;
  SmartPointer<Base>    b = d;
  CHECK(b.isSet(),            "base pointer set after conversion");
  CHECK(b->val() == 2,        "virtual dispatch correct after conversion");
  CHECK(d.getRefCount() == 2, "refcount 2 after conversion");
  return true;
}

bool test_helper_functions() {
  Tracker::reset();
  Tracker *raw = new Tracker;
  auto sp = SmartPtr(raw);
  CHECK(sp.isSet(), "SmartPtr helper works");
  auto wp = WeakPtr(sp);
  CHECK(wp.isSet(), "WeakPtr helper works");
  Tracker t;
  auto pp = PhonyPtr(&t);
  CHECK(pp.isSet(),   "PhonyPtr helper works");
  CHECK(pp.isPhony(), "PhonyPtr isPhony");
  return true;
}

bool test_reassign_frees_old() {
  Tracker::reset();
  SmartPointer<Tracker> p = new Tracker;
  int id1 = p->id;
  p = new Tracker;
  int id2 = p->id;
  CHECK(id1 != id2,        "different objects");
  CHECK(Tracker::live == 1, "first object freed on reassign");
  return true;
}

bool test_chain_assignment() {
  Tracker::reset();
  SmartPointer<Tracker> a = new Tracker;
  SmartPointer<Tracker> b, c, d;
  b = c = d = a;
  CHECK(a.getRefCount() == 4, "refcount 4 after chain assign");
  CHECK(Tracker::live == 1,   "one object");
  b.release(); c.release(); d.release();
  CHECK(a.getRefCount() == 1, "back to 1 after releasing copies");
  return true;
}

bool test_operator_arrow_and_deref() {
  struct Foo { int x = 42; };
  SmartPointer<Foo> p = new Foo;
  CHECK(p->x == 42,  "operator-> works");
  p->x = 7;
  CHECK((*p).x == 7, "operator* works");
  return true;
}

bool test_weak_weak_copy() {
  Tracker::reset();
  SmartPointer<Tracker>       strong = new Tracker;
  SmartPointer<Tracker>::Weak w1     = strong;
  SmartPointer<Tracker>::Weak w2     = w1;
  CHECK(strong.getRefCount(true) == 2, "weak count 2 after weak copy");
  CHECK(Tracker::live == 1,            "one object");
  strong.release();
  CHECK(!w1.isSet(),     "w1 invalid after strong gone");
  CHECK(!w2.isSet(),     "w2 invalid after strong gone");
  CHECK(Tracker::live == 0, "object destroyed");
  w1.release(); w2.release();
  return true;
}

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------
using TestFn = std::function<bool()>;
static std::map<std::string, TestFn> &registry() {
  static std::map<std::string, TestFn> r; return r;
}
#define REGISTER(fn) registry()[#fn] = fn

static void registerAll() {
  REGISTER(test_basic_lifetime);
  REGISTER(test_copy_construction);
  REGISTER(test_assignment);
  REGISTER(test_self_assignment);
  REGISTER(test_null);
  REGISTER(test_release);
  REGISTER(test_adopt);
  REGISTER(test_adopt_multiple_refs);
  REGISTER(test_weak_basic);
  REGISTER(test_weak_after_strong_destroyed);
  REGISTER(test_weak_promote_expired);
  REGISTER(test_weak_promote_alive);
  REGISTER(test_weak_unmanaged_throws);
  REGISTER(test_phony);
  REGISTER(test_array);
  REGISTER(test_malloc);
  REGISTER(test_refcounted_copy);
  REGISTER(test_refcounted_reuse_counter);
  REGISTER(test_cast);
  REGISTER(test_is_instance);
  REGISTER(test_comparisons);
  REGISTER(test_bool_operators);
  REGISTER(test_getrefcount_weak);
  REGISTER(test_counter_lifetime);
  REGISTER(test_multiple_weak_cleanup);
  REGISTER(test_threaded_strong);
  REGISTER(test_threaded_weak_promotion);
  REGISTER(test_threaded_weak_race);
  REGISTER(test_base_conversion);
  REGISTER(test_helper_functions);
  REGISTER(test_reassign_frees_old);
  REGISTER(test_chain_assignment);
  REGISTER(test_operator_arrow_and_deref);
  REGISTER(test_weak_weak_copy);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char *argv[]) {
  registerAll();

  if (argc == 2 && std::strcmp(argv[1], "--list") == 0) {
    for (auto &kv : registry()) std::cout << kv.first << "\n";
    return 0;
  }

  std::vector<std::string> toRun;
  if (argc >= 2) {
    for (int i = 1; i < argc; ++i) {
      if (registry().find(argv[i]) == registry().end()) {
        std::cerr << "Unknown test: " << argv[i] << "\n";
        return 1;
      }
      toRun.push_back(argv[i]);
    }
  } else {
    for (auto &kv : registry()) toRun.push_back(kv.first);
  }

  int failed_tests = 0;
  for (auto &name : toRun) {
    int before = g_fail;
    bool ok = false;
    try { ok = registry()[name](); }
    catch (const std::exception &e) {
      std::cerr << "  EXCEPTION in " << name << ": " << e.what() << "\n";
    } catch (...) {
      std::cerr << "  UNKNOWN EXCEPTION in " << name << "\n";
    }
    if (ok && g_fail == before)
      std::cout << "PASS: " << name << "\n";
    else {
      std::cout << "FAIL: " << name << "\n";
      ++failed_tests;
    }
  }

  std::cout << "\n" << g_pass << " checks passed, "
            << g_fail << " checks failed, "
            << failed_tests << " tests failed.\n";
  return failed_tests ? 1 : 0;
}
