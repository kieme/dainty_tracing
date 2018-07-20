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

#ifndef _DAINTY_TRACING_TRACER_H_
#define _DAINTY_TRACING_TRACER_H_

#include <utility>
#include "dainty_container_freelist.h"
#include "dainty_named_string.h"
#include "dainty_tracing_err.h"

#define DAINTY_TR_(TR, LEVEL, TEXT)                                         \
  dainty::tracing::tracer::ref(TR).post((LEVEL), std::move(TEXT));

#define DAINTY_TR_EMERG(TR, TEXT)                                           \
  DAINTY_TR_(TR, dainty::tracing::tracer::EMERG, TEXT)
#define DAINTY_TR_ALERT(TR, TEXT)                                           \
  DAINTY_TR_(TR, dainty::tracing::tracer::ALERT, TEXT)
#define DAINTY_TR_CRITICAL(TR, TEXT)                                        \
  DAINTY_TR_(TR, dainty::tracing::tracer::CRITICAL, TEXT)
#define DAINTY_TR_ERROR(TR, TEXT)                                           \
  DAINTY_TR_(TR, dainty::tracing::tracer::ERROR, TEXT)
#define DAINTY_TR_WARNING(TR, TEXT)                                         \
  DAINTY_TR_(TR, dainty::tracing::tracer::WARNING, TEXT)
#define DAINTY_TR_NOTICE(TR, TEXT)                                          \
  DAINTY_TR_(TR, dainty::tracing::tracer::NOTICE, TEXT)
#define DAINTY_TR_INFO(TR, TEXT)                                            \
  DAINTY_TR_(TR, dainty::tracing::tracer::INFO, TEXT)
#define DAINTY_TR_DEBUG(TR, TEXT)                                           \
  DAINTY_TR_(TR, dainty::tracing::tracer::DEBUG, TEXT)

///////////////////////////////////////////////////////////////////////////////

namespace dainty
{
namespace tracing
{
namespace tracer
{
  using named::string::t_string;
  using named::p_cstr_;
  using named::t_void;
  using named::t_bool;
  using named::t_validity;
  using named::VALID;
  using named::INVALID;
  using named::string::FMT;

///////////////////////////////////////////////////////////////////////////////

  using t_credit = named::t_uint32;

  enum  t_name_tag_ {};
  using t_name = t_string<t_name_tag_, 32>;

  enum  t_textline_tag_ {};
  using t_textline = t_string<t_textline_tag_>;

///////////////////////////////////////////////////////////////////////////////

  enum t_level {
    NONE     = -1,
    EMERG    = 0,
    ALERT    = 1,
    CRITICAL = 2,
    ERROR    = 3,
    WARNING  = 4,
    NOTICE   = 5,
    INFO     = 6,
    DEBUG    = 7
  };

  enum  t_levelname_tag_ { };
  using t_levelname = t_string<t_levelname_tag_, 10>;

  t_levelname to_name(t_level);

///////////////////////////////////////////////////////////////////////////////

  t_level  default_level ();
  t_credit default_credit();

  class t_params {
  public:
    t_level  level;
    t_bool   bind_to_all;
    t_credit credit;

    t_params(t_level  _level       = default_level(),
             t_bool   _bind_to_all = false,
             t_credit _credit      = default_credit())
      : level      (_level),
        bind_to_all(_bind_to_all),
        credit     (_credit) {
    }
  };

///////////////////////////////////////////////////////////////////////////////

  class t_id {
  public:
    using t_seq      = named::t_int32;
    using t_impl_id_ = container::freelist::t_id;

    t_id() : seq_(-1), id_(0) {
    }
    t_id(const t_id&) = default;

    operator t_validity() const  { return seq_ != -1 ? VALID : INVALID; }

    t_id release() {
      t_id tmp(*this);
      seq_ = -1;
      return tmp;
    }

  private:
    friend class t_point;
    t_id(t_seq seq, t_impl_id_ id) : seq_(seq), id_(id) {
    }

    t_seq      seq_;
    t_impl_id_ id_;
  };

///////////////////////////////////////////////////////////////////////////////

  class t_point {
  public:
    t_point& operator=(const t_point&) = delete;

    t_bool post(       t_level, t_textline&&) const;
    t_bool post(t_err, t_level, t_textline&&) const;

    t_validity waitable_post(       t_level, t_textline&&) const;
    t_validity waitable_post(t_err, t_level, t_textline&&) const;

    t_name  get_name () const;
    t_level get_level() const;

    operator t_validity() const { return id_; }

  private:
    friend class t_tracer;
    t_point() = default;
    t_point(const t_point&) = default;
    t_point(const t_id& id, const t_name& name)
      : id_(id), name_{name} {
    }

    inline t_point release() { return t_point(id_.release(), name_); }

    t_id   id_;
    t_name name_;
  };

////////////////////////////////////////////////////////////////////////////////

  class t_tracer {
  public:
    t_tracer() = default;
    t_tracer(t_tracer&&);
    t_tracer& operator=(t_tracer&&);
    ~t_tracer();

    t_tracer(const t_tracer&)            = delete;
    t_tracer& operator=(const t_tracer&) = delete;

    inline operator t_validity() const        { return point_;  }

    inline       t_point* operator->()        { return &point_; }
    inline const t_point* operator->() const  { return &point_; }

    inline       t_point& operator*()         { return point_; }
    inline const t_point& operator*() const   { return point_; }

    t_point make_point(const t_name&);

  private:
    friend t_tracer mk_(const t_id&, const t_name&);
    inline
    t_tracer(const t_id& id, const t_name& name) : point_(id, name) {
    }

    t_point point_;
  };

///////////////////////////////////////////////////////////////////////////////

  t_validity shared_trace(       t_textline&&);
  t_validity shared_trace(t_err, t_textline&&);

////////////////////////////////////////////////////////////////////////////////

  inline       t_point& ref(      t_point& point)  { return point; }
  inline const t_point& ref(const t_point& point)  { return point; }

  inline       t_point& ref(      t_tracer& point) { return *point;}
  inline const t_point& ref(const t_tracer& point) { return *point;}

////////////////////////////////////////////////////////////////////////////////

}
}
}

#endif
