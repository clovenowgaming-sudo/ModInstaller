#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/loader/Dirs.hpp>
#include <atomic>
#include <memory>
#include <functional>

using namespace geode::prelude;

static constexpr auto GD_VER  = "2.2081";
static constexpr auto SDK_VER = "5.11.2";
static constexpr auto DEFAULT_URL = "https://api.geode-sdk.org";
#ifdef GEODE_IS_ANDROID64
static constexpr auto PLATFORM = "android64";
static constexpr auto PLATFORM_NAME = "Android 64";
#elif defined(GEODE_IS_ANDROID32)
static constexpr auto PLATFORM = "android32";
static constexpr auto PLATFORM_NAME = "Android 32";
#else
static constexpr auto PLATFORM = "windows";
static constexpr auto PLATFORM_NAME = "Windows";
#endif

static constexpr ccColor3B COL_GREEN  = {40, 255, 40};
static constexpr ccColor3B COL_RED    = {255, 60, 60};
static constexpr ccColor3B COL_YELLOW = {255, 220, 60};
static constexpr ccColor3B COL_ORANGE = {255, 160, 40};
static constexpr ccColor3B COL_PURPLE = {190, 40, 255};

struct Entry { std::string id, name, ver, link; };

static std::string enc(std::string const& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string o;
    for (unsigned char c : s) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.') o += (char)c;
        else { o += '%'; o += hex[c >> 4]; o += hex[c & 15]; }
    }
    return o;
}

static std::string cleanUrl(std::string u) {
    const char* ws = " \t\r\n";
    u.erase(0, u.find_first_not_of(ws));
    auto e = u.find_last_not_of(ws);
    if (e == std::string::npos) u.clear(); else u.erase(e + 1);
    while (!u.empty() && u.back() == '/') u.pop_back();
    if (!u.empty() && u.rfind("http://", 0) != 0 && u.rfind("https://", 0) != 0) {
        u = "https://" + u;
    }
    return u;
}

static std::string baseUrl() {
    auto u = cleanUrl(Mod::get()->getSavedValue<std::string>("api-url", DEFAULT_URL));
    return u.empty() ? std::string(DEFAULT_URL) : u;
}

static std::string norm(std::string v) {
    if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) v.erase(0, 1);
    return v;
}

// ---------------------------------------------------------------- settings
class SettingsPopup : public Popup {
protected:
    async::TaskHolder<web::WebResponse> m_task;
    TextInput* m_input = nullptr;
    CCLabelBMFont* m_status = nullptr;
    std::function<void()> m_onSaved;

    bool init(std::function<void()> onSaved) {
        if (!Popup::init(320.f, 220.f, "GJ_square02.png")) return false;
        m_onSaved = std::move(onSaved);
        m_bgSprite->setColor({46, 64, 158});
        this->setTitle("Server Settings", "bigFont.fnt");
        m_title->setColor(COL_PURPLE);

        auto sz = m_mainLayer->getContentSize();
        float cx = sz.width / 2;

        auto lbl = CCLabelBMFont::create("API URL (Geode index compatible)", "chatFont.fnt");
        lbl->setScale(.8f);
        lbl->setPosition({cx, sz.height - 52});
        m_mainLayer->addChild(lbl);

        m_input = TextInput::create(270.f, "https://your-server.com", "chatFont.fnt");
        m_input->setCommonFilter(CommonFilter::Any);
        m_input->setString(baseUrl());
        m_input->setPosition({cx, sz.height - 80});
        m_mainLayer->addChild(m_input);

        auto connect = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Connect", "goldFont.fnt", "GJ_button_01.png", .8f),
            [this](auto) { this->testConnect(); });
        connect->setPosition({cx, sz.height - 120});
        m_buttonMenu->addChild(connect);

        m_status = CCLabelBMFont::create("Not connected", "chatFont.fnt");
        m_status->setScale(.85f);
        m_status->setPosition({cx, sz.height - 152});
        m_mainLayer->addChild(m_status);

