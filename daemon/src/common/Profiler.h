#ifndef COMMON_PROFILER_H_
#define COMMON_PROFILER_H_

namespace Profiler{

        enum Type{ START, END, FRAME };

        class Point{

        public:
            Type type;
            std::string label;
            std::chrono::microseconds::rep time;


            Point(Type,const char *);
        };

        void Update();
        void Stop();

        class Profile
        {
        public:
            const char * label;
            Profile(const char *);

            ~Profile();
        };

}

#define PROFILE() Profiler::Profile Profile_Function(__PRETTY_FUNCTION__);

#endif //COMMON_PROFILER_H_
