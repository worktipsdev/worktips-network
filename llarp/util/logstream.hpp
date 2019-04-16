#ifndef LLARP_UTIL_LOG_STREAM_HPP
#define LLARP_UTIL_LOG_STREAM_HPP
#include <memory>
#include <string>
#include <util/loglevel.hpp>
#include <sstream>
#include <util/time.hpp>

namespace llarp
{
  /// logger stream interface
  struct ILogStream
  {
    virtual ~ILogStream(){};

    virtual void
    PreLog(std::stringstream& out, LogLevel lvl, const char* fname,
           int lineno) const = 0;
    virtual void
    Print(LogLevel lvl, const char* filename, const std::string& msg) = 0;

    virtual void
    PostLog(std::stringstream& out) const = 0;

    /// called every end of event loop tick
    virtual void
    Tick(llarp_time_t now) = 0;
  };

  using ILogStream_ptr = std::unique_ptr< ILogStream >;

}  // namespace llarp
#endif
