# CIM/E 数据访问组件技术说明

本组件提供一个轻量级的表格数据管理与序列化框架，围绕 `Cime / CimeBlock / CimeAccessor` 三个核心概念，实现字段定义、记录维护、条件查询以及 CIM/E 文本格式的读写。

---

## 一、架构概述

- `Cime`：块管理器，内部用按插入顺序保存的容器维护所有 `CimeBlock`，对外通过 `operator[]` 提供 JSON 风格访问，并负责创建、销毁和文件持久化。
- `CimeAccessor`：块访问器，将多级键拼接为块全名（`A::B::C`），访问时若块不存在会自动创建；同时暴露 `fields()`、`records()`、`block()` 等接口获取对应的 Helper/Mutator。
- `CimeBlock`：单个数据块，持有字段列表、记录列表、块属性、字段名到列号的索引表、自增字段索引表、表格显示配置以及父级 `Cime` 指针，友元类负责对其进行增删改查。
- `FieldMutator`、`RecordBuilder`、`BlockMutator`：分别封装字段、记录和块级别的写操作，统一维护 `m_fieldIndexMap` / `m_autoIncrementMap` 等内部结构的一致性。
- `FieldAccessorHelper` / `RecordAccessorHelper`：对外的读写辅助类，通过 `CimeAccessor` 暴露，封装了新增字段/记录、排序、删除等常用操作。

> 注意：块容器内部使用的是保持插入顺序的顺序容器（而非 `std::map`），字段和记录本身的物理顺序也是“按添加/读入顺序”。只有字段名索引 `m_fieldIndexMap` 使用 `std::map`，用于按名称快速定位列号。

---

## 二、核心数据结构

- `TableType`
	- `HORIZONTAL`：标准多列表格（当前主要实现）
	- `SINGLE_COLUMN`：单列表格
	- `MULTI_COLUMN`：多列表格（预留支持）

- `FieldType`
	- `INT`、`FLOAT`、`STRING`、`POINTER`
	- 文本中用 `i / f / s / p` 表示，`CimeField::setType(const std::string&)` 与 `CimeField::Type()` 负责在枚举与字符编码之间转换。

- `CimeField`
	- 元数据：`name`、`alias`、`type`、`length`、`dimension`、`valueLimit`、`isReference`、`refTable`、`refField`、`isAutoIncrement`。
	- 只支持 `INT` 类型字段设置为自增；
	- 别名、单位、限值等可独立控制是否参与序列化显示。

- `CimeRecord`
	- 使用 `std::vector<std::string> values` 存储一行中各字段值，`sparsePointers` 存储额外的稀疏记录指针。
	- `operator[](const std::string&)` 及模板版本通过 `fieldIndex()` 和 `details::value_cast()` 按字段名或其它可转换类型访问列；若字段名不存在会输出日志并抛出异常。

- `RecordProxy`
	- 查询结果中的只读视图，内部保存指向块的指针和行号，访问时再去块的记录数组中取值。
	- 在启用 `CIME_QT` 时，返回 `QString`；否则返回 `std::string`。

- `CimeQueryIterator`
	- 底层持有行号向量迭代器，每次解引用生成一个 `RecordProxy`，供范围 for 遍历查询结果使用。

---

## 三、数据存储与索引规则

- 每个 `CimeBlock` 内部包含：
	- `m_fields`：`std::vector<CimeField>`，字段按添加或文件读入顺序存储；
	- `m_records`：`std::vector<CimeRecord>`，记录按插入顺序存储；
	- `m_attributes`：块级属性字典 `key -> value`；
	- `m_fieldIndexMap`：字段名映射到列号，用于按名称快速访问；
	- `m_autoIncrementMap`：记录所有自增字段名及其列号，用于插入时自动计算新值；
	- 若某条旧记录在新增字段时没有对应列，导出时会落为 `NULL`。

- 字段/记录顺序
	- 字段的显示与导出顺序由 `m_fields` 决定，即“添加/读入顺序”；
	- 记录的遍历与导出顺序由 `m_records` 决定，即“插入顺序”或“排序后顺序”；
	- `m_fieldIndexMap` 仅用于名称查找，若对其直接遍历（`for (auto &kv : getFieldIndexMap())`），顺序会是按字段名字典序自动排序，这是 `std::map` 的特性。

- 自增字段规则
	- 仅 `INT` 类型字段可被标记为自增；
	- 插入记录时，若块存在自增字段，其值取该字段在当前块最后一条记录中的值加一（按十进制整数解析）；
	- 首条记录的自增字段从 `1` 开始。

---

## 四、数据读写流程

### 1. 字段维护（`FieldMutator` / `FieldAccessorHelper`）

- 新增字段
	- 通过 `fields().add("name")` 或 `fields() << "name"` 快速追加默认字段；
	- 若字段已存在，则复用原字段并将 `FieldMutator` 绑定到该列上。

- 修改字段
	- `rename()`：修改字段名，并同步更新 `m_fieldIndexMap` / `m_autoIncrementMap`；
	- `type()`：设置字段类型，仅支持 `INT/FLOAT/STRING/POINTER`；
	- `length()/dimension()/valueLimit()`：设置长度、单位与值约束；
	- `refernceTo(tbl, fld)`：标记为引用字段并记录外表与外字段名；
	- `setAutoIncrement(true)`：将整型字段标记为自增；
	- `setAlias()`：设置显示别名。

- 删除字段
	- `drop()` 会删除 `m_fields` 中对应列，同时对每条记录的 `values` 删除该列，并调整所有后续字段的列号映射与自增映射。

### 2. 记录写入（`RecordBuilder` / `RecordAccessorHelper`）

