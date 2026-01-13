/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2026, Cauldron Development  Oy
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

#pragma once

#include "Base.h"
#include "Event.h"

#include <cbang/Catch.h>
#include <cbang/thread/ThreadPool.h>
#include <cbang/thread/Condition.h>
#include <cbang/thread/SmartLock.h>
#include <cbang/thread/SmartUnlock.h>
#include <cbang/time/Time.h>

#include <queue>
#include <functional>


namespace cb {
  namespace Event {
    class ConcurrentPool : protected ThreadPool, protected Condition {
    public:
      class Task {
        int priority;
        uint64_t ts = Time::now();
        Exception e;
        bool failed = false;

      public:
        Task(int priority) : priority(priority) {}
        virtual ~Task() {}

        bool getFailed() const {return failed;}
        void setException(const Exception &e) {this->e = e; failed = true;}
        const Exception &getException() const {return e;}
        bool shouldShutdown();

        virtual void run() = 0;
        virtual void success() {}
        virtual void error(const Exception &e) {}
        virtual void complete() {}

        bool operator<(const Task &o) const {
          if (priority == o.priority) return ts < o.ts;
          return priority < o.priority;
        }
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
        void success() override {
          // Flush any remaining data from the queue
          while (!queue.empty()) {
            process(queue.front());
            queue.pop();
          }
        }

        void complete() override {event->del();}
      };


      struct TaskFunctions : public Task {
        using run_cb_t      = std::function<void ()>;
        using success_cb_t  = std::function<void ()>;
        using error_cb_t    = std::function<void (const Exception &)>;
        using complete_cb_t = std::function<void ()>;

        run_cb_t      run_cb;
        success_cb_t  success_cb;
        error_cb_t    error_cb;
        complete_cb_t complete_cb;

        TaskFunctions(int priority, run_cb_t run_cb,
          success_cb_t  success_cb  = 0, error_cb_t error_cb = 0,
          complete_cb_t complete_cb = 0) :
          Task(priority), run_cb(run_cb), success_cb(success_cb),
          error_cb(error_cb), complete_cb(complete_cb) {}

        // From Task
        void run() override {run_cb();}
        void error(const Exception &e) override {if (error_cb) error_cb(e);}
        void success()  override {if (success_cb)  success_cb();}
        void complete() override {if (complete_cb) complete_cb();}
      };


      struct TaskPtrCompare {
        bool operator()(const SmartPointer<Task> &a,
          const SmartPointer<Task> &b) const {return *a < *b;}
      };

    protected:
      Base &base;
      SmartPointer<Event> event;

      using queue_t =
        std::priority_queue<SmartPointer<Task>, std::vector<SmartPointer<Task>>,
          TaskPtrCompare>;

      unsigned active = 0;
      queue_t ready;
      queue_t completed;

    public:
      ConcurrentPool(Base &base, unsigned size);
      ~ConcurrentPool();

      Base &getEventBase() const {return base;}
      void setEventPriority(int priority) {event->setPriority(priority);}

      unsigned getNumReady() const;
      unsigned getNumActive() const;
      unsigned getNumCompleted() const;

      void submit(const SmartPointer<Task> &task);


      void submit(
        int priority, typename TaskFunctions::run_cb_t run,
        typename TaskFunctions::success_cb_t  success  = 0,
        typename TaskFunctions::error_cb_t    error    = 0,
        typename TaskFunctions::complete_cb_t complete = 0) {
        submit(new TaskFunctions(priority, run, success, error, complete));
      }


      void submit(std::function<void ()> run,
        std::function<void (bool)> done, int priority = 0) {

        auto successCB = [=] () {done(true);};
        auto errorCB   = [=] (const Exception &e) {done(false);};

        submit(priority, run, successCB, errorCB);
      }


      // From ThreadPool
      using ThreadPool::start;
      void stop() override;
      void join() override;

    protected:
      void run() override;

      void complete();
    };
  }
}
