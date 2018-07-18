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

#ifndef _DAINTY_TRACING_OBSERVER_H_
#define _DAINTY_TRACING_OBSERVER_H_

#include "dainty_container_freelist.h"

namespace dainty
{
namespace tracing
{
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

  enum t_levelname_tag_ { };
  using t_levelname = named::t_string<t_levelname_tag_, 10>;

  t_levelname to_name(t_level);

////////////////////////////////////////////////////////////////////////////////

  enum compstring_tag_t { };
  typedef nstring_t<8,  compstring_tag_t> compstring_t;

  enum typestring_tag_t { };
  typedef nstring_t<3,  typestring_tag_t> typestring_t;

  enum codestring_tag_t { };
  typedef nstring_t<3,  codestring_tag_t> codestring_t;

  enum timestampstring_tag_t { };
  typedef nstring_t<24, timestampstring_tag_t> timestampstring_t;

  enum sourcename_tag_t   { };
  typedef nstring_t<64, sourcename_tag_t> sourcename_t;

  enum wildcardname_tag_t { };
  typedef nstring_t<16, wildcardname_tag_t> wildcardname_t;

  enum textstring_tag_t { };
  typedef nstring_t<TR_MSG_LINE_MAX        -
                    timestampstring_t::MAX -
                    t_levelname::MAX     -
                    sourcename_t::MAX      - 10, /* spaces, etc */
                    textstring_tag_t> textstring_t;

////////////////////////////////////////////////////////////////////////////////

  class t_stats {
  public:
    using t_n = named::t_n;

    t_stats()                                         { reset(); }

    t_n get      (t_level level) const { return   used_[level]; }
    t_n increment(t_level level)       { return ++used_[level]; }

    t_n reset() {
      t_n_ sum = 0;
      for (t_n_ i = 0; i < sizeof(used_)/sizeof(t_n_); ++i) {
        sum += used_[i];
        used_[i] = 0;
      }
      return t_n{sum};
    }

    t_n total() const {
      t_n sum = 0;
      for (t_n_ i = 0; i < sizeof(used_)/sizeof(t_n); ++i)
        sum += used_[i];
      return sum;
    }

  private:
    using t_n_ = named::t_n_;
    t_n_ used_[DEBUG + 1];
  };

////////////////////////////////////////////////////////////////////////////////

namespace observer
{
  enum t_name_tag_ { };
  using t_name = named::t_string<t_name_tag_, 32>;

  class t_id {
    using t_id_ = container::freelist::t_id;
  public:
    t_id() : id_t, seq_(0)                            { }
    operator t_bool () const              { return id_; }

  private:
    using t_seq = named::t_uint32;
    t_id(t_id_ id, t_seq seq) : id_(id), seq_(seq) { }
    friend class access_t;

    t_id_ id_;
  };

  t_bool operator< (const t_id&, const t_id&);
  t_bool operator==(const t_id&, const t_id&);
  t_bool operator!=(const t_id&, const t_id&);

///////////////////////////////////////////////////////////////////////////////

  t_level default_level();

///////////////////////////////////////////////////////////////////////////////

  class params_t {
  public:
    params_t(t_level _level = default_level()) : level(_level) {
    }

    t_level level;
  };

///////////////////////////////////////////////////////////////////////////////

  class t_info {
  public:
    t_info() { }
    t_info(const t_name& _name, const t_params& _params)
      : name(_name), params(_params) {
    }

    t_id     id;
    t_name   name;
    t_params params;
    t_stats  stats;
  };

///////////////////////////////////////////////////////////////////////////////
}
}
}

#endif
