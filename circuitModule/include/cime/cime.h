/**
 * @file cime.h
 * @brief CIM/E 读写库用户手册
 *
 * 本库用于结构化数据的读写、管理与查询，适用于电力系统CIM/E格式的数据处理。
 * 支持数据块（Block）、字段（Field）、记录（Record）的灵活操作
 *
 * =========================
 * 1. 基本定义
 * =========================
 * - Cime         : 数据库管理器，负责所有数据块的创建、存储、检索和持久化。
 * - CimeBlock    : 数据块，包含字段定义和记录数据。
 * - CimeField    : 字段定义，描述字段名、类型、长度、单位等。
 * - CimeRecord   : 单条记录，存储各字段的值。
 * - CimeAccessor : 辅助访问器，支持链式/嵌套访问。
 *
 * =========================
 * 2. 常用接口与用法
 * =========================
 * 2.1 创建与获取数据块
 *   xcime::Cime cime;
 *   // 创建数据块
 *   xcime::CimeBlock* block = cime.createBlock("BlockName", "描述");
 *   // 或通过访问器自动创建
 *   auto accessor = cime["BlockName"];
 *   xcime::CimeBlock* block2 = accessor;
 *
 * 2.2 字段操作
 *   // 添加字段（默认类型为STRING）
 *   accessor.fields().add("id").add("name");
 *   // 或通过 operator<< 添加字段
 *   accessor.fields() << "id" << "name";
 *   // 通过field()或operator[]访问字段并设置字段类型、长度、单位、别名等
 *   accessor.fields().field("id").type(xcime::INT).setAutoIncrement(true);
 *   accessor.fields()["name"].setAlias("名称").length("32");
 *
 * 2.3 添加记录
 *   // 添加一条记录并赋值，commit可省略，省略后在RecordBuilder析构时执行commit
 *   RecordBuilder builder = accessor.records().add();
 *   builder.set("id", 1).set("name", "张三").commit();
 *   // 可链式添加多条记录
 *   builder.set("id", 2).set("name", "李四");
 *   // 也可通过字段索引赋值
 *   builder.set(0, "3").set(1, "王五");
 *
 * 2.4 查询与遍历
 *   // 条件查询
 *   xcime::CimeQuery query(block);     // 查询实例需绑定数据块，不支持跨数据块查询
 *   query.where("name", "张三");
 *   for (auto it = query.begin(); it != query.end(); ++it) {
 *       auto rec = *it;
 *       std::cout << rec["id"] << std::endl;   // 读取记录的字段值可使用operator[]
 *   }
 * 
 *   // 获取数据块中全部数据
 *   xcime::CimeQuery query(block);
 *   query.all();
 *
 * 2.5 记录删除
 *   // 按行号删除
 *   accessor.records().erase(0);
 *   // 按条件删除
 *   accessor.records().erase("name", "张三");
 *
 * 2.6 数据持久化
 *   // 保存到文件
 *   cime.saveToFile("data.cime");
 *   // 从文件加载
 *   cime.loadFromFile("data.cime");
 * 2.7 记录排序
 *   xcime::Cime cime;
 *   xcime::CimeAccessor device = cime["Device"];
 *   device.fields() << "id" << "name";
 *   device.records().add().set("id", "2").set("name", "B");
 *   device.records().add().set("id", "1").set("name", "A");
 *   // 按字段名排序（默认升序）
 *   device.records().sortBy("id", true);
 *   // 按列索引（列号，从0开始）排序（降序）
 *   device.records().sortBy(static_cast<size_t>(1), false);
 *
 *
 * 3. 其他用法
 * =========================
 * - 嵌套块访问：cime["父块"]["子块1"]["子块2"];
 * - 支持字段别名、类型、单位、值限制等元数据设置。
 * - 支持自增字段（仅INT类型）。
 * - 支持链式操作和JSON风格访问。
 *
 * =========================
 * 4. 常用类型与枚举
 * =========================
 * enum xcime::FieldType { INT, FLOAT, STRING, POINTER };
 *
 * =========================
 * 5. 典型代码片段
 * =========================
 *   xcime::Cime cime;
 *   auto block = cime.createBlock("Device", "设备表");
 *   auto fields = block->getFields();
 *   auto records = block->getRecords();
 *   // 通过访问器添加字段和记录
 *   cime["Device"].fields() << "id" << "name";
 *   cime["Device"].records().add().set("id", 1).set("name", "A");
 *   // 查询
 *   xcime::CimeQuery q(cime["Device"]);
 *   q.where("id", "1");
 *   for (auto it = q.begin(); it != q.end(); ++it) {
 *       std::cout << (*it)["name"] << std::endl;
 *   }
 *
 * =========================
 * 6. 注意事项
 * =========================
 * - 字段名、块名区分大小写。
 * - 需先定义字段再添加记录。若添加的记录没有对应字段，控制台会有日志报错但不会中断进程
 * - 访问器可自动创建不存在的数据块。
 * - 支持Qt QString类型（需在头文件中打开CIME_QT宏）。
 *
 * =========================
 * 7. 主要接口速查
 * =========================
 * - Cime::saveToFile(filename)            // 保存到文件
 * - Cime::loadFromFile(filename)          // 从文件加载
 * - CimeAccessor::fields()                // 字段操作
 * - CimeAccessor::records()               // 记录操作
 * - CimeAccessor::block()                 // 块属性操作
 * - FieldMutator/RecordBuilder/BlockMutator // 字段/记录/块操作器
 * - CimeQuery::where(field, value)          // 条件查询
 * - CimeQuery::all()                       // 获取全部数据
 * - CimeQuery::begin/end                    // 迭代器遍历查询结果
 * - RecordProxy                             // 查询结果迭代器返回的记录查看代理
 *
 * =========================
 * 8. 未实现功能
 * =========================
 * - 字段引用
 * - 字段属性继承
 * - 无结构数据描述
 * - 表内嵌套描述
 * - 计算公式
 * - 当前仅实现多列表格（MULTI_COLUMN）
 */
#pragma once
#ifndef XCIME_HEADER
#define XCIME_HEADER
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <fstream>
#include <ctime>
#include <iterator>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

// 启用QString适配，读写不需要额外的 std::string <=> QString 转换
#define CIME_QT
#ifdef CIME_QT
#include <QString>
#include <QByteArray>
#endif

namespace xcime
{
    // ============================================ LOG ============================================
    inline std::ostream &_cime_log_file()
    {
        setlocale(LC_ALL, ""); // 设置本地化环境
        static std::ofstream log_file("cime_log.txt", std::ios::app);
        return log_file;
    }
    inline const char *_cime_now()
    {
        static char buf[20]; // "yyyy-mm-dd hh:mm:ss"
        std::time_t t = std::time(0);
        std::tm *tm = std::localtime(&t);
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
        return buf;
    }

// 打开后日志输出到cime_log.txt
// #define CIME_LOG_TO_FILE
#ifndef CIME_LOG_TO_FILE
#define CIME_LOG_STREAM std::cout
#define CIME_ERR_STREAM std::cerr
#else
#define CIME_LOG_STREAM xcime::_cime_log_file()
#define CIME_ERR_STREAM xcime::_cime_log_file()
#endif
#define CIME_ANSI_RED "\033[31m"
#define CIME_ANSI_RESET "\033[0m"

// 打开后暂停日志输出
// #define CIME_SUPPRESS_LOG
#ifndef CIME_SUPPRESS_LOG
#define _CIME_WRITE(STREAM, COLOR, PREFIX, MSG, RESET)           \
    do                                                           \
    {                                                            \
        std::ostringstream _oss;                                 \
        _oss << MSG;                                             \
        (STREAM) << COLOR << xcime::_cime_now() << ' '          \
                 << PREFIX << __FILE__ << ':' << __LINE__ << ' ' \
                 << _oss.str() << RESET << std::endl;            \
    } while (0)
#else
#define _CIME_WRITE(STREAM, COLOR, PREFIX, MSG, RESET) (void(0))
#endif
// 日志输出宏
#ifdef CIME_LOG_TO_FILE
#define CIME_LOG(MSG) _CIME_WRITE(CIME_LOG_STREAM, "", "[INFO ] ", MSG, "")
#define CIME_ERR(MSG) _CIME_WRITE(CIME_ERR_STREAM, "", "[ERROR] ", MSG, "")
#else
#define CIME_LOG(MSG) _CIME_WRITE(CIME_LOG_STREAM, "", "[INFO ] ", MSG, "")
#define CIME_ERR(MSG) _CIME_WRITE(CIME_ERR_STREAM, CIME_ANSI_RED, "[ERROR] ", MSG, CIME_ANSI_RESET)
#endif

