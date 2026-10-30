import subprocess, os, re
os.chdir(os.path.dirname(os.path.abspath(__file__)))
s = open("src/main.cpp").read()
s = s.replace('.asArray().unwrapOr({})', '.asArray().unwrapOr(std::vector<matjson::Value>{})')
open("src/main.cpp", "w").write(s)
def sh(c):
    p = subprocess.run(c, shell=True, capture_output=True, text=True)
    return p.stdout + p.stderr
out = sh("git add -A && git -c user.name=Vinz -c user.email=vinz@users.noreply.github.com commit -m 'fix unwrapOr' && git push")
time_wait = sh("sleep 8; gh run list --limit 1 --json databaseId -q '.[0].databaseId'")
open("status.txt", "w").write(out + "\nRUN " + time_wait)
