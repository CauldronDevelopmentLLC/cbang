/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2023, Cauldron Development LLC
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

#include "FDPool.h"

#include <list>
#include <map>


namespace cb {
  namespace Event {
    class FDPoolEvent : public FDPool {
      Base &base;
      int priority = 0;
      Rate readRate;
      Rate writeRate;


      class FDQueue : private std::list<SmartPointer<Transfer> > {
        typedef std::list<SmartPointer<Transfer> > Super_T;
        bool read;
        bool closed = false;
        uint64_t last = 0;

      public:
        FDQueue(bool read) : read(read) {}

        using Super_T::empty;
        using Super_T::size;
        bool wantsRead() const;
        bool wantsWrite() const;
        void add(const SmartPointer<Transfer> &t);
        void transfer();
        void transferPending();
        void close();
        void flush();
        uint64_t getTimeout() const;
        void timeout();

      protected:
        void popFinished();
        void pop();
      };


      class FDRec {
        FDPoolEvent &pool;
        int fd;
        SmartPointer<Event> event;
        SmartPointer<Event> timeoutEvent;
        unsigned events = 0;

        FDQueue readQ;
        FDQueue writeQ;

      public:
        FDRec(FDPoolEvent &pool, int fd);

        void read(const SmartPointer<Transfer> &t);
        void write(const SmartPointer<Transfer> &t);
        void flush();

      protected:
        void updateEvent();
        void updateTimeout();
        void update();
        void callback(Event &e, int fd, unsigned events);
        void timeout();
      };

      typedef std::map<int, SmartPointer<FDRec> > fds_t;
      fds_t fds;

    public:
      FDPoolEvent(Base &base);

      Base &getBase() {return base;}

      // From FDPool
      void setEventPriority(int priority) override {this->priority = priority;}
      int getEventPriority() const override {return priority;}
      const Rate &getReadRate() const override {return readRate;}
      const Rate &getWriteRate() const override {return writeRate;}
      void read(const SmartPointer<Transfer> &t) override;
      void write(const SmartPointer<Transfer> &t) override;
      void open(FD &fd) override;
      void flush(int fd, std::function <void ()> cb) override;

    protected:
      FDRec &get(int fd);
    };
  }
}
