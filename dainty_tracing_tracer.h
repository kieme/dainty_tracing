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

#define TRACE_NARG(...)  TRACE_NARG_(__VA_ARGS__, TRACE_POS_)
#define TRACE_NARG_(...) TRACE_ARG_N(__VA_ARGS__)
#define TRACE_ARG_N( \
          a1, a2, a3, a4, a5, a6, a7, a8, a9,a10, \
         a11,a12,a13,a14,a15,a16,a17,a18,a19,a20, \
         a21,a22,a23,a24,a25,a26,a27,a28,a29,a30, \
         a31,a32,a33,a34,a35,a36,a37,a38,a39,a40, \
         a41,a42,a43,a44,a45,a46,a47,a48,a49,a50, \
         a51,a52,a53,a54,a55,a56,a57,a58,a59,a60, \
         a61,a62,a63,POS,...) POS
#define TRACE_POS_               \
         N,N,N,N,             \
         N,N,N,N,N,N,N,N,N,N, \
         N,N,N,N,N,N,N,N,N,N, \
         N,N,N,N,N,N,N,N,N,N, \
         N,N,N,N,N,N,N,N,N,N, \
         N,N,N,N,N,N,N,N,N,N, \
         N,N,N,N,N,N,N,N,1

#define TRACE_WRAP_1(a)  a
#define TRACE_WRAP_N     dainty::tracing::tracer::textstring_t
#define TRACE_CNT_(...)  TRACE_NARG(__VA_ARGS__)
#define TRACE_EXPAND_(x) x
#define TRACE_ADD_(a, b) a ## b
#define TRACE_PRE_(a, b) TRACE_ADD_(a,b)
#define TRACE_PACK_(...) \
  TRACE_PRE_(TRACE_WRAP_, TRACE_EXPAND_(TRACE_CNT_(__VA_ARGS__)))

#define DAINTY_TR_0(SOURCE, LEVEL, PARAMS)                               \
  do {                                                                   \
    if (dainty::tracing::tracer::ref(SOURCE).match(LEVEL))               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        post((LEVEL), TRACE_PACK_ PARAMS PARAMS);                        \
  } while (false)

#define DAINTY_TR_ALWAYS_0(SOURCE, LEVEL, PARAMS)                        \
  do {                                                                   \
    if (dainty::tracing::tracer::ref(SOURCE).match(LEVEL))               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        guaranteed_post((LEVEL), TRACE_PACK_ PARAMS PARAMS);             \
  } while (false)

#define DAINTY_TR_IF_0(COND, SOURCE, LEVEL, PARAMS)                      \
  do {                                                                   \
    if ((COND) && dainty::tracing::tracer::ref(SOURCE).match(LEVEL))     \
      dainty::tracing::tracer::ref(SOURCE).                              \
        post((LEVEL), TRACE_PACK_ PARAMS PARAMS);                        \
  } while (false)

#define DAINTY_TR_ALWAYS_IF_0(COND, SOURCE, LEVEL, PARAMS)               \
  do {                                                                   \
    if ((COND) && dainty::tracing::tracer::ref(SOURCE).match(LEVEL))     \
      dainty::tracing::tracer::ref(SOURCE).                              \
        guaranteed_post((LEVEL), TRACE_PACK_ PARAMS PARAMS);             \
  } while (false)

#define DAINTY_TR_1(SOURCE, LEVEL, ...)                                  \
  do {                                                                   \
    if (dainty::tracing::tracer::ref(SOURCE).match(LEVEL)) {             \
      dainty::tracing::tracer::textstring_t text;                        \
      text += __VA_ARGS__;                                               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        post((LEVEL), text);                                             \
    }                                                                    \
  } while (false)

#define DAINTY_TR_ALWAYS_1(SOURCE, LEVEL, ...)                           \
  do {                                                                   \
    if (dainty::tracing::tracer::ref(SOURCE).match(LEVEL)) {             \
      dainty::tracing::tracer::textstring_t text;                        \
      text += __VA_ARGS__;                                               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        guaranteed_post((LEVEL), text);                                  \
    }                                                                    \
  } while (false)

