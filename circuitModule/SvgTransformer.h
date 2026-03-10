#pragma once
#include "circuitconfig.h"
#include "svgmodel.h"
#include "SvgUtils.h"
#include <qmath.h>
#include <QPainter>
#include <QSvgGenerator>
#include <QTextStream>
#include <QPaintDevice>
#include <QImage>
#include <QBuffer>

#include "Common/SvgTransformerCommon.h"

namespace utils {
	inline QString toQString(const QString& str) { return str; }
	inline QString toQString(const QStringRef& str) { return str.toString(); }
	inline QString toQString(const QChar& c) { return QString(c); }
	inline QString toQString(quint64 value) { return QString::number(value); }
	inline QString toQString(int value) { return QString::number(value); }
	inline QString toQString(double value) { return QString::number(value); }
	inline QString toQString(bool value) { return value ? "true" : "false"; }
	inline QString toQString(const char* s) { return s ? QString::fromUtf8(s) : QString(); }
	// 信息组合/拆分辅助类
	class FieldJoiner {
	public:
		Q_DISABLE_COPY(FieldJoiner)
		FieldJoiner(const QChar& sep = FIELD_SEPARATE_CHAR)
			: separator(sep), first(true), stream(&buffer) {
		}

		template<typename T>
		FieldJoiner& operator<<(const T& value) {
			if (!first)
				stream << separator;

			QString s = toQString(value);
			if (s.isEmpty())
				stream << "NULL";
			else
				stream << s;
			first = false;
			return *this;
		}

		operator QString() const { return buffer; }
	private:
		QString buffer;
		QTextStream stream;
		QChar separator;
		bool first;
	};
} // namespace utils
class LogicSvg;
class OpticalSvg;
class VirtualSvg;
class WholeCircuitSvg;
class IedRect;
class SwitcherRect;
class SvgRect;
class OpticalCircuitLine_old;
class OpticalCircuitLine;
class CircuitLine;
class PlateRect;
class SvgTransformer
{
private:
	typedef pugi::xpath_node_set::const_iterator nodeSetConstIterator;
	// 组合为子串信息
	inline utils::FieldJoiner joinFields() { return utils::FieldJoiner(FIELD_SEPARATE_CHAR); }
	// 组合为组信息
	inline utils::FieldJoiner joinGroups() { return utils::FieldJoiner(GROUP_SEPARATE_CHAR); }
	inline QList<QStringList> splitGroupAndFields(const QString& packed) {
		QList<QStringList> result;
		QStringList groups = packed.split(GROUP_SEPARATE_CHAR);

		for (int i = 0; i < groups.size(); ++i) {
			QStringList fields = groups[i].split(FIELD_SEPARATE_CHAR);
			for (int j = 0; j < fields.size(); ++j) {
				if (fields[j] == "NULL")
					fields[j] = "";
			}
			result.append(fields);
		}
		return result;
	}

	inline QString getField(const QList<QStringList>& groups, int groupIndex, int fieldIndex, const QString& defaultValue = "") {
		if (groupIndex < 0 || groupIndex >= groups.size())
			return defaultValue;
		const QStringList& fields = groups[groupIndex];
		if (fieldIndex < 0 || fieldIndex >= fields.size())
			return defaultValue;
		return fields[fieldIndex];
	}

public:
	SvgTransformer();
	~SvgTransformer();

