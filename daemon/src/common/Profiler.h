#ifndef COMMON_PROFILER_H_
#define COMMON_PROFILER_H_

namespace Profiler{

        enum Type{ START, END, FRAME };

        class Point{

        public:
            Type type;
            const char* label;
            std::chrono::microseconds::rep time;


            Point(Type,const char *);
        };

        void Update();
        void Stop();

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
