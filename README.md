# ResearchRecomputation
Before each run, the files have to match the name of the dataset. Change NW / CAL / NE / LKS in these lines, save(ctrl+s), and rebuild:
Locations need to match
  - src/run_GraphPrep.cpp lines 18 and 64
  - src/run_BatchGen.cpp lines 19 and 37
  - src/Driver.cpp line 529
  - src/run_Replay.cpp line 211

1. For NW
$h = git rev-parse --short HEAD
$h

Final run:
foreach ($N in 10000,30000,90000,270000,810000) {
  ./build/Driver.exe $N final $h 2> "Results/log_NW_${N}_final.txt"
  ./build/run_Replay.exe $N final $h 2> "Results/log_replay_NW_${N}_final.txt"
}

Check and push:
foreach ($N in 10000,30000,90000,270000,810000) {
  "--- $N ---"
  Import-Csv "Results\timed_NW_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count
  Import-Csv "Results\wtotal_NW_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count
}
git add Results
git commit -m "Final runs and replays NW 10k-810k"
git push

2. For the CAL Data Set
$h = git rev-parse --short HEAD
$h

Final run:
foreach ($N in 10000,30000,90000,270000,810000) {
  ./build/Driver.exe $N final $h 2> "Results/log_CAL_${N}_final.txt"
  ./build/run_Replay.exe $N final $h 2> "Results/log_replay_CAL_${N}_final.txt"
}

Check and push:
foreach ($N in 10000,30000,90000,270000,810000) {
  "--- $N ---"
  Import-Csv "Results\timed_CAL_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count
  Import-Csv "Results\wtotal_CAL_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count
}
git add Results
git commit -m "Final runs and replays CAL 10k-810k"
git push

3. NE
$h = git rev-parse --short HEAD
$h

Final run:
foreach ($N in 10000,30000,90000,270000,810000) {
  ./build/Driver.exe $N final $h 2> "Results/log_NE_${N}_final.txt"
  ./build/run_Replay.exe $N final $h 2> "Results/log_replay_NE_${N}_final.txt"
}

Check and push:
foreach ($N in 10000,30000,90000,270000,810000) {
  "--- $N ---"
  Import-Csv "Results\timed_NE_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count
  Import-Csv "Results\wtotal_NE_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count
}
git add Results
git commit -m "Final runs and replays NE 10k-810k"
git push

4. Last one LKS
$h = git rev-parse --short HEAD
$h

Final run:
foreach ($N in 10000,30000,90000,270000,810000) {
  ./build/Driver.exe $N final $h 2> "Results/log_LKS_${N}_final.txt"
  ./build/run_Replay.exe $N final $h 2> "Results/log_replay_LKS_${N}_final.txt"
}

Check and push:
foreach ($N in 10000,30000,90000,270000,810000) {
  "--- $N ---"
  Import-Csv "Results\timed_LKS_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count
  Import-Csv "Results\wtotal_LKS_${N}_final.csv" | Group-Object outcome | Select-Object Name, Count
}
git add Results
git commit -m "Final runs and replays LKS 10k-810k"
git push
git status