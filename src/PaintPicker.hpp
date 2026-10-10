#pragma once
#include <Geode/ui/Popup.hpp>
#include <functional>
#include "Common.hpp"

// 그림판처럼 드래그해서 색을 고르는 팝업
//  - 왼쪽 큰 사각형: 가로 = 채도, 세로 = 밝기 (드래그)
//  - 가운데 세로 막대: 색상(Hue) (드래그)
//  - 오른쪽: 새 색 / 원래 색 미리보기 + HEX 값
//  색을 움직이는 동안 콜백이 계속 호출되어 실시간으로 반영됩니다.
class PaintPickerPopup : public geode::Popup<ccColor3B, std::function<void(ccColor3B)>> {
protected:
    using Base = geode::Popup<ccColor3B, std::function<void(ccColor3B)>>;

    static constexpr float PAD = 160.f;     // 사각형 한 변
    static constexpr float PAD_X = 20.f;
    static constexpr float PAD_Y = 56.f;
    static constexpr float HUE_X = 194.f;
    static constexpr float HUE_W = 22.f;

    CCNode* m_root = nullptr;
    CCLayerColor* m_hueBase = nullptr;
    CCDrawNode* m_cursor = nullptr;
    CCLayerColor* m_newSwatch = nullptr;
    CCLabelBMFont* m_hexLabel = nullptr;

    std::function<void(ccColor3B)> m_callback;
    ccColor3B m_original = { 255, 255, 255 };
    float m_h = 0.f, m_s = 1.f, m_v = 1.f;
    int m_drag = 0; // 0 = 없음, 1 = 사각형, 2 = 색상 막대

    bool setup(ccColor3B color, std::function<void(ccColor3B)> cb) override {
        m_original = color;
        m_callback = std::move(cb);
        auto hsv = studio::rgb2hsv(color);
        m_h = hsv.h; m_s = hsv.s; m_v = hsv.v;

        this->setTitle("Pick Color");

        const float W = m_size.width;
        const float H = m_size.height;

        // 좌표계를 (0,0)~(W,H)로 쓰기 위한 루트 노드
        m_root = CCNode::create();
        m_root->setContentSize({ W, H });
        m_root->setAnchorPoint({ 0.5f, 0.5f });
        m_mainLayer->addChildAtPosition(m_root, Anchor::Center, ccp(0, 0), false);

        // --- 채도/밝기 사각형: 순색 + 흰색 그라데이션(→) + 검정 그라데이션(↓) ---
        m_hueBase = CCLayerColor::create(ccc4(255, 0, 0, 255), PAD, PAD);
        m_hueBase->setPosition({ PAD_X, PAD_Y });
        m_root->addChild(m_hueBase, 1);

        auto white = CCLayerGradient::create(ccc4(255, 255, 255, 255), ccc4(255, 255, 255, 0));
        white->setVector({ 1.f, 0.f });
        white->setContentSize({ PAD, PAD });
        white->setPosition({ PAD_X, PAD_Y });
        m_root->addChild(white, 2);

        auto black = CCLayerGradient::create(ccc4(0, 0, 0, 0), ccc4(0, 0, 0, 255));
        black->setContentSize({ PAD, PAD });
        black->setPosition({ PAD_X, PAD_Y });
        m_root->addChild(black, 3);

        // --- 색상 막대: 6구간 그라데이션 (위 = 0°, 아래 = 360°) ---
        const float seg = PAD / 6.f;
        for (int i = 0; i < 6; i++) {
            auto a = studio::hsv2rgb(60.f * i, 1.f, 1.f);
            auto b = studio::hsv2rgb(60.f * (i + 1), 1.f, 1.f);
            auto g = CCLayerGradient::create(ccc4(a.r, a.g, a.b, 255), ccc4(b.r, b.g, b.b, 255));
            g->setContentSize({ HUE_W, seg });
            g->setPosition({ HUE_X, PAD_Y + PAD - (i + 1) * seg });
            m_root->addChild(g, 1);
        }

        // --- 미리보기: New / Old ---
        addLabel("NEW", 266.f, 210.f);
        m_newSwatch = CCLayerColor::create(ccc4(color.r, color.g, color.b, 255), 68.f, 44.f);
        m_newSwatch->setPosition({ 232.f, 160.f });
        m_root->addChild(m_newSwatch, 1);

        addLabel("OLD", 266.f, 154.f);
        auto oldSwatch = CCLayerColor::create(ccc4(color.r, color.g, color.b, 255), 68.f, 44.f);
        oldSwatch->setPosition({ 232.f, 104.f });
        m_root->addChild(oldSwatch, 1);

        m_hexLabel = CCLabelBMFont::create("#FFFFFF", "bigFont.fnt");
        m_hexLabel->setScale(0.4f);
        m_hexLabel->setPosition({ 266.f, 84.f });
        m_root->addChild(m_hexLabel, 1);

        // 커서
        m_cursor = CCDrawNode::create();
        m_root->addChild(m_cursor, 10);

        // --- 버튼 ---
        addButton("Cancel", W / 2.f - 55.f, 28.f, menu_selector(PaintPickerPopup::onCancel));
        addButton("Done",   W / 2.f + 55.f, 28.f, menu_selector(PaintPickerPopup::onDone));

        apply(false);
        return true;
    }

