# Coding Standard (MISRA C++:2023-aligned)

This project follows a house coding standard aligned with **MISRA C++:2023** for
documentation, formatting and defensive-programming conventions. Full MISRA
tool-certified compliance is a non-goal (the code links Qt and uses
`QPluginLoader`/dynamic allocation, which fall under documented deviations); the
rules below are the ones applied throughout the source.

## 1. File header (Doxygen)

Every translation unit and header begins with a Doxygen file block:

```cpp
/**
 * @file    ScopeError.h
 * @brief   One-line summary of the file's purpose.
 * @details Longer description: what this unit provides and how it fits the
 *          framework.
 *
 * @author  Scope framework
 * @date    2026
 * @note    MISRA C++:2023 — relevant rule notes / documented deviations.
 */
```

## 2. Function header (Doxygen, with prerequisites)

Every function has a Doxygen block documenting intent, parameters, return, and
**prerequisites** (`@pre`) — the functions/state that must hold before the call:

```cpp
/**
 * @brief   Connect the bound scope to its instrument.
 * @param[in]  in_u32ScopeNumber  1-based logical scope number.
 * @param[in]  in_sConfig         Connection parameters / VISA resource.
 * @return  ScopeError — SUCCESS, else a diagnostic code.
 * @pre     createInstance(in_u32ScopeNumber, ...) has succeeded.
 * @pre     The scope is not already connected.
 * @post    On SUCCESS, isConnected(in_u32ScopeNumber) == true.
 */
```

`@param[in]` / `@param[out]` mark direction; parameter names keep the `in_`/`out_`
tags. `@retval` is used where specific codes are meaningful.

## 3. Braces — Allman style with a sequence comment

Opening braces sit on their own line (MISRA readability). Every compound
statement opens with a short comment describing the sequence, so a reader
follows the intent without decoding the code:

```cpp
if (pPlugin == nullptr)
{
    // No instance is bound to this scope number: report and stop.
    return ScopeError(Enum_Scope_ErrorCode::INVALID_SCOPE_NUMBER, ...);
}
```

This applies to `if/else`, `for`, `while`, `switch`, function bodies and blocks.
Single-statement bodies are still braced (MISRA — no unbraced sub-statements).

## 4. Other conventions honored

- Fixed-width house typedefs only (`U32BIT`, `FDOUBLE`, …) — never `long`.
- One `return` is preferred, but early-return guard clauses are allowed for
  precondition checks (documented, keeps nesting shallow).
- No implicit conversions that lose information; casts are explicit
  (`static_cast`), never C-style, in new code.
- Every `switch` has a `default`; every case that falls through says so.
- Pointers checked before dereference; parameters validated before use.
- No use of features newer than the C++11 baseline the build targets.

## 5. Rollout

The standard is applied file-by-file. `ScopeError.h` / `ScopeError.cpp` are the
canonical reference implementation of the style.
