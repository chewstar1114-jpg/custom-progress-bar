#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace geode::prelude;

namespace studio {
    // ---------------------------------------------------------------
    // 저장되는 색 (프리셋은 Classic 하나)
    // ---------------------------------------------------------------
    inline ccColor3B defaultBar()   { return { 255, 255, 255 }; }
    inline ccColor3B defaultTrack() { return { 66, 72, 88 }; }

    inline ccColor3B loadColor(char const* key, ccColor3B def) {
        auto hex = Mod::get()->getSavedValue<std::string>(key, "");
        if (hex.empty()) return def;
        return cc3bFromHexString(hex).unwrapOr(def);
    }
    inline void saveColor(char const* key, ccColor3B c) {
        Mod::get()->setSavedValue<std::string>(key, cc3bToHexString(c));
    }

    // (예전 버전에서 저장한 Classic 색을 그대로 이어서 쓰도록 키 이름 유지)
    inline ccColor3B barColor()   { return loadColor("p0-bar", defaultBar()); }
    inline ccColor3B trackColor() { return loadColor("p0-track", defaultTrack()); }
    inline void setBarColor(ccColor3B c)   { saveColor("p0-bar", c); }
    inline void setTrackColor(ccColor3B c) { saveColor("p0-track", c); }

    // ---------------------------------------------------------------
    // HSV (h: 0~360, s/v: 0~1)
    // ---------------------------------------------------------------
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

    // ---------------------------------------------------------------
    // 알약(양끝이 둥근 막대) 그리기
    // ---------------------------------------------------------------
    inline std::vector<CCPoint> pillPoints(float w, float h) {
        float r = std::min(h / 2.f, w / 2.f);
        constexpr int seg = 12;
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
        return pts;
    }

    // 세로선 x = xmax 의 왼쪽만 남기도록 도형을 자른다 (볼록 도형용)
    inline std::vector<CCPoint> clipRight(std::vector<CCPoint> const& pts, float xmax) {
        std::vector<CCPoint> out;
        size_t n = pts.size();
        for (size_t i = 0; i < n; i++) {
            auto cur = pts[i];
            auto nxt = pts[(i + 1) % n];
            bool cin = cur.x <= xmax;
            bool nin = nxt.x <= xmax;
            if (cin) out.push_back(cur);
            if (cin != nin) {
                float t = (xmax - cur.x) / (nxt.x - cur.x);
                out.push_back({ xmax, cur.y + t * (nxt.y - cur.y) });
            }
        }
        return out;
    }

    // 겹치는(거의 같은) 점 제거: CCDrawNode는 중복 점에서 도형이 깨진다
    inline void dedupe(std::vector<CCPoint>& p) {
        std::vector<CCPoint> out;
        for (auto const& q : p) {
            if (out.empty() || std::hypot(q.x - out.back().x, q.y - out.back().y) > 0.01f) {
                out.push_back(q);
            }
        }
        while (out.size() > 1 &&
               std::hypot(out.front().x - out.back().x, out.front().y - out.back().y) <= 0.01f) {
            out.pop_back();
        }
        p = std::move(out);
    }

    inline void drawPoly(CCDrawNode* n, std::vector<CCPoint> pts, ccColor4F col) {
        dedupe(pts);
        if (pts.size() < 3) return;
        n->drawPolygon(pts.data(), static_cast<unsigned int>(pts.size()), col, 0.f, col);
    }

    // (0,0) ~ (w,h) 알약
    inline void drawPill(CCDrawNode* n, float w, float h, ccColor4F col) {
        if (w <= 0.f || h <= 0.f) return;
        drawPoly(n, pillPoints(w, h), col);
    }

    // 알약 모양 안에서 왼쪽 0 ~ fillW 구간만 채운다 (진행률 표시용)
    // 0%에 가까워도 트랙의 둥근 끝 모양 그대로 잘려서 튀어나오지 않는다
    inline void drawPillFill(CCDrawNode* n, float w, float h, float fillW, ccColor4F col) {
        if (w <= 0.f || h <= 0.f) return;
        fillW = std::clamp(fillW, 0.f, w);
        if (fillW < 0.5f) return;
        auto pts = pillPoints(w, h);
        if (fillW < w) pts = clipRight(pts, fillW);
        drawPoly(n, std::move(pts), col);
    }
}
