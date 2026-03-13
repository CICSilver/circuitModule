#pragma once
#include <QString>
#include <qmath.h>

// == SVG生成和解析相关常量 ==
// SVG交互信息中“组”级别的分隔符，供 joinGroups / splitGroupAndFields 使用 - 已弃用
#define GROUP_SEPARATE_CHAR QChar(0x1F)
// SVG交互信息中“字段”级别的分隔符，供 joinFields / splitGroupAndFields 使用 - 已弃用
#define FIELD_SEPARATE_CHAR QChar(0x1E)
// SVG 文本绘制的默认字体族
#define DEFAULT_FONT_FAMILY "SimSun"
enum SvgTextConfig
{
	DEFAULT_FONT_WEIGHT = 400	// SVG 文本绘制的默认字重
};

// == 建图算法相关常量 ==
// 角度转弧度，供箭头朝向和几何计算使用
#define GET_RADIANS(angle) (angle * M_PI / 180)
enum SvgTransformerConfig
{
	SAFE_DISTANCE = 100,				// 全站/间隔实回路图中，连线离开设备连接点后的首段安全距离
	SCOPED_LANE_STEP = 32,			// 全站/间隔实回路图中，第二段分层折线的累加步长
	SCOPED_DYNAMIC_RESERVED_GAP = 20,	// 全站/间隔实回路图中，仅当层间剩余空间不足该值时才触发动态扩距
	SCOPED_SIDE_MARGIN = 80,		// 全站/间隔实回路图整体左右留白
	SCOPED_ROW_GAP = 70,			// 全站/间隔实回路图同层 IED 之间的横向间距
	SCOPED_TOP_LAYER_Y = 220,		// 全站/间隔实回路图顶层 IED 的基准 Y 坐标
	SCOPED_BASE_LAYER_GAP_Y = 170,	// 全站/间隔实回路图各层之间的基础垂直间距
	SCOPED_SWITCH_GAP_Y = 55,		// 全站/间隔实回路图交换机层内部的纵向间距
	IED_OPTICAL_HORIZONTAL_DISTANCE = 50,	// 单 IED 实回路图中设备之间的横向间距
	IED_OPTICAL_VERTICAL_DISTANCE = 150,	// 单 IED 实回路图中主设备、交换机和下层设备之间的纵向间距
	IED_OPTICAL_MIDPOINT_DISTANCE = 10,	// 单 IED 实回路图中折线中间拐点的错位距离
	OPTICAL_ARROW_EXTRA_OFFSET = 5,	// 旧版 SVG 光纤箭头相对连接点的额外偏移
	OPTICAL_DOUBLE_ARROW_GAP_RATIO = 2	// 旧版 SVG 双向箭头之间的间距倍率，基于箭头长度计算
};

static const qreal IED_OPTICAL_SWITCHER_WIDTH_RATIO = 0.75;	// 单 IED 实回路图中交换机矩形宽度占默认视口宽度的比例