	// Build svg models directly (no SVG output). Caller owns returned pointer.
	//************************************
	// 函数名称:	BuildLogicModelByIedName
	// 函数全名:	SvgTransformer::BuildLogicModelByIedName
	// 访问权限:	public
	// 函数说明:	按IED名称生成逻辑模型(不输出SVG)
	// 函数参数:	const QString& iedName
	// 返回值:	LogicSvg*
	//************************************
	LogicSvg* BuildLogicModelByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildOpticalModelByIedName
	// 函数全名:	SvgTransformer::BuildOpticalModelByIedName
	// 访问权限:	public
	// 函数说明:	按IED名称生成光纤模型(不输出SVG)
	// 函数参数:	const QString& iedName
	// 返回值:	OpticalSvg*
	//************************************
	OpticalSvg* BuildOpticalModelByIedName(const QString& iedName);
	OpticalSvg* BuildOpticalModelByStation();
	OpticalSvg* BuildOpticalModelByBayName(const QString& bayName);
	//************************************
	// 函数名称:	BuildVirtualModelByIedName
	// 函数全名:	SvgTransformer::BuildVirtualModelByIedName
	// 访问权限:	public
	// 函数说明:	按IED名称生成虚回路模型(不输出SVG)
	// 函数参数:	const QString& iedName
	// 返回值:	VirtualSvg*
	//************************************
	VirtualSvg* BuildVirtualModelByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildWholeCircuitModelByIedName
	// 函数全名:	SvgTransformer::BuildWholeCircuitModelByIedName
	// 访问权限:	public
	// 函数说明:	按IED名称生成全回路模型(不输出SVG)
	// 函数参数:	const QString& iedName
	// 返回值:	WholeCircuitSvg*
	//************************************
	WholeCircuitSvg* BuildWholeCircuitModelByIedName(const QString& iedName);

	void GenerateSvgByIedName(const QString& iedName);

	void RenderLogicByIedName(const QString& iedName, QPaintDevice* device);
	void RenderOpticalByIedName(const QString& iedName, QPaintDevice* device);
	void RenderVirtualByIedName(const QString& iedName, QPaintDevice* device);
	void RenderWholeCircuitByIedName(const QString& iedName, QPaintDevice* device);
	// 已开始的QPainter
	void RenderLogicByIedName(const QString& iedName, QPainter* activePainter);
	void RenderOpticalByIedName(const QString& iedName, QPainter* activePainter);
	void RenderVirtualByIedName(const QString& iedName, QPainter* activePainter);
	void RenderWholeCircuitByIedName(const QString& iedName, QPainter* activePainter);

	// 返回绘制后的 QImage，以 SVG 视图框大小作为默认画布尺寸
	QImage RenderLogicToImage(const QString& iedName, const QSize& size = QSize());
	QImage RenderOpticalToImage(const QString& iedName, const QSize& size = QSize());
	QImage RenderVirtualToImage(const QString& iedName, const QSize& size = QSize());
	QImage RenderWholeCircuitToImage(const QString& iedName, const QSize& size = QSize());

	void GenerateLogicSvg(const IED* pIed, const QString& filePath);
	void GenerateOpticalSvg(const IED* pIed, const QString& filePath);
	void GenerateVirtualSvg(const IED* pIed, const QString& filePath);
	//void GenerateWholeCircuitSvg(const IED* pIed, const QString& filePath);

	// 中转，不写盘直接用于显示
	QByteArray GenerateLogicSvgBytes(const QString& iedName);
	QByteArray GenerateOpticalSvgBytes(const QString& iedName);
	// 若提供交换机名称，则只显示该交换机连接的虚回路
	QByteArray GenerateVirtualSvgBytes(const QString& iedName, const QString& swName = "");
	QByteArray GenerateWholeCircuitSvgBytes(const QString& iedName);

protected:

	template <typename SvgType>
	void GenerateSvg(const IED* pIed, const QString& filePath, void (SvgTransformer::* generateSvgFunc)(const IED*, SvgType&), void (SvgTransformer::* drawSvgFunc)(SvgType&))
	{
		if (!pIed || filePath.isEmpty()) return;
		QSvgGenerator svgGenerator;

		svgGenerator.setFileName(filePath);
		//svgGenerator.setViewBox(QRect(0, 0, svg.viewBoxWidth, svg.viewBoxHeight));
		svgGenerator.setViewBox(QRect(0, 0, SVG_VIEWBOX_WIDTH, SVG_VIEWBOX_HEIGHT));

		m_painter->begin(&svgGenerator);
		QPainter painter;
		SvgType svg;
		(this->*generateSvgFunc)(pIed, svg);
		(this->*drawSvgFunc)(svg);

		m_painter->end();
		svgGenerator.setFileName(QString());
		ReSignSvg(filePath, svg);
	}
	//************************************
	// 函数名称:	GenerateLogicSvgByIed
	// 函数全名:	SvgTransformer::GenerateLogicSvgByIed
	// 访问权限:	protected 
	// 函数说明:	生成逻辑图SVG描述结构
	// 函数参数:	const IED * pIed
	// 函数参数:	LogicSvg & svg
	// 返回值:		void
	//************************************
	void GenerateLogicSvgByIed(const IED* pIed, LogicSvg& svg);