    // 空格字符定义
    const char SPACE_CHAR = '\t';
    namespace details
    {
        // 类型适配
        template <typename T>
        inline std::string value_cast(const T& v)
        {
            std::ostringstream oss;
            oss << v;
            return oss.str();
        }
        // 常用变量
        inline std::string value_cast(const std::string& s) { return s; }
        inline std::string value_cast(const char* s) { return s; }
        inline std::string value_cast(const bool v) { return v ? "true" : "false"; }

        /**
         * @brief 按空白字符分割字符串为多个子串
         *
         * 该函数将输入字符串按空白字符（空格、Tab等）分割为若干个子串，并以 vector<string> 形式返回。
         * 常用于解析类似“@ 字段1 字段2 ...”或“/@ 别名1 别名2 ...”等格式的行。
         *
         * @param str 待分割的字符串
         * @return std::vector<std::string> 分割后的子串数组
         */
        inline std::vector<std::string> splitTokens(const std::string& str)
        {
            std::vector<std::string> tokens;
            size_t start = 0;

            while (start <= str.size())
            {
                size_t pos = str.find(SPACE_CHAR, start);
                if (start == 0 && pos == 0)
                {
                    start = 1;
                    continue;
                }
                if (pos == std::string::npos)
                {
                    // 最后一个字段（可能为空）
                    tokens.push_back(str.substr(start));
                    break;
                }
                tokens.push_back(str.substr(start, pos - start));
                start = pos + 1; // 跳过分隔符
            }
            return tokens;
        }
    };
#ifdef CIME_QT
    namespace details
    {
        inline std::string value_cast(const QString& s) { return std::string(s.toLocal8Bit().constData()); }
    };
#endif
    // =================================================================================================

    class Cime;
    class CimeBlock;
    class FieldBuilder;
    class RecordBuilder;
    /**
     * @brief 表格类型枚举
     */
    enum TableType
    {
        HORIZONTAL,    // 横向表格
        SINGLE_COLUMN, // 单列表格
        MULTI_COLUMN   // 多列表格
    };

    enum FieldType
    {
        INT,
        FLOAT,
        STRING,
        POINTER
    };
    /**
     * @brief 数据块字段结构体
     *
     * 用于描述数据块中单个字段的属性，如名称、类型、长度等。
     */
    struct CimeField
    {
        void setType(const std::string &sType)
        {
            if (sType == "i")
            {
                type = INT;
            }
            else if (sType == "f")
            {
                type = FLOAT;
            }
            else if (sType == "s")
            {
                type = STRING;
            }
            else if (sType == "p")
            {
                type = POINTER;
            }
            else
            {
                CIME_ERR("Unknown field type: " << sType);
                type = STRING; // 默认类型为字符串
            }
        }
        std::string Type() const
        {
            switch(type)
            {
                case INT: return "i";
                case FLOAT: return "f";
                case STRING: return "s";
                case POINTER: return "p";
            }
            return "";
        }
        std::string name;       ///< 字段名称
        std::string alias;      ///< 别名，默认为空
        FieldType type;       ///< 字段类型：如 s（字符串），i（整数）等，默认为s
        std::string length;     ///< 字段长度，0表示变长，默认为0
        std::string dimension;  ///< 字段量纲
        std::string valueLimit; ///< 字段值限制
        bool isReference;       ///< 是否是引用字段
        std::string refTable;   ///< 外部引用表名
        std::string refField;   ///< 外部引用字段名
        bool isAutoIncrement;   ///< 是否自增字段
    };

    /**
     * @brief 数据块记录结构体
     *
     * 用于描述数据块中一条记录的数据，各字段的值存储在 values 中。
     */
    struct CimeRecord
    {
        explicit CimeRecord(CimeBlock *blk) : m_owner(blk) {}

        const std::string &operator[](const std::string &field) const;
        std::string &operator[](const std::string &field);
        template <typename T>
        const std::string& operator[](const T& field) const;
        template <typename T>
        std::string& operator[](const T& field);
        std::vector<std::string> values;         ///< 记录中各字段的值
        std::vector<std::string> sparsePointers; ///< 稀疏记录指针（如有）
    private:
        CimeBlock *m_owner;
    };

    /**
     * @brief 数据块类
     *
     * 该类用于存储一个数据块的信息，包括字段定义及记录数据，
     * 同时提供对这些数据进行增添、查询等操作的接口。
     */
    class CimeBlock
    {
        // 操作类友元
        friend class RecordAccessorHelper; // 数据块记录访问器类，增删操作。读取操作由查询类提供
        friend class BlockMutator;       // 数据块基础属性操作类
        friend class RecordProxy;        // 数据块记录访问代理类，作为查询结果的迭代器返回
        friend class FieldMutator;       // 数据块字段操作类
        friend class RecordBuilder;      // 数据块记录添加类
        friend class AttributeMutator;   // 数据块属性操作类
    public:
        /**
         * @brief 构造函数
         * @param name 数据块名称
         * @param description 数据块描述
         */
        CimeBlock(const std::string &name, const std::string &description);

        /// 设置数据块名称
        void setName(const std::string &name);
        /// 设置数据块描述
        void setDescription(const std::string &description);
        /// 获取数据块完整名称（包括命名空间）
        std::string getFullName() const;
        /// 获取字段定义集合
        const std::vector<CimeField> &getFields() const;
        /// 获取记录集合
        const std::vector<CimeRecord> &getRecords() const;
        /// 设置表格类型
        void setTableType(TableType type);
        /// 设置父指针
        void setParent(Cime *parent);
        /// 获取表格类型
        TableType getTableType() const;
        /// 获取数据块属性字符串
        std::string getAttributes() const;
        /// 设置是否显示字段类型
        void setTypeVisible(bool visible) { m_isTypeVisible = visible; }
        /// 设置是否显示字段单位
        void setUnitVisible(bool visible) { m_isUnitVisible = visible; }
        /// 设置是否显示字段值限制
        void setValueLimitVisible(bool visible) { m_isValueLimitVisible = visible; }
        /// 设置是否显示别名
        void setAliasVisible(bool visible) { m_isAliasVisible = visible; }
        /// 判断是否显示字段类型
        bool isTypeVisible() const { return m_isTypeVisible; }
        /// 判断是否显示字段单位
        bool isUnitVisible() const { return m_isUnitVisible; }
        /// 判断是否显示字段值限制
        bool isValueLimitVisible() const { return m_isValueLimitVisible; }
        /// 设置是否显示字段别名
        bool isAliasVisible() const { return m_isAliasVisible; }
        /// 获取字段列号
        int fieldIndex(const std::string &fieldName) const
        {
            std::map<std::string, int>::const_iterator it = m_fieldIndexMap.find(fieldName);
            return it == m_fieldIndexMap.end() ? -1 : it->second;
        }

