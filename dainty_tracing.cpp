/******************************************************************************

 MIT License

 Copyright (c) 2018 kieme, frits.germs@gmx.net

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

******************************************************************************/

#include <stdio.h>
#include <memory>
#include "dainty_mt_event_dispatcher.h"
#include "dainty_mt_waitable_chained_queue.h"
#include "dainty_mt_command.h"
#include "dainty_mt_thread.h"
#include "dainty_os_threading.h"
#include "dainty_os_clock.h"
#include "dainty_tracing.h"

using namespace dainty::container;
using namespace dainty::named;
using namespace dainty::mt;
using namespace dainty::os;
using namespace dainty::tracing::tracer;

using dainty::container::any::t_any;
using dainty::os::threading::t_mutex_lock;
using dainty::os::clock::t_time;
using dainty::mt::thread::t_thread;
using dainty::mt::event_dispatcher::t_dispatcher;
using dainty::mt::event_dispatcher::t_event_logic;
using dainty::mt::event_dispatcher::t_action;
using dainty::mt::event_dispatcher::CONTINUE;
using dainty::mt::event_dispatcher::QUIT_EVENT_LOOP;
using dainty::mt::event_dispatcher::RD;

using t_thd_err       = t_thread::t_logic::t_err;
using t_cmd_err       = command::t_processor::t_logic::t_err;
using t_cmd_client    = command::t_client;
using t_cmd_processor = command::t_processor;
using t_cmd           = command::t_command;
using t_any_user      = any::t_user;
using t_que_chain     = waitable_chained_queue::t_chain;
using t_que_client    = waitable_chained_queue::t_client;
using t_que_processor = waitable_chained_queue::t_processor;

namespace dainty
{
namespace tracing
{
namespace tracer
{
  inline
  t_tracer mk_(R_id id, R_name name) {
    return {id, name};
  }
}

///////////////////////////////////////////////////////////////////////////////

  struct t_item_ {
    t_id       id;
    t_level    level;
    t_textline line;
    t_time     time;
  };

  using R_item_ = named::t_prefix<t_item_>::R_;

  inline
  t_bool operator==(R_item_ lh, R_item_ rh) {
    return lh.line == rh.line;
  }

///////////////////////////////////////////////////////////////////////////////

  struct t_update_params_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 1;
    R_params params;

    inline
    t_update_params_cmd_(R_params _params) : t_cmd{cmd_id}, params(_params) {
    };
  };

  struct t_fetch_params_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 2;
    r_params params;