    void addLabel(char const* text, float x, float y) {
        auto l = CCLabelBMFont::create(text, "bigFont.fnt");
        l->setScale(0.3f);
        l->setPosition({ x, y });
        m_root->addChild(l, 1);
    }

    void addButton(char const* text, float x, float y, SEL_MenuHandler cb) {
        auto spr = ButtonSprite::create(text);
        spr->setScale(0.7f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, cb);
        m_buttonMenu->addChildAtPosition(
            btn, Anchor::Center,
            ccp(x - m_size.width / 2.f, y - m_size.height / 2.f), false
        );
    }

    // 현재 HSV → 화면/미리보기/콜백 갱신
    void apply(bool notify = true) {
        auto rgb = studio::hsv2rgb(m_h, m_s, m_v);

        m_hueBase->setColor(studio::hsv2rgb(m_h, 1.f, 1.f));
        m_newSwatch->setColor(rgb);
        m_hexLabel->setString(fmt::format("#{:02X}{:02X}{:02X}", rgb.r, rgb.g, rgb.b).c_str());

        m_cursor->clear();
        // 사각형 커서
        float px = PAD_X + m_s * PAD;
        float py = PAD_Y + m_v * PAD;
        m_cursor->drawDot({ px, py }, 7.f, { 0.f, 0.f, 0.f, 1.f });
        m_cursor->drawDot({ px, py }, 5.5f, { 1.f, 1.f, 1.f, 1.f });
        m_cursor->drawDot({ px, py }, 3.5f, studio::toF(rgb));
        // 색상 막대 커서
        float hy = PAD_Y + PAD - (m_h / 360.f) * PAD;
        m_cursor->drawSegment({ HUE_X - 3.f, hy }, { HUE_X + HUE_W + 3.f, hy }, 3.f, { 0.f, 0.f, 0.f, 1.f });
        m_cursor->drawSegment({ HUE_X - 3.f, hy }, { HUE_X + HUE_W + 3.f, hy }, 1.8f, { 1.f, 1.f, 1.f, 1.f });

        if (notify && m_callback) m_callback(rgb);
    }

    // ---- 터치(드래그) 처리 ----
    CCPoint localPos(CCTouch* t) {
        return m_root->convertToNodeSpace(t->getLocation());
    }

    void dragTo(CCPoint p) {
        if (m_drag == 1) {
            m_s = std::clamp((p.x - PAD_X) / PAD, 0.f, 1.f);
            m_v = std::clamp((p.y - PAD_Y) / PAD, 0.f, 1.f);
        } else if (m_drag == 2) {
            m_h = std::clamp((PAD_Y + PAD - p.y) / PAD, 0.f, 1.f) * 360.f;
        }
        apply();
    }

    bool ccTouchBegan(CCTouch* t, CCEvent* e) override {
        auto p = localPos(t);
        if (CCRect(PAD_X, PAD_Y, PAD, PAD).containsPoint(p)) {
            m_drag = 1; dragTo(p); return true;
        }
        if (CCRect(HUE_X - 4.f, PAD_Y, HUE_W + 8.f, PAD).containsPoint(p)) {
            m_drag = 2; dragTo(p); return true;
        }
        return Base::ccTouchBegan(t, e);
    }

    void ccTouchMoved(CCTouch* t, CCEvent* e) override {
        if (m_drag) dragTo(localPos(t));
        else Base::ccTouchMoved(t, e);
    }

    void ccTouchEnded(CCTouch* t, CCEvent* e) override {
        if (m_drag) m_drag = 0;
        else Base::ccTouchEnded(t, e);
    }

    void ccTouchCancelled(CCTouch* t, CCEvent* e) override {
        if (m_drag) m_drag = 0;
        else Base::ccTouchCancelled(t, e);
    }

    void onDone(CCObject*) {
        this->onClose(nullptr);
    }

    void onCancel(CCObject*) {
        if (m_callback) m_callback(m_original); // 원래 색으로 되돌림
        this->onClose(nullptr);
    }

public:
    static PaintPickerPopup* create(ccColor3B color, std::function<void(ccColor3B)> cb) {
        auto ret = new PaintPickerPopup();
        if (ret && ret->initAnchored(320.f, 268.f, color, std::move(cb))) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};
