#!/usr/bin/env bash
# =============================================================================
#  package_deliverables.sh - build the Scope tree and assemble deliverables/.
#
#  deliverables/ = include/ + lib/ + plugins/ + bin/ + examples/ + docs/
#
#  Leak guard: ABORT if any internal header (S_ScopeLimits.h,
#  CMockScopeEmulator.h) or any model/mock SOURCE file would be shipped, and
#  (on Linux) verify plugins export only the Qt plugin entry points.
#
#  Usage:  scripts/package_deliverables.sh [release|debug]
# =============================================================================
set -euo pipefail

BUILD_TYPE="${1:-release}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-pkg"
OUT="$ROOT/deliverables"

echo "== Scope packaging =="
echo "root:  $ROOT"
echo "build: $BUILD ($BUILD_TYPE)"

# ---- build (shadow) --------------------------------------------------------
rm -rf "$BUILD"
mkdir -p "$BUILD"
( cd "$BUILD" && qmake "CONFIG+=$BUILD_TYPE" "$ROOT/Scope.pro" && make -j"$(nproc)" )

# ---- assemble --------------------------------------------------------------
rm -rf "$OUT"
mkdir -p "$OUT"/{include,lib,plugins,bin,examples,docs}

# ONLY the public SDK headers ship.
for h in ScopeManager.h IScopePlugin.h ScopeError.h ScopeTypes.h VisaHelper.h visa.h; do
    cp "$ROOT/include/$h" "$OUT/include/"
done

cp -a "$BUILD/lib/." "$OUT/lib/" 2>/dev/null || true
cp -a "$BUILD/plugins/." "$OUT/plugins/" 2>/dev/null || true
cp -a "$BUILD/bin/." "$OUT/bin/" 2>/dev/null || true
cp -a "$ROOT/examples/." "$OUT/examples/" 2>/dev/null || true
cp -a "$ROOT/docs/." "$OUT/docs/" 2>/dev/null || true
[ -f "$ROOT/README.md" ] && cp "$ROOT/README.md" "$OUT/"

# ---- leak guard: internal headers ------------------------------------------
FAIL=0
for bad in S_ScopeLimits.h CMockScopeEmulator.h; do
    if find "$OUT" -name "$bad" | grep -q .; then
        echo "LEAK: internal header '$bad' present in deliverables/" >&2
        FAIL=1
    fi
done

# ---- leak guard: no model/mock SOURCE files (examples may carry .cpp) -------
if find "$OUT" -name '*.cpp' ! -path "$OUT/examples/*" | grep -q .; then
    echo "LEAK: model/mock source present in deliverables/" >&2
    find "$OUT" -name '*.cpp' ! -path "$OUT/examples/*" >&2
    FAIL=1
fi
# private model plugin headers (<Model>Plugin.h) must never ship; the public
# interface header include/IScopePlugin.h is allowed.
if find "$OUT" -name '*Plugin.h' ! -name 'IScopePlugin.h' | grep -q .; then
    echo "LEAK: private model plugin header present in deliverables/" >&2
    find "$OUT" -name '*Plugin.h' ! -name 'IScopePlugin.h' >&2
    FAIL=1
fi

# ---- leak guard: exported-symbol check (Linux) -----------------------------
if command -v nm >/dev/null 2>&1; then
    for so in "$OUT"/plugins/*.so; do
        [ -e "$so" ] || continue
        if nm -D --defined-only "$so" 2>/dev/null | grep -Eq 'CMockScopeEmulator|ScopeLimitsCatalog|ScopeFindLimits'; then
            echo "LEAK: plugin $so exports an internal symbol" >&2
            FAIL=1
        fi
    done
fi

if [ "$FAIL" -ne 0 ]; then
    echo "== packaging FAILED (leak guard) ==" >&2
    exit 1
fi

echo "== packaging OK -> $OUT =="
