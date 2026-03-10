#pragma once
#include <QPainter>

namespace pst {
struct PainterStateCache {
    QPen pen;
    QBrush brush;
    QFont font;
    QTransform transform;
    QPainter::RenderHints hints;
    qreal opacity;
    bool hasClipping;
    QRegion clipRegion;
    QBrush background;
    Qt::BGMode backgroundMode;
    QPointF brushOrigin;
    QPainter::CompositionMode compMode;
    Qt::LayoutDirection layoutDir;

    void capture(QPainter& p) {
        pen = p.pen();
        brush = p.brush();
        font = p.font();
        transform = p.transform();
        hints = p.renderHints();
        opacity = p.opacity();
        hasClipping = p.hasClipping();
        if (hasClipping) clipRegion = p.clipRegion();
        background = p.background();
        backgroundMode = p.backgroundMode();
        brushOrigin = p.brushOrigin();
        compMode = p.compositionMode();
        layoutDir = p.layoutDirection();
    }

    void restore(QPainter& p) const {
        p.setPen(pen);
        p.setBrush(brush);
        p.setFont(font);
        p.setTransform(transform);
        p.setRenderHints(hints);
        p.setOpacity(opacity);
        if (hasClipping) p.setClipRegion(clipRegion);
        else p.setClipping(false);
        p.setBackground(background);
        p.setBackgroundMode(backgroundMode);
        p.setBrushOrigin(brushOrigin);
        p.setCompositionMode(compMode);
        p.setLayoutDirection(layoutDir);
    }
};

class PainterStateGuard {
public:
    explicit PainterStateGuard(QPainter* p) : ptr(p) { cache.capture(*ptr); }
    explicit PainterStateGuard(QPainter& p) : ptr(&p) { cache.capture(*ptr); }
    ~PainterStateGuard() { cache.restore(*ptr); }
private:
    PainterStateGuard(const PainterStateGuard&);
    PainterStateGuard& operator=(const PainterStateGuard&);
private:
    QPainter* ptr;
    PainterStateCache cache;
};
} // namespace pst

#define PST_CONCAT2(a,b) a##b
#define PST_CONCAT(a,b) PST_CONCAT2(a,b)

#define PAINTER_STATE_GUARD(painter_expr) ::pst::PainterStateGuard PST_CONCAT(_pst_guard_, __LINE__)(painter_expr)
