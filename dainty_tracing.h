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
#include "dainty_tracing_err.h"
#include "dainty_tracing_tracer.h"

////////////////////////////////////////////////////////////////////////////////

namespace dainty
{
namespace tracing
{
  enum t_time_mode {
    NS,
    NS_DIFF,
    DATE
  };

  enum  t_time_mode_name_tag_ { };
  using t_time_mode_name = named::t_string<t_time_mode_name_tag_, 20>;

  t_time_mode_name to_name(t_time_mode);

////////////////////////////////////////////////////////////////////////////////

  enum t_mode {
    ALL = 0,
    OFF,
    CONFIG
  };

  enum  t_mode_name_tag_ { };
  using t_mode_name = named::t_string<t_mode_name_tag_, 20>;

  t_mode_name to_name(t_mode);

////////////////////////////////////////////////////////////////////////////////

  enum  t_wildcard_name_tag_ { };
  using t_wildcard_name = named::t_string<t_wildcard_name_tag_, 32>; //XXX

  using t_observer_infos = std::vector<observer::t_info>;
  using t_tracer_infos   = std::vector<tracer::t_info>;

////////////////////////////////////////////////////////////////////////////////

  class t_params {
  public:
    t_bool      to_terminal;
    t_bool      to_observers;
    t_bool      enable_ring;
    t_time_mode time_mode;
    t_mode      mode;
    t_n         queuesize;

    t_params() : to_terminal (true),
                 to_observers(true),
                 enable_ring (false),
                 time_mode   (DATE),
                 mode        (CONFIG),
                 queuesize_  (4000)
    { }

    t_params(t_bool      _to_terminal,
             t_bool      _to_observers,
             t_bool      _enable_ring,
             t_time_mode _time_mode,
             t_mode      _mode,
             t_n         _queuesize)
      : to_terminal (_to_terminal),
        to_observers(_to_observers),
        enable_ring (_enable_ring),
        time_mode   (_time_mode),
        mode        (_mode),
        queuesize   (_queuesize)
    { }
  };

////////////////////////////////////////////////////////////////////////////////

  // can start and stop
  t_bool is_running();
  t_void update(const t_params&);
  t_void fetch (t_params&);

////////////////////////////////////////////////////////////////////////////////

  tracer::t_tracer make_tracer(const tracer::t_name&,
                               const tracer::t_params& = tracer::t_params());
  tracer::t_tracer make_tracer(t_err, const tracer::t_name&,
                               const tracer::t_params& = tracer::t_params());

  t_bool destroy_tracer(tracer::t_tracer&&);

  t_bool update_tracer(const wildcardname_t&, t_level);
  t_bool update_tracer(const tracer::t_name&, const tracer::t_params&);

  t_bool fetch_tracer (const tracer::t_name&, tracer::t_params&);
  t_bool fetch_tracer (const tracer::t_name&, tracer::t_info&,
                       t_bool clearstats = false);

  t_bool fetch_tracers(t_tracer_infos&, t_bool clearstats = false);

////////////////////////////////////////////////////////////////////////////////

  observer::t_id create_observer(const observer::t_name&,
                                 const observer::t_params& =
                                   observer::t_params());
  t_bool destroy_observer(const observer::t_id&);


  t_bool update_observer(const observer::t_name&, const observer::t_params&);

  t_bool fetch_observer (const observer::t_name&, observer::t_params&);
  t_bool fetch_observer (const observer::t_name&, observer::t_info&,
                         t_bool clearstats = false);
  t_bool fetch_observers(t_observer_infos&,
                         t_bool clearstats = false);

////////////////////////////////////////////////////////////////////////////////

   // bind and unbind tracers to/from observers

////////////////////////////////////////////////////////////////////////////////
}
}

#endif