        std::map<std::string, int> &getFieldIndexMap() { return m_fieldIndexMap; }
        std::map<std::string, int> &getAutoIncrementMap() { return m_autoIncrementMap; }

    private:
        std::string m_fullName;                                      ///< 数据块名称
        std::string m_description;                               ///< 数据块描述
        std::vector<std::string> m_namespacePath;                ///< 数据块命名空间路径
        std::vector<CimeField> m_fields;                         ///< 字段定义集合
        std::vector<CimeRecord> m_records;                       ///< 记录集合
        std::map<std::string, std::string> m_attributes;   ///< 数据块属性集合
        std::map<std::string, int> m_fieldIndexMap;    ///< 记录字段的列索引, [fieldName, col]
        std::map<std::string, int> m_autoIncrementMap; ///< 自增字段的列索引, [fieldName, col]
        bool m_isTypeVisible;                                    ///< 是否显示字段类型标识
        bool m_isUnitVisible;                                    ///< 是否显示字段单位
        bool m_isValueLimitVisible;                              ///< 是否显示字段值限制
        bool m_isAliasVisible;                                  ///< 是否显示字段别名
        TableType m_tableType;                                   ///< 表格类型
        Cime* m_parent;                                           ///< Cime指针
    };

    // ----------------- CimeBlock inline 实现 -----------------
    inline CimeBlock::CimeBlock(const std::string &name, const std::string &description)
        : m_fullName(name), m_description(description), m_isTypeVisible(false),
          m_isUnitVisible(false), m_isValueLimitVisible(false), m_isAliasVisible(false), m_tableType(HORIZONTAL), m_parent(NULL)
    {
    }

    inline void CimeBlock::setName(const std::string &name)
    {
        m_fullName = name;
    }

    inline void CimeBlock::setDescription(const std::string &description)
    {
        m_description = description;
    }

    inline std::string CimeBlock::getFullName() const
    {
        std::stringstream ss;
        for (size_t i = 0; i < m_namespacePath.size(); ++i)
        {
            ss << m_namespacePath[i];
            if (i < m_namespacePath.size() - 1)
            {
                ss << "::";
            }
        }
        if (!m_namespacePath.empty())
        {
            ss << "::";
        }
        ss << m_fullName;
        return ss.str();
    }

    inline const std::vector<CimeField> &CimeBlock::getFields() const
    {
        return m_fields;
    }

    inline const std::vector<CimeRecord> &CimeBlock::getRecords() const
    {
        return m_records;
    }

    inline void CimeBlock::setParent(Cime *parent)
    {
        m_parent = parent;
    }

    inline void CimeBlock::setTableType(TableType type)
    {
        m_tableType = type;
    }

    inline TableType CimeBlock::getTableType() const
    {
        return m_tableType;
    }

    inline std::string CimeBlock::getAttributes() const
    {
        std::stringstream ss;
        for (std::map<std::string, std::string>::const_iterator attr = m_attributes.begin(); attr != m_attributes.end();)
        {
            ss << attr->first << "=\"" << attr->second << "\"";
            if (++attr != m_attributes.end())
            {
                ss << " ";
            }
        }
        return ss.str();
    }

    inline const std::string &CimeRecord::operator[](const std::string &field) const
    {
        try
        {
            return values[m_owner->fieldIndex(field)];
        }catch (const std::out_of_range &e)
        {
            CIME_ERR("fieldName: " << field << " not exist, fields are: ");
            throw;
        }
    }

    inline std::string &CimeRecord::operator[](const std::string &field)
    {
        try
        {
            return values[m_owner->fieldIndex(field)];
        }
        catch (const std::out_of_range &e)
        {
            CIME_ERR("fieldName: " << field << " not exist");
            throw;
        }
    }

	template<typename T>
	inline const std::string& CimeRecord::operator[](const T& field) const
	{
        std::string fieldName = details::value_cast(field);
		try
		{
			return values[m_owner->fieldIndex(fieldName)];
		}
		catch (const std::out_of_range& e)
		{
			CIME_ERR("fieldName: " << field << " not exist");
			throw;
		}
	}

	template<typename T>
	inline std::string& CimeRecord::operator[](const T& field)
	{
		std::string fieldName = details::value_cast(field);
		try
		{
			return values[m_owner->fieldIndex(fieldName)];
		}
		catch (const std::out_of_range& e)
		{
			CIME_ERR("fieldName: " << field << " not exist");
			throw;
		}
	}


    /**
     * @brief Field增改操作类
     *
     */
    class FieldMutator
    {
    public:
        FieldMutator(CimeBlock *_blk, const std::string &name)
            : m_block(_blk), m_isFresh(true)
        {
            if (m_block->m_fieldIndexMap.find(name) != m_block->m_fieldIndexMap.end())
            {
                m_isFresh = false;
                m_col = m_block->fieldIndex(name);
            }
            else
            {
                // 按默认设定新建字段
                makeDefault(name);
            }
        }

        /// 设置字段名
        FieldMutator &rename(const std::string &name)
        {
            // 修改旧字段列号映射
            CimeField &field = m_block->m_fields[m_col];
            m_block->m_fieldIndexMap.erase(field.name);
            m_block->m_fieldIndexMap[name] = static_cast<int>(m_col);
            // 修改旧自增字段映射
            if (field.isAutoIncrement)
            {
                m_block->m_autoIncrementMap.erase(field.name);
                m_block->m_autoIncrementMap[name] = static_cast<int>(m_col);
            }
            field.name = name;
            return *this;
        }

        /**
         * @brief 设置字段变量类型，仅支持 INT、FLOAT、STRING、POINTER
         * 
         * @param type enum xcime::FieldType
         * @return FieldMutator& 
         */
        FieldMutator &type(const FieldType &type)
        {
            m_block->m_fields[m_col].type = type;
            return *this;
        }

        FieldMutator &length(const std::string &length)
        {
            m_block->m_fields[m_col].length = length;
            return *this;
        }

        FieldMutator &dimension(const std::string &dimension)
        {
            m_block->m_fields[m_col].dimension = dimension;
            return *this;
        }

        FieldMutator &valueLimit(const std::string &valueLimit)
        {
            m_block->m_fields[m_col].valueLimit = valueLimit;
            return *this;
        }

        FieldMutator &refernceTo(const std::string &tbl, const std::string &fld)
        {
            m_block->m_fields[m_col].isReference = true;
            m_block->m_fields[m_col].refTable = tbl;
            m_block->m_fields[m_col].refField = fld;
            return *this;
        }
        
        /**
         * @brief 设置自增，仅支持INT类型字段
         * 
         * @param autoInc 
         * @return FieldMutator& 
         */
        FieldMutator &setAutoIncrement(bool autoInc)
        {
            if(m_block->m_fields[m_col].type == INT)
            {
                m_block->m_fields[m_col].isAutoIncrement = autoInc;
                m_block->m_autoIncrementMap[m_block->m_fields[m_col].name] = static_cast<int>(m_col);
            }else
            {
                CIME_ERR("Field " << m_block->m_fields[m_col].name << " type must be INT for auto increment");
            }
            return *this;
        }

        /**
         * @brief 设置别名
         * 
         * @param alias 
         * @return FieldMutator& 
         */
        FieldMutator& setAlias(const std::string alias)
        {
            m_block->m_fields[m_col].alias = alias;
            return *this;
        }

