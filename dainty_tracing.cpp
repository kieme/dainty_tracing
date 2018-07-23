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

#include "dainty_mt_waitable_chained_queue.h"
#include "dainty_mt_command.h"
#include "dainty_mt_thread.h"
#include "dainty_os_threading.h"
#include "dainty_tracing.h"

using namespace dainty::container;
using namespace dainty::named;
using namespace dainty::mt;

using dainty::container::any::t_any;
using dainty::os::threading::t_mutex_lock;
using dainty::mt::thread::t_thread;
using dainty::tracing::tracer::t_id;
using dainty::tracing::tracer::t_textline;
using dainty::tracing::tracer::t_tracer;

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
  t_tracer mk_(const t_id& id, const t_name& name) {
    return {id, name};
  }
}

///////////////////////////////////////////////////////////////////////////////

  struct t_item {
    tracer::t_id    id;
    tracer::t_level level;
    t_textline      line;
    // time stamp
    // cnt
  };

  t_bool operator==(const t_item& lh, const t_item& rh) {
    return lh.line == rh.line;
  }

///////////////////////////////////////////////////////////////////////////////

  struct t_update_params_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 1;
    const t_params& params;

    inline
    t_update_params_cmd(const t_params& _params)
      : t_cmd{cmd_id}, params(_params) {
    };
  };

  struct t_fetch_params_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 2;
    t_params& params;

    inline
    t_fetch_params_cmd(t_params& _params) : t_cmd{cmd_id}, params(_params) {
    };
  };

  struct t_get_point_name_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 3;
    const t_id&    id;
    t_tracer_name& name;

    inline
    t_get_point_name_cmd(const t_id& _id, t_tracer_name& _name)
      : t_cmd{cmd_id}, id(_id), name(_name) {
    };
  };

  struct t_get_point_level_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 4;
    const t_id& id;
    t_level&    level;

    inline
    t_get_point_level_cmd(const t_id& _id, t_level& _level)
      : t_cmd{cmd_id}, id(_id), level(_level) {
    };
  };

  struct t_make_tracer_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 5;
    const t_tracer_name&   name;
    const t_tracer_params& params;

    inline
    t_make_tracer_cmd(const t_tracer_name& _name,
                      const t_tracer_params& _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };

  struct t_update_tracer_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 6;
    const t_wildcard_name& name;
          t_level          level;

    inline
    t_update_tracer_cmd(const t_wildcard_name& _name, t_level _level)
      : t_cmd{cmd_id}, name(_name), level(_level) {
    };
  };

  struct t_update_tracer_params_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 7;
    const t_tracer_name& name;
    const t_tracer_params& params;

    inline
    t_update_tracer_params_cmd(const t_tracer_name& _name,
                               const t_tracer_params& _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };

  struct t_fetch_tracer_params_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 8;
    const t_tracer_name& name;
          t_tracer_params& params;

    inline
    t_fetch_tracer_params_cmd(const t_tracer_name& _name,
                              t_tracer_params& _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };

  struct t_fetch_tracer_info_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 9;
    const t_tracer_name& name;
          t_tracer_info& info;
          t_bool         clear_stats;

    inline
    t_fetch_tracer_info_cmd(const t_tracer_name& _name,
                            t_tracer_info& _info,
                            t_bool _clear_stats)
      : t_cmd{cmd_id}, name(_name), info(_info), clear_stats(_clear_stats) {
    };
  };

  struct t_fetch_tracers_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 10;
    t_tracer_infos& infos;
    t_bool          clear_stats;

    inline
    t_fetch_tracers_cmd(t_tracer_infos& _infos,
                        t_bool _clear_stats)
      : t_cmd{cmd_id}, infos(_infos), clear_stats(_clear_stats) {
    };
  };

  struct t_create_observer_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 11;
    const t_observer_name&   name;
    const t_observer_params& params;

    inline
    t_create_observer_cmd(const t_observer_name& _name,
                         t_observer_params& _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };


  struct t_destroy_observer_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 12;
    const t_observer_name& name;

    inline
    t_destroy_observer_cmd(const t_observer_name& _name)
      : t_cmd{cmd_id}, name(_name) {
    };
  };

  struct t_update_observer_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 13;
    const t_observer_name& name;
    const t_observer_params& params;

    inline
    t_update_observer_cmd(const t_observer_name& _name,
                          const t_observer_params& _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };

  struct t_fetch_observer_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 14;
    const t_observer_name& name;
          t_observer_params& params;

    inline
    t_fetch_observer_cmd(const t_observer_name& _name,
                         t_observer_params& _params)
      : t_cmd{cmd_id}, name(_name), params(_params) {
    };
  };

  struct t_fetch_observer_info_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 15;
    const t_observer_name& name;
          t_observer_info& info;

    inline
    t_fetch_observer_info_cmd(const t_observer_name& _name,
                              t_observer_info& _info)
      : t_cmd{cmd_id}, name(_name), info(_info) {
    };
  };

  struct t_fetch_observers_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 16;
    t_observer_infos& infos;
    t_bool            clear_stats;

    inline
    t_fetch_observers_cmd(t_observer_infos& _infos, t_bool _clear_stats)
      : t_cmd{cmd_id}, infos(_infos), clear_stats(_clear_stats) {
    };
  };

  struct t_bind_tracers_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 17;
    const t_observer_name& name;
    const t_wildcard_name& wildcard_name;

    inline
    t_bind_tracers_cmd(const t_observer_name& _name,
                       const t_wildcard_name& _wildcard_name)
      : t_cmd{cmd_id}, name(_name), wildcard_name(_wildcard_name) {
    };
  };

  struct t_unbind_tracers_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 18;
    const t_observer_name& name;
    const t_wildcard_name& wildcard_name;

    inline
    t_unbind_tracers_cmd(const t_observer_name& _name,
                         const t_wildcard_name& _wildcard_name)
      : t_cmd{cmd_id}, name(_name), wildcard_name(_wildcard_name) {
    };
  };

  struct t_is_tracer_bound_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 19;
    const t_observer_name& name;
    const t_tracer_name& tracer_name;

    inline
    t_is_tracer_bound_cmd(const t_observer_name& _name,
                          const t_tracer_name& _tracer_name)
      : t_cmd{cmd_id}, name(_name), tracer_name(_tracer_name) {
    };
  };

  struct t_fetch_bound_tracers_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 20;
    const t_observer_name& name;
          t_tracer_names& tracer_names;

    inline
    t_fetch_bound_tracers_cmd(const t_observer_name& _name,
                                    t_tracer_names& _tracer_names)
      : t_cmd{cmd_id}, name(_name), tracer_names(_tracer_names) {
    };
  };

  struct t_destroy_tracer_cmd : t_cmd {
    constexpr static command::t_id cmd_id = 21;
    const t_id& id;

    inline
    t_destroy_tracer_cmd(const t_id& _id) : t_cmd{cmd_id}, id(_id) {
    };
  };