- `records().add()` 返回 `RecordBuilder`，预先按当前字段数量分配一行空记录；
- 使用 `set(fieldName, value)` 或 `set(index, value)` 写入字段值，模板版本会通过 `details::value_cast()` 自动转为字符串；
- 执行 `commit()` 或在 `RecordBuilder` 析构时自动提交：
	- 对所有自增字段计算下一值并写入；
	- 将该记录推入 `m_records` 尾部。

### 3. 块级修改（`BlockMutator`）

- 通过 `block()` 获取 `BlockMutator`，可：
	- 设置/修改块全名、描述；
	- 设置表格类型 `TableType`；
	- 控制是否显示字段类型、单位、限值、别名；
	- 设置任意字符串属性键值对；
	- `drop()` 删除整个块，由父级 `Cime` 移除并释放内存。

### 4. 记录排序与删除（`RecordAccessorHelper`）

- 排序
	- `records().sortBy("fieldName", true/false)`：按指定字段升/降序排序；
	- `records().sortBy(columnIndex, true/false)`：按列号排序；
	- 模板重载支持任意可通过 `value_cast` 转成字符串的键。

- 删除
	- `erase(iterator)`：按迭代器删除单条记录；
	- `erase(rowIndex)`：按行号删除；
	- `erase(field, value)`：删除该字段等于指定值的所有记录；
	- `erase(CimeQuery&)`：结合查询结果批量删除。

---

## 五、查询机制（`CimeQuery`）

- 条件构建
	- `where(fieldName, value)` 将条件加入内部条件列表，支持链式调用；
	- 模板版本会自动调用 `value_cast` 将任意类型转换为字符串再比较。

- 执行与结果
	- `execute()`：清空旧结果后全表扫描，将满足所有条件的记录行号写入 `m_rowList`；
	- `all()`：不加条件地收集当前块的所有记录行号；
	- `begin()/end()/cbegin()/cend()`：返回对应的查询迭代器，用于范围 for；
	- `first()/last()`：快速获取首/尾匹配记录；
	- `operator[](index)`：根据结果集中行号索引返回对应 `RecordProxy`；
	- `size()/empty()`：返回结果集大小/是否为空。

---

## 六、序列化与文件格式

- 写文件：`Cime::saveToFile(const std::string &filename)`
	- 写入头部：`<!System=OMS Version=1.0 Code=UTF-8 Data=1.0!>`；
	- 每个块用 `<FullName attr="value" ...>` 与 `</FullName>` 包裹；
	- 表头部分（仅 `HORIZONTAL` 表）：
		- `@` 行：字段名列表；
		- `/@` 行：字段别名（可选，需 `setAliasVisible(true)`）；
		- `%` 行：字段类型编码（可选，需 `setTypeVisible(true)`）；
		- `$` 行：字段单位（可选，需 `setUnitVisible(true)`）；
		- `:` 行：字段值限制（可选，需 `setValueLimitVisible(true)`）。
	- 记录部分：
		- 每条记录一行，以 `#` 开头，字段之间用制表符分隔；
		- 若某字段在该记录中无值或缺失列，则输出 `NULL`；
		- 稀疏记录指针行直接逐行追加在记录后。

- 读文件：`Cime::loadFromFile(const std::string &filename)`
	- 跳过头部与空行；
	- 解析块起始行 `<FullName ...>`，创建对应块并通过 `BlockMutator` 写入属性；
	- 使用 `details::splitTokens()` 按约定前缀依次解析字段名、别名、类型、单位、限值等信息，填充 `m_fields` 与 `m_fieldIndexMap`；
	- 对每个 `#` 记录行构造 `CimeRecord`，按字段顺序填充 `values`，并加入 `m_records`；
	- 读取到结束标签 `</FullName>` 时结束当前块的解析。

---

## 七、日志与 Qt 适配

- 日志
	- 通过 `_CIME_WRITE` 宏族实现统一日志输出，默认输出到 `std::cout` / `std::cerr`；
	- 若定义 `CIME_LOG_TO_FILE`，则日志写入 `cime_log.txt`；
	- 若定义 `CIME_SUPPRESS_LOG`，则屏蔽所有日志输出；
	- 使用 `CIME_LOG(MSG)` 与 `CIME_ERR(MSG)` 记录普通信息与错误信息。

- Qt 适配
	- 定义 `CIME_QT`（当前头文件中默认开启）后，引入 `QString` / `QByteArray`：
		- `details::value_cast(const QString&)`：将 `QString` 转为 `std::string`；
		- `RecordProxy::operator[]` 返回 `QString`；
	- 若不使用 Qt，可移除或关闭 `CIME_QT` 宏以减少依赖。

---

## 八、使用建议

- 推荐通过 `cime["Block"]` 获取 `CimeAccessor`，再使用 `fields()`、`records()`、`block()` 完成所有增删改查，避免直接操作 `CimeBlock` 内部容器。
- 遍历字段/记录时，如需保持“添加顺序”，请使用 `getFields()` / `getRecords()` 返回的 `vector`；仅在按名字随机访问时使用 `fieldIndex()` 或 `operator[](fieldName)`。
- 批量导入/导出前，可按需要配置 `setTypeVisible()`、`setUnitVisible()`、`setValueLimitVisible()`、`setAliasVisible()`，控制表头额外信息是否写入文件。
- 使用 `CimeQuery` 做条件筛选，结合 `first()/last()/size()` 等接口，可以在无需手动遍历全部记录的情况下快速定位结果。
- 对大数据量表格，可以在上层根据查询结果维护缓存或额外索引，以降低多次全表扫描的成本。