        auto save = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Save", "goldFont.fnt", "GJ_button_01.png", .7f),
            [this](auto) {
                Mod::get()->setSavedValue<std::string>("api-url", cleanUrl(m_input->getString()));
                if (m_onSaved) m_onSaved();
                this->onClose(nullptr);
            });
        save->setPosition({cx - 55, 26});
        m_buttonMenu->addChild(save);

        auto reset = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Default", "goldFont.fnt", "GJ_button_06.png", .7f),
            [this](auto) {
                m_input->setString(DEFAULT_URL);
                m_status->setString("Not connected");
                m_status->setColor({255, 255, 255});
            });
        reset->setPosition({cx + 55, 26});
        m_buttonMenu->addChild(reset);
        return true;
    }

    void setStatus(std::string const& t, ccColor3B c) {
        m_status->setString(t.c_str());
        m_status->setColor(c);
    }

    void testConnect() {
        auto url = cleanUrl(m_input->getString());
        if (url.empty()) return setStatus("Enter a URL first", COL_RED);
        Mod::get()->setSavedValue<std::string>("api-url", url);
        setStatus("Connecting...", COL_YELLOW);
        auto full = fmt::format("{}/v1/mods?gd={}&geode={}&platforms={}&per_page=1&page=1",
                                url, GD_VER, SDK_VER, PLATFORM);
        m_task.spawn(web::WebRequest().timeout(std::chrono::seconds(12)).get(full),
            [this](web::WebResponse res) {
                if (!res.ok()) {
                    return setStatus(fmt::format("Failed (HTTP {})", res.code()), COL_RED);
                }
                auto js = res.json();
                if (!js) return setStatus("Failed: invalid response", COL_RED);
                auto v = js.unwrap();
                auto c = v["payload"]["count"].asInt();
                if (!c) return setStatus("Failed: not a mod server", COL_RED);
                setStatus(fmt::format("Connected! {} mods found", c.unwrap()), COL_GREEN);
            });
    }

public:
    static SettingsPopup* create(std::function<void()> cb) {
        auto r = new SettingsPopup();
        if (r->init(std::move(cb))) { r->autorelease(); return r; }
        delete r;
        return nullptr;
    }
};

// ---------------------------------------------------------------- main popup
class InstallerPopup : public Popup {
protected:
    async::TaskHolder<web::WebResponse> m_listTask, m_dlTask;
    std::vector<Entry> m_entries;
    int m_page = 1, m_total = 0;
    std::string m_query;
    std::string m_activeId;
    std::shared_ptr<std::atomic<float>> m_prog = std::make_shared<std::atomic<float>>(0.f);
    CCLabelBMFont* m_latest = nullptr;
    CCLabelBMFont* m_server = nullptr;
    CCLabelBMFont* m_pageLbl = nullptr;
    CCLabelBMFont* m_msg = nullptr;
    CCLabelBMFont* m_pct = nullptr;
    CCLayerColor* m_barFill = nullptr;
    CCMenu* m_rows = nullptr;
    TextInput* m_input = nullptr;
    static constexpr float BAR_W = 220.f;

    bool init() {
        if (!Popup::init(360.f, 270.f, "GJ_square02.png")) return false;
        m_bgSprite->setColor({46, 64, 158});
        this->setTitle("Mod Installer", "bigFont.fnt");
        m_title->setColor(COL_PURPLE);

        auto sz = m_mainLayer->getContentSize();
        float cx = sz.width / 2;

        m_latest = CCLabelBMFont::create("Mods: ...", "bigFont.fnt");
        m_latest->setColor(COL_PURPLE);
        m_latest->setScale(.55f);
        m_latest->setPosition({cx, sz.height - 46});
        m_mainLayer->addChild(m_latest);

        auto plat = CCLabelBMFont::create(
            fmt::format("Platform: {} | GD {} | Geode {}", PLATFORM_NAME, GD_VER, SDK_VER).c_str(),
            "chatFont.fnt");
        plat->setScale(.75f);
        plat->setPosition({cx, sz.height - 64});
        m_mainLayer->addChild(plat);

        m_server = CCLabelBMFont::create("Server: ...", "chatFont.fnt");
        m_server->setScale(.8f);
        m_server->setPosition({cx, sz.height - 80});
        m_mainLayer->addChild(m_server);

        m_input = TextInput::create(200.f, "Search mods...", "chatFont.fnt");
        m_input->setPosition({cx - 30, sz.height - 104});
        m_mainLayer->addChild(m_input);

        m_rows = CCMenu::create();
        m_rows->setPosition({0, 0});
        m_rows->setContentSize(sz);
        m_mainLayer->addChild(m_rows);

        auto go = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Search", "goldFont.fnt", "GJ_button_01.png", .6f),
            [this](auto) { m_query = m_input->getString(); m_page = 1; this->fetch(); });
        go->setPosition({cx + 110, sz.height - 104});
        m_buttonMenu->addChild(go);

        // settings (gear) button, top right
        auto gear = CCSprite::createWithSpriteFrameName("GJ_optionsBtn02_001.png");
        gear->setScale(.6f);
        auto gearBtn = CCMenuItemExt::createSpriteExtra(gear, [this](auto) {
            Ref<InstallerPopup> self = this;
            if (auto p = SettingsPopup::create([self]() { self->m_page = 1; self->fetch(); })) p->show();
        });
        gearBtn->setPosition({sz.width - 24, sz.height - 24});
        m_buttonMenu->addChild(gearBtn);