	//************************************
	// 函数名称:	ParseCircuitFromIed
	// 函数全名:	SvgTransformer::ParseCircuitFromIed
	// 访问权限:	protected 
	// 函数说明:	从IED结构中获取逻辑链路及相关IED，填充svg描述
	// 函数参数:	LogicSvg & svg
	// 函数参数:	const IED * pIed
	// 返回值:		void
	//************************************
	void ParseCircuitFromIed(LogicSvg& svg, const IED* pIed);

	void ParsePlate(VirtualCircuit& svg);

	//************************************
	// 函数名称:	GenerateOpticalSvgByIed
	// 函数全名:	SvgTransformer::GenerateOpticalSvgByIed
	// 访问权限:	protected 
	// 函数说明:	生成光纤图SVG描述结构，上下结构，上下为IED，中间为交换机
	// 函数参数:	const IED * pIed
	// 函数参数:	OpticalSvg & svg
	// 返回值:		void
	//************************************
	//void GenerateOpticalSvgByIed_old(const IED* pIed, OpticalSvg& svg);

	//************************************
	// 函数名称:	GenerateOpticalSvgByIed_old
	// 函数全名:	SvgTransformer::GenerateOpticalSvgByIed_old
	// 访问权限:	protected 
	// 函数说明:	生成光纤图SVG描述结构，上下结构，主IED在上层中心，中间为交换机，下层为周边IED
	// 函数参数:	const IED * pIed
	// 函数参数:	OpticalSvg & svg
	// 返回值:		void
	//************************************
	//void GenerateOpticalSvgByIed(const IED* pIed, OpticalSvg& svg);
	void GenerateOpticalSvgByIed(const IED* pIed, OpticalSvg& svg);

	//************************************
	// 函数名称:	GenerateVirtualSvgByIed
	// 函数全名:	SvgTransformer::GenerateVirtualSvgByIed
	// 访问权限:	protected 
	// 函数说明:	生成虚回路图SVG描述结构（相较于完整二次回路图，不显示交换机连接关系）
	// 函数参数:	const IED * pIed
	// 函数参数:	VirtualSvg & svg
	// 返回值:		void
	//************************************
	void GenerateVirtualSvgByIed(const IED* pIed, VirtualSvg& svg/*, const QString& swName = ""*/);
	//************************************
	// 函数名称:	GenerateVirtualSvgByIed
	// 函数全名:	SvgTransformer::GenerateVirtualSvgByIed
	// 访问权限:	protected 
	// 函数说明:	生成完整二次回路图SVG描述结构
	// 函数参数:	const IED * pIed
	// 函数参数:	VirtualSvg & svg
	// 返回值:		void
	//************************************
	void GenerateWholeCircuitSvgByIed(const IED* pIed, WholeCircuitSvg& svg);

	//************************************
	// 函数名称:	DrawLogicSvg
	// 函数全名:	SvgTransformer::DrawLogicSvg
	// 访问权限:	protected 
	// 函数说明:	绘制指定逻辑链路svg描述结构
	// 函数参数:	LogicSvg & svg
	// 返回值:		void
	//************************************
	void DrawLogicSvg(LogicSvg& svg);
	//************************************
	// 函数名称:	DrawOpticalSvg
	// 函数全名:	SvgTransformer::DrawOpticalSvg
	// 访问权限:	protected 
	// 函数说明:	绘制光纤链路svg描述结构
	// 函数参数:	OpticalSvg & svg
	// 返回值:		void
	//************************************
	void DrawOpticalSvg(OpticalSvg& svg);

