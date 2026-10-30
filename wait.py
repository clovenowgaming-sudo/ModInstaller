import subprocess, time, os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
time.sleep(12)
p = subprocess.run("gh run view 41022208216 --json status,conclusion,jobs -q '[.status,.conclusion,(.jobs[]|.name+\":\"+.status+\":\"+.conclusion)]|@json'; tail -n 5 deploy.log", shell=True, capture_output=True, text=True)
open("status.txt", "w").write(p.stdout + p.stderr)