///////////////////////////////////////////////////////////////////////////////

  class t_logic : public t_thread::t_logic,
                  public t_cmd_processor::t_logic,
                  public t_que_processor::t_logic {
  public:
    t_logic(tracing::t_err err, const t_params& params)
      : params_{params},
        cmd_processor_{err},
        que_processor_{err, params.queuesize} {
    }

    t_cmd_client make_cmd_client() {
      return cmd_processor_.make_client(command::t_user{0L});
    }

    t_que_client make_que_client() {
      return que_processor_.make_client(waitable_chained_queue::t_user{0L});
    }

///////////////////////////////////////////////////////////////////////////////

    virtual t_validity update(t_thd_err err,
                              ::pthread_attr_t&) noexcept override {
      return VALID;
    }

    virtual t_validity prepare(t_thd_err err) noexcept override {
      return VALID;
    }

    virtual p_void run() noexcept override {
      // loop
      //event_dispatcher
      return nullptr;
    }

///////////////////////////////////////////////////////////////////////////////

    virtual t_void async_process(t_user, p_command cmd) noexcept override {
      // not used
    }

    virtual t_void async_process(t_chain chain) noexcept override {
      // unpacking of chain/any
    }

///////////////////////////////////////////////////////////////////////////////

    t_void process(tracing::t_err err, t_update_params_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_params_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_get_point_name_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_get_point_level_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_make_tracer_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_update_tracer_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_update_tracer_params_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_tracer_params_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_tracer_info_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_tracers_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_create_observer_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_destroy_observer_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_update_observer_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_observer_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_observer_info_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_observers_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_bind_tracers_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_unbind_tracers_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_is_tracer_bound_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_fetch_bound_tracers_cmd&) noexcept {
      // work done
    }

    t_void process(tracing::t_err err, t_destroy_tracer_cmd&) noexcept {
      // work done
    }

    virtual t_void process(t_cmd_err err, t_user,
                           r_command cmd) noexcept override {
      switch (cmd.id) {
        case t_update_params_cmd::cmd_id: {
          process(err, static_cast<t_update_params_cmd&>(cmd));
        } break;
        case t_fetch_params_cmd::cmd_id: {
          process(err, static_cast<t_fetch_params_cmd&>(cmd));
        } break;
        case t_get_point_name_cmd::cmd_id: {
          process(err, static_cast<t_get_point_name_cmd&>(cmd));
        } break;
        case t_get_point_level_cmd::cmd_id: {
          process(err, static_cast<t_get_point_level_cmd&>(cmd));
        } break;
        case t_make_tracer_cmd::cmd_id: {
          process(err, static_cast<t_make_tracer_cmd&>(cmd));
        } break;
        case t_update_tracer_cmd::cmd_id: {
          process(err, static_cast<t_update_tracer_cmd&>(cmd));
        } break;
        case t_update_tracer_params_cmd::cmd_id: {
          process(err, static_cast<t_update_tracer_params_cmd&>(cmd));
        } break;
        case t_fetch_tracer_params_cmd::cmd_id: {
          process(err, static_cast<t_fetch_tracer_params_cmd&>(cmd));
        } break;
        case t_fetch_tracer_info_cmd::cmd_id: {
          process(err, static_cast<t_fetch_tracer_info_cmd&>(cmd));
        } break;
        case t_fetch_tracers_cmd::cmd_id: {
          process(err, static_cast<t_fetch_tracers_cmd&>(cmd));
        } break;
        case t_create_observer_cmd::cmd_id: {
          process(err, static_cast<t_create_observer_cmd&>(cmd));
        } break;
        case t_destroy_observer_cmd::cmd_id: {
          process(err, static_cast<t_destroy_observer_cmd&>(cmd));
        } break;
        case t_update_observer_cmd::cmd_id: {
          process(err, static_cast<t_update_observer_cmd&>(cmd));
        } break;
        case t_fetch_observer_cmd::cmd_id: {
          process(err, static_cast<t_fetch_observer_cmd&>(cmd));
        } break;
        case t_fetch_observer_info_cmd::cmd_id: {
          process(err, static_cast<t_fetch_observer_info_cmd&>(cmd));
        } break;
        case t_fetch_observers_cmd::cmd_id: {
          process(err, static_cast<t_fetch_observers_cmd&>(cmd));
        } break;
        case t_bind_tracers_cmd::cmd_id: {
          process(err, static_cast<t_bind_tracers_cmd&>(cmd));
        } break;
        case t_unbind_tracers_cmd::cmd_id: {
          process(err, static_cast<t_unbind_tracers_cmd&>(cmd));
        } break;
        case t_is_tracer_bound_cmd::cmd_id: {
          process(err, static_cast<t_is_tracer_bound_cmd&>(cmd));
        } break;
        case t_fetch_bound_tracers_cmd::cmd_id: {
          process(err, static_cast<t_fetch_bound_tracers_cmd&>(cmd));
        } break;
      }
      // process all the commands
    }

///////////////////////////////////////////////////////////////////////////////

  private:
    t_params        params_;
    t_cmd_processor cmd_processor_;
    t_que_processor que_processor_;
    // observers
    // tracers
  };