        // 删除字段
        void drop()
        {
            // 删除数据块中该字段列所有记录的值
            for (std::vector<CimeRecord>::iterator it = m_block->m_records.begin(); it != m_block->m_records.end(); ++it)
            {
                it->values.erase(it->values.begin() + m_col);
            }
            // 删除字段列映射
            m_block->m_fieldIndexMap.erase(m_block->m_fields[m_col].name);
            // 删除自增字段映射
            if (m_block->m_fields[m_col].isAutoIncrement)
            {
                m_block->m_autoIncrementMap.erase(m_block->m_fields[m_col].name);
            }
            // 删除字段
            m_block->m_fields.erase(m_block->m_fields.begin() + m_col);
            // 更新后续字段列号映射
            for (std::map<std::string, int>::iterator it = m_block->m_fieldIndexMap.begin(); it != m_block->m_fieldIndexMap.end(); ++it)
            {
                if (it->second > static_cast<int>(m_col))
                {
                    it->second -= 1; // 更新后续字段列号
                }
            }
        }
    protected:
        void makeDefault(const std::string &name)
        {
            CimeField newField;
            m_col = m_block->m_fields.size();
            newField.name = name;
            newField.alias = "";
            newField.type = STRING;
            newField.length = "0";
            newField.dimension = "";
            newField.valueLimit = "";
            newField.isReference = false;
            newField.isAutoIncrement = false;
            m_block->m_fields.push_back(newField);
            m_block->m_fieldIndexMap[name] = static_cast<int>(m_col);
        }

    protected:
        CimeBlock *m_block; ///< 数据块指针
        size_t m_col;       ///< 字段列号
        bool m_isFresh;     ///< 是否是新字段
    };

    class RecordBuilder
    {
    public:
        RecordBuilder(CimeBlock *blk)
            : m_block(blk), m_r(blk), m_committed(false)
        {
            m_r.values.resize(blk->getFields().size(), ""); // 按字段数量初始化单条记录大小
        }
        ~RecordBuilder()
        {
            if (!m_committed)
                commit();
        }
        void commit()
        {
            if (!m_committed)
            {
                if (!m_block->getAutoIncrementMap().empty())
                {
                    // 处理自增字段
                    for (std::map<std::string, int>::iterator it = m_block->getAutoIncrementMap().begin(); it != m_block->getAutoIncrementMap().end(); ++it)
                    {
                        // 自增字段值为最后一条记录同字段值加1
                        int col = it->second;
                        if (m_block->m_records.size() == 0)
                        {
                            m_r.values[col] = "1";
                        }
                        else
                        {
                            std::string lastVal = m_block->m_records.back().values[col];
                            char buf[32];
                            int n = atoi(lastVal.c_str());
                            sprintf(buf, "%d", (n + 1));
                            m_r.values[col] = buf;
                        }
                    }
                }
                m_block->m_records.push_back(m_r);
                m_committed = true;
            }
        }

        RecordBuilder &set(const std::string &fieldName, const std::string &value)
        {
            assign(fieldName, value);
            return *this;
        }
        template <typename T>
        RecordBuilder& set(const std::string& fieldName, const T& val)
        {
            assign(fieldName, details::value_cast(val));
            return *this;
        }

        RecordBuilder &set(const size_t index, const std::string &value)
        {
            if (index >= m_r.values.size())
            {
                CIME_ERR("Index out of range: " << index);
                return *this;
            }
            m_r.values[index] = value;
            return *this;
        }

    private:
        void assign(const std::string& fieldName, const std::string & value)
        {
			int index = m_block->fieldIndex(fieldName);
			if (index < 0)
			{
				CIME_ERR("fieldName: " << fieldName << " not exist");
				return;
			}
			m_r.values[index] = value;
        }
        CimeBlock *m_block; ///< 数据块指针
        CimeRecord m_r;     ///< 记录数据
        bool m_committed;   ///< 标识记录是否已提交
    };

    class BlockMutator
    {
    public:
        BlockMutator(CimeBlock *blk)
            : m_block(blk) {}
        /**
         * @brief 设置/修改数据块全名
         * 
         * @param name xxx::zzz
         * @return BlockMutator& 
         */
        BlockMutator &setFullName(const std::string &name)
        {
            m_block->m_fullName = name;
            return *this;
        }
        /**
         * @brief 设置数据块描述，该属性暂作占位，未使用
         * 
         * @param description 
         * @return BlockMutator& 
         */
        BlockMutator &setDescription(const std::string &description)
        {
            m_block->m_description = description;
            return *this;
        }

        BlockMutator &setTableType(TableType type)
        {
            m_block->setTableType(type);
            return *this;
        }
        BlockMutator &setTypeVisible(bool visible)
        {
            m_block->setTypeVisible(visible);
            return *this;
        }
        BlockMutator &setUnitVisible(bool visible)
        {
            m_block->setUnitVisible(visible);
            return *this;
        }
        BlockMutator &setValueLimitVisible(bool visible)
        {
            m_block->setValueLimitVisible(visible);
            return *this;
        }
        BlockMutator &setAliasVisible(bool visible)
        {
            m_block->m_isAliasVisible = visible;
            return *this;
        }
        BlockMutator &setAttribute(const std::string &key, const std::string &value)
        {
            m_block->m_attributes[key] = value;
            return *this;
        }

        /**
         * @brief 设置字段列号，当指定列号大于当前字段数量时，设置为最后一列；当指定为已有字段时，后续字段全部增加
         * 
         * @param fieldName 
         * @param col 
         * @return BlockMutator& 
         */
        BlockMutator &setFieldIndex(const std::string &fieldName, size_t col)
        {
            if (m_block->m_fieldIndexMap.find(fieldName) != m_block->m_fieldIndexMap.end())
            {
                CIME_ERR("Field name: " << fieldName << " already exists");
                return *this;
            }
            if(col <= m_block->m_fieldIndexMap.size())
            {
                // 列号小于当前列数，是修改列顺序，修改后续全部列顺序
                for(std::map<std::string, int>::iterator it = m_block->m_fieldIndexMap.begin(); it != m_block->m_fieldIndexMap.end(); ++it)
                {
                    if(it->second >= (int)col && it->second < (int)col)
                    {
                        // 当前插入的位置之后，原列号之前的全部加1
                        it->second += 1;
                    }
                }
            }
            else
            {
                // 列号大于当前列数，新增列，固定列数为最后一列
                col = m_block->m_fieldIndexMap.size();
            }
            // 设置指定列号
            m_block->m_fieldIndexMap[fieldName] = col;
            return *this;
        }

        // getter
        std::string attribute(const std::string &key) const
        {
            std::map<std::string, std::string>::iterator it = m_block->m_attributes.find(key);
            if (it != m_block->m_attributes.end())
            {
                return it->second;
            }
            else
            {
                CIME_ERR("Attribute key: " << key << " not found");
                return "";
            }
        }

        void drop();
    private:
        CimeBlock *m_block; ///< 数据块指针
    };

    class BaseAccessorHelper
    {
    protected:
        explicit BaseAccessorHelper(CimeBlock *blk) : m_block(blk) {}
        CimeBlock *m_block; ///< 数据块指针
    };

    class FieldAccessorHelper : public BaseAccessorHelper
    {
    public:
        FieldAccessorHelper(CimeBlock *blk) : BaseAccessorHelper(blk) {}
        /**
         * @brief 快速添加字段，使用默认字段设置（不支持重名字段）
         *
         * @param name
         * @return FieldAccessorHelper&
         */
        FieldAccessorHelper &add(const std::string &name)
        {
            FieldMutator(m_block, name);
            return *this;
        }
        /**
         * @brief 添加/修改字段，不可链式添加
         *
         * @param name
         * @return FieldMutator
         */
        FieldMutator field(const std::string &name)
        {
            return FieldMutator(m_block, name);
        }
        /**
         * @brief 添加/修改字段，不可链式添加
         *
         * @param name
         * @return FieldMutator
         */
        FieldMutator operator[](const std::string& name)
        {
            return FieldMutator(m_block, name);
        }
        template <typename T>
        FieldMutator operator[](const T& name)
        {
            return FieldMutator(m_block, details::value_cast(name));
        }
        FieldAccessorHelper& operator<<(const std::string& name)
        {
            FieldMutator(m_block, name);
            return *this;
        }
    };

