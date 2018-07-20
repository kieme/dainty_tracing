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

#ifndef _DAINTY_TRACING_H_
#define _DAINTY_TRACING_H_

#include <vector>
#include "dainty_tracing_tracer.h"

namespace dainty
{
namespace tracing
{
  using named::t_n;
  using tracer::t_string;
  using tracer::p_cstr_;
  using tracer::t_void;
  using tracer::t_bool;
  using tracer::t_validity;
  using tracer::t_level;
  using tracer::t_credit;
  using tracer::VALID;
  using tracer::INVALID;
  using tracer::FMT;
  using tracer::NONE;
  using tracer::EMERG;
  using tracer::ALERT;
  using tracer::CRITICAL;
  using tracer::ERROR;
  using tracer::WARNING;
  using tracer::NOTICE;
  using tracer::INFO;
  using tracer::DEBUG;
  using tracer::to_name;

////////////////////////////////////////////////////////////////////////////////

  using t_tracer_name   = tracer::t_name;
  using t_tracer_params = tracer::t_params;

  enum  t_wildcard_name_tag_ { };
  using t_wildcard_name = t_string<t_wildcard_name_tag_, 32>;

  enum  t_observer_name_tag_ { };
  using t_observer_name = t_string<t_observer_name_tag_, 32>;

////////////////////////////////////////////////////////////////////////////////

  enum t_output {
    SHM,
    LOGGER,
    FTRACE
  };

  enum  t_output_name_tag_ { };
  using t_output_name = t_string<t_output_name_tag_, 20>;

  t_output_name to_name(t_output);

////////////////////////////////////////////////////////////////////////////////

  enum t_time_mode {
    NS,
    NS_DIFF,
    DATE
  };

  enum  t_time_mode_name_tag_ { };
  using t_time_mode_name = t_string<t_time_mode_name_tag_, 20>;

  t_time_mode_name to_name(t_time_mode);

////////////////////////////////////////////////////////////////////////////////

  enum t_mode {
    ALL = 0,
    OFF,
    CONFIG
  };

  enum  t_mode_name_tag_ { };
  using t_mode_name = t_string<t_mode_name_tag_, 20>;

  t_mode_name to_name(t_mode);

////////////////////////////////////////////////////////////////////////////////

  class t_stats {
  public:
    using t_n = tracing::t_n;

    t_stats()                                             { reset(); }

    t_n get      (t_level level) const { return t_n{  used_[level]}; }
    t_n increment(t_level level)       { return t_n{++used_[level]}; }

    t_n reset() {
      t_n_ sum = 0;
      for (t_n_ i = 0; i < sizeof(used_)/sizeof(t_n_); ++i) {
        sum += used_[i];
        used_[i] = 0;
      }
      return t_n{sum};
    }

    t_n total() const {
      t_n_ sum = 0;
      for (t_n_ i = 0; i < sizeof(used_)/sizeof(t_n_); ++i)
        sum += used_[i];
      return t_n{sum};
    }

  private:
    using t_n_ = named::t_n_;
    t_n_ used_[DEBUG + 1];
  };

////////////////////////////////////////////////////////////////////////////////

  t_level default_observer_level();

  class t_observer_params {
  public:
    t_observer_params(t_level _level = default_observer_level())
      : level(_level) {
    }

    t_level level;
  };

////////////////////////////////////////////////////////////////////////////////

  class t_tracer_info {
  public:
    tracer::t_id    id;
    t_tracer_name   name;
    t_tracer_params params;
    t_stats         stats;
  };

  class t_observer_info {
  public:
    t_observer_info() { }
    t_observer_info(const t_observer_name&  _name,
                    const t_observer_params& _params)
      : name(_name), params(_params) {
    }

    tracer::t_id      id;
    t_observer_name   name;
    t_observer_params params;
    t_stats           stats;
  };

  using t_tracer_names   = std::vector<t_tracer_name>;
  using t_observer_infos = std::vector<t_observer_info>;
  using t_tracer_infos   = std::vector<t_tracer_info>;

////////////////////////////////////////////////////////////////////////////////

  class t_params {
  public:
    t_bool      to_terminal;
    t_bool      to_observers;
    t_time_mode time_mode;
    t_mode      mode;
    t_n         textline_len;
    const t_n   queuesize;

    t_params() : to_terminal (true),
                 to_observers(true),
                 time_mode   (DATE),
                 mode        (CONFIG),
                 textline_len(100),
                 queuesize   (4000) {
    }

    t_params(t_bool      _to_terminal,
             t_bool      _to_observers,
             t_time_mode _time_mode,
             t_mode      _mode,
             t_n         _textline_len,
             t_n         _queuesize)
      : to_terminal (_to_terminal),
        to_observers(_to_observers),
        time_mode   (_time_mode),
        mode        (_mode),
        textline_len(_textline_len),
        queuesize   (_queuesize) {
    }
  };

////////////////////////////////////////////////////////////////////////////////

  t_validity start(t_err);
  t_bool     is_running();
  t_void     update(const t_params&);
  t_void     fetch (t_params&);

////////////////////////////////////////////////////////////////////////////////

  tracer::t_tracer make_tracer(t_err, const t_tracer_name&,
                                      const t_tracer_params& =
                                        t_tracer_params());


  t_bool     update_tracer(t_err, const t_wildcard_name&, t_level);
  t_validity update_tracer(t_err, const t_tracer_name&,
                                  const t_tracer_params&);

  t_bool fetch_tracer (t_err, const t_tracer_name&, t_tracer_params&);
  t_bool fetch_tracer (t_err, const t_tracer_name&, t_tracer_info&,
                              t_bool clearstats = false);

  t_bool fetch_tracers(t_tracer_infos&, t_bool clearstats = false);

////////////////////////////////////////////////////////////////////////////////

  t_validity create_observer (t_err, const t_observer_name&,
                                     const t_observer_params& =
                                      t_observer_params());
  t_validity destroy_observer(t_err, const t_observer_name&);

  t_validity update_observer (t_err, const t_observer_name&,
                                     const t_observer_params&);

  t_bool fetch_observer(t_err, const t_observer_name&, t_observer_params&);
  t_bool fetch_observer(t_err, const t_observer_name&, t_observer_info&,
                               t_bool clearstats = false);
  t_bool fetch_observers(t_err, t_observer_infos&, t_bool clearstats = false);

////////////////////////////////////////////////////////////////////////////////

  t_bool   bind_tracers (t_err, const t_observer_name&, const t_wildcard_name&);
  t_bool unbind_tracers (t_err, const t_observer_name&, const t_wildcard_name&);
  t_bool is_tracer_bound(t_err, const t_observer_name&, const t_tracer_name&);

  t_validity fetch_bound_tracers(t_err, const t_observer_name&,
                                        t_tracer_names&);

////////////////////////////////////////////////////////////////////////////////

  t_bool destroy_tracer_(tracer::t_id&);

////////////////////////////////////////////////////////////////////////////////
}
}

#endif