        auto l = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto r = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        r->setFlipX(true);
        auto prev = CCMenuItemExt::createSpriteExtra(l, [this](auto) {
            if (m_page > 1) { m_page--; this->fetch(); }
        });
        auto next = CCMenuItemExt::createSpriteExtra(r, [this](auto) {
            if (m_page * 3 < m_total) { m_page++; this->fetch(); }
        });
        prev->setPosition({cx - 70, 18});
        next->setPosition({cx + 70, 18});
        m_buttonMenu->addChild(prev);
        m_buttonMenu->addChild(next);

        m_pageLbl = CCLabelBMFont::create("1", "bigFont.fnt");
        m_pageLbl->setScale(.5f);
        m_pageLbl->setPosition({cx, 18});
        m_mainLayer->addChild(m_pageLbl);

        m_msg = CCLabelBMFont::create("", "chatFont.fnt");
        m_msg->setScale(.75f);
        m_msg->setPosition({cx, 40});
        m_mainLayer->addChild(m_msg);

        auto credit = CCLabelBMFont::create("Mod By Vinz :3", "chatFont.fnt");
        credit->setScale(.45f);
        credit->setOpacity(150);
        credit->setPosition({sz.width - 45, 8});
        m_mainLayer->addChild(credit);

        this->scheduleUpdate();
        this->fetch();
        return true;
    }

    void update(float) override {
        if (m_activeId.empty() || !m_barFill) return;
        float p = m_prog->load();
        if (p < 0.f) p = 0.f;
        if (p > 1.f) p = 1.f;
        m_barFill->setContentSize({BAR_W * p, 6.f});
        if (m_pct) m_pct->setString(fmt::format("{}%", (int)(p * 100)).c_str());
    }

    void setServer(std::string const& t, ccColor3B c) {
        m_server->setString(t.c_str());
        m_server->setColor(c);
    }

    void setMsg(std::string const& t, ccColor3B c) {
        m_msg->setString(t.c_str());
        m_msg->setColor(c);
    }

    void fetch() {
        setServer("Server: Loading...", COL_YELLOW);
        auto url = fmt::format(
            "{}/v1/mods?gd={}&geode={}&platforms={}&per_page=3&page={}&query={}",
            baseUrl(), GD_VER, SDK_VER, PLATFORM, m_page, enc(m_query));
        m_listTask.spawn(web::WebRequest().timeout(std::chrono::seconds(15)).get(url),
            [this](web::WebResponse res) { this->onList(std::move(res)); });
    }

    void onList(web::WebResponse res) {
        m_entries.clear();
        if (!res.ok()) {
            setServer("Server: Offline", COL_RED);
            return this->render();
        }
        auto js = res.json();
        if (!js) {
            setServer("Server: Offline", COL_RED);
            return this->render();
        }
        auto v = js.unwrap();
        m_total = (int)v["payload"]["count"].asInt().unwrapOr((int64_t)0);
        auto dataRes = v["payload"]["data"].asArray();
        if (!dataRes) {
            setServer("Server: Offline", COL_RED);
            return this->render();
        }
        for (auto& m : dataRes.unwrap()) {
            auto versRes = m["versions"].asArray();
            if (!versRes) continue;
            auto& vers = versRes.unwrap();
            if (vers.empty()) continue;
            Entry e;
            e.id   = m["id"].asString().unwrapOr(std::string());
            e.name = vers[0]["name"].asString().unwrapOr(std::string(e.id));
            e.ver  = vers[0]["version"].asString().unwrapOr(std::string());
            e.link = vers[0]["download_link"].asString().unwrapOr(std::string());
            if (e.link.empty()) {
                e.link = fmt::format(
                    "{}/v1/mods/{}/versions/latest/download?gd={}&platform={}&geode={}",
                    baseUrl(), e.id, GD_VER, PLATFORM, SDK_VER);
            }
            m_entries.push_back(e);
        }
        setServer("Server: Online", COL_GREEN);
        this->render();
    }

    void render() {
        m_rows->removeAllChildren();
        m_barFill = nullptr;
        m_pct = nullptr;
        auto sz = m_mainLayer->getContentSize();
        m_latest->setString(fmt::format("Compatible mods: {}", m_total).c_str());
        m_pageLbl->setString(std::to_string(m_page).c_str());
        float y = sz.height - 136;
        for (auto const& e : m_entries) {
            bool active = (m_activeId == e.id);
            auto lbl = CCLabelBMFont::create(
                fmt::format("{} {}", e.name, e.ver).c_str(), "chatFont.fnt");
            lbl->limitLabelWidth(180.f, .8f, .3f);
            lbl->setAnchorPoint({0, .5f});
            lbl->setPosition({22, y + 5});
            m_rows->addChild(lbl);

            auto spr = active
                ? ButtonSprite::create("Downloading", "goldFont.fnt", "GJ_button_06.png", .5f)
                : ButtonSprite::create("Get", "goldFont.fnt", "GJ_button_01.png", .6f);
            auto btn = CCMenuItemExt::createSpriteExtra(spr, [this, e](auto) { this->onGet(e); });
            btn->setPosition({sz.width - 55, y + 3});
            m_rows->addChild(btn);

            if (active) {
                auto bg = CCLayerColor::create({15, 20, 60, 255}, BAR_W, 6.f);
                bg->setPosition({22, y - 12});
                m_rows->addChild(bg);
                m_barFill = CCLayerColor::create({60, 255, 90, 255}, 1.f, 6.f);
                m_barFill->setPosition({22, y - 12});
                m_rows->addChild(m_barFill);
                m_pct = CCLabelBMFont::create("0%", "chatFont.fnt");
                m_pct->setScale(.65f);
                m_pct->setAnchorPoint({0, .5f});
                m_pct->setPosition({22 + BAR_W + 6, y - 9});
                m_rows->addChild(m_pct);
            }
            y -= 34;
        }
    }

    void onGet(Entry const& e) {
        if (!m_activeId.empty()) {
            if (m_activeId == e.id) return this->cancelDownload();
            return setMsg("Another download is in progress", COL_ORANGE);
        }
        if (auto mod = Loader::get()->getInstalledMod(e.id)) {
            if (norm(mod->getVersion().toVString()) == norm(e.ver)) {
                Ref<InstallerPopup> self = this;
                createQuickPopup(
                    "Replace Version",
                    fmt::format("<cy>{}</c> v{} is already installed.\nReplace it with the same version?",
                                e.name, norm(e.ver)),
                    "Cancel", "Replace",
                    [self, e](FLAlertLayer*, bool yes) { if (yes) self->startDownload(e); });
                return;
            }
        }
        this->startDownload(e);
    }

    void cancelDownload() {
        m_dlTask.cancel();
        m_activeId.clear();
        setMsg("Download cancelled", COL_ORANGE);
        this->render();
    }

    void startDownload(Entry e) {
        if (!m_activeId.empty()) return;
        m_activeId = e.id;
        m_prog->store(0.f);
        setMsg("Downloading " + e.name + "...", COL_YELLOW);
        this->render();
        auto prog = m_prog;
        auto req = web::WebRequest();
        req.onProgress([prog](web::WebProgress const& p) {
            prog->store(p.downloadTotal() ? (float)p.downloaded() / (float)p.downloadTotal() : 0.f);
        });
        m_dlTask.spawn(req.get(e.link), [this, e](web::WebResponse res) {
            this->onDownloaded(e, std::move(res));
        });
    }

    void onDownloaded(Entry e, web::WebResponse res) {
        m_activeId.clear();
        if (!res.ok()) {
            setMsg(fmt::format("Download failed (HTTP {})", res.code()), COL_RED);
            return this->render();
        }
        auto data = res.data();

        std::filesystem::path target = dirs::getModsDir() / (e.id + ".geode");
        if (auto old = Loader::get()->getInstalledMod(e.id)) {
            auto p = old->getPackagePath();
            if (p.extension() == ".geode") target = p;
        }
        auto w = file::writeBinary(target, data);
        if (!w) {
            setMsg("Install failed: " + w.unwrapErr(), COL_RED);
            return this->render();
        }
#ifdef GEODE_IS_ANDROID
        {   // best-effort copy to the Download folder
            std::error_code ec;
            std::filesystem::path dl = "/storage/emulated/0/Download";
            std::filesystem::create_directories(dl, ec);
            (void)file::writeBinary(dl / (e.id + ".geode"), data);
        }
#endif
        setMsg(fmt::format("Installed {} {} - restart GD to apply", e.name, e.ver), COL_GREEN);
        this->render();
    }

public:
    static InstallerPopup* create() {
        auto r = new InstallerPopup();
        if (r->init()) { r->autorelease(); return r; }
        delete r;
        return nullptr;
    }
};

class $modify(InstallerMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        auto spr = ButtonSprite::create("Mods+", "goldFont.fnt", "GJ_button_01.png", .7f);
        auto btn = CCMenuItemExt::createSpriteExtra(spr, [](auto) {
            if (auto p = InstallerPopup::create()) p->show();
        });
        btn->setID("mod-installer-button"_spr);
        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }
};
