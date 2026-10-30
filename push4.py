import subprocess, os, re
os.chdir(os.path.dirname(os.path.abspath(__file__)))
s = open("src/main.cpp").read()
a = "prog->store(p.downloadProgress().value_or(0.f) / 100.f);"
b = "prog->store(p.downloadTotal() ? (float)p.downloaded() / (float)p.downloadTotal() : 0.f);"
assert a in s
open("src/main.cpp", "w").write(s.replace(a, b))
m = open("mod.json").read().replace("v1.0.0", "v1.1.0")
open("mod.json", "w").write(m)
def sh(c):
    p = subprocess.run(c, shell=True, capture_output=True, text=True)
    return p.stdout + p.stderr
out = sh("git add -A && git -c user.name=Vinz -c user.email=vinz@users.noreply.github.com commit -m 'v1.1.0 progress, cancel, install, server settings' && git push")
rid = sh("sleep 8; gh run list --limit 1 --json databaseId -q '.[0].databaseId'").strip()
open("status.txt", "w").write(out + "\nRUN " + rid)
t = open("wait.py").read()
open("wait.py", "w").write(re.sub(r"gh run view \d+", "gh run view " + rid, t))
