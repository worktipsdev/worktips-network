#if defined(ANDROID)
#include <util/android_logger.hpp>
#include <util/logger_internal.hpp>
#include <android/log.h>
namespace llarp
{
  void
  AndroidLogStream::PreLog(std::stringstream& ss, LogLevel lvl,
                           const char* fname, int lineno) const
  {
    switch(lvl)
    {
      case eLogNone:
        break;
      case eLogDebug:
        ss << "[DBG] ";
        break;
      case eLogInfo:
        ss << "[NFO] ";
        break;
      case eLogWarn:
        ss << "[WRN] ";
        break;
      case eLogError:
        ss << "[ERR] ";
        break;
    }

    ss << "(" << thread_id_string() << ") " << log_timestamp() << " " << fname
       << ":" << lineno << "\t";
  }

  void
  AndroidLogStream::PostLog(std::stingstream&) const
  {
  }

  void
  AndroidLogStream::Print(LogLevel lvl, const char* tag, const std::string& msg)
  {
    std::string str("lokinet|");
    str += tag;
    switch(lvl)
    {
      case eLogDebug:
        __android_log_write(ANDROID_LOG_DEBUG, str.c_str(), msg.c_str());
        return;
      case eLogInfo:
        __android_log_write(ANDROID_LOG_INFO, str.c_str(), msg.c_str());
        return;
      case eLogWarn:
        __android_log_write(ANDROID_LOG_WARN, str.c_str(), msg.c_str());
        return;
      case eLogError:
        __android_log_write(ANDROID_LOG_ERROR, str.c_str(), msg.c_str());
        return;
    }

  }  // namespace llarp
}  // namespace llarp
#endif