    // 记录访问器代理类，可以使用字段名访问对应记录
    struct RecordProxy
    {
        CimeBlock *blk; ///< 数据块指针
        size_t row;     ///< 记录的行号
        RecordProxy()
        {
            blk = NULL;
            row = 0;
        }
        RecordProxy(CimeBlock* _blk, size_t _row) : blk(_blk), row(_row) {}
#ifdef CIME_QT
		QString operator[](const std::string& field)
		{
			return getRecord(field);
		}
		const QString operator[](const std::string& field) const
		{
			return getRecord(field);
		}
		template <typename T>
        QString operator[](const T& field)
		{
			std::string fieldName = details::value_cast(field);
			return getRecord(field);
		}
		template <typename T>
		const QString operator[](const T& field) const
		{
			std::string fieldName = details::value_cast(field);
			return getRecord(field);
		}
    private:
		QString getRecord(const std::string& field)
		{
			try
			{
				return QString::fromLocal8Bit(blk->m_records[row].values[blk->fieldIndex(field)].c_str());
			}
			catch (const std::out_of_range& e)
			{
				CIME_ERR("fieldName: " << field << " not exist");
                return "";
			}
		}
        QString getRecord(const std::string& field) const
		{
			try
			{
				return QString::fromLocal8Bit(blk->m_records[row].values[blk->fieldIndex(field)].c_str());
			}
			catch (const std::out_of_range& e)
			{
				CIME_ERR("fieldName: " << field << " not exist");
                return "";
			}
		}
#else
        std::string operator[](const std::string &field)
        {
            return getRecord(field);
        }
        const std::string operator[](const std::string &field) const
        {
            return getRecord(field);
        }
        template <typename T>
		std::string operator[](const T& field)
        {
            std::string fieldName = details::value_cast(field);
            return getRecord(field);
        }
        template <typename T>
        const std::string operator[](const T& field) const
        {
            std::string fieldName = details::value_cast(field);
            return getRecord(field);
        }
    private:
        std::string getRecord(const std::string &field)
        {
			try
			{
				return blk->m_records[row].values[blk->fieldIndex(field)];
			}
			catch (const std::out_of_range& e)
			{
				CIME_ERR("fieldName: " << field << " not exist");
				throw;
			}
        }
		std::string getRecord(const std::string& field) const
		{
			try
			{
				return blk->m_records[row].values[blk->fieldIndex(field)];
			}
			catch (const std::out_of_range& e)
			{
				CIME_ERR("fieldName: " << field << " not exist");
				throw;
			}
		}
#endif
    };

    /**
     * @brief 查询结果迭代器
     *
     * @tparam VecIter
     */
    template <typename VecIter>
    class CimeQueryIterator
    {
        typedef std::ptrdiff_t difference_type;
        typedef std::forward_iterator_tag iterator_category;

    public:
        CimeQueryIterator(CimeBlock *blk, VecIter base)
            : blk(blk), base(base) {}
        CimeQueryIterator &operator++()
        {
            ++base;
            return *this;
        }
        CimeQueryIterator &operator--()
        {
            --base;
            return *this;
        }
        bool operator==(const CimeQueryIterator &o) const { return base == o.base; }
        bool operator!=(const CimeQueryIterator &o) const { return base != o.base; }
        RecordProxy operator*() const 
        { 
            RecordProxy proxy(blk, *base);
            return proxy; 
        }

    private:
        CimeBlock *blk;
        VecIter base; ///< 行号it
    };
    /**
     * @brief 查询类
     *
     * 为 CimeBlock 提供查询功能，以检索满足条件的记录。
     * 每次execute会清空上一次的查询结果。
     * 支持链式 where 调用，最终通过 all() 方法获取结果集。
     */
    class CimeQuery
    {
    public:
        typedef std::pair<std::string, std::string> Criteria;                          ///< 查询条件类型：字段名及期望值
        typedef std::vector<Criteria> CriteriaList;                                    ///< 查询条件列表类型
        typedef CimeQueryIterator<std::vector<size_t>::iterator> iterator;             ///< 常规迭代器
        typedef CimeQueryIterator<std::vector<size_t>::const_iterator> const_iterator; ///< 常量迭代器
        /**
         * @brief 构造函数
         * @param block 目标数据块指针
         */
        explicit CimeQuery(CimeBlock *block) : m_block(block), m_executed(false) {}
        /**
        * @brief 获取迭代器开始位置
        */
        iterator begin()
        {
            if (!m_executed)
                execute();
            return iterator(m_block, m_rowList.begin());
        }
		/**
		* @brief 获取迭代器结束位置
		*/
        iterator end()
        {
            if (!m_executed)
                execute();
            return iterator(m_block, m_rowList.end());
        }
		/**
		* @brief 获取常量迭代器开始位置
		*/
        const_iterator cbegin() const
        {
            if (!m_executed)
                execute();
            return const_iterator(m_block, m_rowList.begin());
        }
		/**
		* @brief 获取常量迭代器结束位置
		*/
        const_iterator cend() const
        {
            if (!m_executed)
                execute();
            return const_iterator(m_block, m_rowList.end());
        }
		/**
		* @brief 获取首位结果，若结果集为空则返回空记录访问器
		* @return 首位记录访问器
		*/
        RecordProxy first() const
        {
            if (!m_executed)
                execute();
            if (m_rowList.empty())
                return RecordProxy();
            RecordProxy record(m_block, m_rowList[0]);
            return record;
            
        }
        RecordProxy last() const
        {
			if (!m_executed)
				execute();
			if (m_rowList.empty())
				return RecordProxy();
			RecordProxy record(m_block, m_rowList[m_rowList.size() - 1]);
			return record;
        }
		/**
		* @brief 根据结果行号获取查询结果
        * @return 指定的记录访问器
		*/
        RecordProxy operator[](size_t index)
        {
            // m_block->getRecords()
            if (index >= m_rowList.size())
            {
                CIME_ERR("Index out of range: " << index);
                throw;
            }
            RecordProxy proxy(m_block, m_rowList[index]);
            return proxy;
        }
        /**
         * @brief 添加单一条件查询
         * @param fieldName 查询字段名称
         * @param value 查询期望值
         * @return 当前查询对象引用，支持链式调用
         */
        CimeQuery &where(const std::string &fieldName, const std::string &value)
        {
            // 添加条件后需重新执行
            m_executed = false;
            // fieldName合法性检测
            if (m_block->getFieldIndexMap().find(fieldName) == m_block->getFieldIndexMap().end())
            {
                CIME_ERR("fieldName: " << fieldName << " not exist");
                return *this;
            }
            m_criteriaList.push_back(std::make_pair(fieldName, value));
            return *this;
        }
        template <typename T>
        CimeQuery& where(const std::string& fieldName, const T& value)
        {
            return where(fieldName, details::value_cast(value));
        }
        /**
         * @brief 添加单一条件查询（const重载）
         * @param fieldName 查询字段名称
         * @param value 查询期望值
         * @return 当前查询对象引用，支持链式调用
         */
        CimeQuery &where(const std::string &fieldName, const std::string &value) const
        {
            return const_cast<CimeQuery &>(this->where(fieldName, value));
        }

