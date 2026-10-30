import subprocess, os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
s = open("src/main.cpp").read()
s = s.replace('.asString().unwrapOr("")', '.asString().unwrapOr(std::string())')
s = s.replace('.asString().unwrapOr(e.id)', '.asString().unwrapOr(std::string(e.id))')
s = s.replace('.asInt().unwrapOr(0)', '.asInt().unwrapOr((int64_t)0)')
open("src/main.cpp", "w").write(s)
def sh(c):
    p = subprocess.run(c, shell=True, capture_output=True, text=True)
    return p.stdout + p.stderr
out = sh("git add -A && git -c user.name=Vinz -c user.email=vinz@users.noreply.github.com commit -m 'fix unwrapOr types' && git push")
rid = sh("sleep 8; gh run list --limit 1 --json databaseId -q '.[0].databaseId'").strip()
open("status.txt", "w").write(out + "\nRUN " + rid)
t = open("wait.py").read()
import re
t = re.sub(r"gh run view \d+", "gh run view " + rid, t)
open("wait.py", "w").write(t)
e = open("errs.py").read()
e = re.sub(r"gh run (cancel|view) \d+", lambda m: "gh run %s %s" % (m.group(1), rid), e)
open("errs.py", "w").write(e)
