#ifndef COMMON_PROFILER_H_
#define COMMON_PROFILER_H_


namespace Profiler{

        enum Type{ START, END, FRAME };

        class Point{

        public:
            std::string label;
            long long time;
            char type;

            Point(char type,std::string label="", long long time=0): label(label), time(time), type(type){   }
        };





        long long time_elapsed(std::chrono::high_resolution_clock::time_point since);

        void update();


        void print_raw();

        void show();



        class Profile{
            std::string label;
        public:
            Profile(std::string label_);

            ~Profile();
        };

}

#endif //COMMON_PROFILER_H_
