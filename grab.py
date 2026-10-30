import subprocess, os, re, glob, shutil
os.chdir(os.path.dirname(os.path.abspath(__file__)))
rid = re.search(r"gh run view (\d+)", open("wait.py").read()).group(1)
shutil.rmtree("out", ignore_errors=True)
log = []
p = subprocess.run("gh run download %s -D out" % rid, shell=True, capture_output=True, text=True, timeout=25)
log.append(p.stdout + p.stderr)
files = glob.glob("out/**/*.geode", recursive=True)
log.append("FILES: %s" % files)
dl = None
for d in (os.path.expanduser("~/storage/downloads"), "/storage/emulated/0/Download", "/sdcard/Download"):
    if os.path.isdir(d):
        dl = d
        break
log.append("DL DIR: %s" % dl)
if dl:
    for f in files:
        try:
            shutil.copy(f, dl)
            log.append("COPIED %s -> %s" % (f, dl))
        except Exception as e:
            log.append("COPY FAIL %s: %s" % (f, e))
open("status.txt", "w").write("\n".join(log))
