#include "SvgUtils.h"
#include <QtGlobal>
#include <QStringList>

namespace utils {

static inline QPointF cubicPoint(const QPointF& p0, const QPointF& p1,
                                 const QPointF& p2, const QPointF& p3, double t)
{
    const double it = 1.0 - t;
    const double a = it * it * it;
    const double b = 3.0 * it * it * t;
    const double c = 3.0 * it * t * t;
    const double d = t * t * t;
    return QPointF(a*p0.x() + b*p1.x() + c*p2.x() + d*p3.x(),
                   a*p0.y() + b*p1.y() + c*p2.y() + d*p3.y());
}

static inline QPointF quadPoint(const QPointF& p0, const QPointF& p1,
                                const QPointF& p2, double t)
{
    const double it = 1.0 - t;
    const double a = it * it;
    const double b = 2.0 * it * t;
    const double c = t * t;
    return QPointF(a*p0.x() + b*p1.x() + c*p2.x(),
                   a*p0.y() + b*p1.y() + c*p2.y());
}

static inline bool isCommand(QChar ch)
{
    const ushort u = ch.unicode();
    return (u=='M'||u=='m'||u=='L'||u=='l'||u=='H'||u=='h'||u=='V'||u=='v'||
            u=='C'||u=='c'||u=='S'||u=='s'||u=='Q'||u=='q'||u=='T'||u=='t'||
            u=='A'||u=='a'||u=='Z'||u=='z');
}

static inline void skipSep(const QString& s, int& i)
{
    while (i < s.size()) {
        const QChar c = s[i];
        if (c.isSpace() || c==',') ++i; else break;
    }
}

static bool readNumber(const QString& s, int& i, double& out)
{
    skipSep(s, i);
    if (i >= s.size()) return false;

    int start = i;
    bool sawDigit = false;

    if (s[i] == '+' || s[i] == '-') ++i;

    while (i < s.size() && s[i].isDigit()) { ++i; sawDigit = true; }

    if (i < s.size() && s[i] == '.') {
        ++i;
        while (i < s.size() && s[i].isDigit()) { ++i; sawDigit = true; }
    }

    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        int epos = i++;
        if (i < s.size() && (s[i] == '+' || s[i] == '-')) ++i;
        bool hasExpDigit = false;
        int j = i;
        while (j < s.size() && s[j].isDigit()) { ++j; hasExpDigit = true; }
        if (!hasExpDigit) {
            i = epos;
        } else {
            i = j;
        }
    }

    if (!sawDigit) return false;
    out = s.mid(start, i - start).toDouble();
    skipSep(s, i);
    return true;
}

QVector<QPointF> parseSvgPathToPolyline(const QString& d, int curveSegments)
{
    QVector<QPointF> pts;
    pts.reserve(16);

    QPointF cur(0,0), start(0,0);
    QPointF prevCtlQ(0,0), prevCtlC(0,0);
    bool hasPrevQ = false, hasPrevC = false;

    int i = 0;
    QChar cmd = QChar(' ');
    QString s = d;

    while (i < s.size()) {
        skipSep(s, i);
        if (i >= s.size()) break;

        QChar ch = s[i];
        if (isCommand(ch)) {
            cmd = ch;
            ++i;
            skipSep(s, i);
        } else if (cmd.isNull() || cmd == QChar(' ')) {
            cmd = QChar('L');
        }

        const bool rel =
            (cmd=='m'||cmd=='l'||cmd=='h'||cmd=='v'||
             cmd=='c'||cmd=='s'||cmd=='q'||cmd=='t'||cmd=='a');

        switch (cmd.unicode()) {
        case 'M': case 'm': {
            double x, y;
            if (!readNumber(s, i, x) || !readNumber(s, i, y)) break;
            QPointF p = rel ? QPointF(cur.x()+x, cur.y()+y) : QPointF(x,y);
            cur = p; start = p;
            pts.append(p);
            while (true) {
                int save = i;
                if (!readNumber(s, i, x) || !readNumber(s, i, y)) { i = save; break; }
                QPointF p2 = rel ? QPointF(cur.x()+x, cur.y()+y) : QPointF(x,y);
                cur = p2; pts.append(p2);
            }
            hasPrevQ = hasPrevC = false;
            break;
        }
        case 'L': case 'l': {
            while (true) {
                double x, y; int save = i;
                if (!readNumber(s, i, x) || !readNumber(s, i, y)) { i = save; break; }
                QPointF p = rel ? QPointF(cur.x()+x, cur.y()+y) : QPointF(x,y);
                cur = p; pts.append(p);
            }
            hasPrevQ = hasPrevC = false;
            break;
        }
        case 'H': case 'h': {
            while (true) {
                double v; int save = i;
                if (!readNumber(s, i, v)) { i = save; break; }
                QPointF p = rel ? QPointF(cur.x()+v, cur.y()) : QPointF(v, cur.y());
                cur = p; pts.append(p);
            }
            hasPrevQ = hasPrevC = false;
            break;
        }
        case 'V': case 'v': {
            while (true) {
                double v; int save = i;
                if (!readNumber(s, i, v)) { i = save; break; }
                QPointF p = rel ? QPointF(cur.x(), cur.y()+v) : QPointF(cur.x(), v);
                cur = p; pts.append(p);
            }
            hasPrevQ = hasPrevC = false;
            break;
        }
        case 'Z': case 'z': {
            if (pts.isEmpty() || (pts.last() != start)) pts.append(start);
            cur = start; hasPrevQ = hasPrevC = false;
            break;
        }
        case 'Q': case 'q': {
            while (true) {
                double cx, cy, x, y; int save = i;
                if (!readNumber(s, i, cx) || !readNumber(s, i, cy) ||
                    !readNumber(s, i, x)  || !readNumber(s, i, y)) { i = save; break; }
                QPointF c1 = rel ? QPointF(cur.x()+cx, cur.y()+cy) : QPointF(cx,cy);
                QPointF p2 = rel ? QPointF(cur.x()+x , cur.y()+y ) : QPointF(x ,y );
                for (int k=1; k<=curveSegments; ++k) {
                    double t = double(k)/double(curveSegments);
                    pts.append(quadPoint(cur, c1, p2, t));
                }
                prevCtlQ = c1; hasPrevQ = true;
                cur = p2; hasPrevC = false;
            }
            break;
        }
        case 'C': case 'c': {
            while (true) {
                double cx1, cy1, cx2, cy2, x, y; int save = i;
                if (!readNumber(s, i, cx1) || !readNumber(s, i, cy1) ||
                    !readNumber(s, i, cx2) || !readNumber(s, i, cy2) ||
                    !readNumber(s, i, x)   || !readNumber(s, i, y)) { i = save; break; }
                QPointF c1 = rel ? QPointF(cur.x()+cx1, cur.y()+cy1) : QPointF(cx1,cy1);
                QPointF c2 = rel ? QPointF(cur.x()+cx2, cur.y()+cy2) : QPointF(cx2,cy2);
                QPointF p2 = rel ? QPointF(cur.x()+x  , cur.y()+y  ) : QPointF(x  ,y  );
                for (int k=1; k<=curveSegments; ++k) {
                    double t = double(k)/double(curveSegments);
                    pts.append(cubicPoint(cur, c1, c2, p2, t));
                }
                prevCtlC = c2; hasPrevC = true;
                cur = p2; hasPrevQ = false;
            }
            break;
        }
        default: {
            double dummy; if (!readNumber(s, i, dummy)) ++i;
            hasPrevQ = hasPrevC = false;
            break;
        }
        }
    }

    pts.squeeze();
    return pts;
}

} // namespace utils
