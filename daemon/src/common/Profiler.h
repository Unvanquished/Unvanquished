#ifndef COMMON_PROFILER_H_
#define COMMON_PROFILER_H_

namespace Profiler{

        enum Type{ START, END, FRAME };

        class Point{

        public:
            std::string label;
            std::chrono::microseconds::rep time;
            Type type;

            Point(Type,std::string);
        };

        void Update();
        void Stop();

        class Profile
        {
        public:
            std::string label;
            Profile(std::string);

            ~Profile();
        };

}

#define PROFILE() Profiler::Profile Profile_Function(__PRETTY_FUNCTION__);

#endif //COMMON_PROFILER_H_