    inline
    t_fetch_params_cmd_(r_params _params) : t_cmd{cmd_id}, params(_params) {
    };
  };

  struct t_get_point_name_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 3;
    R_id           id;
    t_tracer_name& name;

    inline
    t_get_point_name_cmd_(R_id _id, t_tracer_name& _name)
      : t_cmd{cmd_id}, id(_id), name(_name) {
    };
  };

  struct t_get_point_level_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 4;
    R_id     id;
    t_level& level;

    inline
    t_get_point_level_cmd_(R_id _id, t_level& _level)
      : t_cmd{cmd_id}, id(_id), level(_level) {
    };
  };

  struct t_make_tracer_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 5;
    R_tracer_name   name;
    R_tracer_params params;
    t_id&           id;

    inline
    t_make_tracer_cmd_(R_tracer_name _name, R_tracer_params _params, t_id& _id)
      : t_cmd{cmd_id}, name(_name), params(_params), id(_id) {
    };
  };

  struct t_update_tracer_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 6;
    R_wildcard_name name;
    t_level         level;
    t_bool&         updated;

    inline
    t_update_tracer_cmd_(R_wildcard_name _name, t_level _level,
                         t_bool& _updated)
      : t_cmd{cmd_id}, name(_name), level(_level), updated(_updated) {
    };
  };

  struct t_update_tracer_params_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 7;
    R_tracer_name   name;
    R_tracer_params params;

    inline
    t_update_tracer_params_cmd_(R_tracer_name _name, R_tracer_params _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };

  struct t_fetch_tracer_params_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 8;
    R_tracer_name    name;
    t_tracer_params& params;
    t_bool&          found;

    inline
    t_fetch_tracer_params_cmd_(R_tracer_name _name, t_tracer_params& _params,
                               t_bool& _found)
      : t_cmd{cmd_id}, name(_name), params(_params), found(_found) {
    };
  };

  struct t_fetch_tracer_info_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 9;
    R_tracer_name name;
    r_tracer_info info;
    t_bool         clear_stats;
    t_bool&        found;

    inline
    t_fetch_tracer_info_cmd_(R_tracer_name _name, r_tracer_info _info,
                             t_bool _clear_stats, t_bool _found)
      : t_cmd{cmd_id}, name(_name), info(_info), clear_stats(_clear_stats),
        found(_found) {
    };
  };

  struct t_fetch_tracers_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 10;
    t_tracer_infos& infos;
    t_bool          clear_stats;
    t_bool&         found;

    inline
    t_fetch_tracers_cmd_(t_tracer_infos& _infos, t_bool _clear_stats,
                         t_bool& _found)
      : t_cmd{cmd_id}, infos(_infos), clear_stats(_clear_stats),
        found(_found) {
    };
  };

  struct t_create_observer_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 11;
    R_observer_name   name;
    R_observer_params params;

    inline
    t_create_observer_cmd_(R_observer_name _name, R_observer_params _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };


  struct t_destroy_observer_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 12;
    R_observer_name name;

    inline
    t_destroy_observer_cmd_(R_observer_name _name)
      : t_cmd{cmd_id}, name(_name) {
    };
  };

  struct t_update_observer_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 13;
    R_observer_name   name;
    R_observer_params params;

    inline
    t_update_observer_cmd_(R_observer_name _name, R_observer_params _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };

  struct t_fetch_observer_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 14;
    R_observer_name name;
          r_observer_params params;
          t_bool& found;

    inline
    t_fetch_observer_cmd_(R_observer_name _name,
                          r_observer_params _params,
                          t_bool& _found)
      : t_cmd{cmd_id}, name(_name), params(_params), found(_found) {
    };
  };

  struct t_fetch_observer_info_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 15;
    R_observer_name name;
          t_observer_info& info;
          t_bool           clear_stats;
          t_bool&          found;

    inline
    t_fetch_observer_info_cmd_(R_observer_name _name,
                               t_observer_info& _info,
                               t_bool _clear_stats,
                               t_bool& _found)
      : t_cmd{cmd_id}, name(_name), info(_info), clear_stats(_clear_stats),
        found(_found) {
    };
  };

  struct t_fetch_observers_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 16;
    t_observer_infos& infos;
    t_bool            clear_stats;
    t_bool&           found;

    inline
    t_fetch_observers_cmd_(t_observer_infos& _infos, t_bool _clear_stats,
                           t_bool& _found)
      : t_cmd{cmd_id}, infos(_infos), clear_stats(_clear_stats),
        found(_found) {
    };
  };

  struct t_bind_tracers_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 17;
    R_observer_name name;
    R_wildcard_name wildcard_name;
    t_bool&         found;

    inline
    t_bind_tracers_cmd_(R_observer_name _name, R_wildcard_name _wildcard_name,
                        t_bool& _found)
      : t_cmd{cmd_id}, name(_name), wildcard_name(_wildcard_name),
        found(_found) {
    };
  };

  struct t_unbind_tracers_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 18;
    R_observer_name name;
    R_wildcard_name wildcard_name;
    t_bool&         found;

    inline
    t_unbind_tracers_cmd_(R_observer_name _name,
                          R_wildcard_name _wildcard_name, t_bool& _found)
      : t_cmd{cmd_id}, name(_name), wildcard_name(_wildcard_name),
        found(_found) {
    };
  };

  struct t_is_tracer_bound_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 19;
    R_observer_name name;
    R_tracer_name   tracer_name;
    t_bool&         found;

    inline
    t_is_tracer_bound_cmd_(R_observer_name _name, R_tracer_name _tracer_name,
                           t_bool& _found)
      : t_cmd{cmd_id}, name(_name), tracer_name(_tracer_name),
        found(_found) {
    };
  };

  struct t_fetch_bound_tracers_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 20;
    R_observer_name name;
    r_tracer_names  tracer_names;
    t_bool&         found;

    inline
    t_fetch_bound_tracers_cmd_(R_observer_name _name,
                               r_tracer_names _tracer_names, t_bool& _found)
      : t_cmd{cmd_id}, name(_name), tracer_names(_tracer_names),
        found(_found) {
    };
  };

  struct t_fetch_bound_observers_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 21;
    R_tracer_name    name;
    r_observer_names observer_names;
    t_bool&          found;

    inline
    t_fetch_bound_observers_cmd_(R_tracer_name _name,
                                 r_observer_names _observer_names,
                                 t_bool& _found)
      : t_cmd{cmd_id}, name(_name), observer_names(_observer_names),
        found(_found) {
    };
  };

  struct t_destroy_tracer_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 22;
    R_id    id;
    t_bool& done;

    inline
    t_destroy_tracer_cmd_(R_id _id, t_bool& _done)
      : t_cmd{cmd_id}, id(_id), done(_done) {
    };
  };

  struct t_do_chain_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 23;
    t_que_chain& chain;

    inline
    t_do_chain_cmd_(t_que_chain& _chain)
      : t_cmd{cmd_id}, chain(_chain) {
    };
  };

  struct t_clean_death_cmd_ : t_cmd {
    constexpr static command::t_id cmd_id = 24;

    inline
    t_clean_death_cmd_() : t_cmd{cmd_id} {
    };
  };