        /**
         * @brief 执行查询，筛选出满足条件的记录。执行后清空原来的条件组
         */
        void execute() const
        {
            m_executed = true;
            m_rowList.clear();
            const std::vector<CimeRecord> &records = m_block->getRecords();
            for (size_t i = 0; i < records.size(); ++i)
            {
                const CimeRecord &record = records[i];
                bool match = true;
                for (CriteriaList::const_iterator criteriaIt = m_criteriaList.begin(); criteriaIt != m_criteriaList.end(); ++criteriaIt)
                {
                    const std::string &field = criteriaIt->first;
                    const std::string &value = criteriaIt->second;
                    int col = m_block->fieldIndex(field);
                    if (col < 0 || record.values[col] != value)
                    {
                        match = false;
                        break; // 不匹配则跳出
                    }
                }
                if (match)
                {
                    m_rowList.push_back(i); // 记录匹配行号
                }
            }
            m_criteriaList.clear();
        }
        /**
         * @brief 获取匹配条件的结果集
         * @return 包含所有匹配记录的指针数组
         */
        void all()
        {
            m_executed = true;
            m_rowList.clear();
            const std::vector<CimeRecord> &records = m_block->getRecords();
            // 返回全部记录
            for (size_t i = 0; i < records.size(); ++i)
            {
                m_rowList.push_back(i);
            }
        }

        /**
        * @brief 获取结果集记录数量
        * @return 结果集记录数量
        */
        size_t size() const
        {
            return m_rowList.size();
        }

        bool empty() const
        {
            return m_rowList.empty();
        }

    private:
        mutable CriteriaList m_criteriaList; ///< 查询条件列表
        CimeBlock *m_block;                   ///< 目标数据块指针
        mutable std::vector<size_t> m_rowList; ///< 匹配记录列表
        mutable bool m_executed;               ///< 查询是否已执行标志
    };

    class RecordAccessorHelper : public BaseAccessorHelper
    {
    public:
        typedef std::vector<CimeRecord>::iterator iterator; ///< 记录迭代器类型
        typedef std::vector<CimeRecord>::const_iterator const_iterator; ///< 常量记录迭代器类型
        RecordAccessorHelper(CimeBlock *blk) : BaseAccessorHelper(blk) {}
        /**
         * @brief 添加记录
         *
         * @return RecordBuilder
         */
        RecordBuilder add()
        {
            return RecordBuilder(m_block);
        }

        /**
         * @brief 根据指定字段排序记录
         * @param field 字段名称
         * @param ascending 排序顺序，true表示升序
         * @return 链式调用引用
         */
        RecordAccessorHelper &sortBy(const std::string &field, bool ascending = true)
        {
            int col = m_block->fieldIndex(field);
            if (col < 0)
            {
                CIME_ERR("fieldName: " << field << " not exist");
                return *this;
            }

            return sortBy(static_cast<size_t>(col), ascending);
        }

        /**
         * @brief 根据指定列号排序记录
         * @param column 列号，从0开始
         * @param ascending 排序顺序，true表示升序
         * @return 链式调用引用
         */
        RecordAccessorHelper &sortBy(size_t column, bool ascending = true)
        {
            if (column >= m_block->m_fields.size())
            {
                CIME_ERR("column index out of range: " << column);
                return *this;
            }

            struct SortComparator
            {
                size_t col;
                bool asc;
                SortComparator(size_t c, bool a) : col(c), asc(a) {}
                const std::string &valueOf(const CimeRecord &rec) const
                {
                    static const std::string empty;
                    if (col >= rec.values.size())
                        return empty;
                    return rec.values[col];
                }
                bool operator()(const CimeRecord &lhs, const CimeRecord &rhs) const
                {
                    const std::string &lv = valueOf(lhs);
                    const std::string &rv = valueOf(rhs);
                    if (asc)
                        return lv < rv;
                    return lv > rv;
                }
            };

            std::stable_sort(m_block->m_records.begin(), m_block->m_records.end(), SortComparator(column, ascending));
            return *this;
        }

        template <typename T>
        RecordAccessorHelper &sortBy(const T &field, bool ascending = true)
        {
            return sortBy(details::value_cast(field), ascending);
        }

        iterator erase(iterator pos)
        {
            return m_block->m_records.erase(pos);
        }

        size_t erase(size_t row)
        {
            if (row >= m_block->m_records.size())
            {
                CIME_ERR("Index out of range: " << row);
                return 0;
            }
            m_block->m_records.erase(m_block->m_records.begin() + row);
            return 1;
        }

        size_t erase(const std::string &field, const std::string &value)
        {
            int col = m_block->fieldIndex(field);
            if(col < 0)
            {
                CIME_ERR("fieldName: " << field << " not exist");
                return 0;
            }
            int removed = 0;
            for(size_t i = m_block->m_records.size(); i-- > 0;)
            {
                if (m_block->m_records[i].values[col] == value)
                {
                    m_block->m_records.erase(m_block->m_records.begin() + i);
                    ++removed;
                }
            }
            return removed;
        }

        size_t erase(CimeQuery &query)
        {
            int removed = 0;
            for(CimeQuery::iterator it = query.begin(); it != query.end(); ++it)
            {
                RecordProxy record = *it;
                removed += erase(record.row);
            }
            return removed;
        }
    };

    

    /**
     * @brief 辅助访问器类
     * 解析CIM/E的数据块嵌套访问，
     * 支持通过隐式转换获得对应的 CimeBlock 指针进行直接操作。
     */
    class CimeAccessor
    {
    public:
        /**
         * @brief 构造函数
         * @param mgr 指向 CIM/E 管理器的指针
         * @param blockName 当前块的全名或命名空间
         */
        CimeAccessor(class Cime *mgr, const std::string &blockName)
            : m_mgr(mgr), m_blockName(blockName) {}
        /**
         * @brief 嵌套访问运算符
         *
         * 通过将传入的 key 与当前块名以 "::" 拼接，生成新的 CimeAccessor 对象，
         * 用于访问子块。
         * @param key 子块的名称
         * @return 更新块名后的 CimeAccessor 对象
         */
        CimeAccessor operator[](const std::string &key) const
        {
            std::string newName = m_blockName.empty() ? key : m_blockName + "::" + key;
            return CimeAccessor(m_mgr, newName);
        }
        template <typename T>
        CimeAccessor operator[](const T& key) const
        {
            std::string strKey = details::value_cast(key);
			std::string newName = m_blockName.empty() ? strKey : m_blockName + "::" + strKey;
			return CimeAccessor(m_mgr, newName);
        }
        /**
         * @brief 隐式转换运算符
         *
         * 允许将 CimeAccessor 对象直接转换为 CimeBlock 指针，
         * 如果对应块不存在则自动创建一个空数据块。
         * @return 当前块对应的 CimeBlock 指针
         */
        operator CimeBlock *() const;
        /**
         * @brief 获取记录操作辅助对象
         *
         * 用于对当前数据块中记录的访问和操作。
         * @return 用于记录操作的辅助对象 RecordAccessorHelper
         */
        RecordAccessorHelper records() const;
        /**
         * @brief 获取字段操作辅助对象
         *
         * 用于对当前数据块中字段的添加与修改。
         * @return 用于字段操作的辅助对象 FieldAccessorHelper
         */
        FieldAccessorHelper fields() const;

        // AttributeMutator attributes() const;

        BlockMutator block() const;

    private:
        class Cime *m_mgr;       ///< 指向 CIM/E 管理器的指针
        std::string m_blockName; ///< 当前块全名或命名空间
    };

