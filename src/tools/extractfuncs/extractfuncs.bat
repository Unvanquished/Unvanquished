cd ..\game
..\extractfuncs\extractfuncs *.c -d MISSIONPACK -d GAMEDLL
cd ..\botai
..\extractfuncs\extractfuncs *.c -o ai_funcs.h ai_func_decs.h -d MISSIONPACK -d GAMEDLL
cd ..