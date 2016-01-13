 
#include "Profiler.h"
#include "LogSystem.h"

namespace Profiler{


        std::vector <Profiler::Point> times;
        std::string label;

        std::chrono::high_resolution_clock::time_point start;


        long long time_elapsed(std::chrono::high_resolution_clock::time_point since){
            auto tmp=std::chrono::high_resolution_clock::now() - since;

            return std::chrono::duration_cast<std::chrono::microseconds>(tmp).count();
        }

        void update(){

            times.push_back(Point(FRAME));

            start=std::chrono::high_resolution_clock::now();
        }


        void print_raw(){
            for (auto& i : times) {
                std::string type;

                switch(i.type){
                case START:
                    type="start";
                    break;
                case END:
                    type="end";
                    break;
                case FRAME:
                    type="frame";
                    break;
                }

                //std::cout << type << ", "<< i.label << ", " << i.time <<"\n";
            }


            times.clear();

        }

        void show(){

            unsigned padding=0;
            std::map <unsigned, long long> levels;

            for (auto& i : times) {

                if(i.type==FRAME){
                    Log::Debug("");
                    continue;
                }




                if(i.type==START){
                    padding++;
                    levels[padding]=i.time;
                }else if (i.type==END){
                    std::string padstr;
                    for(char j=0;j<padding;j++)
                        padstr+= "    ";

                    Log::Debug("%s %s %d",padstr, i.label,i.time - levels[padding] );
                   padstr="";

                    padding--;
                }

            }

            times.clear();

        }



            Profile::Profile(std::string label_){
                label=label_;
                times.push_back(Point(START,label,time_elapsed(start)));
            }

            Profile::~Profile(){

                times.push_back(Point(END,label,time_elapsed(start)));
            }


}