///////////////////////////////////////////////////////////////////////////////

  // tracer   - freelist
  // observer - freelist
  // tracer_lk
  // observer_lk

  struct t_data_ {
    t_params params;

    t_data_(R_params _params) : params{_params} {
    }
  };

///////////////////////////////////////////////////////////////////////////////

  class t_logic_ : public t_thread::t_logic,
                   public t_cmd_processor::t_logic,
                   public t_que_processor::t_logic,
                   public t_dispatcher::t_logic {
  public:
    t_logic_(tracing::t_err err, R_params params)
      : data_         {params},
        cmd_processor_{err},
        que_processor_{err, data_.params.queuesize},
        dispatcher_   {err, {t_n{2}, "epoll_service"}} {
    }

    t_cmd_client make_cmd_client() {
      return cmd_processor_.make_client(command::t_user{0L});
    }

    t_que_client make_que_client() {
      return que_processor_.make_client(waitable_chained_queue::t_user{0L});
    }

///////////////////////////////////////////////////////////////////////////////

    virtual t_validity update(t_thd_err err,
                              r_pthread_attr) noexcept override {
      printf("thread update - before thread is created\n");
      return VALID;
    }

    virtual t_validity prepare(t_thd_err err) noexcept override {
      printf("thread prepare - after thread is created\n");
      return VALID;
    }

    virtual p_void run() noexcept override {
      printf("thread run - its main loop\n");

      tracing::t_err err;

      t_cmd_proxy_ cmd_proxy{err, action_, cmd_processor_, *this};
      dispatcher_.add_event (err, {cmd_processor_.get_fd(), RD}, &cmd_proxy);

      t_que_proxy_ que_proxy{err, action_, que_processor_, *this};
      dispatcher_.add_event (err, {que_processor_.get_fd(), RD}, &que_proxy);

      dispatcher_.event_loop(err, this);

      if (err) {
        err.print();
        err.clear();
      }

      return nullptr;
    }

///////////////////////////////////////////////////////////////////////////////

    virtual t_void may_reorder_events (r_event_infos) override {
    }

    virtual t_void notify_event_remove(r_event_info) override {
    }

    virtual t_quit notify_timeout(t_microseconds) override {
      return true;
    }

    virtual t_quit notify_error(t_errn)  override {
      return true;
    }

///////////////////////////////////////////////////////////////////////////////

    t_void process_item(t_any&& any) {
      t_item_& item = any.ref<t_item_>();
      printf("line = %s", get(item.line.c_str()));
    }

    t_void process_chain(t_chain& chain) {
      for (auto item = chain.head; item; item = item->next())
        process_item(std::move(item->ref()));
    }

    virtual t_void async_process(t_chain chain) noexcept override {
      printf("recv a trace\n");
      process_chain(chain);
      action_.cmd = CONTINUE;
    }

    virtual t_void async_process(t_user, p_command cmd) noexcept override {
      printf("thread async command - none is used at this point\n");
      // not used
      action_.cmd = CONTINUE;
    }

///////////////////////////////////////////////////////////////////////////////

    t_void process(tracing::t_err err, t_do_chain_cmd_& cmd) noexcept {
      process_chain(cmd.chain);
    }

    t_void process(tracing::t_err err, t_update_params_cmd_&) noexcept {
      printf("thread update_params_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_fetch_params_cmd_&) noexcept {
      printf("thread fetch_params_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_get_point_name_cmd_&) noexcept {
      printf("thread get_point_name_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_get_point_level_cmd_&) noexcept {
      printf("thread get_point_level_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_make_tracer_cmd_&) noexcept {
      printf("thread make_tracer_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_update_tracer_cmd_&) noexcept {
      printf("thread update_tracer_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_update_tracer_params_cmd_&) noexcept {
      printf("thread update_tracer_params_cmd received\n");
      action_.cmd = CONTINUE;
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_tracer_params_cmd_&) noexcept {
      printf("thread fetch_tracer_params_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_fetch_tracer_info_cmd_&) noexcept {
      printf("thread fetch_tracer_info_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_fetch_tracers_cmd_&) noexcept {
      printf("thread fetch_tracers_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_create_observer_cmd_&) noexcept {
      printf("thread create_observer_cmd received\n");
      // work done
    }

    t_void process(tracing::t_err err, t_destroy_observer_cmd_&) noexcept {
      printf("thread destroy_observer_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_update_observer_cmd_&) noexcept {
      printf("thread update_observer_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_fetch_observer_cmd_&) noexcept {
      printf("thread fetch_observer_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_fetch_observer_info_cmd_&) noexcept {
      printf("thread fetch_observer_info_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_fetch_observers_cmd_&) noexcept {
      printf("thread fetch_observers_cmd received\n");
      // work done
    }

    t_void process(tracing::t_err err, t_bind_tracers_cmd_&) noexcept {
      printf("thread bind_tracers_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_unbind_tracers_cmd_&) noexcept {
      printf("thread unbind_tracers_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_is_tracer_bound_cmd_&) noexcept {
      printf("thread is_tracer_bound_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_fetch_bound_tracers_cmd_&) noexcept {
      printf("thread fetch_bound_tracers_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_fetch_bound_observers_cmd_&) noexcept {
      printf("thread fetch_bound_observers_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_destroy_tracer_cmd_&) noexcept {
      printf("thread destroy_tracer_cmd received\n");
      // work done
      action_.cmd = CONTINUE;
    }

    t_void process(tracing::t_err err, t_clean_death_cmd_&) noexcept {
      printf("thread clean_death_cmd received\n");
      action_.cmd = QUIT_EVENT_LOOP;
    }

    virtual t_void process(t_cmd_err err, t_user,
                           r_command cmd) noexcept override {
      switch (cmd.id) {
        case t_update_params_cmd_::cmd_id:
          process(err, static_cast<t_update_params_cmd_&>(cmd));
          break;
        case t_fetch_params_cmd_::cmd_id:
          process(err, static_cast<t_fetch_params_cmd_&>(cmd));
          break;
        case t_get_point_name_cmd_::cmd_id:
          process(err, static_cast<t_get_point_name_cmd_&>(cmd));
          break;
        case t_get_point_level_cmd_::cmd_id:
          process(err, static_cast<t_get_point_level_cmd_&>(cmd));
          break;
        case t_make_tracer_cmd_::cmd_id:
          process(err, static_cast<t_make_tracer_cmd_&>(cmd));
          break;
        case t_update_tracer_cmd_::cmd_id:
          process(err, static_cast<t_update_tracer_cmd_&>(cmd));
          break;
        case t_update_tracer_params_cmd_::cmd_id:
          process(err, static_cast<t_update_tracer_params_cmd_&>(cmd));
          break;
        case t_fetch_tracer_params_cmd_::cmd_id:
          process(err, static_cast<t_fetch_tracer_params_cmd_&>(cmd));
          break;
        case t_fetch_tracer_info_cmd_::cmd_id:
          process(err, static_cast<t_fetch_tracer_info_cmd_&>(cmd));
          break;
        case t_fetch_tracers_cmd_::cmd_id:
          process(err, static_cast<t_fetch_tracers_cmd_&>(cmd));
          break;
        case t_create_observer_cmd_::cmd_id:
          process(err, static_cast<t_create_observer_cmd_&>(cmd));
          break;
        case t_destroy_observer_cmd_::cmd_id:
          process(err, static_cast<t_destroy_observer_cmd_&>(cmd));
          break;
        case t_update_observer_cmd_::cmd_id:
          process(err, static_cast<t_update_observer_cmd_&>(cmd));
          break;
        case t_fetch_observer_cmd_::cmd_id:
          process(err, static_cast<t_fetch_observer_cmd_&>(cmd));
          break;
        case t_fetch_observer_info_cmd_::cmd_id:
          process(err, static_cast<t_fetch_observer_info_cmd_&>(cmd));
          break;
        case t_fetch_observers_cmd_::cmd_id:
          process(err, static_cast<t_fetch_observers_cmd_&>(cmd));
          break;
        case t_bind_tracers_cmd_::cmd_id:
          process(err, static_cast<t_bind_tracers_cmd_&>(cmd));
          break;
        case t_unbind_tracers_cmd_::cmd_id:
          process(err, static_cast<t_unbind_tracers_cmd_&>(cmd));
          break;
        case t_is_tracer_bound_cmd_::cmd_id:
          process(err, static_cast<t_is_tracer_bound_cmd_&>(cmd));
          break;
        case t_fetch_bound_tracers_cmd_::cmd_id:
          process(err, static_cast<t_fetch_bound_tracers_cmd_&>(cmd));
          break;
        case t_fetch_bound_observers_cmd_::cmd_id:
          process(err, static_cast<t_fetch_bound_observers_cmd_&>(cmd));
          break;
        case t_destroy_tracer_cmd_::cmd_id:
          process(err, static_cast<t_destroy_tracer_cmd_&>(cmd));
          break;
        case t_do_chain_cmd_::cmd_id:
          process(err, static_cast<t_do_chain_cmd_&>(cmd));
          break;
        case t_clean_death_cmd_::cmd_id:
          process(err, static_cast<t_clean_death_cmd_&>(cmd));
          break;
        default:
          // made a mess
          // XXX- 16
          break;
      }
    }

///////////////////////////////////////////////////////////////////////////////

  private:
    class t_cmd_proxy_ : public t_event_logic {
    public:
      t_cmd_proxy_(tracing::t_err& err, t_action& action,
                   t_cmd_processor& processor, t_cmd_processor::t_logic& logic)
        : err_(err), action_(action), processor_(processor), logic_{logic} {
      }

      virtual t_name get_name() const override {
        return {"cmd logic"};
      }

      virtual t_action notify_event(r_event_params params) override {
        processor_.process(err_, logic_);
        return action_;
      }

    private:
      tracing::t_err&           err_;
      t_action&                 action_;
      t_cmd_processor&          processor_;
      t_cmd_processor::t_logic& logic_;
    };

    class t_que_proxy_ : public t_event_logic {
    public:
      t_que_proxy_(tracing::t_err& err, t_action& action,
                   t_que_processor& processor, t_que_processor::t_logic& logic)
        : err_(err), action_(action), processor_(processor), logic_{logic} {
      }

      virtual t_name get_name() const override {
        return {"queue logic"};
      }

      virtual t_action notify_event(r_event_params params) override {
        processor_.process(err_, logic_);
        return action_;
      }

    private:
      tracing::t_err&           err_;
      t_action&                 action_;
      t_que_processor&          processor_;
      t_que_processor::t_logic& logic_;
    };

    t_action        action_;
    t_data_         data_;
    t_cmd_processor cmd_processor_;
    t_que_processor que_processor_;
    t_dispatcher    dispatcher_;
  };

///////////////////////////////////////////////////////////////////////////////

  class t_tracing_ {
  public:
    t_tracing_(t_err& err, R_params params)
      : logic_     {err, params},
        cmd_client_{logic_.make_cmd_client()},
        que_client_{logic_.make_que_client()},
        thread_    {err, P_cstr{"tracing"}, &logic_, false},
        shared_tr_ {make_tracer(err, P_cstr{"shared"}, t_tracer_params())} {
    }

    t_tracer_name get_point_name(R_id id) {
      t_tracer_name name;
      t_get_point_name_cmd_ cmd(id, name);
      if (cmd_client_.request(cmd) == VALID)
        return name;
      return {};
    }

    t_level get_point_level(R_id id) {
      t_level level;
      t_get_point_level_cmd_ cmd(id, level);
      if (cmd_client_.request(cmd) == VALID)
        return level;
      return {};
    }

    t_validity update(t_err& err, R_params params) {
      t_update_params_cmd_ cmd(params);
      cmd_client_.request(err, cmd);
      return !err ? VALID : INVALID;
    }

    t_validity fetch(t_err& err, t_params& params) {
      t_fetch_params_cmd_ cmd(params);
      cmd_client_.request(err, cmd);
      return !err ? VALID : INVALID;
    }

    t_tracer make_tracer(t_err& err, R_tracer_name name,
                         R_tracer_params params) {
      t_id id{};
      t_make_tracer_cmd_ cmd(name, params, id);
      cmd_client_.request(err, cmd);
      return tracer::mk_(!err ? id : t_id{}, name);
    }

    t_bool update_tracer(t_err& err, R_wildcard_name name, t_level level) {
      t_bool updated = false;
      t_update_tracer_cmd_ cmd(name, level, updated);
      cmd_client_.request(err, cmd);
      return !err ? updated : false;
    }

    t_validity update_tracer(t_err& err, R_tracer_name name,
                             R_tracer_params params) {
      t_update_tracer_params_cmd_ cmd(name, params);
      cmd_client_.request(err, cmd);
      return !err ? VALID : INVALID;
    }

    t_bool fetch_tracer(t_err& err, R_tracer_name name,
                        r_tracer_params params) {
      t_bool found = false;
      t_fetch_tracer_params_cmd_ cmd(name, params, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool fetch_tracer(t_err& err, R_tracer_name name,
                        r_tracer_info info, t_bool clear_stats) {
      t_bool found = false;
      t_fetch_tracer_info_cmd_ cmd(name, info, clear_stats, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool fetch_tracers(t_err& err, t_tracer_infos& infos,
                         t_bool clear_stats) {
      t_bool found = false;
      t_fetch_tracers_cmd_ cmd(infos, clear_stats, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_validity create_observer(t_err& err, R_observer_name name,
                               R_observer_params params) {
      t_create_observer_cmd_ cmd(name, params);
      cmd_client_.request(err, cmd);
      return !err ? VALID : INVALID;
    }

    t_validity destroy_observer(t_err& err, R_observer_name name) {
      t_destroy_observer_cmd_ cmd(name);
      cmd_client_.request(err, cmd);
      return !err ? VALID : INVALID;
    }

    t_validity update_observer(t_err& err, R_observer_name name,
                               R_observer_params params) {
      t_update_observer_cmd_ cmd(name, params);
      cmd_client_.request(err, cmd);
      return !err ? VALID : INVALID;
    }

    t_bool fetch_observer(t_err& err, R_observer_name name,
                          r_observer_params params) {
      t_bool found = false;
      t_fetch_observer_cmd_ cmd(name, params, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool fetch_observer(t_err& err, R_observer_name name,
                          r_observer_info info, t_bool clear_stats) {
      t_bool found = false;
      t_fetch_observer_info_cmd_ cmd(name, info, clear_stats, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool fetch_observers(t_err& err, r_observer_infos infos,
                           t_bool clear_stats) {
      t_bool found = false;
      t_fetch_observers_cmd_ cmd(infos, clear_stats, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool bind_tracers (t_err& err, R_observer_name name,
                         R_wildcard_name wildcard_name) {
      t_bool found = false;
      t_bind_tracers_cmd_ cmd(name, wildcard_name, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool unbind_tracers(t_err& err, R_observer_name name,
                          R_wildcard_name wildcard_name) {
      t_bool found = false;
      t_unbind_tracers_cmd_ cmd(name, wildcard_name, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool is_tracer_bound(t_err& err, R_observer_name name,
                           R_tracer_name tracer_name) {
      t_bool found = false;
      t_is_tracer_bound_cmd_ cmd(name, tracer_name, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool fetch_bound_tracers(t_err& err, R_observer_name name,
                               r_tracer_names tracer_names) {
      t_bool found = false;
      t_fetch_bound_tracers_cmd_ cmd(name, tracer_names, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool fetch_bound_observers(t_err& err, R_tracer_name name,
                                 r_observer_names observer_names) {
      t_bool found = false;
      t_fetch_bound_observers_cmd_ cmd(name, observer_names, found);
      cmd_client_.request(err, cmd);
      return !err ? found : false;
    }

    t_bool destroy_tracer(tracer::t_id id) {
      t_bool done = false;
      t_destroy_tracer_cmd_ cmd(id, done);
      return cmd_client_.request(cmd) == VALID ? done : false;
    }

    t_validity do_chain(t_err& err, t_que_chain& chain) {
      t_do_chain_cmd_ cmd(chain);
      cmd_client_.request(err, cmd);
      return !err ? VALID : INVALID;
    }

    t_validity do_chain(t_que_chain& chain) {
      t_do_chain_cmd_ cmd(chain);
      return cmd_client_.request(cmd);
    }

    t_validity clean_death() {
      t_clean_death_cmd_ cmd;
      return cmd_client_.request(cmd);
    }

///////////////////////////////////////////////////////////////////////////////

    t_validity shared_trace(t_level level, R_textline line) {
      return ref(shared_tr_).post(level, line);
    }

    t_validity shared_trace(t_err err, t_level level, R_textline line) {
      return ref(shared_tr_).post(err, level, line);
    }

    t_validity post(R_id id, t_level level, R_textline line) {
      t_que_chain chain = que_client_.acquire();
      if (get(chain.cnt)) {
        t_item_& item = chain.head->ref().emplace<t_item_>(t_any_user{0L});
        item.line  = line;
        item.level = level;
        item.id    = id;
        item.time  = clock::realtime_now();
        return level > CRITICAL ? que_client_.insert(chain) :
                                  do_chain(chain);
      }
      return INVALID;
    }

    t_validity post(t_err& err, R_id id, t_level level, R_textline line) {
      t_que_chain chain = que_client_.acquire(err);
      if (!err) {
        t_item_& item = chain.head->ref().emplace<t_item_>(t_any_user{0L});
        item.line  = line;
        item.level = level;
        item.id    = id;
        item.time  = clock::realtime_now();
        return level > CRITICAL ?  que_client_.insert(err, chain) :
                                   do_chain(err, chain);
      }
      return INVALID;
    }

    t_validity waitable_post(R_id id, t_level level, R_textline line) {
      t_que_chain chain = que_client_.waitable_acquire();
      if (get(chain.cnt)) {
        t_item_& item = chain.head->ref().emplace<t_item_>(t_any_user{0L});
        item.line  = line;
        item.level = level;
        item.id    = id;
        item.time  = clock::realtime_now();
        return level > CRITICAL ?  que_client_.insert(chain) :
                                   do_chain(chain);
      }
      return INVALID;
    }

    t_validity waitable_post(t_err& err, R_id id, t_level level,
                             R_textline line) {
      t_que_chain chain = que_client_.waitable_acquire(err);
      if (!err) {
        t_item_& item = chain.head->ref().emplace<t_item_>(t_any_user{0L});
        item.line  = line;
        item.level = level;
        item.id    = id;
        item.time  = clock::realtime_now();
        return level > CRITICAL ?  que_client_.insert(err, chain) :
                                   do_chain(err, chain);
      }
      return INVALID;
    }

///////////////////////////////////////////////////////////////////////////////

  private:
    t_logic_     logic_;
    t_cmd_client cmd_client_;
    t_que_client que_client_;
    t_thread     thread_;
    t_tracer     shared_tr_;
  };

  std::unique_ptr<t_tracing_> tr_ = nullptr; // shared_ptr thread safe

///////////////////////////////////////////////////////////////////////////////

namespace tracer
{
  t_levelname to_name(t_level level) {
    const char* tbl[] = { "emerg",
                          "alert",
                          "critical",
                          "error",
                          "warning",
                          "notice",
                          "info",
                          "debug" };
    return P_cstr(tbl[level]);
  }

  t_level default_level() {
    return NOTICE;
  }

  t_credit default_credit() {
    return 50;
  }

  inline
  t_bool operator==(R_id lh, R_id rh) {
    return lh.seq_ == rh.seq_ && get(lh.id_)  == get(rh.id_);
  }

  inline
  t_bool operator!=(R_id lh, R_id rh) {
    return !(lh == rh);
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity t_point::post(t_level level, R_textline line) const {
    if (tracing::tr_)
      return tracing::tr_->post(id_, level, line);
    return INVALID;
  }

  t_validity t_point::post(t_err err, t_level level, R_textline line) const {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->post(err, id_, level, line);
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity t_point::waitable_post(t_level level, R_textline line) const {
    if (tracing::tr_)
      return tracing::tr_->waitable_post(id_, level, line);
    return INVALID;
  }

  t_validity t_point::waitable_post(t_err err, t_level level,
                                    R_textline line) const {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->waitable_post(err, id_, level, line);
      err = E_XXX;
    }
    return INVALID;
  }

  t_name t_point::get_name() const {
    if (tracing::tr_)
      return tracing::tr_->get_point_name(id_);
    return t_name();
  }

  t_level t_point::get_level() const {
    if (tracing::tr_)
      return tracing::tr_->get_point_level(id_);
    return EMERG;
  }

///////////////////////////////////////////////////////////////////////////////

  t_tracer::t_tracer(x_tracer tracer) : point_{tracer.point_.release()} {
  }

  t_tracer& t_tracer::operator=(x_tracer tracer) {
    if (point_ == VALID && tracing::tr_)
      tracing::tr_->destroy_tracer(point_.id_.release());
    point_.id_ = tracer.point_.id_.release();
    return *this;
  }

  t_tracer::~t_tracer() {
    if (point_ == VALID && tracing::tr_)
      tracing::tr_->destroy_tracer(point_.id_.release());
  }

  t_point t_tracer::make_point(R_name name) {
    // could do more with it but not now
    return t_point{point_.id_, name};
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity shared_trace(t_level level, R_textline line) {
    if (tracing::tr_)
      return tracing::tr_->shared_trace(level, line);
    return INVALID;
  }

  t_validity shared_trace(t_err err, t_level level, R_textline line) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->shared_trace(err, level, line);
      err = E_XXX;
    }
    return INVALID;
  }
}

///////////////////////////////////////////////////////////////////////////////

  t_output_name to_name(t_output output) {
    const char* tbl[] = { "logger",
                          "ftrace",
                          "shm" };
    return P_cstr(tbl[output]);
  }

  t_time_mode_name to_name(t_time_mode mode) {
    const char* tbl[] = { "ns",
                          "ns_diff",
                          "date" };
    return P_cstr(tbl[mode]);
  }

  t_mode_name to_name(t_mode mode) {
    const char* tbl[] = { "all",
                          "off",
                          "config" };
    return P_cstr(tbl[mode]);
  }

  t_level default_observer_level() {
    return NOTICE;
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity start(t_err err, P_params params) {
    T_ERR_GUARD(err) {
      static t_mutex_lock lock(err);
      <% auto scope = lock.make_locked_scope(err);
        if (scope == VALID) {
          if (!tr_) {
            tr_.reset(new t_tracing_{err, params ? *params : t_params{}});
            if (err)
              delete tr_.release();
          }
          return VALID;
        }
      %>
    }
    return INVALID;
  }

  t_void destroy() {
    if (tr_) {
      tr_->clean_death();
      delete tr_.release();
    }
  }

  t_bool is_running() {
    if (tr_)
      return true;
    return false;
  }

  t_validity update(t_err err, R_params params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->update(err, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity fetch(t_err err, r_params params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch(err, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_tracer make_tracer(t_err err, R_tracer_name name) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->make_tracer(err, name, t_tracer_params());
      err = E_XXX;
    }
    return mk_(tracer::t_id{}, tracer::t_name{});
  }

  t_tracer make_tracer(t_err err, R_tracer_name name, R_tracer_params params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->make_tracer(err, name, params);
      err = E_XXX;
    }
    return mk_(tracer::t_id{}, tracer::t_name{});
  }

  t_bool update_tracer(t_err err, R_wildcard_name name, t_level level) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->update_tracer(err, name, level);
      err = E_XXX;
    }
    return false;
  }

  t_validity update_tracer(t_err err, R_tracer_name name,
                           R_tracer_params params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->update_tracer(err, name, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_bool fetch_tracer(t_err err, R_tracer_name name, r_tracer_params params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_tracer(err, name, params);
      err = E_XXX;
    }
    return false;
  }

  t_bool fetch_tracer(t_err err, R_tracer_name name, r_tracer_info info,
                      t_bool clear_stats) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_tracer(err, name, info, clear_stats);
      err = E_XXX;
    }
    return false;
  }

  t_bool fetch_tracers(t_err err, r_tracer_infos infos, t_bool clear_stats) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_tracers(err, infos, clear_stats);
      err = E_XXX;
    }
    return false;
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity create_observer(t_err err, R_observer_name name) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->create_observer(err, name, t_observer_params());
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity create_observer(t_err err, R_observer_name name,
                             R_observer_params params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->create_observer(err, name, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity destroy_observer(t_err err, R_observer_name name) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->destroy_observer(err, name);
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity update_observer(t_err err, R_observer_name name,
                             R_observer_params params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->update_observer(err, name, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_bool fetch_observer(t_err err, R_observer_name name,
                        r_observer_params params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_observer(err, name, params);
    }
    return false;
  }

  t_bool fetch_observer(t_err err, R_observer_name name,
                        r_observer_info info, t_bool clear_stats) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_observer(err, name, info, clear_stats);
    }
    return false;
  }

  t_bool fetch_observers(t_err err, r_observer_infos infos,
                         t_bool clear_stats) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_observers(err, infos, clear_stats);
    }
    return false;
  }

///////////////////////////////////////////////////////////////////////////////

  t_bool bind_tracers (t_err err, R_observer_name name,
                       R_wildcard_name wildcard) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->bind_tracers(err, name, wildcard);
    }
    return false;
  }

  t_bool unbind_tracers(t_err err, R_observer_name name,
                        R_wildcard_name wildcard) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->unbind_tracers(err, name, wildcard);
    }
    return false;
  }

  t_bool is_tracer_bound(t_err err, R_observer_name name,
                         R_tracer_name tracer_name) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->is_tracer_bound(err, name, tracer_name);
    }
    return false;
  }

  t_bool fetch_bound_tracers(t_err err, R_observer_name name,
                             r_tracer_names tracer_names) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_bound_tracers(err, name, tracer_names);
    }
    return false;
  }

  t_bool fetch_bound_observers(t_err err, R_tracer_name name,
                               r_observer_names observer_names) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_bound_observers(err, name, observer_names);
    }
    return false;
  }

///////////////////////////////////////////////////////////////////////////////

  struct automatic_start_ {
    automatic_start_() {
      t_err err;
      if (start(err) == INVALID) {
        // what to do
        // XXX - 11
      }
    }

    ~automatic_start_() {
      if (tr_)
        destroy();
    }
  };

  automatic_start_ start_;

///////////////////////////////////////////////////////////////////////////////
}
}
