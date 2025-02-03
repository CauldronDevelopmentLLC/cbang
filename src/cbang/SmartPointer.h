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

#include "Exception.h"

#ifndef CBANG_SMART_POINTER_H
#define CBANG_SMART_POINTER_H

#include "RefCounter.h"


namespace cb {
  /**
   * This is a thread-safe implementation of a smart pointer.
   *
   * You can use a SmartPointer on a dynamically allocated instance
   * of a class A like this:
   *
   *   SmartPointer<A> aPtr = new A;
   *
   * Since aPtr is on the stack, when it goes out of scope it will be
   * destructed.  When this happens, the reference counter will be
   * decremented and if it is zero the dynamic instance of A will be
   * destructed.
   *
   * You can make a copy of a smart pointer like this:
   *
   *   SmartPointer<A> anotherPtr = aPtr;
   *
   * Now the dynamic instance of A will not be destructed until both
   * aPtr and anotherPtr go out of scope.
   *
   * Correct deallocation can vary depending on the type of resource
   * being protected by the smart pointer.  The CounterT template
   * parameter selects the reference counter and the DeallocT template
   * parameter selects the deallocator.  The default deallocator is
   * used for any data allocated with the 'new' operator.
   *
   * DeallocArray is necessary for arrays allocated with the 'new []'
   * operator.  DeallocMalloc will correctly deallocate data allocated
   * with calls to malloc(), calloc(), realloc(), et.al.  Other
   * deallocators can be created to handle other resources. The managed
   * resources need not even be memory.
   *
   * SmartPointers are only thread safe in the sense that multiple threads
   * can safely access different copies of the same SmartPointer
   * concurrently.  Accessing the data pointed to by the SmartPointer
   * from multiple threads safely requires further synchronization.
   *
   * See http://ootips.org/yonat/4dev/smart-pointers.html for more
   * information about smart pointers and why to use them.
   */
  class SmartPointerBase {
  public:
    static void castError();
    static void referenceError(const std::string &msg);
  };


  template <typename T, typename DeallocT = DeallocNew<T>,
            typename CounterT = RefCounterImpl<T, DeallocT>,
            bool weak = false>
  class SmartPointer : public SmartPointerBase {
  protected:
    /// A pointer to the reference counter.
    RefCounter *refCounter = 0;

    /// The actual pointer.
    T *ptr = 0;

  public:
    typedef RefCounterPhonyImpl                CounterPhony;
    typedef RefCounterImpl<T, DeallocMalloc>   CounterMalloc;
    typedef RefCounterImpl<T, DeallocArray<T>> CounterArray;

    typedef SmartPointer<T, DeallocT,        CounterT,      weak> PointerT;
    typedef SmartPointer<T, DeallocT,        CounterT,      true> Weak;
    typedef SmartPointer<T, DeallocPhony,    CounterPhony,  weak> Phony;
    typedef SmartPointer<T, DeallocMalloc,   CounterMalloc, weak> Malloc;
    typedef SmartPointer<T, DeallocArray<T>, CounterArray,  weak> Array;

    /**
     * The copy constructor.  If the smart pointer being copied
     * is not NULL, the object reference count will be incremented.
     *
     * @param smartPtr The pointer to copy.
     */
    SmartPointer(const PointerT &smartPtr) {*this = smartPtr;}

    /**
     * Create a smart pointer from a pointer value.  If ptr is
     * non-NULL the reference count will be set to one.
     *
     * NOTE: It is an error to create more than one smart pointer
     * through this constructor for a single pointer value because
     * this will cause the object to which it points to be deallocated
     * more than once.  To create a copy of a smart pointer either use
     * the copy constructor or smart pointer assignment.
     *
     * If the pointer has RefCounted as a base class, then it will store
     * a pointer to it's own reference counter which can be reused by
     * future SmartPointers.
     *
     * @param ptr The pointer to point to.
     */
    SmartPointer(T *_ptr = 0, RefCounter *_refCounter = 0) :
      refCounter(_refCounter), ptr(_ptr) {
      if (!ptr) return;

      // Get RefCounter
      if (!refCounter) refCounter = CounterT::getCounter(ptr);
      refCounter->incCount(weak);

      if (weak && isPhony()) {
        release();
        referenceError("Weak pointer cannot have phony reference");
      }
    }

