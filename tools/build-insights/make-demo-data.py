#!/usr/bin/env python3
"""レポートの見た目を手元で確かめるためのデモデータを作る (Windows でなくても動く)。

nox_build_insights.exe の出力と同じ形の JSON を、乱数で組み立てる。

    python3 tools/build-insights/make-demo-data.py --out /tmp/bi-demo
    python3 tools/build-insights/make-demo-data.py --out /tmp/bi-base --seed 2
    python3 .github/scripts/build-insights-report.py --data-dir /tmp/bi-demo --base-dir /tmp/bi-base \\
        --out-html /tmp/bi-demo/report.html
"""

import argparse
import json
import os
import random

WS = "D:\\a\\Nox\\Nox"
MSVC = "C:\\Program Files\\Microsoft Visual Studio\\18\\Enterprise\\VC\\Tools\\MSVC\\14.50.35717\\include"
SDK = "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.26100.0\\um"
VCPKG = WS + "\\vcpkg_installed\\x64-windows\\include"

STD = ["vector", "string", "memory", "type_traits", "tuple", "functional", "unordered_map", "algorithm",
       "xutility", "xmemory", "xstring", "utility", "atomic", "chrono", "array", "span", "optional", "variant"]
PROJECT = ["core/types.h", "core/memory/allocator.h", "core/memory/pool.h", "core/containers/vector.h",
           "core/containers/hash_map.h", "core/string/string.h", "core/delegate.h", "core/log.h",
           "reflection/type_info.h", "reflection/attribute.h", "reflection/function_signature.h",
           "reflection/property.h", "kernel/entity.h", "kernel/world.h", "kernel/archetype.h",
           "kernel/system.h", "app/application.h", "modules/render/renderer.h"]
TEMPLATES = ["std::vector", "std::basic_string", "std::tuple", "std::unordered_map", "std::function",
             "nox::reflection::TypeInfo", "nox::reflection::detail::FunctionSignature", "nox::Delegate",
             "nox::HashMap", "nox::Vector", "std::is_convertible", "std::invoke_result", "nox::Span",
             "nox::reflection::is_only_attribute", "std::allocator", "nox::kernel::Query"]


