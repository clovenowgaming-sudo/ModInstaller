import subprocess, os, time
os.chdir(os.path.dirname(os.path.abspath(__file__)))
def sh(c):
    return subprocess.run(c, shell=True, capture_output=True, text=True)
sh("gh run cancel 41022520328")
time.sleep(8)
p = sh("gh run view 41022520328 --log-failed")
lines = [l.split("Z ", 1)[-1] for l in p.stdout.splitlines() if ("error:" in l or "error C" in l) and "Android64" in l.split("\t")[0]]
open("errors.txt", "w").write("\n".join(l[:300] for l in lines[:30]) or p.stdout[-2500:])