    /**
     * Destroy this smart pointer.  If this smart pointer is set to
     * a non-NULL value and there are no other references to the
     * object to which it points the object will be deallocated.
     */
    ~SmartPointer() {release();}

    /**
     * Compare two smart pointers for equality.  Two smart pointers
     * are equal if and only if their internal pointers are equal.
     *
     * @param smartPtr The pointer to compare with.
     *
     * @return True if the smart pointers are equal, false otherwise.
     */
    bool operator==(const PointerT &smartPtr) const {
      return get() == smartPtr.get();
    }

    /**
     * Compare two smart pointers for inequality.  Two smart pointers
     * are not equal if and only if their internal pointers are not equal.
     *
     * @param smartPtr The pointer to compare with.
     *
     * @return True if the smart pointers are not equal, false otherwise.
     */
    bool operator!=(const PointerT &smartPtr) const {
      return get() != smartPtr.get();
    }

    /**
     * Compare two smart pointers.  Useful for ordering smart pointers.
     *
     * @param smartPtr The pointer to compare with.
     *
     * @return True if less than @param smartPtr, false otherwise.
     */
    bool operator<(const PointerT &smartPtr) const {
      return get() < smartPtr.get();
    }

    /**
     * Compare this smart pointer to an actual pointer value.
     *
     * @param ptr The pointer to compare with.
     *
     * @return True if this smart pointers internal pointer equals ptr,
     *         false otherwise.
     */
    bool operator==(const T *ptr) const {return get() == ptr;}

    /**
     * Compare this smart pointer to an actual pointer value.
     *
     * @param ptr The pointer to compare with.
     *
     * @return False if this smart pointers internal pointer equals ptr.
     *         True otherwise.
     */
    bool operator!=(const T *ptr) const {return get() != ptr;}

    /**
     * Compare this smart pointer to an actual pointer value.
     *
     * @param ptr The pointer to compare with.
     *
     * @return True if less than @param ptr, false otherwise.
     */
    bool operator<(const T *ptr) const {return get() < ptr;}

    /**
     * Assign this smart pointer to another.  If the passed smart pointer
     * is non-NULL a reference will be added.
     *
     * @param smartPtr The pointer to copy.
     *
     * @return A reference to this smart pointer.
     */
    PointerT &operator=(const PointerT &smartPtr) {
      if (*this == smartPtr) return *this;

      release();

      if (smartPtr.isSet()) {
        if (weak && smartPtr.isPhony())
          referenceError("Weak pointer cannot have phony reference");
        refCounter = smartPtr.refCounter;
        refCounter->incCount(weak);
        ptr = smartPtr.ptr;
      }

      return *this;
    }

    /**
     * Dereference this smart pointer.
     * A Exception will be thrown if the pointer is NULL.
     *
     * @return A reference to the object pointed to by this smart pointer.
     */
    T *operator->() const {check(); return ptr;}

    /**
     * Dereference this smart pointer.
     * A Exception will be thrown if the pointer is NULL.
     *
     * @return A reference to the object pointed to by this smart pointer.
     */
    T &operator*() const {check(); return *ptr;}

    /**
     * Dereference this smart pointer with an array index.
     * A Exception will be thrown if the pointer is NULL.
     *
     * @return A reference to an object in the array pointed to by
     * this smart pointer.
     */
    T &operator[](const unsigned x) const {check(); return ptr[x];}

    /**
     * Access this smart pointer's internal object pointer.
     *
     * @return The value of the internal object pointer.
     */
    T *get() const {return isSet() ? ptr : 0;}

    /**
     * Access this smart pointer's internal object pointer but check for NULL.
     *
     * @return The value of the internal object pointer.
     */
    T *access() const {check(); return get();}

    /**
     * Not operator
     * @return true if the pointer is NULL.
     */
    bool operator!() const {return isNull();}


    /// Convert to a base type or weak pointer.
    template
      <typename _BaseT, typename _DeallocT, typename _CounterT, bool _weak>
    operator SmartPointer<_BaseT, _DeallocT, _CounterT, _weak> () const {
      return SmartPointer<_BaseT, _DeallocT, _CounterT, _weak>(ptr, refCounter);
    }


