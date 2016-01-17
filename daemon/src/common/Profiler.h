#ifndef COMMON_PROFILER_H_
#define COMMON_PROFILER_H_

namespace Profiler{

        enum Type{ START, END, FRAME };

        class Point{

        public:
            std::string label;
            std::chrono::microseconds::rep time;
            Type type;

            Point(Type type,std::string label="", std::chrono::microseconds::rep time=0): label(label), time(time), type(type){   }
        };

        std::chrono::microseconds::rep TimeElapsed(std::chrono::high_resolution_clock::time_point since);

        void Update();

        class Profile
        {
        std::string label;

        public:
            Profile(std::string);

            ~Profile();
        };

}

#define PROFILE() Profiler::Profile Profile_Function(__PRETTY_FUNCTION__);

#endif //COMMON_PROFILER_H_