	//************************************
	// 函数名称:	DrawVirtualSvg
	// 函数全名:	SvgTransformer::DrawVirtualSvg
	// 访问权限:	protected 
	// 函数说明:	绘制虚回路svg描述结构
	// 函数参数:	VirtualSvg & svg
	// 返回值:		void
	//************************************
	void DrawVirtualSvg(VirtualSvg& svg);

	//************************************
	// 函数名称:	DrawWholeSvg
	// 函数全名:	SvgTransformer::DrawVirtualSvg
	// 访问权限:	protected 
	// 函数说明:	绘制虚回路svg描述结构（相较于完整二次回路，不显示交换机部分）
	// 函数参数:	WholeCircuitSvg & svg
	// 返回值:		void
	//************************************
	void DrawWholeSvg(WholeCircuitSvg& svg);

	//************************************
	// 函数名称:	DrawConnCircle
	// 函数全名:	SvgTransformer::DrawConnCircle
	// 访问权限:	protected 
	// 函数说明:	对于指定点绘制连接点
	// 函数参数:	const QPoint & pt 指定点
	// 函数参数:	int radius 连接点半径
	// 函数参数:	bool isCircleUnderPt 连接点是否位于指定点下方
	// 返回值:		void
	//************************************
	void DrawConnCircle(const QPoint& pt, int radius, bool isCircleUnderPt = true);

	//************************************
	// 函数名称:	AdjustOtherIedRectPosition
	// 函数全名:	SvgTransformer::AdjustOtherIedRectPosition
	// 访问权限:	protected 
	// 函数说明:	生成全部IED后，根据关联IED数量，将其调整到主IED两侧位置
	// 函数参数:	QList<IedRect * > & rectList
	// 函数参数:	const IedRect & mainIedRect
	// 返回值:		void
	//************************************
	void AdjustOtherIedRectPosition(QList<IedRect*>& rectList, const IedRect* mainIedRect);

	//************************************
	// 函数名称:	AdjustCircuitLinePosition
	// 函数全名:	SvgTransformer::AdjustCircuitLinePosition
	// 访问权限:	protected 
	// 函数说明:	在确定IED位置后，调整链路位置
	// 函数参数:	LogicSvg & svg
	// 返回值:		void
	//************************************
	void AdjustLogicCircuitLinePosition(LogicSvg& svg);

	//************************************
	// 函数名称:	AdjustVirtualCircuitLinePosition
	// 函数全名:	SvgTransformer::AdjustVirtualCircuitLinePosition
	// 访问权限:	protected 
	// 函数说明:	调整虚回路位置
	// 函数参数:	VirtualSvg & svg
	// 返回值:		void
	//************************************
	void AdjustVirtualCircuitLinePosition(VirtualSvg& svg);

	//************************************
	// 函数名称:	AdjustExtendRectByCircuit
	// 函数全名:	SvgTransformer::AdjustExtendRectByCircuit
	// 访问权限:	protected 
	// 函数说明:	根据IED的链路数量，调整外部矩形高度
	// 函数参数:	IedRect & pRect
	// 返回值:		void
	//************************************
	void AdjustExtendRectByCircuit(IedRect& pRect);
	void AdjustExtendRectByCircuit(QList<IedRect*>& iedList, LogicSvg& svg);

private:
	inline double AngleToRadians(double angle)
	{
		return angle * M_PI / 180.0;
	}

	void SetArrowStateDirect(const QList<LogicCircuit*>& inList,
		const QList<LogicCircuit*>& outList,
		const QString& mainIedName,
		const QString& peerIedName,
		OpticalCircuitLine* pLine);

	void SetArrowStateThroughSwitch(const QList<LogicCircuit*>& inList,
		const QList<LogicCircuit*>& outList,
		const QString& mainIedName,
		const QString& peerIedName,
		OpticalCircuitLine* pOppLine,
		OpticalCircuitLine* pMainSwitchLine);

