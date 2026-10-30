import os, subprocess, sys, glob, shutil, time

HERE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(HERE, "deploy.log")
os.chdir(HERE)

def log(*a):
    with open(LOG, "a") as f:
        f.write(" ".join(str(x) for x in a) + "\n")

def run(cmd, check=False):
    log("$", cmd)
    p = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    log(p.stdout.strip(), p.stderr.strip(), "[rc=%d]" % p.returncode)
    return p

def find_download_dir():
    for d in (os.path.expanduser("~/storage/downloads"), "/storage/emulated/0/Download", "/sdcard/Download"):
        if os.path.isdir(d):
            return d
    return None

if len(sys.argv) > 1 and sys.argv[1] == "finish":
    # stage 2: wait for build, download artifact, copy to Download
    time.sleep(10)
    p = run("gh run list --limit 1 --json databaseId -q '.[0].databaseId'")
    rid = p.stdout.strip()
    if not rid:
        log("NO RUN FOUND"); sys.exit(1)
    run("gh run watch %s --exit-status" % rid)
    shutil.rmtree("out", ignore_errors=True)
    run("gh run download %s -D out" % rid)
    files = glob.glob("out/**/*.geode", recursive=True)
    if not files:
        log("BUILD FAILED OR NO .geode FILE. Check: gh run view %s --log-failed" % rid)
        run("gh run view %s --log-failed | tail -n 60" % rid)
        sys.exit(1)
    dl = find_download_dir()
    if not dl:
        log("Download folder not found. Run: termux-setup-storage"); sys.exit(1)
    for f in files:
        shutil.copy(f, dl)
        log("COPIED", f, "->", dl)
    log("DONE")
    sys.exit(0)

# stage 1: git init, commit, create repo, push
open(LOG, "w").close()
if not os.path.isdir(".git"):
    run("git init -b main")
run("git add -A")
run("git -c user.name=Vinz -c user.email=vinz@users.noreply.github.com commit -m 'Mod Installer'")
p = run("gh repo create ModInstaller --public --source=. --push")
if p.returncode != 0:
    run("git remote -v")
    run("git push -u origin main")
# start stage 2 detached
subprocess.Popen([sys.executable, os.path.abspath(__file__), "finish"],
                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                 start_new_session=True)
log("PUSHED, finish stage started in background")
