#include "SvgUtils.h"
#include <QtGlobal>
#include <QStringList>
#include <QRegExp>
#include <cmath>

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

QTransform parseTransformMatrix(const pugi::xml_node& node)
{
    const char* transformStr = node.attribute("transform").as_string();
    if (transformStr && strncmp(transformStr, "matrix(", 7) == 0) {
        QString s(transformStr + 7);
        s.chop(1); // 去掉尾部 ')'
        QStringList parts = s.split(QRegExp("[\\s,]+"), QString::SkipEmptyParts);
        if (parts.size() == 6) {
            return QTransform(
                parts[0].toDouble(), parts[1].toDouble(),
                parts[2].toDouble(), parts[3].toDouble(),
                parts[4].toDouble(), parts[5].toDouble()
            );
        }
    }
    return QTransform();
}

QVector<QPointF> parsePointsAttr(const QString& pointsStr)
{
    QVector<QPointF> pts;
    QStringList ptList = pointsStr.split(' ', QString::SkipEmptyParts);
    for (int i = 0; i < ptList.size(); ++i) {
        const QString& token = ptList[i];
        QStringList xy = token.split(',', QString::SkipEmptyParts);
        if (xy.size() == 2)
            pts.append(QPointF(xy[0].toDouble(), xy[1].toDouble()));
    }
    return pts;
}

double pointToSegmentDistance(const QPointF& pt, const QPointF& a, const QPointF& b)
{
    const double x = pt.x(), y = pt.y();
    const double x1 = a.x(), y1 = a.y();
    const double x2 = b.x(), y2 = b.y();
    const double dx = x2 - x1;
    const double dy = y2 - y1;
    if (dx == 0.0 && dy == 0.0) return std::sqrt((x - x1)*(x - x1) + (y - y1)*(y - y1));
    double t = ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy);
    if (t < 0.0) return std::sqrt((x - x1)*(x - x1) + (y - y1)*(y - y1));
    if (t > 1.0) return std::sqrt((x - x2)*(x - x2) + (y - y2)*(y - y2));
    const double projx = x1 + t * dx;
    const double projy = y1 + t * dy;
    return std::sqrt((x - projx)*(x - projx) + (y - projy)*(y - projy));
}

QColor parseColor(const char* s, double opacity)
{
    if (!s || !*s) return Qt::transparent;
    QColor c(QString::fromLatin1(s));
    if (!c.isValid()) return Qt::transparent;
    c.setAlphaF(qBound(0.0, opacity, 1.0));
    return c;
}

bool computePathBoundingRect(const pugi::xml_node& node, const QTransform& extraTransform, QRectF& outRect)
{
    // 搜集该节点下的所有 <path> 的 d 的折线点，合并成一个包围盒
    double minX = 1e100, minY = 1e100, maxX = -1e100, maxY = -1e100;
    bool hasAny = false;
    QTransform tf = extraTransform * parseTransformMatrix(node);

    if (qstrcmp(node.name(), "path") == 0) {
        const char* d = node.attribute("d").value();
        if (d && *d) {
            QVector<QPointF> local = parseSvgPathToPolyline(QString::fromLatin1(d));
            for (int i = 0; i < local.size(); ++i) {
                const QPointF& p = local[i];
                if (p.x() < minX) minX = p.x(); if (p.x() > maxX) maxX = p.x();
                if (p.y() < minY) minY = p.y(); if (p.y() > maxY) maxY = p.y();
                hasAny = true;
            }
        }
    }

    for (pugi::xml_node path = node.child("path"); path; path = path.next_sibling("path")) {
        const char* d = path.attribute("d").value();
        if (!d || !*d) continue;
        QVector<QPointF> local = parseSvgPathToPolyline(QString::fromLatin1(d));
        for (int i = 0; i < local.size(); ++i) {
            const QPointF& p = local[i];
            if (p.x() < minX) minX = p.x(); if (p.x() > maxX) maxX = p.x();
            if (p.y() < minY) minY = p.y(); if (p.y() > maxY) maxY = p.y();
            hasAny = true;
        }
    }

    if (!hasAny) return false;
    QRectF localRect(QPointF(minX, minY), QPointF(maxX, maxY));
    outRect = tf.mapRect(localRect);
    return true;
}

void simplifyPolyline(QVector<QPointF>& pts, qreal tol)
{
	if (pts.size() < 3) return;
	QVector<QPointF> out;
	out.reserve(pts.size());
	out.append(pts[0]);
	int lastKeep = 0;
	for (int i = 1; i < pts.size() - 1; ++i) {
		const QPointF& A = pts[lastKeep];
		const QPointF& B = pts[i];
		const QPointF& C = pts[i + 1];
		// 点到线段 AC 的距离
		QLineF ac(A, C);
		qreal dist;
		if (ac.length() == 0) dist = QLineF(A, B).length();
		else {
			// 投影参数 t
			QPointF ap = B - A; QPointF ab = C - A;
			qreal ab2 = ab.x() * ab.x() + ab.y() * ab.y();
			qreal t = (ap.x() * ab.x() + ap.y() * ab.y()) / (ab2 > 0 ? ab2 : 1);
			if (t < 0) dist = QLineF(B, A).length();
			else if (t > 1) dist = QLineF(B, C).length();
			else {
				QPointF proj = A + t * (C - A);
				dist = QLineF(B, proj).length();
			}
		}
		if (dist > tol) {
			out.append(B);
			lastKeep = i;
		}
		else {
			// 丢弃 B
		}
	}
	out.append(pts.last());
	out.squeeze();
	pts.swap(out);
}

} // namespace utils