def build(seed, config, scale):
    # hash() は実行ごとにランダム化されるので使わない (文字列の種は毎回同じ乱数列になる)
    rnd = random.Random(f"{seed}-{config}")
    paths = []
    index = {}

    def pid(p):
        if p not in index:
            index[p] = len(paths)
            paths.append(p)
        return index[p]

    headers = {}
    units = []
    t = 0
    sources = [f"runtime\\core\\{n}.cpp" for n in ("archetype", "asset", "memory", "log", "string", "delegate", "type_db")] + \
              [f"runtime\\kernel\\{n}.cpp" for n in ("world", "entity", "system", "query", "scheduler")] + \
              [f"runtime\\reflection_generated\\gen\\reflect_{i}.cpp" for i in range(6)] + ["runtime\\pch.cpp"]
    for si, src in enumerate(sources):
        is_pch = src.endswith("pch.cpp")
        fe = int(rnd.uniform(800_000, 4_000_000) * scale * (3 if is_pch else 1))

        def make(path, depth, budget):
            node_children = []
            n = 0 if depth > 3 else rnd.randint(1, 6 if depth < 2 else 3)
            used = 0
            for _ in range(n):
                if depth == 0 and not is_pch and rnd.random() < 0.5:
                    child = WS + "\\runtime\\" + rnd.choice(PROJECT).replace("/", "\\")
                elif rnd.random() < 0.5:
                    child = MSVC + "\\" + rnd.choice(STD)
                elif rnd.random() < 0.2:
                    child = SDK + "\\" + rnd.choice(["windows.h", "winbase.h", "winuser.h"])
                elif rnd.random() < 0.1:
                    child = VCPKG + "\\" + rnd.choice(["fmt\\format.h", "spdlog\\spdlog.h"])
                else:
                    child = WS + "\\runtime\\" + rnd.choice(PROJECT).replace("/", "\\")
                b = int(budget * rnd.uniform(0.05, 0.4))
                if used + b > budget * 0.9:
                    break
                used += b
                node_children.append(make(child, depth + 1, b))
            folded = rnd.randint(0, 12)
            folded_us = folded * rnd.randint(50, 400)
            incl = max(budget, used + folded_us)
            excl = incl - used - folded_us
            return [pid(path), incl, max(0, excl), int(incl * rnd.uniform(0.2, 0.5)), node_children, folded, folded_us]

        root = make(WS + "\\" + src, 0, int(fe * 0.8))

        seen = set()

        def agg(node, parent):
            if parent is not None:
                h = headers.setdefault(node[0], {"p": node[0], "incl_us": 0, "excl_us": 0, "wctr_us": 0, "passes": 0,
                                                 "pch_passes": 0, "parses": 0, "max_us": 0, "parents": {}})
                h["incl_us"] += node[1]
                h["excl_us"] += node[2]
                h["wctr_us"] += node[3]
                h["parses"] += 1
                if node[0] not in seen:
                    seen.add(node[0])
                    h["passes"] += 1
                    if is_pch:
                        h["pch_passes"] += 1
                h["max_us"] = max(h["max_us"], node[1])
                ps = h["parents"].setdefault(parent, [parent, 0, 0])
                ps[1] += 1
                ps[2] += node[1]
            for k in node[4]:
                agg(k, node[0])

        agg(root, None)
        be = int(rnd.uniform(100_000, 1_500_000) * scale)
        start = t + rnd.randint(0, 500_000)
        t += rnd.randint(200_000, 900_000)
        units.append({"source": WS + "\\" + src, "object": "", "inv": 1 + si // 5, "start_us": start, "fe_us": fe,
                      "fe_wctr_us": fe // 3, "be_us": be, "be_wctr_us": be // 3, "pch": is_pch,
                      "parses": rnd.randint(200, 900), "tree": root})

    for h in headers.values():
        h["parents"] = sorted(h["parents"].values(), key=lambda p: -p[2])[:10]

    templates = []
    for name in TEMPLATES:
        incl = int(rnd.uniform(50_000, 3_000_000) * scale)
        specs = [[f"{name}<{rnd.choice(['int', 'float', 'nox::Entity', 'std::string', 'nox::World*'])},{i}>", int(incl * rnd.uniform(0.01, 0.2)), rnd.randint(1, 40)] for i in range(12)]
        specs.sort(key=lambda s: -s[1])
        templates.append({"name": name, "kind": rnd.choice([0, 0, 1, 2]), "incl_us": incl, "excl_us": int(incl * 0.4),
                          "count": rnd.randint(100, 20000), "passes": rnd.randint(1, len(sources)), "specs": specs,
                          "spec_count": rnd.randint(12, 400),
                          "files": [[pid(WS + "\\runtime\\" + rnd.choice(PROJECT).replace("/", "\\")), int(incl * 0.3)]]})
    templates.sort(key=lambda x: -x["incl_us"])

    functions = []
    for i in range(60):
        name = f"nox::kernel::World::Update{i}(float)" if i % 3 else f"nox::reflection::detail::Invoke<{i}>(void*)"
        dur = int(rnd.uniform(5_000, 900_000) * scale)
        functions.append({"name": name, "decorated": f"?Update{i}@World@kernel@nox@@QEAAXM@Z", "dur_us": dur,
                          "wctr_us": dur // 3, "count": rnd.randint(1, 20), "max_us": dur // 2,
                          "where": "ltcg" if config != "Debug" else "cl", "inlinees": rnd.randint(0, 30),
                          "inline_size": rnd.randint(0, 4000), "top_inlinees": [["nox::Vector<int>::operator[]", 120]]})
    functions.sort(key=lambda f: -f["dur_us"])

    end = t + 3_000_000
    invocations = [{"id": 99, "type": "link", "start_us": end - 2_000_000, "dur_us": 1_900_000, "wctr_us": 1_900_000,
                    "outputs": [WS + "\\runtime\\build\\runtime\\x64\\" + config + "\\runtime.exe"]}]
    return {
        "format": 1,
        "stats": {"result": "SUCCESS", "msvc_events_lost": 0, "msvc_buffers_lost": 0, "system_events_lost": 0, "system_buffers_lost": 0},
        "trace": {"duration_us": end + 1_000_000, "logical_processors": 4},
        "build": {"start_us": 0, "end_us": end},
        "totals": {"fe_us": sum(u["fe_us"] for u in units), "fe_wctr_us": 0, "be_us": sum(u["be_us"] for u in units),
                   "be_wctr_us": 0, "template_instantiations": sum(x["count"] for x in templates),
                   "functions": 12345, "file_parses": sum(u["parses"] for u in units), "passes": len(units)},
        "invocations": invocations,
        "paths": paths,
        "units": units,
        "headers": list(headers.values()),
        "templates": templates,
        "functions": functions,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", required=True)
    ap.add_argument("--seed", type=int, default=1)
    args = ap.parse_args()
    os.makedirs(args.out, exist_ok=True)
    for config, scale in (("Debug", 1.0), ("Release", 1.3), ("Master", 1.35)):
        data = build(args.seed, config, scale * (1 + 0.05 * (args.seed - 1)))
        meta = {"config": config, "compiler": "MSVC", "sha": "0123456789abcdef0123456789abcdef01234567",
                "ref": "work/demo", "workspace": WS, "templates": config == "Debug" or args.seed == 1,
                "build_seconds": 420.0, "subject": "デモ: ビルド計測のレポート"}
        with open(os.path.join(args.out, f"bi-{config}.json"), "w", encoding="utf-8") as f:
            json.dump(data, f)
        with open(os.path.join(args.out, f"bi-{config}.meta.json"), "w", encoding="utf-8") as f:
            json.dump(meta, f, ensure_ascii=False)
    print(f"デモデータを書いた: {args.out}")


if __name__ == "__main__":
    main()
