#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>

using namespace geode::prelude;

static constexpr auto GD_VER  = "2.2081";
static constexpr auto SDK_VER = "5.11.2";
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

class InstallerPopup : public Popup {
protected:
    async::TaskHolder<web::WebResponse> m_listTask, m_dlTask;
    std::vector<Entry> m_entries;
    int m_page = 1, m_total = 0;
    std::string m_query;
    CCLabelBMFont* m_latest = nullptr;
    CCLabelBMFont* m_server = nullptr;
    CCLabelBMFont* m_pageLbl = nullptr;
    CCMenu* m_rows = nullptr;
    TextInput* m_input = nullptr;

    bool init() {
        if (!Popup::init(360.f, 250.f, "GJ_square02.png")) return false;
        m_bgSprite->setColor({46, 64, 158});
        this->setTitle("Mod Installer", "bigFont.fnt");
        m_title->setColor({190, 40, 255});

        auto sz = m_mainLayer->getContentSize();
        float cx = sz.width / 2;

        m_latest = CCLabelBMFont::create("Mods: ...", "bigFont.fnt");
        m_latest->setColor({190, 40, 255});
        m_latest->setScale(.55f);
        m_latest->setPosition({cx, sz.height - 48});
        m_mainLayer->addChild(m_latest);

        auto plat = CCLabelBMFont::create(
            fmt::format("Platform: {} | GD {} | Geode {}", PLATFORM_NAME, GD_VER, SDK_VER).c_str(),
            "chatFont.fnt");
        plat->setScale(.75f);
        plat->setPosition({cx, sz.height - 68});
        m_mainLayer->addChild(plat);

        m_server = CCLabelBMFont::create("Server: ...", "chatFont.fnt");
        m_server->setScale(.8f);
        m_server->setPosition({cx, sz.height - 86});
        m_mainLayer->addChild(m_server);

        m_input = TextInput::create(200.f, "Search mods...", "chatFont.fnt");
        m_input->setPosition({cx - 30, sz.height - 112});
        m_mainLayer->addChild(m_input);

        m_rows = CCMenu::create();
        m_rows->setPosition({0, 0});
        m_rows->setContentSize(sz);
        m_mainLayer->addChild(m_rows);

        auto go = CCMenuItemExt::createSpriteExtra(
            ButtonSprite::create("Search", "goldFont.fnt", "GJ_button_01.png", .6f),
            [this](auto) { m_query = m_input->getString(); m_page = 1; this->fetch(); });
        go->setPosition({cx + 110, sz.height - 112});
        m_buttonMenu->addChild(go);

        auto l = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        auto r = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        r->setFlipX(true);
        auto prev = CCMenuItemExt::createSpriteExtra(l, [this](auto) {
            if (m_page > 1) { m_page--; this->fetch(); }
        });
        auto next = CCMenuItemExt::createSpriteExtra(r, [this](auto) {
            if (m_page * 3 < m_total) { m_page++; this->fetch(); }
        });
        prev->setPosition({cx - 70, 22});
        next->setPosition({cx + 70, 22});
        m_buttonMenu->addChild(prev);
        m_buttonMenu->addChild(next);

        m_pageLbl = CCLabelBMFont::create("1", "bigFont.fnt");
        m_pageLbl->setScale(.5f);
        m_pageLbl->setPosition({cx, 22});
        m_mainLayer->addChild(m_pageLbl);

        auto credit = CCLabelBMFont::create("Mod By Vinz :3", "chatFont.fnt");
        credit->setScale(.45f);
        credit->setOpacity(150);
        credit->setPosition({sz.width - 45, 8});
        m_mainLayer->addChild(credit);

        this->fetch();
        return true;
    }

    void setServer(std::string const& t, ccColor3B c) {
        m_server->setString(t.c_str());
        m_server->setColor(c);
    }

    void fetch() {
        setServer("Server: Loading...", {255, 220, 60});
        auto url = fmt::format(
            "https://api.geode-sdk.org/v1/mods?gd={}&geode={}&platforms={}&per_page=3&page={}&query={}",
            GD_VER, SDK_VER, PLATFORM, m_page, enc(m_query));
        m_listTask.spawn(web::WebRequest().get(url), [this](web::WebResponse res) {
            this->onList(std::move(res));
        });
    }

    void onList(web::WebResponse res) {
        m_entries.clear();
        if (!res.ok()) {
            setServer("Server: Offline", {255, 60, 60});
            return this->render();
        }
        auto js = res.json();
        if (!js) {
            setServer("Server: Offline", {255, 60, 60});
            return this->render();
        }
        auto v = js.unwrap();
        m_total = (int)v["payload"]["count"].asInt().unwrapOr((int64_t)0);
        auto dataRes = v["payload"]["data"].asArray();
        if (!dataRes) {
            setServer("Server: Offline", {255, 60, 60});
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
                    "https://api.geode-sdk.org/v1/mods/{}/versions/latest/download?gd={}&platform={}&geode={}",
                    e.id, GD_VER, PLATFORM, SDK_VER);
            }
            m_entries.push_back(e);
        }
        setServer("Server: Online", {40, 255, 40});
        this->render();
    }

    void render() {
        m_rows->removeAllChildren();
        auto sz = m_mainLayer->getContentSize();
        m_latest->setString(fmt::format("Compatible mods: {}", m_total).c_str());
        m_pageLbl->setString(std::to_string(m_page).c_str());
        float y = sz.height - 148;
        for (auto const& e : m_entries) {
            auto lbl = CCLabelBMFont::create(
                fmt::format("{} {}", e.name, e.ver).c_str(), "chatFont.fnt");
            lbl->limitLabelWidth(230.f, .8f, .3f);
            lbl->setAnchorPoint({0, .5f});
            lbl->setPosition({22, y});
            m_rows->addChild(lbl);

            auto btn = CCMenuItemExt::createSpriteExtra(
                ButtonSprite::create("Get", "goldFont.fnt", "GJ_button_01.png", .6f),
                [this, e](auto) { this->download(e); });
            btn->setPosition({sz.width - 45, y});
            m_rows->addChild(btn);
            y -= 30;
        }
    }

    void download(Entry e) {
        setServer(fmt::format("Downloading {}...", e.name), {255, 220, 60});
        m_dlTask.spawn(web::WebRequest().get(e.link), [this, e](web::WebResponse res) {
            if (!res.ok()) {
                return setServer("Download failed", {255, 60, 60});
            }
            auto data = res.data();
            auto fname = e.id + ".geode";
#ifdef GEODE_IS_ANDROID
            std::filesystem::path dir = "/storage/emulated/0/Download";
#else
            std::filesystem::path dir = Mod::get()->getSaveDir();
#endif
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            auto w = file::writeBinary(dir / fname, data);
            if (!w) {
                auto alt = Mod::get()->getSaveDir() / fname;
                if (file::writeBinary(alt, data)) {
                    return setServer("Saved in mod save folder", {255, 160, 40});
                }
                return setServer("Save failed", {255, 60, 60});
            }
            setServer("Saved to Download: " + fname, {40, 255, 40});
        });
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