#define DAINTY_TR_IF_1(COND, SOURCE, LEVEL, ...)                         \
  do {                                                                   \
    if ((COND) && dainty::tracing::tracer::ref(SOURCE).match(LEVEL)) {   \
      dainty::tracing::tracer::textstring_t text;                        \
      text += __VA_ARGS__;                                               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        post((LEVEL), text);                                             \
    }                                                                    \
  } while (false)

#define DAINTY_TR_ALWAYS_IF_1(COND, SOURCE, LEVEL, ...)                  \
  do {                                                                   \
    if ((COND) && dainty::tracing::tracer::ref(SOURCE).match(LEVEL)) {   \
      dainty::tracing::tracer::textstring_t text;                        \
      text += __VA_ARGS__;                                               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        guaranteed_post((LEVEL), text);                                  \
    }                                                                    \
  } while (false)

#define DAINTY_TR(SOURCE, LEVEL, COMP, TYPE, CODE, ...)                  \
  do {                                                                   \
    if (dainty::tracing::tracer::ref(SOURCE).match(LEVEL)) {             \
      dainty::tracing::tracer::textstring_t text;                        \
      text += __VA_ARGS__;                                               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        post((LEVEL), (COMP), (TYPE), (CODE), text);                     \
    }                                                                    \
  } while (false)

#define DAINTY_TR_ALWAYS(SOURCE, LEVEL, COMP, TYPE, CODE, ...)           \
  do {                                                                   \
    if (dainty::tracing::tracer::ref(SOURCE).match(LEVEL)) {             \
      dainty::tracing::tracer::textstring_t text;                        \
      text += __VA_ARGS__;                                               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        guaranteed_post((LEVEL), (COMP), (TYPE), (CODE), text);          \
    }                                                                    \
  } while (false)

#define DAINTY_TR_IF(COND, SOURCE, LEVEL, COMP, TYPE, CODE, ...)         \
  do {                                                                   \
    if ((COND) && dainty::tracing::tracer::ref(SOURCE).match(LEVEL)) {   \
      dainty::tracing::tracer::textstring_t text;                        \
      text += __VA_ARGS__;                                               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        post((LEVEL), (COMP), (TYPE), (CODE), text);                     \
    }                                                                    \
  } while (false)

#define DAINTY_TR_ALWAYS_IF(COND, SOURCE, LEVEL, COMP, TYPE, CODE, ...)  \
  do {                                                                   \
    if ((COND) && dainty::tracing::tracer::ref(SOURCE).match(LEVEL)) {   \
      dainty::tracing::tracer::textstring_t text;                        \
      text += __VA_ARGS__;                                               \
      dainty::tracing::tracer::ref(SOURCE).                              \
        guaranteed_post((LEVEL), (COMP), (TYPE), (CODE), text);          \
    }                                                                    \
  } while (false)

///////////////////////////////////////////////////////////////////////////////

#include "dainty_tracing_observer.h"

namespace dainty
{
namespace tracing
{
namespace tracer
{
  using named::p_cstr;

  int_t shared_printf (p_cstr fm, ...) __attribute__((format(printf, 1, 2)));
  int_t shared_vprintf(p_cstr fmt, va_list vars);

///////////////////////////////////////////////////////////////////////////////

  enum t_option {
    sourceoption_datetimestamp,     // date/time or nanosec
    sourceoption_autobindobservers, // bind all observers
  };

  library::misc::nflags_offset_t t_optiono_offset(t_option);
  t_option t_optiono_enum(library::misc::nflags_offset_t);

  typedef library::misc::nflags_t<t_option, 8,
                                  t_optiono_offset,
                                  t_optiono_enum> sourceoptions_t;

  inline sourceoptions_t operator+(t_option o1, t_option o2) {
    sourceoptions_t options(o1);
    options += o2;
    return options;
  }

///////////////////////////////////////////////////////////////////////////////

  sourceoptions_t default_options();
  level_t         default_sourcelevel();
  credit_t        default_sourcecredit();

///////////////////////////////////////////////////////////////////////////////

  struct t_params {
    t_params(level_t         level   = default_sourcelevel(),
                   sourceoptions_t options = default_options(),
                   credit_t        credit  = default_sourcecredit())
      : level_  (level),
        options_(options),
        credit_ (credit)
    { }

