import subprocess, os
os.chdir(os.path.dirname(os.path.abspath(__file__)))
p = subprocess.run("gh run view 41022208216 --log-failed", shell=True, capture_output=True, text=True)
lines = [l.split("Z ", 1)[-1] for l in p.stdout.splitlines() if "error:" in l or "error C" in l]
open("errors.txt", "w").write("\n".join(l[:260] for l in lines[:30]) or p.stdout[-3000:])
