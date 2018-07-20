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

using namespace dainty::named;
using namespace dainty::mt;

using dainty::os::threading::t_mutex_lock;
using dainty::mt::thread::t_thread;
using dainty::tracing::tracer::t_textline;

using t_thd_err       = t_thread::t_logic::t_err;
using t_cmd_err       = command::t_processor::t_logic::t_err;
using t_cmd_client    = command::t_client;
using t_cmd_processor = command::t_processor;
using t_que_client    = waitable_chained_queue::t_client;
using t_que_processor = waitable_chained_queue::t_processor;

namespace dainty
{
namespace tracing
{
///////////////////////////////////////////////////////////////////////////////

  //commands

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

    virtual t_void process(t_cmd_err err, t_user,
                           r_command cmd) noexcept override {
      // process all the commands
    }

    virtual t_void async_process(t_user, p_command cmd) noexcept override {
      // not used
    }

    virtual t_void async_process(t_chain chain) noexcept override {
      // handle items
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
    t_tracing(t_err err, const t_params& params)
      : logic_     {err, params},
        cmd_client_{logic_.make_cmd_client()},
        que_client_{logic_.make_que_client()},
        thread_    {err, p_cstr{"tracing"}, &logic_, false} {
    }

///////////////////////////////////////////////////////////////////////////////

    t_bool post(t_level level, t_textline&& line) const {
      return false;
    }

    t_bool post(t_err err, t_level level, t_textline&& line) const {
      T_ERR_GUARD(err) {
      }
      return false;
    }

    t_validity waitable_post(t_level level, t_textline&& line) const {
      return INVALID;
    }

    t_validity waitable_post(t_err err, t_level level,
                             t_textline&& line) const {
      T_ERR_GUARD(err) {
      }
      return INVALID;
    }

///////////////////////////////////////////////////////////////////////////////

  private:
    t_logic      logic_;
    t_cmd_client cmd_client_;
    t_que_client que_client_;
    t_thread     thread_;
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

  t_bool t_point::post(t_level level, t_textline&& line) const {
    if (tracing::tr_)
      return tracing::tr_->post(level, std::move(line));
    return false;
  }

  t_bool t_point::post(t_err err, t_level level, t_textline&& line) const {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->post(err, level, std::move(line));
      err = E_XXX;
    }
    return false;
  }

  t_validity t_point::waitable_post(t_level level, t_textline&& line) const {
    if (tracing::tr_)
      return tracing::tr_->waitable_post(level, std::move(line));
    return INVALID;
  }

  t_validity t_point::waitable_post(t_err err, t_level level,
                                    t_textline&& line) const {
    T_ERR_GUARD(err) {
      if (tracing::tr_)
        return tracing::tr_->waitable_post(err, level, std::move(line));
      err = E_XXX;
    }
    return INVALID;
  }

  t_name t_point::get_name() const {
    return t_name();
  }

  t_level t_point::get_level() const {
    return NONE;
  }

///////////////////////////////////////////////////////////////////////////////

  t_tracer::t_tracer(t_tracer&& tracer) : point_{tracer.point_.release()} {
  }

  t_tracer& t_tracer::operator=(t_tracer&& tracer) {
    if (point_ == VALID)
      destroy_tracer_(point_.id_);
    point_.id_ = tracer.point_.id_.release();
    return *this;
  }

  t_tracer::~t_tracer() {                          //XXX
  }

  t_point t_tracer::make_point(const t_name& name) {
    return t_point{point_.id_, name};
  }

  t_validity shared_trace(const t_textline&) {
    return INVALID;
  }

  t_validity shared_trace(t_err, const t_textline&) {
    return INVALID;
  }
}

///////////////////////////////////////////////////////////////////////////////

  t_output_name to_name(t_output) {
    return t_output_name();
  }

  t_time_mode_name to_name(t_time_mode) {
    return t_time_mode_name();
  }

  t_mode_name to_name(t_mode) {
    return t_mode_name();
  }

  t_level default_observer_level() {
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

  t_void update(const t_params&) {
  }

  t_void fetch(t_params&) {
  }

///////////////////////////////////////////////////////////////////////////////

  tracer::t_tracer make_tracer(t_err, const t_tracer_name& name,
                                      const t_tracer_params&) {
    return tracer::mk_(tracer::t_id{}, name);
  }

  t_bool update_tracer(t_err, const t_wildcard_name&, t_level) {
    return false;
  }

  t_validity update_tracer(t_err, const t_tracer_name&,
                                  const t_tracer_params&) {
    return VALID;
  }

  t_bool fetch_tracer(t_err, const t_tracer_name&, t_tracer_params&) {
    return false;
  }

  t_bool fetch_tracer(t_err, const t_tracer_name&, t_tracer_info&,
                              t_bool clearstats) {
    return false;
  }

  t_bool fetch_tracers(t_tracer_infos&, t_bool clearstats) {
    return false;
  }

///////////////////////////////////////////////////////////////////////////////

  t_validity create_observer(t_err, const t_observer_name&,
                                    const t_observer_params&) {
    return VALID;
  }

  t_validity destroy_observer(t_err, const t_observer_name&) {
    return VALID;
  }

  t_validity update_observer(t_err, const t_observer_name&,
                                    const t_observer_params&) {
    return VALID;
  }

  t_bool fetch_observer(t_err, const t_observer_name&, t_observer_params&) {
    return false;
  }

  t_bool fetch_observer(t_err, const t_observer_name&, t_observer_info&,
                               t_bool clearstats) {
    return false;
  }

  t_bool fetch_observers(t_err, t_observer_infos&, t_bool clearstats) {
    return false;
  }

///////////////////////////////////////////////////////////////////////////////

  t_bool bind_tracers (t_err, const t_observer_name&,
                              const t_wildcard_name&) {
    return false;
  }

  t_bool unbind_tracers(t_err, const t_observer_name&,
                               const t_wildcard_name&) {
    return false;
  }

  t_bool is_tracer_bound(t_err, const t_observer_name&, const t_tracer_name&) {
    return false;
  }

  t_validity fetch_bound_tracers(t_err, const t_observer_name&,
                                        t_tracer_names&) {
    return VALID;
  }

///////////////////////////////////////////////////////////////////////////////

  t_bool destroy_tracer_(tracer::t_id) {
    return false;
  }

///////////////////////////////////////////////////////////////////////////////
}
}