    level_t         level_;
    sourceoptions_t options_;
    credit_t        credit_;
  };

///////////////////////////////////////////////////////////////////////////////

  struct t_id {
    t_id()                           : offset_(-1),     seq_(0)   { }
    t_id(offset_t offset, seq_t seq) : offset_(offset), seq_(seq) { }

    operator bool_t() const                 { return offset_ != -1; }

    t_id release() {
      t_id tmp(*this);
      offset_ = -1;
      return tmp;
    }

    offset_t offset_;
    seq_t    seq_;
  };

  bool_t operator==(const t_id&, const t_id&);
  bool_t operator!=(const t_id&, const t_id&);

///////////////////////////////////////////////////////////////////////////////

  // does not require to be here
  struct t_info_ {
    t_id           id_;
    sourcename_t   name_;
    t_params       params_;
    usestats_t     usestats_;
  };

///////////////////////////////////////////////////////////////////////////////

  class t_point {
  public:
    void_t  post(level_t, const char_t*) const;
    void_t  post(level_t, const textstring_t&) const;
    void_t  post(level_t, const compstring_t&, const typestring_t&,
                          const codestring_t&, const textstring_t&) const;

    void_t  guaranteed_post(level_t, const char_t*) const;
    void_t  guaranteed_post(level_t, const textstring_t&) const;
    void_t  guaranteed_post(level_t, const compstring_t&,
                                     const typestring_t&,
                                     const codestring_t&,
                                     const textstring_t&) const;

    bool_t       set_name(const sourcename_t&);
    sourcename_t get_name() const;

    level_t get_level() const;
    void_t  set_level(level_t);
    bool_t  match    (level_t) const;

    bool_t          add_options(sourceoptions_t);
    void_t          del_options(sourceoptions_t);
    sourceoptions_t get_options() const;

    void_t   set_credit(credit_t);
    credit_t get_credit() const;

    bool_t bind(const observerid_t&);
    bool_t bind(const observername_t&);

    void_t unbind();
    bool_t unbind(const observerid_t&);
    bool_t unbind(const observername_t&);

    operator bool_t() const                                { return id_; }

  private:
    t_point(const t_id& id) : id_(id)                                { }
    inline t_point release()          { return t_point(id_.release()); }

    friend class t_tracer;
    friend class access_t;

    t_id id_;
  };

////////////////////////////////////////////////////////////////////////////////

  class t_tracer {
  public:
    t_tracer() : point_(t_id())                             { }
    t_tracer(t_tracer&&);
    t_tracer& operator=(t_tracer&&);
    ~t_tracer();

    t_tracer(const t_tracer&)            = delete;
    t_tracer& operator=(const t_tracer&) = delete;

    inline operator t_validity() const        { return point_.id_; }

    inline       t_point* operator->()        { return &point_; }
    inline const t_point* operator->() const  { return &point_; }

    inline       t_point& operator* ()        { return  point_; }
    inline const t_point& operator* () const  { return  point_; }

    inline       t_point*  ptr ()             { return &point_; }
    inline const t_point*  ptr () const       { return &point_; }
    inline const t_point* cptr () const       { return &point_; }

    inline       t_point&  ref ()             { return point_; }
    inline const t_point&  ref () const       { return point_; }
    inline const t_point& cref () const       { return point_; }

    inline t_void release   ()                { return *this;   }

    inline t_point make_point(t_user);

  private:
    friend class access_t;
    inline t_tracer(const t_id& id) : point_(id)            { }

    t_point point_;
  };

////////////////////////////////////////////////////////////////////////////////

  inline       t_point& ref(      t_point* point)  { return *point;}
  inline const t_point& ref(const t_point* point)  { return *point;}

  inline       t_point& ref(      t_point& point)  { return point; }
  inline const t_point& ref(const t_point& point)  { return point; }

  inline       t_point& ref(      t_tracer& point) { return *point;}
  inline const t_point& ref(const t_tracer& point) { return *point;}

////////////////////////////////////////////////////////////////////////////////
}
}
}

#endif
