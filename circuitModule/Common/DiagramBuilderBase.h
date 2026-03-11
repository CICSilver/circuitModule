#pragma once
#include "circuitconfig.h"
#include "svgmodel.h"
#include "SvgUtils.h"
#include "Common/SvgTransformerCommon.h"
#include <QString>
#include <QtGlobal>

// 建图公共基类，负责共享配置和通用布局算法。
class DiagramBuilderBase
{
public:
	//************************************
	// 函数名称:	DiagramBuilderBase
	// 函数全名:	DiagramBuilderBase::DiagramBuilderBase
	// 函数权限:	public
	// 函数说明:	初始化建图公共上下文
	// 函数参数:	无
	// 函数返回:	无
	//************************************
	DiagramBuilderBase();
	//************************************
	// 函数名称:	~DiagramBuilderBase
	// 函数全名:	DiagramBuilderBase::~DiagramBuilderBase
	// 函数权限:	public
	// 函数说明:	释放建图公共上下文
	// 函数参数:	无
	// 函数返回:	无
	//************************************
	virtual ~DiagramBuilderBase();
	//************************************
	// 函数名称:	GetErrorString
	// 函数全名:	DiagramBuilderBase::GetErrorString
	// 函数权限:	public
	// 函数说明:	获取建图阶段记录的错误信息
	// 函数参数:	无
	// 函数返回:	const QString&
	//************************************
	const QString& GetErrorString() const;

protected:
	//************************************
	// 函数名称:	ParseCircuitFromIed
	// 函数全名:	DiagramBuilderBase::ParseCircuitFromIed
	// 函数权限:	protected
	// 函数说明:	根据IED关系填充逻辑连接和对侧IED矩形
	// 函数参数:	LogicSvg& svg
	// 函数参数:	const IED* pIed
	// 函数返回:	void
	//************************************
	void ParseCircuitFromIed(LogicSvg& svg, const IED* pIed);
	//************************************
	// 函数名称:	AdjustOtherIedRectPosition
	// 函数全名:	DiagramBuilderBase::AdjustOtherIedRectPosition
	// 函数权限:	protected
	// 函数说明:	根据主IED位置调整两侧IED矩形位置
	// 函数参数:	QList<IedRect*>& rectList
	// 函数参数:	const IedRect* mainIedRect
	// 函数返回:	void
	//************************************
	void AdjustOtherIedRectPosition(QList<IedRect*>& rectList, const IedRect* mainIedRect);
	//************************************
	// 函数名称:	AdjustExtendRectByCircuit
	// 函数全名:	DiagramBuilderBase::AdjustExtendRectByCircuit
	// 函数权限:	protected
	// 函数说明:	根据回路数量计算单个IED扩展区域高度
	// 函数参数:	IedRect& pRect
	// 函数返回:	void
	//************************************
	void AdjustExtendRectByCircuit(IedRect& pRect);
	//************************************
	// 函数名称:	AdjustExtendRectByCircuit
	// 函数全名:	DiagramBuilderBase::AdjustExtendRectByCircuit
	// 函数权限:	protected
	// 函数说明:	批量调整IED扩展区域并重新排布位置
	// 函数参数:	QList<IedRect*>& iedList
	// 函数参数:	LogicSvg& svg
	// 函数返回:	void
	//************************************
	void AdjustExtendRectByCircuit(QList<IedRect*>& iedList, LogicSvg& svg);
	//************************************
	// 函数名称:	GetOtherIedRect
	// 函数全名:	DiagramBuilderBase::GetOtherIedRect
	// 函数权限:	protected
	// 函数说明:	按索引生成对侧IED矩形初始位置
	// 函数参数:	quint16 rectIndex
	// 函数参数:	IedRect* mainIedRect
	// 函数参数:	const IED* pIed
	// 函数参数:	const quint32 border_color
	// 函数参数:	const quint32 underground_color
	// 函数返回:	IedRect*
	//************************************
	IedRect* GetOtherIedRect(quint16 rectIndex, IedRect* mainIedRect, const IED* pIed, const quint32 border_color, const quint32 underground_color = utils::ColorHelper::pure_black);
	//************************************
	// 函数名称:	AdjustMainSideCircuitLinePosition
	// 函数全名:	DiagramBuilderBase::AdjustMainSideCircuitLinePosition
	// 函数权限:	protected
	// 函数说明:	根据IED矩形位置调整主IED一侧的逻辑连线
	// 函数参数:	const size_t circuitCnt
	// 函数参数:	QList<IedRect*>& rectList
	// 函数参数:	IedRect* mainIedRect
	// 函数参数:	bool isLeft
	// 函数返回:	void
	//************************************
	void AdjustMainSideCircuitLinePosition(const size_t circuitCnt, QList<IedRect*>& rectList, IedRect* mainIedRect, bool isLeft = true);

	CircuitConfig* m_pCircuitConfig;
	QString m_errStr;

private:
	Q_DISABLE_COPY(DiagramBuilderBase)
};
