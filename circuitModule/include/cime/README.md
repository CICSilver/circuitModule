# CIM/E 数据访问组件技术文档

## 架构概述
- `Cime` 作为管理器维护所有 `CimeBlock`，以块全名为键存入 `std::map<std::string, CimeBlock*>`，负责创建、销毁、持久化与统一入口 `operator[]`（cime.h:1465-1537）。
- `CimeAccessor` 负责将层级键拼接为块全名，必要时自动创建缺失块，同时暴露 `fields()`、`records()`、`block()` 等接口获取对应 Mutator/Helper（cime.h:1397-1455）。
- `CimeBlock` 表示单个数据块，持有字段、记录、属性和索引，并维护表格显示配置及 `Cime` 父指针，友元类协助执行增删改查（cime.h:356-419）。
- `FieldMutator`、`RecordBuilder`、`BlockMutator` 实现字段、记录、块的写操作封装，确保索引、自增、属性等规则保持一致（cime.h:569-934）。

## 核心数据结构
- `TableType` 定义表格布局（`HORIZONTAL`、`SINGLE_COLUMN`、`MULTI_COLUMN`），`FieldType` 定义字段类型（`INT`、`FLOAT`、`STRING`、`POINTER`），影响序列化与解析（cime.h:261-276）。
- `CimeField` 描述字段元数据（名称、别名、类型、长度、单位、限值、外键、自增状态），并提供字符与枚举转换以支持文件和 UI（cime.h:280-323）。
- `CimeRecord` 以 `std::vector<std::string>` 存储行值及稀疏指针，`operator[]` 结合 `fieldIndex` 定位字段，模板重载使用 `details::value_cast` 支持多键类型（cime.h:334-564, 213-247）。
- `RecordProxy` 是查询结果的轻量视图，持有块指针和行号，在访问时返回实际字段值，兼容 Qt/标准字符串（cime.h:990-1086）。

## 数据存储与索引规则
- 每个 `CimeBlock` 内含字段列表、记录列表、属性字典、字段索引表和自增索引表，并保留父级 `Cime` 指针（cime.h:356-419）。
- `m_fieldIndexMap` 将字段名映射到列号；`m_autoIncrementMap` 标记自动增长的整型列，用于插入记录时计算下一个值，支持十六进制输入（cime.h:399-408, 748-782）。
- 新增字段时 `FieldMutator` 会为既有记录插入默认空值并更新索引，删除字段则同步移除记录列并重排索引（cime.h:569-681）。

## 数据读写流程
- **字段维护**：`FieldMutator` 构造时若字段不存在会追加默认字段；`rename/type/length/dimension/valueLimit/refernceTo/setAutoIncrement/setAlias` 等链式接口修改元数据；`drop()` 用于真删除字段并清理相关索引（cime.h:569-681）。
- **记录写入**：`RecordBuilder` 预分配字段数量的空值，通过 `set(field, value)` 或下标设置字段，`commit()` 触发自增值写入并推入记录；析构自动补齐未提交的记录（cime.h:722-818）。
- **块级修改**：`BlockMutator` 更新块全名、描述、表类型、显示控制及属性；`drop()` 会让 `Cime` 移除整个块（cime.h:820-934）。
- **访问接口**：`FieldAccessorHelper`/`RecordAccessorHelper` 封装常用的新增、访问、删除操作，供 `CimeAccessor` 统一对外暴露（cime.h:942-1041, 1326-1376）。

## 查询机制
- `CimeQuery` 延迟执行条件筛选：`where()` 收集字段与值并在 `execute()` 中遍历记录，将匹配行号写入 `m_rowList`，`all()` 则直接收集全部行（cime.h:1138-1324）。
- 迭代器 `CimeQueryIterator` 以行号容器为底层，生成 `RecordProxy` 供外部只读访问；`first()`、`last()`、`operator[]` 提供快速定位（cime.h:1101-1268）。
- `RecordAccessorHelper::erase` 支持按行号、按字段值或结合查询结果批量删除记录，依赖 `fieldIndex` 保证字段解析效率（cime.h:1334-1376）。

## 序列化与文件格式
- `saveToFile()` 输出自定义 CSV：文件头写 `<!System=OMS ...>`，块由 `<FullName attr="value"> ... </FullName>` 包裹；字段名、别名、类型、单位、限值分别以 `@`、`/@`、`%`、`$`、`:` 前缀行描述；记录使用 `#` 行写入，空值输出 `NULL`，稀疏指针逐行附加（cime.h:1627-1702, 1814-1890）。
- `loadFromFile()` 逆向解析上述格式，`details::splitTokens()` 用于按空白分割字段，按顺序解析字段、别名、类型、单位、限值和记录数据，并通过 `BlockMutator`/`FieldMutator`/`RecordBuilder` 补全块内容（cime.h:1647-1812, 229-242）。

## 使用建议
- 通过 `cime["Block"]` 获取 `CimeAccessor` 时，优先使用 `fields()`、`records()` 提供的 Helper/Mutator 操作，避免直接操作底层容器导致索引不一致（cime.h:942-1041, 1326-1376）。
- 在批量导入/导出前根据显示需求调用 `setTypeVisible()`、`setUnitVisible()`、`setValueLimitVisible()`、`setAliasVisible()`，否则相关行不会出现在序列化结果中（cime.h:398-407, 1814-1890）。
- 使用 `CimeQuery` 时可链式 `where()` 再遍历迭代器；若仅需单条记录可使用 `first()`/`last()` 减少遍历开销（cime.h:1138-1278）。
- 对大数据量场景，可在外层基于查询结果维护额外缓存或索引，以降低 `execute()` 全表扫描的成本。
