#pragma once
#include <Geode/ui/Popup.hpp>
#include "Common.hpp"
#include "PaintPicker.hpp"

// Progress Bar Studio
//  - 맨 위: 게임 기본 바 ↔ 커스텀 바 교체 스위치
//  - 아래: 바 미리보기 + BG(배경) / BAR(바) 색 칸 (누르면 드래그 색 선택 창)
class StudioPopup : public geode::Popup {
protected:
    CCDrawNode* m_preview = nullptr;
    CCLayerColor* m_trackSwatch = nullptr;
    CCLayerColor* m_barSwatch = nullptr;

    static constexpr float BAR_W = 200.f;
    static constexpr float BAR_H = 8.f;

    // 팝업 좌하단 기준 좌표 → 팝업 중앙 기준 오프셋
    CCPoint ctr(float x, float y) const {
        return { x - m_size.width / 2.f, y - m_size.height / 2.f };
    }

    bool init() {
        if (!Popup::init(360.f, 190.f)) return false;

        this->setTitle("Progress Bar Studio");

        const float W = m_size.width;

        // 카드 배경 (단색 레이어)
        auto card = CCLayerColor::create(ccc4(38, 43, 58, 255), W - 24.f, 124.f);
        m_mainLayer->addChildAtPosition(card, Anchor::Center, ctr(12.f, 18.f), false);

        // --- 1줄: 커스텀 바 사용 스위치 (끄면 게임 기본 바) ---
        auto label = CCLabelBMFont::create("Use Custom Bar", "bigFont.fnt");
        label->setScale(0.5f);
        label->setAnchorPoint({ 0.f, 0.5f });
        m_mainLayer->addChildAtPosition(label, Anchor::Center, ctr(24.f, 112.f), false);

        auto toggler = CCMenuItemToggler::createWithStandardSprites(
            this, menu_selector(StudioPopup::onToggle), 0.8f
        );
        toggler->toggle(Mod::get()->getSettingValue<bool>("enabled"));
        m_buttonMenu->addChildAtPosition(toggler, Anchor::Center, ctr(W - 48.f, 112.f), false);

        // --- 2줄: 바 미리보기 + 색 칸 ---
        m_preview = CCDrawNode::create();
        m_mainLayer->addChildAtPosition(m_preview, Anchor::Center, ctr(24.f, 56.f), false);

        addSwatch(m_trackSwatch, "BG",  W - 112.f, W - 88.f, 60.f, 0);
        addSwatch(m_barSwatch,   "BAR", W - 62.f,  W - 36.f, 60.f, 1);

        // 안내 문구
        auto hint = CCLabelBMFont::create("Applies when you enter a level", "bigFont.fnt");
        hint->setScale(0.28f);
        hint->setColor({ 150, 160, 180 });
        m_mainLayer->addChildAtPosition(hint, Anchor::Center, ctr(W / 2.f, 32.f), false);

        refresh();
        return true;
    }

    // 흰 테두리 + 안쪽 단색: 어떤 색이든 눈에 잘 보이는 색 칸
    void addSwatch(CCLayerColor*& inner, char const* text, float labelX, float swatchX, float y, int tag) {
        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setScale(0.28f);
        m_mainLayer->addChildAtPosition(label, Anchor::Center, ctr(labelX, y), false);

        auto holder = CCNode::create();
        holder->setContentSize({ 30.f, 22.f });
        holder->setAnchorPoint({ 0.5f, 0.5f });

        auto border = CCLayerColor::create(ccc4(255, 255, 255, 255), 30.f, 22.f);
        holder->addChild(border, 0);

        inner = CCLayerColor::create(ccc4(0, 0, 0, 255), 26.f, 18.f);
        inner->setPosition({ 2.f, 2.f });
        holder->addChild(inner, 1);

        auto btn = CCMenuItemSpriteExtra::create(holder, this, menu_selector(StudioPopup::onSwatch));
        btn->setTag(tag);
        m_buttonMenu->addChildAtPosition(btn, Anchor::Center, ctr(swatchX, y), false);
    }

    void refresh() {
        auto track = studio::trackColor();
        auto bar = studio::barColor();

        m_trackSwatch->setColor(track);
        m_barSwatch->setColor(bar);

        m_preview->clear();
        studio::drawPill(m_preview, BAR_W, BAR_H, studio::toF(track));
        studio::drawPillFill(m_preview, BAR_W, BAR_H, BAR_W * 0.68f, studio::toF(bar));
    }

    void onToggle(CCObject* sender) {
        // 토글러 콜백에서는 isToggled()가 "바뀌기 전" 상태를 돌려준다
        auto t = static_cast<CCMenuItemToggler*>(sender);
        Mod::get()->setSettingValue<bool>("enabled", !t->isToggled());
    }

    void onSwatch(CCObject* sender) {
        int kind = static_cast<CCNode*>(sender)->getTag(); // 0 = 배경(트랙), 1 = 바
        auto cur = kind == 0 ? studio::trackColor() : studio::barColor();

        // 드래그하는 동안 실시간으로 저장 + 미리보기 갱신
        PaintPickerPopup::create(cur, [this, kind](ccColor3B c) {
            if (kind == 0) studio::setTrackColor(c);
            else studio::setBarColor(c);
            this->refresh();
        })->show();
    }

public:
    static StudioPopup* create() {
        auto ret = new StudioPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
