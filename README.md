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

Real batch file  build
g++ -std=c++17 -O2 -Wall -Wextra src/run_BatchGen.cpp src/BatchGen.cpp src/Graph.cpp -o build/run_BatchGen.exe
./build/run_BatchGen.exe

Builds Driver
g++ -std=c++17 -O2 -Wall -Wextra src/Driver.cpp src/DDFLPD_WL.cpp src/DDFLPD.cpp src/Dijkstra.cpp src/Validator.cpp src/BatchGen.cpp src/Graph.cpp src/MinHeap.cpp -o build/Driver.exe -lpsapi

Builds the cuts in the dataset
./build/run_BatchGen.exe

To see the results from the batches in a file in results 
Get-FileHash data\batches\NW_*.txt -Algorithm SHA256 | Format-Table Hash, Path -AutoSize | Out-File Results\batch_manifest.txt -Width 300

testing for dev

$h = git rev-parse --short HEAD
$h

Before every driver run commit so the tree is clean then save the hash
git status
$h = git rev-parse --short HEAD

Pilot runs
1. Pipeline check:
./build/Driver.exe 10000 pilot $h 2> Results/log_NW_10000_pilot.txt
./build/Driver.exe 30000 final $h 2> Results/log_NW_30000_final.txt
./build/Driver.exe 90000 final $h 2> Results/log_NW_90000_final.txt
./build/Driver.exe 270000 final $h 2> Results/log_NW_270000_final.txt
./build/Driver.exe 810000 final $h 2> Results/log_NW_810000_final.txt

This is for Pilot runs with count 
g++ -std=c++17 -O2 -Wall -Wextra src/run_Replay.cpp src/Replay.cpp src/DDFLPD_WL.cpp src/DDFLPD.cpp src/Dijkstra.cpp src/Validator.cpp src/BatchGen.cpp src/Graph.cpp src/MinHeap.cpp -o build/run_Replay.exe
./build/run_Replay.exe 10000 pilot $h 2> Results/log_replay_NW_10000_pilot.txt
Import-Csv Results\wtotal_NW_10000_pilot.csv | Group-Object outcome | Select-Object Name, Count
./build/run_Replay.exe 810000 pilot $h 2> Results/log_replay_NW_810000_pilot.txt
Import-Csv Results\wtotal_NW_810000_pilot.csv | Group-Object outcome | Select-Object Name, Count