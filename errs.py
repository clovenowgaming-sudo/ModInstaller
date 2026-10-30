import subprocess, os, re
os.chdir(os.path.dirname(os.path.abspath(__file__)))
rid = re.search(r"gh run view (\d+)", open("wait.py").read()).group(1)
subprocess.run("gh run view %s --log-failed > full.log 2>&1" % rid, shell=True, timeout=25)
out = []
for l in open("full.log", errors="ignore"):
    if "Android64" in l.split("\t")[0] and ("error" in l or "note:" in l or "required from" in l):
        out.append(l.split("Z ", 1)[-1].strip()[:400])
open("errors.txt", "w").write("RUN " + rid + "\n" + "\n".join(out[:40]))
