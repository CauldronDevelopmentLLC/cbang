# cbang SmartPointer

`cb::SmartPointer<T>` is cbang's thread-safe, intrusive-friendly
reference-counting smart pointer.  It manages dynamically-allocated
objects, arrays, malloc'd buffers, and even non-owning "phony"
references — all with one type.  Almost every long-lived object in
cbang and its applications is owned through a `SmartPointer`.

## Concepts

- **Strong / weak.**  A strong pointer keeps the target alive; a weak
  pointer doesn't but can be promoted to a strong one safely.
- **Reference counter.**  Independent of the object by default, or
  embedded if the object derives from `cb::RefCounted` (so converting
  a raw `this` to a `SmartPointer` reuses the existing counter).
- **Deallocators.**  The template's `DeallocT` parameter chooses how
  the object is freed.  `DeallocNew` is the default; `DeallocArray`
  uses `delete []`, `DeallocMalloc` uses `free()`, `DeallocPhony`
  doesn't free at all.
- **Phony pointer.**  A SmartPointer that does *not* manage lifetime.
  Useful for callbacks that need a temporary "borrow" without bumping
  the refcount.
- **`WeakCall`.**  A helper that adapts a member function to a
  callback which fires only if `this` is still alive.

## Construction

```cpp
#include <cbang/SmartPointer.h>

cb::SmartPointer<MyObj> p = new MyObj();    // takes ownership
auto                    q = p;              // copy → refcount 2
cb::SmartPointer<MyObj> r;                  // null
```

The single-arg constructor takes ownership.  **Never create two
SmartPointers from the same raw pointer** — they will each try to
free it.  Copy-construct from another SmartPointer instead.

### `SmartPtr(x)` helper

A free function that wraps a raw `new` expression and returns a
strong SmartPointer.  Stylistic; equivalent to the constructor.

```cpp
auto p = cb::SmartPtr(new MyObj);
```

### `RefCounted` integration

If your class derives from `cb::RefCounted`, a SmartPointer made from
a raw `this` pointer reuses the existing reference counter.  This is
the safe way to vend `SmartPointer<this>`:

```cpp
class Worker : public cb::RefCounted {
public:
  cb::SmartPointer<Worker> ref() { return cb::SmartPtr(this); }
};
```

Without `RefCounted`, doing the same would create a second independent
counter — a double-free waiting to happen.

## Using a pointer

```cpp
if (p.isSet())   p->doIt();              // null check, member access
if (p.isNull())  return;                 // explicit null
MyObj &obj  = *p;                        // dereference
MyObj *raw  = p.get();                   // raw pointer (no transfer)
```

`->`, `*`, and `[]` assert non-null on access.  Use `isSet()` first if
the pointer may be null.

`get()` returns `nullptr` if the SmartPointer is null and does NOT
transfer ownership — the SmartPointer still owns the object.

## Releasing and transferring

```cpp
p.release();        // drop this reference; deletes if last
T *raw = p.adopt(); // transfer ownership to a raw pointer; p becomes null
```

`adopt()` is used when you need to hand the object to an API that
takes ownership of a raw pointer (rare; most cbang APIs accept
SmartPointers directly).

## Weak references

Weak pointers don't extend lifetime but can be safely tested for
liveness:

```cpp
cb::SmartPointer<Worker> strong = new Worker;
cb::SmartPointer<Worker>::Weak weak = strong;

strong.release();

if (weak.isSet()) weak->doIt();   // safe: false now that strong is gone
```

To safely use a weak pointer in a callback, *promote* it to strong:

```cpp
cb::SmartPointer<Worker> alive = weak;   // null if target gone
if (alive.isSet()) alive->doIt();
```

This is how `WeakCall` works internally.

## Casting

```cpp
cb::SmartPointer<Base>    b = new Derived;
cb::SmartPointer<Derived> d = b.cast<Derived>();    // dynamic_cast, throws on mismatch
Derived                  *raw = b.castPtr<Derived>(); // raw, also throws

if (b.isInstance<Derived>()) ...                    // type check
```

`cast<T>` produces a new SmartPointer of type `T` sharing the same
counter — no second counter, no double free.

## Phony pointers (non-owning)

A `SmartPointer<T>::Phony` (or the `PhonyPtr(x)` helper) wraps a raw
pointer in a SmartPointer-shaped object without taking ownership.
Useful when an API requires a SmartPointer but the caller still owns
the object:

```cpp
void Server::add(const cb::SmartPointer<Remote> &r);
// ...
Remote remote(...);
server.add(cb::PhonyPtr(&remote));      // server gets a pointer, doesn't own
```

Phony pointers compare equal to each other when they wrap the same
raw pointer.  They never free their target.

## WeakCall — callbacks that don't keep `this` alive