	//************************************
	// 函数名称:	drawArrowHeader
	// 函数全名:	SvgTransformer::drawArrowHeader
	// 访问权限:	private 
	// 函数说明:	绘制箭头头部
	// 函数参数:	const QPoint & endPoint
	// 函数参数:	double arrowAngle
	// 返回值:		void
	//************************************
	void drawArrowHeader(const QPoint& endPoint, double arrowAngle, QColor color = utils::ColorHelper::pure_green, int arrowLen = ARROW_LEN);
	double GetAngleByVec(const QPointF& vec) const;
	QPoint GetArrowPt(const QPoint& pt, int arrowLen, int conn_r, double angle, bool isUnderConnpt, int offset = 10);
	// 关联IED图形，两侧绘制。rectIndex为ied矩形编号，用于控制矩形位置
	IedRect* GetOtherIedRect(quint16 rectIndex, IedRect* mainIedRect, const IED* pIed, const quint32 border_color, const quint32 underground_color = utils::ColorHelper::pure_black);

	//IedRect* GetIedRect(QString iedName, QString iedDesc, const quint16 x, const quint16 y, const quint16 width, const quint16 height, const quint32 border_color, const quint32 underground_color = ColorHelper::pure_black);

	//void GetBaseRect(SvgRect* rect, const quint16 x, const quint16 y, const quint16 width, const quint16 height, const quint32 border_color, const quint32 underground_color = ColorHelper::pure_black);

	// 调整主IED两侧链路位置
	void AdjustMainSideCircuitLinePosition(const size_t circuitCnt, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft = true);

	void AdjustVirtualCircuitLinePosition(VirtualSvg& svg, QList<IedRect*>& rectList, IedRect* mainIed, bool isLeft = true);

	//************************************
	// 函数名称:	AdjustVirtualCircuitPlatePosition
	// 函数全名:	SvgTransformer::AdjustVirtualCircuitPlatePosition
	// 访问权限:	private 
	// 函数说明:	
	// 函数参数:	QHash<QString
	// 函数参数:	PlateRect> & hash			svg描述中的软压板矩形索引
	// 函数参数:	QString iedName				压板对侧IED名称
	// 函数参数:	const QPoint & linePt		回路起点/终点位置，作为软压板矩形位置参考
	// 函数参数:	const QString & plateName
	// 函数参数:	const QString & plateRef
	// 函数参数:	bool isSrcPlate				是否是开出压板
	// 函数参数:	bool isSideSrc
	// 函数参数:	bool isLeft
	// 返回值:		void
	//************************************
	void AdjustVirtualCircuitPlatePosition(QHash<QString, PlateRect>& hash, const QString& iedName, const QPoint& linePt, const QString& plateName, const QString& plateRef, quint64 plateCode, bool isSrcPlate, bool isSideSrc, bool isLeft);

	// 绘制单个IED矩形
	void DrawIedRect(IedRect* rect);
	void DrawIedRect(IedRect* rect, QPainter* _painter);
	// 绘制两侧关联矩形列表
	void DrawIedRect(QList<IedRect*>& rectList);
	void DrawIedRect(QList<IedRect*>& rectList, QPainter* _painter);
	// 绘制交换机矩形
	void DrawSwitcherRect(IedRect* rect);
	void DrawSwitcherRect(IedRect* rect, QPainter* _painter);
	// 绘制描述矩形
	void DrawDescRect(const IedRect& leftTopRect);
	void DrawDescRect(const IedRect& leftTopRect, QPainter* _painter);
	// 居中绘制文本，超过矩形长度自动换行
	void DrawTextInRect(SvgRect* rect, QString name, QString desc, int pointSize = 15);
	void DrawTextInRect(QPainter* _painter, SvgRect* rect, QString name, QString desc, int pointSize = 15);
	// 根据传入的IED列表，绘制外部矩形
	void DrawExternalRect(QList<IedRect*>& rectList, QString title, bool hasDescIedRect = false);
	void DrawExternalRect(IedRect* rect, QString title, bool hasDescIedRect = false);
	// 绘制带链路图标的矩形
	void DrawWholeRect(IedRect* rect);

