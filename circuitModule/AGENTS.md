# AGENTS.md

This repo is legacy: Qt 4.8.6 + C++03 only.

Hard constraints (must follow)
- Language: C++03 only. Do NOT use any C++11+ features (auto, nullptr, lambda, range-for, override, enum class, etc).
- Framework: Qt 4.8.6 APIs only. Do NOT use Qt5/Qt6 classes or headers.
- Encoding: source files are GB2312. Keep existing encoding; do NOT introduce UTF-8 literals / emoji / non-ASCII identifiers.
- Do not “modernize” code unless explicitly asked.

Default libraries
- Prefer Qt types by default: QString/QByteArray, QMap/QHash, QList/QVector, QSet, etc.
- Use STL containers only in designated standalone/common libraries or when the existing code in that module already uses STL.

Formatting
- Indentation: TAB, width 4.
- Braces: Allman style. Opening brace on its own line for functions/classes/namespaces/control blocks.
- Do not assign inside conditions: no `if (a = b)` / `while (x = y)`. Do assignment on a separate line, then check.
- Braces: strict Allman style.
- The opening brace `{` must always be placed on its own line.
- Never place `{` at the end of a function/class/namespace/control-statement line.
- This rule applies to functions, classes, structs, namespaces, if/else, for, while, switch, and other control blocks.
- Keep the repository's existing brace style unchanged: opening brace on the next line, not on the same line.

Naming
- Member variables must use `m_` prefix.
- Avoid single-letter local variables. All variable names must clearly express meaning.

Comments
- Comments are in Chinese.
- For function-level comments, use the following header template exactly (keep alignment with TAB indent):

	//************************************
	// 函数名称:
	// 函数全名:
	// 访问权限:
	// 函数说明:
	// 函数参数:
	// 函数参数:
	// 返回值:
	//************************************

Formatting
- Indentation: TAB, width 4.
- Braces: Allman style. Opening brace on its own line for functions/classes/namespaces/control blocks.
- Do not assign inside conditions: no `if (a = b)` / `while (x = y)`. Do assignment on a separate line, then check.

Vertical spacing (strict)
- Do NOT insert unnecessary blank lines.
- Never generate multiple consecutive empty lines.
- Inside function bodies, keep spacing compact.
- Do NOT add blank lines between simple consecutive statements.
- At most one empty line between logical blocks, and zero is preferred.
- Do NOT add blank lines immediately after `{` or before `}`.
- Do NOT add blank lines between a function signature and its opening brace.

Example of undesirable formatting:

void Foo()

{

	int a = 1;


	if (a)
	{

		doSomething();

	}

}

Correct formatting:

void Foo()
{
	int a = 1;

	if (a)
	{
		doSomething();
	}
}

Code structure
- Do not introduce helper functions unless they are clearly reused, significantly simplify complex logic, or are explicitly requested.
- Prefer keeping short local logic inline.
- Avoid over-decomposition.

Numeric constants
- Do not scatter magic numbers across multiple functions.
- If a numeric value affects layout, spacing, rendering, thresholds, retries, or sizing, define it once and reuse it.
- Prefer existing project macros / enums / named constants for tunable values.
- If multiple places use the same numeric rule, centralize it.
- Do not replace an existing macro-based style with repeated literals.

Function parameters
- Do not add unused parameters to preserve a uniform signature unless explicitly required by an existing callback/interface type.
- Every function parameter must have a real use in that function.
- If a parameter is unused, remove it instead of silencing it with `(void)param`.
- `(void)param` is allowed only when required by an existing external interface, callback typedef, or virtual override that cannot be changed.
- Do not create wrapper/adaptor functions that only forward to another function while ignoring extra parameters, unless this is strictly required by the calling interface.

File references
- When reporting changed locations, use local file paths with line numbers.
- Preferred format: absolute or workspace-relative file path followed by `:line` or `:start-end`.
- format your file link as `/e:/XXX/XXX`.
- If a clickable editor link cannot be generated reliably, always include a plain text file path with line numbers.

String literals and encoding
- Source files in this repository use GB2312-compatible local encoding.
- When Chinese text is needed in C++ source, write the Chinese characters directly in the source file.
- Prefer `QString::fromLocal8bit("中文")` for Chinese string literals when conversion is needed.
- Do NOT encode Chinese text as UTF-8 byte escapes such as `"\xE6\x..."`.
- Do NOT use `QString::fromUtf8("\x...")` for Chinese literals in this repository.
- Do not replace readable Chinese literals with escaped byte sequences.

Before writing code
- Restate constraints in one line: Qt 4.8.6 / C++03 / GB2312 / CRLF / TAB(4) / strict Allman braces / no same-line `{` / compact spacing / Qt containers default / m_ members / no assignment in conditions / Chinese comments / readable Chinese literals via fromLocal8bit when needed.