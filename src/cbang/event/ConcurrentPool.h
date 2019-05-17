/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2019, Cauldron Development LLC
                   Copyright (c) 2003-2017, Stanford University
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

#pragma once

#include "Base.h"
#include "Event.h"

#include <cbang/os/ThreadPool.h>
#include <cbang/os/Condition.h>
#include <cbang/util/SmartLock.h>
#include <cbang/util/SmartUnlock.h>

#include <queue>
#include <functional>


namespace cb {
  namespace Event {
    class ConcurrentPool : protected ThreadPool, protected Condition {
    public:
      class Task {
        int priority;
        Exception e;
        bool failed;

      public:
        Task(int priority) : priority(priority), failed(false) {}
        virtual ~Task() {}

        bool getFailed() const {return failed;}
        void setException(const Exception &e) {this->e = e; failed = true;}
        const Exception &getException() const {return e;}
        bool shouldShutdown();

        virtual void run() = 0;
        virtual void success() {}
        virtual void error(const Exception &e) {}
        virtual void complete() {}

        bool operator<(const Task &o) const {return priority < o.priority;}
      };


      template <typename Data>
      struct QueuedTask : public Task, public Mutex {
        std::queue<Data> queue;
        SmartPointer<Event> event;

        QueuedTask(Base &base, int priority) :
          Task(priority),
          event(base.newEvent(this, &QueuedTask<Data>::dequeue, 0)) {}


        virtual void process(Data) = 0;


        void dequeue() {
          SmartLock lock(this);

          while (!queue.empty()) {
            Data data = queue.front();
            queue.pop();

            SmartUnlock unlock(this);
            process(data);
          }
        }


        void enqueue(Data data) {
          SmartLock lock(this);
          queue.push(data);
          event->activate();
        }


        // From Task
        void success() {
          // Flush any remaining data from the queue
          while (!queue.empty()) {
            process(queue.front());
            queue.pop();
          }
        }

        void complete() {event->del();}
      };


      template <typename Data>
      struct TaskFunctions : public Task {
        typedef std::function<Data ()> run_cb_t;
        typedef std::function<void (Data &)> success_cb_t;
        typedef std::function<void (const Exception &)> error_cb_t;
        typedef std::function<void ()> complete_cb_t;

        run_cb_t run_cb;
        success_cb_t success_cb;
        error_cb_t error_cb;
        complete_cb_t complete_cb;

        Data data;

        TaskFunctions(int priority, run_cb_t run_cb,
                      success_cb_t success_cb = 0, error_cb_t error_cb = 0,
                      complete_cb_t complete_cb = 0) :
          Task(priority), run_cb(run_cb), success_cb(success_cb),
          error_cb(error_cb), complete_cb(complete_cb) {}

        // From Task
        void run() {data = run_cb();}
        void error(const Exception &e) {if (error_cb) error_cb(e);}
        void success() {if (success_cb) success_cb(data);}
        void complete() {if (complete_cb) complete_cb();}
      };


      struct TaskPtrCompare {
        bool operator()(const SmartPointer<Task> &a,
                        const SmartPointer<Task> &b) const {return *a < *b;}
      };

    protected:
      Base &base;
      SmartPointer<Event> event;

      typedef std::priority_queue<SmartPointer<Task>,
                                  std::vector<SmartPointer<Task> >,
                                  TaskPtrCompare> queue_t;
      queue_t ready;
      queue_t completed;

    public:
      ConcurrentPool(Base &base, unsigned size);
      ~ConcurrentPool();

      void submit(const SmartPointer<Task> &task);

      template <typename Data>
      void submit(int priority, typename TaskFunctions<Data>::run_cb_t run,
                  typename TaskFunctions<Data>::success_cb_t success = 0,
                  typename TaskFunctions<Data>::error_cb_t error = 0,
                  typename TaskFunctions<Data>::complete_cb_t complete = 0) {
        submit(new TaskFunctions<Data>
               (priority, run, success, error, complete));
      }

      // From ThreadPool
      using ThreadPool::start;
      void stop();
      void join();

    protected:
      void run();

      void complete();
    };
  }
}
