#ifndef WOOPS_UTIL_LOGGING_H_
#define WOOPS_UTIL_LOGGING_H_

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <ostream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>


#define LOG_INFO LogMessage("INFO", __FILE__, __LINE__)
#define LOG_WARNING LogMessage("WARNING", __FILE__, __LINE__)
#define LOG_ERROR LogMessage("ERROR", __FILE__, __LINE__)
#define LOG_FATAL LogMessageFatal(__FILE__, __LINE__)

#define LOG(severity) LOG_ ## severity.stream()

class LogMessage {
public:
    LogMessage(const char* type, std::string file, int line)
        : flushed_(false) {
            auto now = std::chrono::system_clock::now();
            auto tt_now = std::chrono::system_clock::to_time_t(now);
            auto tm_now = std::localtime(&tt_now);

            auto slash = file.rfind('/');
            if (slash != std::string::npos && 
                slash + 1 <= file.size()) {
                file = file.substr(slash+1);
            }
            stream() << type << " " << std::put_time(tm_now, "%Y-%m-%d %H:%M:%S") << " " << file << ":" << line << ": ";
        }
    void Flush() {
        stream() << "\n";
        std::string s = str_.str();
        size_t n = s.size();
        if (fwrite(s.data(), 1, n, stderr) < n) {}  // shut up gcc
        flushed_ = true;
    }
    ~LogMessage() {
        if (!flushed_) {
            Flush();
        }
    }
    std::ostream& stream() { return str_; }

private:
    bool flushed_;
    std::ostringstream str_;

    LogMessage(const LogMessage&) = delete;
    LogMessage& operator=(const LogMessage&) = delete;
};

class LogMessageFatal : public LogMessage {
public:
    LogMessageFatal(const char* file, int line)
        : LogMessage("FATAL", file, line) {}
    ~LogMessageFatal() {
        Flush();
        abort();
    }
private:
    LogMessageFatal(const LogMessageFatal&) = delete;
    LogMessageFatal& operator=(const LogMessageFatal&) = delete;
};



#endif /* end of include guard: WOOPS_UTIL_LOGGING_H_ */
