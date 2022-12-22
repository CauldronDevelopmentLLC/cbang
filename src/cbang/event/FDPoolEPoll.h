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

#include <cbang/config.h>

#ifdef HAVE_EPOLL

#include "FDPool.h"

#include <cbang/os/Thread.h>
#include <cbang/util/SPSCQueue.h>

#include <map>
#include <set>
#include <queue>


namespace cb {
  namespace Event {
    class Base;

    class FDPoolEPoll : public FDPool, public Thread {
      int fd = -1;

      SmartPointer<Event> event;

      typedef enum {
        CMD_READ,
        CMD_WRITE,
        CMD_FLUSH,
        CMD_FLUSHED,
        CMD_COMPLETE,
        CMD_READ_PROGRESS,
        CMD_WRITE_PROGRESS,
        CMD_READ_SIZE,
        CMD_WRITE_SIZE,
        CMD_READ_FINISHED,
        CMD_WRITE_FINISHED,
        CMD_STATUS,
      } cmd_t;

      enum {
        STATUS_READ_CLOSED    = 1 << 4,
        STATUS_WRITE_CLOSED   = 1 << 5,
        STATUS_READ_TIMEDOUT  = 1 << 6,
        STATUS_WRITE_TIMEDOUT = 1 << 7,
      };

      struct Command {
        cmd_t cmd;
        int fd;
        SmartPointer<Transfer> tran;
        uint64_t time;
        int value;
      };

      struct TimeoutMember {
        int fd;
        bool read;

        bool operator<(const TimeoutMember &o) const {
          if (fd == o.fd) return read < o.read;
          return fd < o.fd;
        }
      };

      struct Timeout {
        uint64_t time;
        bool read;
        int fd;

        bool operator<(const Timeout &o) const {return o.time < time;}
      };

      class FDRec;

      class FDQueue : public std::queue<SmartPointer<Transfer> > {
        FDRec &fdr;
        bool read;
        bool closed = false;
        bool timedout = false;
        uint64_t last = 0;
        bool newTransfer = true;

      public:
        FDQueue(FDRec &fdr, bool read) : fdr(fdr), read(read) {}

        bool isClosed() const {return closed;}
        bool isTimedout() const {return timedout;}
        bool wantsRead() const;
        bool wantsWrite() const;
        uint64_t getTimeout() const;
        uint64_t getNextTimeout() const;
        void updateTimeout(bool wasActive, bool nowActive);
        void timeout(uint64_t now);
        void transfer();
        void transferPending();
        void flush();
        void add(const SmartPointer<Transfer> &tran);

      protected:
        void close();
        void pop();
      };

      class FDRec {
        FDPoolEPoll &pool;
        int fd = -1;
        unsigned events = 0;
        FDQueue readQ;
        FDQueue writeQ;

      public:
        FDRec(FDPoolEPoll &pool, int fd);

        FDPoolEPoll &getPool() {return pool;}
        int getFD() const {return fd;}

        void timeout(uint64_t now, bool read);
        unsigned getEvents() const;
        int getStatus() const;
        void update();
        void transfer(unsigned events);
        void flush();
        void process(cmd_t cmd, const SmartPointer<Transfer> &tran);
      };

      SPSCQueue<Command> cmds;
      SPSCQueue<Command> results;
      std::set<TimeoutMember> inTimeoutQ;
      std::priority_queue<Timeout> timeoutQ;
      std::map<int, std::function<void ()> > flushing;

      typedef std::map<int, SmartPointer<FDRec> > pool_t;
      pool_t pool;

      typedef std::map<int, FD *> fds_t;
      fds_t fds;
      Rate readRate = 60;
      Rate writeRate = 60;

    public:
      FDPoolEPoll(Base &base);
      ~FDPoolEPoll();

      int getFD() const {return fd;}

      // From FDPool
      void setEventPriority(int priority);
      int getEventPriority() const;
      const Rate &getReadRate() const {return readRate;}
      const Rate &getWriteRate() const {return writeRate;}

      void read(const SmartPointer<Transfer> &t);
      void write(const SmartPointer<Transfer> &t);
      void open(FD &fd);
      void flush(int fd, std::function <void ()> cb);

      void queueTimeout(uint64_t time, bool read, int fd);
      void queueComplete(const SmartPointer<Transfer> &t);
      void queueFlushed(int fd);
      void queueProgress(cmd_t cmd, int fd, uint64_t time, int value);

    protected:
      void queueStatus(int fd, int status);
      void queueCommand(cmd_t cmd, int fd, const SmartPointer<Transfer> &tran);
      FDRec &getFD(int fd);
      void processResults();

      // From Thread
      void run();
    };
  }
}

#endif // HAVE_EPOLL
