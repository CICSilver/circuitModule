#ifndef CIRCUITMODULEAPI_H
#define CIRCUITMODULEAPI_H

#include "circuitmodule_global.h"
class QWidget;
class CircuitModuleWidget;

#ifdef __cplusplus
extern "C" {
#endif

CIRCUITMODULE_API QWidget* CM_CreateModuleWidget(bool useSvgBytes, QWidget* parent);	// 创建模块组件，静态存在动态库堆
CIRCUITMODULE_API QWidget* CM_GetModuleWidget();										// 获取模块组件
CIRCUITMODULE_API void CM_Destroy();													// 销毁模块组件
CIRCUITMODULE_API bool CM_Refresh();

#ifdef __cplusplus
}
#endif

#endif // CIRCUITMODULEAPI_H
