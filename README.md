# ResearchRecomputation
This is the foundation of our research over maintaining and swapping with an alpha swap versus recomutation. 

commands used in the process of creating 

mkdir

for the test of a mini graph: fig1 .gr/.txt  graph.h /,pp test_graph.cpp 
g++ -std=c++17 -O2 -Wall -Wextra src/tst_Graph.cpp src/Graph.cpp -o build/tst_graph.exe  
./build/tst_graph.exe 

for the test of mini heap befor dijkjstra quality check 
g++ -std=c++17 -O2 -Wall -Wextra src/tst_MinHeap.cpp src/MinHeap.cpp -o build/tst_MinHeap.exe
./build/tst_MinHeap.exe	 

For the test on Dijkstra 
g++ -std=c++17 -O2 -Wall -Wextra src/tst_Dijkstra.cpp src/Dijkstra.cpp src/Graph.cpp src/MinHeap.cpp -o build/tst_Dijkstra.exe
./build/tst_Dijkstra.exe

For the validator
g++ -std=c++17 -O2 -Wall -Wextra src/tst_Validator.cpp src/Validator.cpp src/Dijkstra.cpp src/Graph.cpp src/MinHeap.cpp -o build/tst_validator.exe
./build/tst_validator.exe

For BtachGen
g++ -std=c++17 -O2 -Wall -Wextra src/tst_BatchGen.cpp src/BatchGen.cpp src/Graph.cpp -o build/tst_BatchGen.exe
./build/tst_BatchGen.exe

For Graph prep 
g++ -std=c++17 -O2 -Wall -Wextra src/tst_GraphPrep.cpp src/GraphPrep.cpp src/Graph.cpp -o build/tst_GraphPrep.exe
./build/tst_GraphPrep.exe

For DD
g++ -std=c++17 -O2 -Wall -Wextra src/run_GraphPrep.cpp src/GraphPrep.cpp src/Graph.cpp src/Dijkstra.cpp src/MinHeap.cpp -o build/run_GraphPrep.exe
./build/run_GraphPrep.exe