    /// Dynamic cast
    template <typename CastT>
    CastT *castPtr() const {
      CastT *ptr = dynamic_cast<CastT *>(get());
      if (!ptr && isSet()) castError();

      return ptr;
    }

    template <typename CastT>
    SmartPointer<CastT, DeallocT, CounterT, weak> cast() const {
      return SmartPointer<CastT, DeallocT, CounterT, weak>(
        castPtr<CastT>(), refCounter);
    }

    // Type check
    template <typename InstanceT>
    bool isInstance() const {return dynamic_cast<InstanceT *>(ptr);}

    /**
     * Release this smart pointer's reference.
     * If the reference count is one the object to which this
     * smart pointer points will be deleted.
     */
    void release() {
      RefCounter *_refCounter = refCounter;

      ptr = 0;
      refCounter = 0;

      if (_refCounter) _refCounter->decCount(weak);
    }

    /**
     * Assume responsibility for this pointer.  If the reference
     * count is more than one an Exception will be thrown.
     * When successful this smart pointer will be NULL.
     *
     * @return The value of the internal pointer.
     */
    T *adopt() {
      if (isNull()) return 0;

      refCounter->adopted();
      refCounter = 0;

      T *tmp = ptr;
      ptr = 0;

      return tmp;
    }

    /**
     * Get the number of references to the object pointed to by
     * this smart pointer.  The reference count will be zero
     * if this is a NULL smart pointer other wise it will be
     * one or more.
     *
     * @return The reference count.
     */
    unsigned getRefCount(bool _weak = false) const {
      return refCounter ? refCounter->getCount(_weak) : 0;
    }

    /**
     * Check for NULL pointer.
     *
     * @return True if the pointer is NULL, false otherwise.
     */
    bool isNull() const {return !isSet();}

    /// @return True if the pointer is non-NULL, false otherwise.
    bool isSet() const {return refCounter && refCounter->isActive() && ptr;}

    bool isPhony() const {return dynamic_cast<CounterPhony *>(refCounter);}

  protected:
    void check() const {
      if (!isSet()) referenceError("Can't dereference NULL pointer!");
    }
  };


  template<typename T> inline static SmartPointer<T> SmartPtr(T *ptr)
  {return ptr;}

  template<typename T> inline static SmartPointer<T>
  SmartPtr(const typename SmartPointer<T>::Weak &ptr) {return ptr;}

  template<typename T> inline static typename SmartPointer<T>::Weak
  WeakPtr(T *ptr) {return ptr;}

  template<typename T> inline static typename SmartPointer<T>::Weak
  WeakPtr(const SmartPointer<T> &ptr) {return ptr;}

  template<typename T> inline static SmartPointer<T> PhonyPtr(T *ptr)
  {return typename SmartPointer<T>::Phony(ptr);}

  template<typename T> inline static SmartPointer<T> MallocPtr(T *ptr)
  {return typename SmartPointer<T>::Malloc(ptr);}

  template<typename T> inline static SmartPointer<T> ArrayPtr(T *ptr)
  {return typename SmartPointer<T>::Array(ptr);}
}

#define CBANG_SP(T)        cb::SmartPointer<T>
#define CBANG_SP_WEAK(T)   cb::SmartPointer<T>::Weak
#define CBANG_SP_PHONY(T)  cb::SmartPointer<T>::Phony
#define CBANG_SP_MALLOC(T) cb::SmartPointer<T>::Malloc
#define CBANG_SP_ARRAY(T)  cb::SmartPointer<T>::Array
#define CBANG_SP_DEALLOC(T, DEALLOC) \
  cb::SmartPointer<T, cb::DeallocFunc<T, DEALLOC> >

#ifdef USING_CBANG
#define SP(T)                  CBANG_SP(T)
#define SP_WEAK(T)             CBANG_SP_WEAK(T)
#define SP_PHONY(T)            CBANG_SP_PHONY(T)
#define SP_MALLOC(T)           CBANG_SP_MALLOC(T)
#define SP_ARRAY(T)            CBANG_SP_ARRAY(T)
#define SP_DEALLOC(T, DEALLOC) CBANG_SP_DEALLOC(T, DEALLOC)
#endif

#endif // CBANG_SMART_POINTER_H
