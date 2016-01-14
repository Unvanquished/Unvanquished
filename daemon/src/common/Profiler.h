#ifndef COMMON_PROFILER_H_
#define COMMON_PROFILER_H_


namespace Profiler{

        enum Type{ START, END, FRAME };

        class Point{

        public:
            std::string label;
            std::chrono::microseconds::rep time;
            char type;

            Point(char type,std::string label="", std::chrono::microseconds::rep time=0): label(label), time(time), type(type){   }
        };





        std::chrono::microseconds::rep TimeElapsed(std::chrono::high_resolution_clock::time_point since);

        void Update();


        void PrintRaw();

        void Show();



        class Profile{
            std::string label;
        public:
            Profile(std::string label_);

            ~Profile();
        };

}

#define PROFILE() Profiler::Profile Profile_Function(__PRETTY_FUNCTION__);

#endif //COMMON_PROFILER_H_
