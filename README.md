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



2. Run to move each dtaset into ther own folder bc did frst on acc
New-Item -ItemType Directory Results\early_bb9940a
Get-ChildItem Results -Filter "*NW_*_final*"
Get-ChildItem Results -Filter "*NW_30000_final*" | ForEach-Object { git mv $_.FullName Results\early_bb9940a\ }
Get-ChildItem Results -Filter "*NW_90000_final*" | ForEach-Object { git mv $_.FullName Results\early_bb9940a\ }
git push
3.  Run the final runs and replays for all five sizes. The 810k size can take about an hour.
$h = git rev-parse --short HEAD
foreach ($N in 10000,30000,90000,270000,810000) {
  ./build/Driver.exe $N final $h 2> "Results/log_NW_${N}_final.txt"
  ./build/run_Replay.exe $N final $h 2> "Results/log_replay_NW_${N}_final.txt"
}
5.  Check each size. Only PASS should show, 3780 in the timed file and 630 in the replay file.
 foreach ($N in 10000,30000,90000,270000,810000) { 
    "--- $N ---" 
    Import-Csv "Results\timed_NW_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count 
    Import-Csv "Results\wtotal_NW_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count 
 }

6.  Save and push.
git add Results
git commit -m "Final runs and replays NW 10k-810k"
git push

For the Cal Dat Set
1.(me only) move datset
Move-Item "$Home\Downloads\USA-road-t.CAL.gr.gz" data\raw\
2. (me only) unzip n test
$in=[IO.File]::OpenRead("$PWD\USA-road-t.CAL.gr.gz"); $gz=New-Object IO.Compression.GZipStream($in,[IO.Compression.CompressionMode]::Decompress); $out=[IO.File]::Create("$PWD\USA-road-t.CAL.gr"); $gz.CopyTo($out); $out.Close(); $gz.Close(); $in.Close()
3. (me only) show where need to change file name to match \Projects\ResearchRecomputation> Select-String -Path src\run_GraphPrep.cpp,src\run_BatchGen.cpp,src\Driver.cpp,src\run_Replay.cpp -Pattern "NW" -CaseSensitive
4.chnage fields
5. compile
g++ -std=c++17 -O2 -Wall -Wextra src/run_GraphPrep.cpp src/GraphPrep.cpp src/Graph.cpp src/Dijkstra.cpp src/MinHeap.cpp -o build/run_GraphPrep.exe
g++ -std=c++17 -O2 -Wall -Wextra src/run_BatchGen.cpp src/BatchGen.cpp src/Graph.cpp -o build/run_BatchGen.exe
g++ -std=c++17 -O2 -Wall -Wextra src/Driver.cpp src/DDFLPD_WL.cpp src/DDFLPD.cpp src/Dijkstra.cpp src/Validator.cpp src/BatchGen.cpp src/Graph.cpp src/MinHeap.cpp -o build/Driver.exe -lpsapi
g++ -std=c++17 -O2 -Wall -Wextra src/run_Replay.cpp src/Replay.cpp src/DDFLPD_WL.cpp src/DDFLPD.cpp src/Dijkstra.cpp src/Validator.cpp src/BatchGen.cpp src/Graph.cpp src/MinHeap.cpp -o build/run_Replay.exe
6. Prep Graphs ./build/run_GraphPrep.exe | Tee-Object Results\prep_CAL.txt
Outcome
Arc lines: 4657742
Self-loops: 0
Merged duplicates: 2342520
Unique undirected edges: 2315222
Source component size: 1890815
N=10000 m=11685
N=30000 m=36160
N=90000 m=112332
N=270000 m=337688
N=810000 m=1006364
7. Make Batches
./build/run_BatchGen.exe | Tee-Object Results\batchgen_CAL.txt
8. checksum
9. push chnages
git commit -m "CAL graph prep and batches"
git push
$h = git rev-parse --short HEAD
$h
10 . Run pilot
foreach ($N in 10000,30000,90000,270000,810000) {
  ./build/Driver.exe $N pilot $h 2> "Results/log_CAL_${N}_pilot.txt"
  Import-Csv "Results\timed_CAL_${N}_pilot.csv" | Group-Object outcome | Select-Object Name, Count
}
./build/run_Replay.exe 10000 pilot $h 2> Results/log_replay_CAL_10000_pilot.txt
./build/run_Replay.exe 810000 pilot $h 2> Results/log_replay_CAL_810000_pilot.txt
11. save pilot

12. run final run
$h = git rev-parse --short HEAD
$h
foreach ($N in 10000,30000,90000,270000,810000) {
  ./build/Driver.exe $N final $h 2> "Results/log_CAL_${N}_final.txt"
  ./build/run_Replay.exe $N final $h 2> "Results/log_replay_CAL_${N}_final.txt"
}
13.scheck and post