    /**
     * @brief CIM/E 管理器类
     *
     * 负责整个系统中数据块的创建、存储、检索和持久化，
     * 并提供 JSON 风格的接口供用户操作各个块。
     */
    class Cime
    {
    public:
        typedef std::pair<std::string, CimeBlock *> BlockEntry;
        typedef std::vector<BlockEntry> BlockContainer;
        /// 构造函数
        Cime();
        /// 析构函数，负责释放所有创建的数据块
        ~Cime();
        /**
         * @brief 创建一个新的数据块
         * @param name 数据块名称
         * @param description 数据块描述，默认为空字符串
         * @return 创建好的数据块指针
         */
        CimeBlock *createBlock(const std::string &name, const std::string &description = "");
        void dropBlock(const std::string &name)
        {
            for (BlockContainer::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
            {
                if (it->first == name)
                {
                    delete it->second;
                    m_blocks.erase(it);
                    break;
                }
            }
        }
        /// 添加已有数据块到管理器中
        void addBlock(CimeBlock *block);
        /**
         * @brief 根据名称获取数据块
         * @param name 数据块名称（全名或命名空间）
         * @return 对应的数据块指针，如果不存在则返回 0
         */
        CimeBlock *getBlock(const std::string &name) const;
        /**
         * @brief 将所有数据块保存到指定文件
         * @param filename 文件名（路径）
         * @return 保存成功返回 true，否则返回 false
         */
        bool saveToFile(const std::string &filename) const;
        /**
         * @brief 从指定文件加载数据块
         * @param filename 文件名（路径）
         * @return 加载成功返回 true，否则返回 false
		 */
        bool loadFromFile(const std::string &filename);

        /// 获取所有数据块的映射
        const BlockContainer &getBlocks() const;

        /**
         * @brief JSON 风格接口运算符
         *
         * 返回一个 CimeAccessor 对象，用于嵌套访问数据块、字段以及记录等操作。
         * @param blockName 数据块名称或命名空间
         * @return 对应的 CimeAccessor 对象
         */
        CimeAccessor operator[](const std::string &blockName)
        {
            return CimeAccessor(this, blockName);
        }
        template <typename T>
        CimeAccessor operator[](const T& blockName)
        {
            return CimeAccessor(this, details::value_cast(blockName));
        }


    private:
        BlockContainer m_blocks; ///< 存储所有数据块，保持插入顺序
        /**
         * @brief 将单个数据块转换为 CSV 格式输出
         * @param os 输出流
         * @param block 数据块指针
         */
        void blockToCsv(std::ostream &os, const CimeBlock *block) const;
    };

    // ----------------- CimeAccessor 的 inline 实现 -----------------
    inline FieldAccessorHelper CimeAccessor::fields() const
    {
        CimeBlock *block = m_mgr->getBlock(m_blockName);
        if (!block)
        {
            block = m_mgr->createBlock(m_blockName, "");
        }
        return FieldAccessorHelper(block);
    }

    inline RecordAccessorHelper CimeAccessor::records() const
    {
        CimeBlock *block = m_mgr->getBlock(m_blockName);
        if (!block)
        {
            block = m_mgr->createBlock(m_blockName, "");
        }
        return RecordAccessorHelper(block);
    }

    inline BlockMutator CimeAccessor::block() const
    {
        CimeBlock *block = m_mgr->getBlock(m_blockName);
        if (!block)
        {
            block = m_mgr->createBlock(m_blockName, "");
        }
        return BlockMutator(block);
    }

    inline CimeAccessor::operator CimeBlock *() const
    {
        CimeBlock *block = m_mgr->getBlock(m_blockName);
        if (!block)
        {
            block = m_mgr->createBlock(m_blockName, "");
        }
        return block;
    }

    // ----------------- CimeBlock 的其他实现 -----------------

    inline std::ostream &operator<<(std::ostream &os, const CimeBlock &block)
    {
        os << "<" << block.getFullName() << ">" << std::endl;
        // 此处可扩展输出格式
        os << "</" << block.getFullName() << ">" << std::endl;
        return os;
    }

    // ----------------- Cime 的实现 -----------------
    inline Cime::Cime() {}

