import subprocess, os, re
os.chdir(os.path.dirname(os.path.abspath(__file__)))
s = open("src/main.cpp").read()
a = '''        auto data = v["payload"]["data"].asArray().unwrapOr(std::vector<matjson::Value>{});
        for (auto& m : data) {
            auto vers = m["versions"].asArray().unwrapOr(std::vector<matjson::Value>{});
            if (vers.empty()) continue;'''
b = '''        auto dataRes = v["payload"]["data"].asArray();
        if (!dataRes) {
            setServer("Server: Offline", {255, 60, 60});
            return this->render();
        }
        for (auto& m : dataRes.unwrap()) {
            auto versRes = m["versions"].asArray();
            if (!versRes) continue;
            auto& vers = versRes.unwrap();
            if (vers.empty()) continue;'''
assert a in s
s = s.replace(a, b)
open("src/main.cpp", "w").write(s)
def sh(c):
    p = subprocess.run(c, shell=True, capture_output=True, text=True)
    return p.stdout + p.stderr
out = sh("git add -A && git -c user.name=Vinz -c user.email=vinz@users.noreply.github.com commit -m 'fix asArray result' && git push")
rid = sh("sleep 8; gh run list --limit 1 --json databaseId -q '.[0].databaseId'").strip()
open("status.txt", "w").write(out + "\nRUN " + rid)
t = open("wait.py").read()
open("wait.py", "w").write(re.sub(r"gh run view \d+", "gh run view " + rid, t))