	// 绘制链路
	void DrawLogicCircuitLine(QList<IedRect*>& rectList);
	// 绘制虚回路
	void DrawVirtualCircuitLine(QList<IedRect*>& rectList);
	// 绘制光纤链路
	void DrawOpticalLine(OpticalCircuitLine* optLine);

	// 绘制光纤链路
	void DrawPortText(OpticalCircuitLine* line, int conn_r);
	//void DrawPortText(OpticalCircuitLine* line);

	void DrawVirtualIcon(const QPoint& pt, VirtualType _type, const quint32 color = utils::ColorHelper::pure_green);

	// 绘制通道值
	void DrawVirtualText(const QRect& rect, QString val, QString typeStr);

	// 绘制压板
	void DrawPlate(const QHash<QString, PlateRect>& hash);

	//************************************
	// 函数名称:	DrawGseIcon
	// 函数全名:	SvgTransformer::DrawGseIcon
	// 访问权限:	private 
	// 函数说明:	绘制GSE图标，输入点为图标左上角坐标
	// 函数参数:	const QPoint & pt
	// 函数参数:	const quint32 color
	// 返回值:		void
	//************************************
	void DrawGseIcon(const QPoint& pt, const quint32 color = utils::ColorHelper::line_gse);

	//************************************
	// 函数名称:	DrawSvIcon
	// 函数全名:	SvgTransformer::DrawSvIcon
	// 访问权限:	private 
	// 函数说明:	绘制SV图标，输入点为图标左上角坐标
	// 函数参数:	const QPoint & pt
	// 函数参数:	const quint32 color
	// 返回值:		void
	//************************************
	void DrawSvIcon(const QPoint& pt, const quint32 color = utils::ColorHelper::line_smv);
	//************************************
	// 函数名称:	DrawPlateIcon
	// 函数全名:	SvgTransformer::DrawPlateIcon
	// 访问权限:	private 
	// 函数说明:	绘制检修压板图标，输入点为左侧圆的圆心位置。
	// 函数参数:	const QPoint & pt
	// 函数参数:    bool status 通断状态
	// 函数参数:    const QString & info 压板描述和引用信息
	// 返回值:		void
	//************************************
    void DrawPlateIcon(const QPoint& pt, bool status, const QString& info);

	// 对SVG文件进行解析重标识
	void ReSignSvg(const QString&filename, BaseSvg& svg);
	// 内存文档重签名（不进行文件读写）
	void ReSignSvgDoc(pugi::xml_document& doc, BaseSvg& svg);
	// 重标识IED属性
	void ReSignIedRect(pugi::xml_document& doc);
	// 重标识链路属性
	void ReSignCircuitLine(pugi::xml_document& doc);
	// 重标识压板信息
	void ReSignPlate(pugi::xml_document& doc);
	// 重标识虚回路数值占位，便于交互层识别
	void ReSignVirtualValuePlaceholders(pugi::xml_document& doc);
	// 调整SVG视图框大小
	void ReSignSvgViewBox(pugi::xml_document& doc, int x, int y, int width, int height);
private:
	CircuitConfig* m_circuitConfig;
	QSvgGenerator* m_svgGenerator;
	QPainter* m_painter;
	QString m_errStr;
	// 用于SVG中图形组标识
	int m_element_id;		// 组成同一部分的图形元素的标识，例如一个压板图形的两个○和矩形
	enum svgType
	{
		TYPE_IED = 90,
		TYPE_Switcher = 100,
		TYPE_Plate_HITBOX = 110,
		TYPE_Plate_ICON,
		TYPE_Plate_RECT,
		TYPE_LogicCircuit = 120,
		TYPE_OpticalCircuit = 130,
		TYPE_Optical_ConnCircle,		// 光纤连接点圆圈
		TYPE_VirtualCircuit = 140,
		TYPE_Circuit_Arrow,				// 回路箭头
		TYPE_Virtual_Value				// 虚回路数值占位标签
	};
};