///////////////////////////////////////////////////////////////////////////////

  class t_tracing {
  public:
    t_tracing(t_err& err, const t_params& params)
      : logic_     {err, params},
        cmd_client_{logic_.make_cmd_client()},
        que_client_{logic_.make_que_client()},
        thread_    {err, p_cstr{"tracing"}, &logic_, false},
        shared_tr_ {make_tracer(err, p_cstr{"shared"}, t_tracer_params())} {
    }

///////////////////////////////////////////////////////////////////////////////

    t_validity shared_trace(t_level level, const t_textline& line) {
      return ref(shared_tr_).post(level, line);
    }

    t_validity shared_trace(t_err err, t_level level, const t_textline& line) {
      return ref(shared_tr_).post(err, level, line);
    }

    t_validity post(const t_id& id, t_level level, const t_textline& line) {
      t_que_chain chain = que_client_.acquire();
      if (get(chain.cnt)) {
        // XXX- timestamp
        t_item item {};
        item.line  = line;
        item.level = level;
        item.id    = id;
        chain.head->ref().assign(t_any_user{0L}, std::move(item));
        return que_client_.insert(chain);
      }
      return INVALID;
    }

    t_validity post(t_err& err, const t_id& id, t_level level,
                    const t_textline& line) {
      t_que_chain chain = que_client_.acquire(err);
      if (!err) {
        // XXX- timestamp
        t_item item {};
        item.line  = line;
        item.level = level;
        item.id    = id;
        chain.head->ref().assign(t_any_user{0L}, std::move(item));
        return que_client_.insert(err, chain);
      }
      return INVALID;
    }

    t_validity waitable_post(const t_id& id, t_level level,
                             const t_textline& line) {
      t_que_chain chain = que_client_.waitable_acquire();
      if (get(chain.cnt)) {
        // XXX- timestamp
        t_item item {};
        item.line  = line;
        item.level = level;
        item.id    = id;
        chain.head->ref().assign(t_any_user{0L}, std::move(item));
        return que_client_.insert(chain);
      }
      return INVALID;
    }

    t_validity waitable_post(t_err& err, const t_id& id, t_level level,
                             const t_textline& line) {
      t_que_chain chain = que_client_.waitable_acquire(err);
      if (!err) {
        // XXX- timestamp
        t_item item {};
        item.line  = line;
        item.level = level;
        item.id    = id;
        chain.head->ref().assign(t_any_user{0L}, std::move(item));
        return que_client_.insert(err, chain);
      }
      return INVALID;
    }

///////////////////////////////////////////////////////////////////////////////

    t_tracer_name get_point_name(const t_id& id) {
      t_tracer_name name;
      t_get_point_name_cmd cmd(id, name);
      if (cmd_client_.request(cmd) == VALID)
        return name;
      return {};
    }

    t_level get_point_level(const t_id& id) {
      t_level level;
      t_get_point_level_cmd cmd(id, level);
      if (cmd_client_.request(cmd) == VALID)
        return level;
      return {};
    }

    t_validity update(t_err& err, const t_params& params) {
      t_update_params_cmd cmd(params);
      return cmd_client_.request(err, cmd);
    }

    t_validity fetch(t_err& err, t_params& params) {
      t_fetch_params_cmd cmd(params);
      return cmd_client_.request(err, cmd);
    }

    t_tracer make_tracer(t_err& err, const t_tracer_name& name,
                         const t_tracer_params& params) {
      return tracer::mk_(tracer::t_id{}, name); // XXX-1
    }

    t_bool update_tracer(t_err& err, const t_wildcard_name& name,
                         t_level level) {
      return false; //XXX-2
    }

    t_validity update_tracer(t_err& err, const t_tracer_name& name,
                             const t_tracer_params& params) {
      return VALID; //XXX-3
    }

    t_bool fetch_tracer(t_err& err, const t_tracer_name& name,
                        t_tracer_params& params) {
      return false; //XXX-4
    }

    t_bool fetch_tracer(t_err& err, const t_tracer_name& name,
                        t_tracer_info& info, t_bool clearstats) {
      return false; // XXX - 5
    }

    t_bool fetch_tracers(t_err& err, t_tracer_infos& infos,
                         t_bool clearstats) {
      return false; //XXX -6
    }

    t_validity create_observer(t_err& err, const t_observer_name& name,
                               const t_observer_params& params) {
      return VALID; //XXX-7
    }

    t_validity destroy_observer(t_err& err, const t_observer_name& name) {
      return VALID; //XXX-8
    }

    t_validity update_observer(t_err& err, const t_observer_name& name,
                               const t_observer_params& params) {
      return VALID; //XXX-9
    }

    t_bool fetch_observer(t_err& err, const t_observer_name& name,
                          t_observer_params& params) {
      return false; //XXX-9
    }

    t_bool fetch_observer(t_err& err, const t_observer_name& name,
                          t_observer_info& info, t_bool clearstats) {
      return false; //XXX-10
    }

    t_bool fetch_observers(t_err& err, t_observer_infos& infos,
                           t_bool clearstats) {
      return false; //XXX-11
    }

    t_bool bind_tracers (t_err& err, const t_observer_name& name,
                         const t_wildcard_name& wildcard_name) {
      return false; //XXX-12
    }

    t_bool unbind_tracers(t_err& err, const t_observer_name& name,
                          const t_wildcard_name& wildcard_name) {
      return false; //XXX-13
    }

    t_bool is_tracer_bound(t_err& err, const t_observer_name& name,
                           const t_tracer_name& tracer_name) {
      return false; //XXX-14
    }

    t_bool fetch_bound_tracers(t_err& err, const t_observer_name& name,
                               t_tracer_names& tracer_names) {
      return false; //XXX-15
    }

    t_bool destroy_tracer(tracer::t_id) {
      return false; //XXX-16
    }

///////////////////////////////////////////////////////////////////////////////

  private:
    t_logic      logic_;
    t_cmd_client cmd_client_;
    t_que_client que_client_;
    t_thread     thread_;
    t_tracer     shared_tr_;
  };


