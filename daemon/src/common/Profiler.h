#ifndef COMMON_PROFILER_H_
#define COMMON_PROFILER_H_
#include <chrono>
#include <thread>

namespace Profiler{

        enum Type{ START, END, FRAME };

        class Point{

        public:
            Type type;
            const char* label;
            std::chrono::microseconds::rep time;
            std::thread::id tid;


            Point(Type,const char *);

            bool operator< (const Point& p2){
                return time < p2.time;
            }

        };

        void Frame();
        void Stop();
        void Start();
        void Sync();

        class ProfilerGuard
        {
        public:
            const char * label;
            ProfilerGuard(const char *);

            ~ProfilerGuard();
        };

}

#define PROFILE() Profiler::ProfilerGuard ___Profile_Function(__PRETTY_FUNCTION__);

#endif //COMMON_PROFILER_H_
