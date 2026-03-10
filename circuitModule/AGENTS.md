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

Before writing code
- Restate constraints in one line: Qt 4.8.6 / C++03 / GB2312 / TAB(4) / Allman braces / Qt containers default / m_ members / no assignment in conditions / Chinese comments.