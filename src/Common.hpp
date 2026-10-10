#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

using namespace geode::prelude;

namespace studio {
    struct Preset {
        const char* name;
        ccColor3B bar;
        ccColor3B track;
    };

    // 기본 프리셋 (이름은 고정, 색은 유저가 덮어쓸 수 있음)
    inline const std::array<Preset, 3>& defaults() {
        static const std::array<Preset, 3> d = {{
            { "Classic",   { 255, 255, 255 }, { 66, 72, 88 } },
            { "Neon Blue", { 56, 189, 248 },  { 66, 72, 88 } },
            { "Crimson",   { 255, 77, 106 },  { 66, 72, 88 } },
        }};
        return d;
    }

    inline ccColor3B loadColor(int i, char const* kind, ccColor3B def) {
        auto hex = Mod::get()->getSavedValue<std::string>(fmt::format("p{}-{}", i, kind), "");
        if (hex.empty()) return def;
        return cc3bFromHexString(hex).unwrapOr(def);
    }
    inline void saveColor(int i, char const* kind, ccColor3B c) {
        Mod::get()->setSavedValue<std::string>(fmt::format("p{}-{}", i, kind), cc3bToHexString(c));
    }

    inline ccColor3B barColor(int i)   { return loadColor(i, "bar",   defaults()[i].bar); }
    inline ccColor3B trackColor(int i) { return loadColor(i, "track", defaults()[i].track); }
    inline void setBarColor(int i, ccColor3B c)   { saveColor(i, "bar", c); }
    inline void setTrackColor(int i, ccColor3B c) { saveColor(i, "track", c); }

    inline int selected() {
        return std::clamp(Mod::get()->getSavedValue<int>("selected", 0), 0, 2);
    }
    inline void select(int i) {
        Mod::get()->setSavedValue<int>("selected", std::clamp(i, 0, 2));
    }

    // ---- HSV (h: 0~360, s/v: 0~1) ----
    struct HSV { float h, s, v; };

    inline ccColor3B hsv2rgb(float h, float s, float v) {
        h = std::fmod(h, 360.f);
        if (h < 0.f) h += 360.f;
        float c = v * s;
        float x = c * (1.f - std::abs(std::fmod(h / 60.f, 2.f) - 1.f));
        float m = v - c;
        float r = 0, g = 0, b = 0;
        switch (static_cast<int>(h / 60.f)) {
            case 0: r = c; g = x; break;
            case 1: r = x; g = c; break;
            case 2: g = c; b = x; break;
            case 3: g = x; b = c; break;
            case 4: r = x; b = c; break;
            default: r = c; b = x; break;
        }
        auto to8 = [](float f) { return static_cast<GLubyte>(std::clamp(std::round(f * 255.f), 0.f, 255.f)); };
        return { to8(r + m), to8(g + m), to8(b + m) };
    }

    inline HSV rgb2hsv(ccColor3B c) {
        float r = c.r / 255.f, g = c.g / 255.f, b = c.b / 255.f;
        float mx = std::max({ r, g, b }), mn = std::min({ r, g, b });
        float d = mx - mn;
        float h = 0.f;
        if (d > 0.f) {
            if (mx == r) h = 60.f * std::fmod((g - b) / d, 6.f);
            else if (mx == g) h = 60.f * ((b - r) / d + 2.f);
            else h = 60.f * ((r - g) / d + 4.f);
            if (h < 0.f) h += 360.f;
        }
        return { h, mx > 0.f ? d / mx : 0.f, mx };
    }

    inline ccColor4F toF(ccColor3B c, float a = 1.f) {
        return { c.r / 255.f, c.g / 255.f, c.b / 255.f, a };
    }

    // (0,0) ~ (w,h) 영역에 알약(양끝이 둥근 막대)을 채운다
    inline void drawPill(CCDrawNode* n, float w, float h, ccColor4F col) {
        if (w <= 0.f) return;
        float r = std::min(h / 2.f, w / 2.f);
        constexpr int seg = 10;
        constexpr float PI = 3.14159265f;

        std::vector<CCPoint> pts;
        pts.reserve((seg + 1) * 2);
        for (int i = 0; i <= seg; i++) {           // 오른쪽 캡
            float a = -PI / 2 + PI * i / seg;
            pts.push_back({ w - r + std::cos(a) * r, h / 2 + std::sin(a) * r });
        }
        for (int i = 0; i <= seg; i++) {           // 왼쪽 캡
            float a = PI / 2 + PI * i / seg;
            pts.push_back({ r + std::cos(a) * r, h / 2 + std::sin(a) * r });
        }
        n->drawPolygon(pts.data(), pts.size(), col, 0.f, col);
    }
}