The biggest hazard with async callbacks: the loop fires the callback
after the registering object has been destroyed.  `WeakCall` solves
this for member-function callbacks.

```cpp
#include <cbang/util/WeakCallback.h>

class Worker : public cb::RefCounted {
public:
  void start() {
    event = base.newEvent(WeakCall(this, &Worker::tick), 0);
    event->add(60);
  }

  void tick() { /* may run, or may not if `this` was destroyed */ }
};
```

If the worker is freed before the timer fires, the callback is a
no-op.  Without `WeakCall`, the timer would call into freed memory.

`WeakCall` accepts a raw `this` (if the class is RefCounted) or a
SmartPointer.  It also accepts arbitrary lambdas — first arg is the
owner, second is the callable:

```cpp
WeakCall(this, [this] { tick(); });
WeakCall(this, [this] (int n) { handle(n); });   // multi-arg
```

`WeakCall` is the canonical pattern for any callback wired up at
construction time and torn down by destruction.  Use it instead of
naked `[this]` lambdas in event registration, HTTP callbacks, signal
handlers, etc.

## Specialised deallocators

The template's `DeallocT` parameter chooses how the object is freed.
Three pre-defined typedefs cover the common needs:

```cpp
cb::SmartPointer<MyObj>::Array   arr  = new MyObj[10];   // delete[]
cb::SmartPointer<char>::Malloc   buf  = (char *)malloc(N); // free()
cb::SmartPointer<MyObj>::Phony   ref  = &on_stack;       // no free
cb::SmartPointer<MyObj>::Weak    w    = strongRef;       // weak ref
```

Roll your own by defining a `DeallocT` with a static `dealloc(T *ptr)`
function.  Rarely needed.

## Thread safety

The reference counting itself is atomic — you can copy/move/assign
SmartPointers across threads safely.  **Access to the underlying
object is not synchronised** by the SmartPointer; if multiple threads
touch the pointed-to object, you must add your own locking.

cbang itself is largely single-threaded (event-loop based), so the
common case is "one thread owns and accesses the object."

## Conventions in cbang

- **Vend `SmartPointer<this>` only when the class is `RefCounted`.**
  Otherwise the new SmartPointer creates an independent counter and
  the object becomes double-tracked.
- **Pass SmartPointers by `const &` in signatures** to avoid bumping
  the count for transient parameters.
- **Cancel async work by `release()`-ing the strong reference**, not
  by deleting raw memory.  Anything that holds a weak reference can
  detect the disappearance.
- **Never mix raw `delete` with SmartPointer-managed objects.**  The
  counter expects to be the sole owner.
- **`isSet()` instead of `!= nullptr`** for consistency.

## Common patterns

### Vend a SmartPointer to `this`

```cpp
class Foo : public cb::RefCounted {
public:
  void schedule() {
    base.newEvent([self = cb::SmartPtr(this)] { self->run(); }, 0)->add(1);
  }
};
```

The lambda captures a strong ref; the Foo lives at least until the
event fires.

### Optional ownership transfer

```cpp
cb::SmartPointer<Job> dequeue() {
  if (queue.empty()) return cb::SmartPointer<Job>();
  auto job = queue.front();
  queue.pop_front();
  return job;          // refcount unchanged; caller now holds it
}
```

### Adopting an external pointer

```cpp
// Some C API hands us ownership of a malloc'd buffer:
char *raw = get_buffer_from_c_api();
cb::SmartPointer<char>::Malloc owned(raw);   // free()'d on destruction
```

### Casting a heterogeneous container

```cpp
cb::SmartPointer<Value>          v = parsed;
cb::SmartPointer<JSON::Dict>     d = v.cast<JSON::Dict>();
auto                             units = d->getList("units").cast<JSON::List>();
```

This is the pattern used throughout fah-client's `Config` and `Unit`
classes (e.g. `App.cpp:240`: `get("groups").cast<Groups>()`).

## Errors

- **Double-free.**  Caused by creating two SmartPointers from the
  same raw pointer (without `RefCounted`).  Symptom: crash on
  destruction of the second one.
- **Use after release.**  Dereferencing a SmartPointer after
  `release()` throws (`->`, `*` assert non-null).
- **Cast mismatch.**  `cast<T>` and `castPtr<T>` throw a
  `castError` when the dynamic_cast fails on a non-null pointer.

## See also

- `cbang/SmartPointer.h`, `cbang/RefCounter.h` for the full API.
- `cbang/util/WeakCallback.h` for `WeakCall`.
- [WebServer.md](WebServer.md) and [JSON.md](JSON.md) — the
  HTTP/WS/JSON APIs use SmartPointers pervasively.
- fah-client-bastet `App.cpp`, `Unit.cpp`, `Account.cpp` for
  production examples — `SmartPtr(new X)`, `PhonyPtr(this)`,
  `WeakCall(this, &Self::method)`, `cast<T>()`.
