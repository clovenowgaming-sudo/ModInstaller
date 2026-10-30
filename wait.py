import subprocess, time, os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
time.sleep(12)
p = subprocess.run("gh run view 41023084006 --json status,conclusion,jobs -q '[.status,.conclusion,(.jobs[]|.name+\":\"+.status+\":\"+.conclusion)]|@json'", shell=True, capture_output=True, text=True)
open("status.txt", "w").write(p.stdout + p.stderr)
