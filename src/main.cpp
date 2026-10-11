#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "Common.hpp"
#include "StudioPopup.hpp"

using namespace geode::prelude;

// ---------------------------------------------------------------
// 메인 메뉴 하단에 Studio 열기 버튼 추가 (전용 이미지 사용)
// ---------------------------------------------------------------
class $modify(StudioMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) {
            log::warn("bottom-menu 노드를 찾지 못해 Studio 버튼을 추가하지 못했습니다");
            return true;
        }

        CCNode* icon = nullptr;
        if (auto spr = CCSprite::create("studio-button.png"_spr)) {
            // 이미지 크기에 상관없이 한 변이 약 50이 되도록 맞춤
            auto size = spr->getContentSize();
            float m = std::max(size.width, size.height);
            if (m > 0.f) spr->setScale(50.f / m);
            icon = spr;
        } else {
            // 이미지를 못 불러오면 글자 버튼으로 대체
            auto label = CCLabelBMFont::create("Bar", "bigFont.fnt");
            label->setScale(0.8f);
            icon = CircleButtonSprite::create(label);
        }

        auto btn = CCMenuItemSpriteExtra::create(
            icon, this, menu_selector(StudioMenuLayer::onStudio)
        );
        btn->setID("studio-button"_spr);
        menu->addChild(btn);
        menu->updateLayout();
        return true;
    }

    void onStudio(CCObject*) {
        StudioPopup::create()->show();
    }
};

// ---------------------------------------------------------------
// 인게임 프로그래스 바 (기본형 하나)
// ---------------------------------------------------------------
class $modify(BarPlayLayer, PlayLayer) {
    struct Fields {
        bool showPercent = true;
        float width = 240.f;
        float height = 8.f;
        float last = -1.f;
        ccColor3B barColor = { 255, 255, 255 };
        ccColor3B trackColor = { 66, 72, 88 };

        CCNode* origBar = nullptr;     // 게임 기본 바
        CCNode* origLabel = nullptr;   // 게임 기본 퍼센트 라벨
        CCNode* root = nullptr;
        CCDrawNode* draw = nullptr;
        CCLabelBMFont* label = nullptr;
    };

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        auto mod = Mod::get();
        if (!mod->getSettingValue<bool>("enabled")) return;

        auto f = m_fields.self();

        int sel = studio::selected();
        f->barColor = studio::barColor(sel);
        f->trackColor = studio::trackColor(sel);
        f->width = static_cast<float>(mod->getSettingValue<double>("bar-width"));
        f->height = static_cast<float>(mod->getSettingValue<double>("bar-height"));
        f->showPercent = mod->getSettingValue<bool>("show-percent");

        // 자식 깊이와 상관없이 기본 바/라벨을 찾는다
        f->origBar = this->getChildByIDRecursive("progress-bar");
        f->origLabel = this->getChildByIDRecursive("percentage-label");

        CCNode* parent = this;
        CCPoint pos;
        int z = 100;

        if (f->origBar && f->origBar->getParent()) {
            // 기본 바와 같은 부모, 같은 위치
            parent = f->origBar->getParent();
            pos = f->origBar->getPosition();
            z = f->origBar->getZOrder();
        } else {
            // 못 찾으면 화면 상단 중앙에 직접 표시
            log::warn("progress-bar 노드를 찾지 못해 화면 상단 중앙에 표시합니다");
            auto ws = CCDirector::get()->getWinSize();
            pos = ccp(ws.width / 2.f, ws.height - 14.f);
        }

        auto root = CCNode::create();
        root->setID("custom-bar-root"_spr);
        root->setContentSize({ f->width, f->height });
        root->setAnchorPoint({ 0.5f, 0.5f });
        root->setPosition(pos);
        parent->addChild(root, z);
        f->root = root;

        f->draw = CCDrawNode::create();
        root->addChild(f->draw, 1);

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

    void refresh(float pct) {
        auto f = m_fields.self();
        float w = f->width, h = f->height;

        f->draw->clear();
        // 배경(트랙)
        studio::drawPill(f->draw, w, h, studio::toF(f->trackColor));
        // 채워진 부분
        if (pct > 0.f) {
            studio::drawPill(f->draw, std::max(w * pct / 100.f, h), h, studio::toF(f->barColor));
        }

        if (f->label) {
            f->label->setString(fmt::format("{}%", static_cast<int>(pct)).c_str());
        }
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto f = m_fields.self();
        if (!f->root) return;

        // 기본 바는 보이지 않게만 만들고(scale), 게임의 visible 설정은 그대로 읽어서
        // "Show Progress Bar" 옵션을 따라간다.
        if (f->origBar) {
            f->root->setVisible(f->origBar->isVisible());
            if (f->origBar->getScale() > 0.001f) f->origBar->setScale(0.0001f);
        }
        if (f->origLabel && f->origLabel->getScale() > 0.001f) {
            f->origLabel->setScale(0.0001f);
        }

        float pct = std::clamp(static_cast<float>(this->getCurrentPercent()), 0.f, 100.f);
        if (std::abs(pct - f->last) < 0.01f) return;
        f->last = pct;
        refresh(pct);
    }
};