///////////////////////////////////////////////////////////////////////////////

  t_tracing* tr_ = nullptr;

  struct automatic_start {
    automatic_start() {
      t_err err;
      if (start(err) == INVALID) {
      }
    }

    ~automatic_start() {
      if (tr_) {
        //t_die_cmd;
        // tr_.die_now();
      }
    }
  };

///////////////////////////////////////////////////////////////////////////////

  automatic_start start_;

///////////////////////////////////////////////////////////////////////////////

namespace tracer
{
  t_levelname to_name(t_level) {
    return t_levelname();
  }

  t_level default_level() {
    return NONE;
  }

  t_credit default_credit() {
    return 0;
  }

  t_bool operator==(const t_id&, const t_id&) { //XXX
    return false;
  }

  t_bool operator!=(const t_id&, const t_id&) { //XXX
    return false;
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity t_point::post(t_level level, const t_textline& line) const {
    if (tracing::tr_)
      return tracing::tr_->post(id_, level, line);
    return INVALID;
  }

  t_validity t_point::post(t_err err, t_level level,
                           const t_textline& line) const {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->post(err, id_, level, line);
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity t_point::waitable_post(t_level level,
                                    const t_textline& line) const {
    if (tracing::tr_)
      return tracing::tr_->waitable_post(id_, level, line);
    return INVALID;
  }

  t_validity t_point::waitable_post(t_err err, t_level level,
                                    const t_textline& line) const {
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
    return NONE;
  }

///////////////////////////////////////////////////////////////////////////////

  t_tracer::t_tracer(t_tracer&& tracer) : point_{tracer.point_.release()} {
  }

  t_tracer& t_tracer::operator=(t_tracer&& tracer) {
    if (point_ == VALID)
      tracing::tr_->destroy_tracer(point_.id_);
    point_.id_ = tracer.point_.id_.release();
    return *this;
  }

  t_tracer::~t_tracer() {
    if (point_ == VALID)
      tracing::tr_->destroy_tracer(point_.id_);
  }

  t_point t_tracer::make_point(const t_name& name) {
    return t_point{point_.id_, name};
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity shared_trace(t_level level, const t_textline& line) {
    if (tracing::tr_)
      return tracing::tr_->shared_trace(level, line);
    return INVALID;
  }

  t_validity shared_trace(t_err err, t_level level, const t_textline& line) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->shared_trace(err, level, line);
      err = E_XXX;
    }
    return INVALID;
  }
}

///////////////////////////////////////////////////////////////////////////////

