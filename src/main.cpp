#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "Common.hpp"
#include "StudioPopup.hpp"

using namespace geode::prelude;

// ---------------------------------------------------------------
// 메인 메뉴 하단에 Studio 열기 버튼 추가
// ---------------------------------------------------------------
class $modify(StudioMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        if (auto menu = this->getChildByID("bottom-menu")) {
            auto label = CCLabelBMFont::create("Bar", "bigFont.fnt");
            label->setScale(0.8f);
            auto spr = CircleButtonSprite::create(label);
            auto btn = CCMenuItemSpriteExtra::create(
                spr, this, menu_selector(StudioMenuLayer::onStudio)
            );
            btn->setID("studio-button"_spr);
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }

    void onStudio(CCObject*) {
        StudioPopup::create()->show();
    }
};

// ---------------------------------------------------------------
// 인게임 프로그래스 바
// ---------------------------------------------------------------
class $modify(BarPlayLayer, PlayLayer) {
    struct Fields {
        std::string style;
        bool showPercent = true;
        bool barEnabled = true;   // 유저가 GD에서 프로그래스 바를 켜뒀는지
        float width = 240.f;
        float height = 8.f;
        float last = -1.f;
        ccColor3B color1 = { 90, 200, 255 };   // 바 색 (Studio에서 선택한 프리셋)
        ccColor3B track = { 66, 72, 88 };      // 배경 색 (Studio에서 선택한 프리셋)
        ccColor3B color2 = { 180, 120, 255 };  // 보조 색 (모드 설정)

        CCNode* root = nullptr;
        CCDrawNode* draw = nullptr;          // Modern
        CCLayerGradient* fill = nullptr;     // Gradient
        CCLabelBMFont* label = nullptr;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        auto mod = Mod::get();
        auto f = m_fields.self();

        f->style = mod->getSettingValue<std::string>("style");
        if (f->style == "Vanilla") return;

        auto bar = this->getChildByID("progress-bar");
        if (!bar) return;

        int sel = studio::selected();
        f->color1 = studio::barColor(sel);
        f->track = studio::trackColor(sel);
        f->color2 = mod->getSettingValue<ccColor3B>("color-2");
        f->width = static_cast<float>(mod->getSettingValue<double>("bar-width"));
        f->height = static_cast<float>(mod->getSettingValue<double>("bar-height"));
        f->showPercent = mod->getSettingValue<bool>("show-percent");
        f->barEnabled = bar->isVisible(); // GD 자체 설정을 존중

        auto root = CCNode::create();
        root->setID("custom-bar-root"_spr);
        root->setContentSize({ f->width, f->height });
        root->setAnchorPoint({ 0.5f, 0.5f });
        root->setPosition(bar->getPosition());
        root->setVisible(f->barEnabled);
        bar->getParent()->addChild(root, bar->getZOrder());
        f->root = root;

        if (f->style == "Gradient") buildGradient();
        else buildModern();

        if (f->showPercent) {
            auto label = CCLabelBMFont::create("0%", "bigFont.fnt");
            label->setScale(0.35f);
            label->setAnchorPoint({ 0.f, 0.5f });
            label->setPosition({ f->width + 8.f, f->height / 2.f });
            root->addChild(label, 5);
            f->label = label;
        }

        refresh(0.f);
    }

    // ---- 내장 바 1: Gradient ----
    void buildGradient() {
        auto f = m_fields.self();
        float w = f->width, h = f->height;

        auto border = CCLayerColor::create({ 0, 0, 0, 160 }, w + 4.f, h + 4.f);
        border->setPosition({ -2.f, -2.f });
        f->root->addChild(border, 0);

        auto bg = CCLayerColor::create(
            ccc4(f->track.r, f->track.g, f->track.b, 220), w, h
        );
        f->root->addChild(bg, 1);

        auto fill = CCLayerGradient::create(
            ccc4(f->color1.r, f->color1.g, f->color1.b, 255),
            ccc4(f->color2.r, f->color2.g, f->color2.b, 255)
        );
        fill->setVector({ 1.f, 0.f }); // 좌 → 우 그라데이션
        fill->setContentSize({ 0.f, h });
        f->root->addChild(fill, 2);
        f->fill = fill;
    }

    // ---- 내장 바 2: Modern ----
    void buildModern() {
        auto f = m_fields.self();
        auto draw = CCDrawNode::create();
        f->root->addChild(draw, 1);
        f->draw = draw;
    }

    void refresh(float pct) {
        auto f = m_fields.self();
        float w = f->width, h = f->height;
        float fillW = w * pct / 100.f;

        if (f->fill) {
            f->fill->setContentSize({ fillW, h });
        }

        if (f->draw) {
            f->draw->clear();
            studio::drawPill(f->draw, w, h, studio::toF(f->track, 0.85f)); // 트랙
            if (pct > 0.f) {
                float fw = std::max(fillW, h);
                studio::drawPill(f->draw, fw, h, studio::toF(f->color1));  // 채움
                float x = fw - h / 2.f;                                    // 끝부분 글로우
                f->draw->drawDot({ x, h / 2.f }, h * 1.1f, studio::toF(f->color2, 0.30f));
                f->draw->drawDot({ x, h / 2.f }, h * 0.55f, { 1.f, 1.f, 1.f, 1.f });
            }
        }

        if (f->label) {
            f->label->setString(fmt::format("{}%", static_cast<int>(pct)).c_str());
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto f = m_fields.self();
        if (!f->root) return;

        // 원본 바/퍼센트 라벨은 계속 숨김 (게임이 다시 켜는 경우 대비)
        if (auto bar = this->getChildByID("progress-bar")) bar->setVisible(false);
        if (auto lbl = this->getChildByID("percentage-label")) lbl->setVisible(false);

        float pct = std::clamp(static_cast<float>(this->getCurrentPercent()), 0.f, 100.f);
        if (std::abs(pct - f->last) < 0.01f) return;
        f->last = pct;
        refresh(pct);
    }
};
