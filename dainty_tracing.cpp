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

#include "dainty_tracing.h"

namespace dainty
{
namespace tracing
{
namespace {
  // all my worker code
}

///////////////////////////////////////////////////////////////////////////////
namespace tracer
{
  t_levelname to_name(t_level) {
    return t_levelname();
  }

///////////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////////

  t_bool t_point::post(t_level, t_textline&&) const {
    return true;
  }

  t_bool t_point::post(t_err, t_level, t_textline&&) const {
    return true;
  }

  t_validity t_point::waitable_post(t_level, t_textline&&) const {
    return INVALID;
  }

  t_validity t_point::waitable_post(t_err, t_level, t_textline&&) const {
    return INVALID;
  }

  t_name t_point::get_name() const {
    return t_name();
  }

  t_level t_point::get_level() const {
    return NONE;
  }

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

  t_validity start(t_err) {
    return VALID;
  }

  t_bool is_running() {
    return false;
  }

  t_void update(const t_params&) {
  }

  t_void fetch(t_params&) {
  }

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////////////////////

  t_bool destroy_tracer_(tracer::t_id) {
    return false;
  }

////////////////////////////////////////////////////////////////////////////////
}
}