    inline Cime::~Cime()
    {
        for (BlockContainer::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
        {
            delete it->second;
        }
    }

    inline CimeBlock *Cime::createBlock(const std::string &name, const std::string &description)
    {
        CimeBlock *block = new CimeBlock(name, description);
        block->setParent(this); // 设置父指针为当前 Cime 实例
        for (BlockContainer::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
        {
            if (it->first == name)
            {
                it->second = block;
                return block;
            }
        }
        m_blocks.push_back(BlockEntry(name, block));
        return block;
    }

    inline void Cime::addBlock(CimeBlock *block)
    {
        if (block)
        {
            const std::string fullName = block->getFullName();
            for (BlockContainer::iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
            {
                if (it->first == fullName)
                {
                    it->second = block;
                    return;
                }
            }
            m_blocks.push_back(BlockEntry(fullName, block));
        }
    }

    inline CimeBlock *Cime::getBlock(const std::string &name) const
    {
        for (BlockContainer::const_iterator it = m_blocks.begin(); it != m_blocks.end(); ++it)
        {
            if (it->first == name)
                return it->second;
        }
        return 0;
    }

    inline const Cime::BlockContainer &Cime::getBlocks() const
    {
        return m_blocks;
    }

    inline bool Cime::saveToFile(const std::string &filename) const
    {
        std::ofstream ofs(filename.c_str());
        if (!ofs)
        {
            CIME_ERR("无法打开文件: " << filename);
            return false;
        }
        ofs << "<!System=OMS Version=1.0 Code=UTF-8 Data=1.0!>" << std::endl
            << std::endl;
        for (BlockContainer::const_iterator mit = m_blocks.begin();
             mit != m_blocks.end(); ++mit)
        {
            blockToCsv(ofs, mit->second);
            ofs << std::endl;
        }
        ofs.close();
        return true;
    }

    inline bool Cime::loadFromFile(const std::string &filename)
    {
        std::ifstream ifs(filename.c_str());
        if (!ifs)
        {
            CIME_ERR("无法打开文件: " << filename);
            return false;
        }
        // 清空当前数据块
        m_blocks.clear();
        std::string line;
        while (std::getline(ifs, line))
        {
            // 跳过空行或仅包含空白字符的行
            if(line.empty()) continue;
            // 跳过系统标识头部
            if(line.find("<!System=") != std::string::npos)
                continue;
            // 检查是否为块起始行，如：<IED::CommunicationBlock attr1="value1" ...>
            if(!line.empty() && line[0] == '<' && line.size() > 1 && line[1] != '/')
            {
                size_t endBracket = line.find('>');
                if(endBracket == std::string::npos)
                    continue;
                std::string headerContent = line.substr(1, endBracket - 1);
                // 使用空格分割，首个 token 为块全名，其后为属性字符串（可选）
                std::istringstream headerStream(headerContent);
                std::string fullName;
                headerStream >> fullName;
                std::string attrPart;
                std::getline(headerStream, attrPart); // 后续属性部分
                // 创建数据块
                CimeBlock* block = createBlock(fullName, "");
                BlockMutator blockMutator(block);
                // 处理属性，如果有定义（例如 key="value" 格式）
                if(!attrPart.empty())
                {
                    std::istringstream attrStream(attrPart);
                    std::string attr;
                    while(attrStream >> attr)
                    {
                        size_t eqPos = attr.find('=');
                        if(eqPos != std::string::npos)
                        {
                            std::string key = attr.substr(0, eqPos);
                            std::string value = attr.substr(eqPos + 1);
                            // 去除两侧可能有的引号
                            if(value.size() >= 2 && value[0]=='"' && value[value.size()-1]=='"')
                                value = value.substr(1, value.size()-2);
                            // 通过 BlockMutator 设置属性
                            blockMutator.setAttribute(key, value);
                        }
                    }
                }
                // 解析块内内容直到遇到结束标签
                // 假定文件格式：先一行以 @ 开头定义字段，再有若干行以 # 开头的记录
                bool fieldsParsed = false;
                std::vector<CimeField> &fields = const_cast<std::vector<CimeField> &>(block->getFields());
                std::vector<CimeRecord> &records = const_cast<std::vector<CimeRecord> &>(block->getRecords());
                std::map<std::string, int> &fieldIndexMap = block->getFieldIndexMap();
                while(std::getline(ifs, line))
                {
                    if(line.empty()) continue;
                    // 结束当前块，当遇到以 "</" 开头的行
                    if(line.substr(0,2) == "</")
                        break;
                    // 字段定义行：以 @ 开头
                    if(!line.empty() && line[0] == '@')
                    {
                        // 仅按制表符分割
                        std::vector<std::string> fieldNames = details::splitTokens(line.substr(1));
                        // 去掉可能的指针标记前缀 '*'
                        for (size_t i = 0; i < fieldNames.size(); ++i)
                        {
                            std::string& token = fieldNames[i];
                            if (!token.empty() && token[0] == '*')
                                token = token.substr(1);
                        }

                        // 对每个字段创建默认设置
                        for (std::vector<std::string>::iterator it = fieldNames.begin(); it != fieldNames.end(); ++it)
                        {
                            if (it->empty()) continue; // 跳过空名字
                            std::string fname = *it;
                            CimeField field;
                            field.name = fname;
                            field.alias = "";
                            field.type = STRING;
                            field.length = "0";
                            field.dimension = "";
                            field.valueLimit = "";
                            field.isReference = false;
                            field.isAutoIncrement = false;
                            fields.push_back(field);

                            fieldIndexMap[fname] = static_cast<int>(fields.size() - 1);
                        }

                        fieldsParsed = true;
                    }
                    // "/@" 开头，别名显示
                    else if (line.size() > 1 && line[0] == '/' && line.at(1) == '@')
                    {
                        std::vector<std::string> values = details::splitTokens(line.substr(2));
                        for(size_t i = 0; i < values.size(); ++i)
                        {
                            fields[i].alias = values[i] = NULL ? "" : values[i];
                        }
                        block->setAliasVisible(true);
                    }
                    else if(!line.empty() && line[0] == '%')
                    {
                        std::vector<std::string> values = details::splitTokens(line.substr(1));
                        for(size_t i = 0; i < values.size(); ++i)
                        {
                            fields[i].setType(values[i]);
                        }
                        block->setTypeVisible(true);
                    }
                    else if(!line.empty() && line[0] == '$')
                    {
                        std::vector<std::string> values = details::splitTokens(line.substr(1));
                        for(size_t i = 0; i < values.size(); ++i)
                        {
                            fields[i].dimension = values[i] = NULL ? "" : values[i];
                        }
                        block->setUnitVisible(true);
                    }
                    // ':' 开头，值范围限制
                    else if(!line.empty() && line[0] == ':')
                    {
                        std::vector<std::string> values = details::splitTokens(line.substr(1));
                        for(size_t i = 0; i < values.size(); ++i)
                        {
                            fields[i].valueLimit = values[i] = NULL ? "" : values[i];
                        }
                        block->setValueLimitVisible(true);
                    }
                    // 记录行：以 '#' 开头
                    else if(!line.empty() && line[0] == '#')
                    {
                        if(!fieldsParsed)
                        {
                            CIME_ERR("记录行在字段定义之前出现,跳过该行");
                            continue;
                        }

                        std::vector<std::string> values = details::splitTokens(line.substr(1));
                        for (size_t i = 0; i < values.size(); ++i)
                        {
                            if (values[i] == "null" || values[i] == "NULL")
                                values[i].clear();
                        }

                        // 新建记录，确保记录数与字段数一致
                        CimeRecord record(block);
                        record.values = values;
                        while(record.values.size() < fields.size())
                            record.values.push_back("");
                        records.push_back(record);
                    }
                    // 其它行可视为注释或扩展信息，暂不处理
                }
            }
        }
        ifs.close();
        return true;
    }

    inline void Cime::blockToCsv(std::ostream &os, const CimeBlock *block) const
    {
        std::string attr_str = block->getAttributes();
        if (!attr_str.empty())
            attr_str = " " + attr_str;
        os << "<" << block->getFullName() << attr_str << ">" << std::endl;
        const std::vector<CimeField> &fields = block->getFields();
        std::string firstLineMarker = block->getTableType() == HORIZONTAL ? "@" :
        block->getTableType() == SINGLE_COLUMN ? "@@" : "@#";
        if (block->getTableType() == HORIZONTAL)
        {
            if (!fields.empty())
            {
                os << "@";
                for (size_t i = 0; i < fields.size(); ++i)
                    os << SPACE_CHAR << fields[i].name;
                os << std::endl;
            }
            if (!fields.empty() && block->isAliasVisible())
            {
                os << "/@";
                for (size_t i = 0; i < fields.size(); ++i)
                    os << SPACE_CHAR << (fields[i].alias.empty() ? "NULL" : fields[i].alias);
                os << std::endl;
            }
            if (!fields.empty() && block->isTypeVisible())
            {
                os << "%";
                for (size_t i = 0; i < fields.size(); ++i)
                    os << SPACE_CHAR << fields[i].Type();
                os << std::endl;
            }
            if (!fields.empty() && block->isUnitVisible())
            {
                os << "$";
                for (size_t i = 0; i < fields.size(); ++i)
                    os << SPACE_CHAR << (fields[i].dimension.empty() ? "NULL" : fields[i].dimension);
                os << std::endl;
            }
            if (!fields.empty() && block->isValueLimitVisible())
            {
                os << ":";
                for (size_t i = 0; i < fields.size(); ++i)
                    os << SPACE_CHAR << (fields[i].valueLimit.empty() ? "NULL" : fields[i].valueLimit);
                os << std::endl;
            }
            const std::vector<CimeRecord> &records = block->getRecords();
            for (size_t r = 0; r < records.size(); ++r)
            {
                os << "#";
                for (size_t i = 0; i < fields.size(); ++i)
                {
                    if (i < records[r].values.size() && !records[r].values[i].empty())
                        os << SPACE_CHAR << records[r].values[i];
                    else
                        os << SPACE_CHAR << "NULL";
                }
                os << std::endl;
                for (size_t p = 0; p < records[r].sparsePointers.size(); ++p)
                    os << records[r].sparsePointers[p] << std::endl;
            }
        }
        else
        {
            if (!fields.empty())
            {
                os << "@";
                for (size_t i = 0; i < fields.size(); ++i)
                    os << SPACE_CHAR << fields[i].name << "(" << fields[i].type << ")";
                os << std::endl;
            }
            const std::vector<CimeRecord> &records = block->getRecords();
            for (size_t r = 0; r < records.size(); ++r)
            {
                os << "#";
                for (size_t i = 0; i < records[r].values.size(); ++i)
                    os << SPACE_CHAR << records[r].values[i];
                os << std::endl;
            }
        }
        os << "</" << block->getFullName() << ">" << std::endl;
    }

    inline void BlockMutator::drop()
    {
        // 删除数据块，删除时会删除所有字段和记录
        m_block->m_parent->dropBlock(m_block->getFullName());
        m_block = NULL;
    }
    
} // namespace xcime
#endif // !XCIME_HEADER