  t_output_name to_name(t_output) {
    // XXX
    return t_output_name();
  }

  t_time_mode_name to_name(t_time_mode) {
    // XXX
    return t_time_mode_name();
  }

  t_mode_name to_name(t_mode) {
    // XXX
    return t_mode_name();
  }

  t_level default_observer_level() {
    // XXX
    return NONE;
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity start(t_err err) {
    T_ERR_GUARD(err) {
      static t_mutex_lock lock(err);
      <% auto scope = lock.make_locked_scope(err);
        if (scope == VALID) {
          if (!tr_) {
            tr_ = new t_tracing{err, t_params{}};
            if (err) {
              delete tr_;
              tr_ = nullptr;
            }
          }
          return VALID;
        }
      %>
    }
    return INVALID;
  }

  t_bool is_running() {
    if (tr_)
      return true;
    return false;
  }

  t_validity update(t_err err, const t_params& params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->update(err, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity fetch(t_err err, t_params& params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch(err, params);
      err = E_XXX;
    }
    return INVALID;
  }

///////////////////////////////////////////////////////////////////////////////

  t_tracer make_tracer(t_err err, const t_tracer_name& name,
                       const t_tracer_params& params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->make_tracer(err, name, params);
      err = E_XXX;
    }
    return mk_(tracer::t_id{}, tracer::t_name{});
  }

  t_bool update_tracer(t_err err, const t_wildcard_name& name, t_level level) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->update_tracer(err, name, level);
      err = E_XXX;
    }
    return false;
  }

  t_validity update_tracer(t_err err, const t_tracer_name& name,
                           const t_tracer_params& params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->update_tracer(err, name, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_bool fetch_tracer(t_err err, const t_tracer_name& name,
                      t_tracer_params& params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_tracer(err, name, params);
      err = E_XXX;
    }
    return false;
  }

  t_bool fetch_tracer(t_err err, const t_tracer_name& name,
                      t_tracer_info& info, t_bool clear_stats) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_tracer(err, name, info, clear_stats);
      err = E_XXX;
    }
    return false;
  }

  t_bool fetch_tracers(t_err err, t_tracer_infos& infos, t_bool clear_stats) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_tracers(err, infos, clear_stats);
      err = E_XXX;
    }
    return false;
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity create_observer(t_err err, const t_observer_name& name,
                             const t_observer_params& params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->create_observer(err, name, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity destroy_observer(t_err err, const t_observer_name& name) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->destroy_observer(err, name);
      err = E_XXX;
    }
    return INVALID;
  }

  t_validity update_observer(t_err err, const t_observer_name& name,
                             const t_observer_params& params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->update_observer(err, name, params);
      err = E_XXX;
    }
    return INVALID;
  }

  t_bool fetch_observer(t_err err, const t_observer_name& name,
                        t_observer_params& params) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_observer(err, name, params);
    }
    return false;
  }

  t_bool fetch_observer(t_err err, const t_observer_name& name,
                        t_observer_info& info, t_bool clear_stats) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_observer(err, name, info, clear_stats);
    }
    return false;
  }

  t_bool fetch_observers(t_err err, t_observer_infos& infos,
                         t_bool clear_stats) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_observers(err, infos, clear_stats);
    }
    return false;
  }

///////////////////////////////////////////////////////////////////////////////

  t_bool bind_tracers (t_err err, const t_observer_name& name,
                       const t_wildcard_name& wildcard) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->bind_tracers(err, name, wildcard);
    }
    return false;
  }

  t_bool unbind_tracers(t_err err, const t_observer_name& name,
                        const t_wildcard_name& wildcard) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->unbind_tracers(err, name, wildcard);
    }
    return false;
  }

  t_bool is_tracer_bound(t_err err, const t_observer_name& name,
                         const t_tracer_name& tracer_name) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->is_tracer_bound(err, name, tracer_name);
    }
    return false;
  }

  t_bool fetch_bound_tracers(t_err err, const t_observer_name& name,
                             t_tracer_names& tracer_names) {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->fetch_bound_tracers(err, name, tracer_names);
    }
    return false;
  }

///////////////////////////////////////////////////////////////////////////////
}
}
