#pragma once
#include <Geode/ui/Popup.hpp>
#include "Common.hpp"
#include "PaintPicker.hpp"

// 프리셋 리스트: [이름] [바 미리보기] [BG 색] [BAR 색] [Select]
// BG / BAR 스와치를 누르면 그림판 스타일 드래그 피커가 열립니다.
class StudioPopup : public geode::Popup {
protected:
    struct Row {
        CCLayerColor* bg = nullptr;
        CCLayerColor* trackSwatch = nullptr;
        CCLayerColor* barSwatch = nullptr;
        CCDrawNode* preview = nullptr;
        ButtonSprite* selectBtn = nullptr;
    };

    std::array<Row, 3> m_rows;

    static constexpr float BAR_W = 200.f;
    static constexpr float BAR_H = 8.f;

    // 팝업 좌하단 기준 좌표 → 팝업 중앙 기준 오프셋
    CCPoint ctr(float x, float y) const {
        return { x - m_size.width / 2.f, y - m_size.height / 2.f };
    }

    bool init() {
        if (!Popup::init(360.f, 262.f)) return false;

        this->setTitle("Progress Bar Studio");

        const float W = m_size.width;
        const float H = m_size.height;
        constexpr float rowH = 62.f;
        constexpr float gap = 8.f;

        for (int i = 0; i < 3; i++) {
            float cy = H - 52.f - rowH / 2.f - i * (rowH + gap);
            auto& row = m_rows[i];

            // 행 배경 (단색 레이어: 색이 그대로 보임)
            row.bg = CCLayerColor::create(ccc4(38, 43, 58, 255), W - 24.f, rowH);
            m_mainLayer->addChildAtPosition(
                row.bg, Anchor::Center, ctr(12.f, cy - rowH / 2.f), false
            );

            // 프리셋 이름 (고정)
            auto name = CCLabelBMFont::create(studio::defaults()[i].name, "bigFont.fnt");
            name->setScale(0.45f);
            name->setAnchorPoint({ 0.f, 0.5f });
            m_mainLayer->addChildAtPosition(name, Anchor::Center, ctr(24.f, cy + 18.f), false);

            // 바 미리보기
            row.preview = CCDrawNode::create();
            m_mainLayer->addChildAtPosition(row.preview, Anchor::Center, ctr(24.f, cy - 18.f), false);

            // BG / BAR 색상 스와치
            addSwatch(row.trackSwatch, "BG",  W - 112.f, W - 88.f, cy - 14.f, i * 10 + 0);
            addSwatch(row.barSwatch,   "BAR", W - 62.f,  W - 36.f, cy - 14.f, i * 10 + 1);

            // Select 버튼 (고정)
            row.selectBtn = ButtonSprite::create("Select");
            row.selectBtn->setScale(0.6f);
            auto btn = CCMenuItemSpriteExtra::create(
                row.selectBtn, this, menu_selector(StudioPopup::onSelect)
            );
            btn->setTag(i);
            m_buttonMenu->addChildAtPosition(btn, Anchor::Center, ctr(W - 60.f, cy + 14.f), false);

            refreshRow(i);
        }
        return true;
    }

    // 흰 테두리 + 안쪽 단색: 어떤 색이든 눈에 잘 보이는 스와치
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

    void refreshRow(int i) {
        auto& row = m_rows[i];
        bool sel = (i == studio::selected());
        auto track = studio::trackColor(i);
        auto bar = studio::barColor(i);

        row.bg->setColor(sel ? ccColor3B{ 52, 62, 88 } : ccColor3B{ 38, 43, 58 });
        row.trackSwatch->setColor(track);
        row.barSwatch->setColor(bar);
        row.selectBtn->setString(sel ? "Selected" : "Select");

        row.preview->clear();
        studio::drawPill(row.preview, BAR_W, BAR_H, studio::toF(track));
        studio::drawPill(row.preview, BAR_W * 0.68f, BAR_H, studio::toF(bar));
    }

    void refreshAll() {
        for (int i = 0; i < 3; i++) refreshRow(i);
    }

    void onSelect(CCObject* sender) {
        studio::select(static_cast<CCNode*>(sender)->getTag());
        refreshAll();
    }

    void onSwatch(CCObject* sender) {
        int tag = static_cast<CCNode*>(sender)->getTag();
        int idx = tag / 10;
        int kind = tag % 10; // 0 = 배경(트랙), 1 = 바

        auto cur = kind == 0 ? studio::trackColor(idx) : studio::barColor(idx);

        // 드래그하는 동안 실시간으로 저장 + 미리보기 갱신
        PaintPickerPopup::create(cur, [this, idx, kind](ccColor3B c) {
            if (kind == 0) studio::setTrackColor(idx, c);
            else studio::setBarColor(idx, c);
            this->refreshRow(idx);
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
