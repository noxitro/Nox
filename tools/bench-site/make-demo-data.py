#!/usr/bin/env python3
"""ベンチマーク結果ページ (tools/bench-site) を手元で確かめるためのデモデータを作る。

本物のデータは CI (bench-report.py) が main の計測を積み上げて作るので、手元には無い。
ページの見た目や操作を直すたびに CI を回すのは遅すぎるため、同じ形式 (DESIGN §4.1 の
nox-bench-history/1 と §4.2 の nox-bench-latest/1) のそれらしいデータをここで作る。

入れてあるもの (ページの表示分岐をひととおり通すため):
  - 実際のベンチマーク名とタイトル (runtime/bench と同じ一覧)
  - 2〜6 % のばらつき。スレッドを使うベンチマークはさらに大きい
  - CPU の入れ替わり (EPYC 7763 と Xeon 8370C。後者は 2 割ほど遅い) と、ランナー
    イメージの更新 (同時にコンパイラのパッチ版も上がる)
  - 本物の悪化 2 件と改善 2 件 (うち 1 件ずつは最新のコミット)。対の比較 (paired) の
    判定つき。ばらつき大の判定もときどき混ざる
  - ヒープ確保回数 (ほとんど 0、予算つきのものあり、最新のコミットで 1 件増える)
  - --preview: 最新の結果を「未マージのブランチの計測」(candidate) にする。予算違反と、
    ラウンドごとに確保回数が揃わなかった (stable = false) ものも 1 件ずつ入る

乱数の種を固定しているので、同じ引数なら毎回同じ内容になる (差分で見た目の変化を追える)。
生成物 (data/) は .gitignore 済みで、コミットしない。

使い方:
    python3 tools/bench-site/make-demo-data.py              # data/history.js と latest.js を作る
    python3 tools/bench-site/make-demo-data.py --preview    # ブランチのプレビューとして作る
    python3 tools/bench-site/make-demo-data.py --commits 400
その後 tools/bench-site/index.html をブラウザで開く (file:// のままで動く)。

終了コード: 0 成功、1 書き込み失敗、2 引数の誤り。
"""

import argparse
import datetime as dt
import hashlib
import json
import math
import os
import random
import sys
import zlib

REPO = "noxitro/Nox"
HISTORY_SCHEMA = "nox-bench-history/1"
LATEST_SCHEMA = "nox-bench-latest/1"
RUN_SCHEMA = "nox-bench-run/1"
ROUNDS = 10
THRESHOLD = 0.05
# 1 ラウンドあたりのサンプル数。本物は計測時間で決まるが、統計の形を作るには十分
SAMPLES_PER_ROUND = 15
BOOTSTRAP = 2000

# 最新のコミットの日時。実行時刻を使うと出力が毎回変わるので固定する
ANCHOR_DATE = dt.datetime(2026, 9, 27, 9, 41, tzinfo=dt.timezone(dt.timedelta(hours=9)))
GENERATED = "2026-09-27T01:30:00Z"

CPU_EPYC = "AMD EPYC 7763 64-Core Processor"
CPU_XEON = "Intel(R) Xeon(R) Platinum 8370C CPU @ 2.80GHz"
# QueryThreadCycleTime の周波数の代わり (head.cycles を作るためだけに使う。この値は不変 TSC の
# 基準ティックで ns に定数を掛けただけなので、ページには出していない)
CPU_GHZ = {CPU_EPYC: 2.45, CPU_XEON: 2.80}
IMAGE_OLD = "win25/20260810.1"
IMAGE_NEW = "win25/20260907.2"
COMPILER = {
    ("MSVC", IMAGE_OLD): "MSVC 19.50.35717",
    ("MSVC", IMAGE_NEW): "MSVC 19.50.35719",
    ("ClangCL", IMAGE_OLD): "Clang 20.1.8 (clang-cl)",
    ("ClangCL", IMAGE_NEW): "Clang 20.1.8 (clang-cl)",
}
LOGICAL_CPUS = 4
# JobSystem を使うベンチマークのスレッド数 (既定ワーカー数 3 + 呼び出し側 1)
THREADS_W = LOGICAL_CPUS

VARIANTS = [
    # id, label, compiler, config, 全体の速さの係数
    ("msvc-master", "MSVC / Master", "MSVC", "Master", 1.00),
    ("msvc-release", "MSVC / Release", "MSVC", "Release", 1.04),
    ("clangcl-master", "ClangCL / Master", "ClangCL", "Master", 0.96),
    ("clangcl-release", "ClangCL / Release", "ClangCL", "Release", 1.00),
]

# name, group, per, threads ("1" or "W"), budget, title, 基準の ns/op, allocs/op, bytes/op
CATALOG = [
    ("ecs/query_iterate/10k", "ecs", "entity", "1", 0, "EntitySystem::Execute で 1 万体を列挙 (Archetype 1 個)", 0.62, 0, 0),
    ("ecs/query_iterate_frag16/10k", "ecs", "entity", "1", 0, "1 万体を列挙 (Archetype 16 個に分散)", 0.95, 0, 0),
    ("ecs/query_parallel/100k", "ecs", "entity", "W", 0, "Chunk 単位の並列列挙 (10 万体, 既定ワーカー数)", 0.31, 0, 0),
    ("ecs/get_component_random/10k", "ecs", "lookup", "1", 0, "TryGetComponent のランダムアクセス (1 万体)", 7.8, 0, 0),
    ("ecs/tag_toggle/1k", "ecs", "entity", "1", None, "タグ Add→Remove による Archetype 移動 (1024 体)", 142.0, 1 / 256, 16),
    ("ecs/spawn_despawn/1k", "ecs", "entity", "1", None, "即時 API で生成 + 2 コンポーネント追加 + 破棄 (1024 体)", 265.0, 1 / 512, 64),
    ("ecs/deferred_spawn/1k", "ecs", "entity", "1", None, "EntityCommands で生成を記録 → Flush → 破棄 (1024 体)", 318.0, 0.0234, 410),
    ("ecs/layer_indices/64", "ecs", "call", "1", 0, "UpdaterGraph のレイヤー分割 (64 ノード)", 1850.0, 0, 0),
    ("ecs/graph_rebuild/8", "ecs", "call", "1", None, "UpdaterGraph::Rebuild (System 8 個)", 5400.0, 14, 3264),
    ("job/dispatch_wait/4", "job", "dispatch", "W", 0, "空ジョブ 4 個の Dispatch + Wait", 4200.0, 0, 0),
    ("job/dispatch_wait/64", "job", "dispatch", "W", 0, "空ジョブ 64 個の Dispatch + Wait", 21500.0, 0, 0),
    ("job/dispatch_inline/64", "job", "dispatch", "1", 0, "ワーカー 0 (その場で実行) の Dispatch + Wait", 1650.0, 0, 0),
    ("job/fanout/1024", "job", "job", "W", 0, "小さな計算ジョブ 1024 個の分配", 185.0, 0, 0),
    ("memory/nox_alloc_free/64", "memory", "pair", "1", 1, "nox::memory::Allocate + Deallocate (64 B)", 21.5, 1, 64),
    ("memory/nox_alloc_free/4096", "memory", "pair", "1", 1, "nox::memory::Allocate + Deallocate (4 KiB)", 48.0, 1, 4096),
    ("memory/new_delete/64", "memory", "pair", "1", 1, "グローバル operator new + delete (64 B)", 23.8, 1, 64),
    ("memory/crt_malloc_free/64", "memory", "pair", "1", 0, "比較用: CRT の malloc + free (64 B, 計数対象外)", 19.2, 0, 0),
    ("container/nox_vector_grow/256", "container", "op", "1", None, "nox::Vector に 256 個 push_back (reserve なし)", 610.0, 9, 2044),
    ("container/nox_vector_reserve/256", "container", "op", "1", 1, "nox::Vector に reserve してから 256 個 push_back", 205.0, 1, 1024),
    ("container/nox_vector_reuse/256", "container", "op", "1", 0, "clear して使い回す nox::Vector に 256 個 push_back", 118.0, 0, 0),
    ("container/stack_alloc_vector/256", "container", "op", "1", 0, "StackAllocVector に 256 個 push_back", 131.0, 0, 0),
    ("container/fixed_vector/256", "container", "op", "1", 0, "FixedVector に 256 個 PushBack", 96.0, 0, 0),
    ("string/format_span/u8", "string", "call", "1", 0, "nox::util::Format を固定バッファへ (uint32 × 2)", 38.5, 0, 0),
    ("string/format_heap/u8", "string", "call", "1", None, "nox::util::Format で文字列を返す (float × 3)", 412.0, 1, 48),
    ("string/crc32/256", "string", "call", "1", 0, "nox::util::Crc32 (256 バイト)", 62.0, 0, 0),
    ("string/utf16_to_utf8/64", "string", "call", "1", 0, "UTF-16 → UTF-8 変換 (64 文字)", 41.0, 0, 0),
    ("delegate/move_only_invoke", "delegate", "call", "1", 0, "MoveOnlyDelegate の呼び出し", 1.45, 0, 0),
    ("delegate/std_function_invoke", "delegate", "call", "1", 0, "比較用: std::function の呼び出し", 1.92, 0, 0),
    ("math/vec3_integrate/10k", "math", "entity", "1", 0, "Vec3 の位置更新 (1 万要素)", 0.41, 0, 0),
    ("lock/srw_uncontended", "lock", "pair", "1", 0, "StaticLock (SRWLOCK) の Lock/Unlock (競合なし)", 3.1, 0, 0),
]

SUBJECTS = [
    "ecs: Archetype の検索をハッシュ表に置き換える",
    "ecs: EntityCommands の記録バッファを再利用する",
    "ecs: UpdaterGraph の依存解決で一時配列を減らす",
    "job: ワーカーの待機をスピンから WaitOnAddress に変える",
    "job: Dispatch のキューを固定長リングにする",
    "memory: 小サイズ確保のフリーリストを整理する",
    "memory: 確保カウンタをスレッドローカルにまとめる",
    "container: FixedVector に EmplaceBack を足す",
    "container: StackAllocVector の境界チェックを Debug だけにする",
    "util: Crc32 をテーブル引き 4 バイト単位にする",
    "util: UTF-16 → UTF-8 変換の ASCII 高速経路を足す",
    "reflection: 属性の列挙を生成テーブルから引く",
    "reflection: NOX_ATTR_DECLARE をグローバル関数でも受け付ける",
    "kernel: ログの書式化を遅延させる",
    "kernel: StaticLock を SRWLOCK の薄いラッパーにする",
    "delegate: MoveOnlyDelegate のムーブを noexcept にする",
    "Editor: Inspector の同期間隔を調整する",
    "Editor: ContextMenu のテーマが白に戻るのを直す",
    "Editor: RemoteObject の再接続でツリーを保つ",
    "Editor: ドッキング配置を保存する",
    "net: Query / Response のヘッダを 16 バイトに詰める",
    "net: RemoteInstanceId の採番を Runtime 側で負にする",
    "ci: vcpkg のキャッシュキーを見直す",
    "ci: ClangCL の警告をエラーとして扱う",
    "docs: README のビルド手順を更新する",
    "docs: AGENTS.md に作業ツリーの手順を書く",
    "test: JobSystem の Dispatch テストを足す",
    "test: EntityCommandBuffer の Flush 順を検査する",
    "math: Vec3 の演算子を constexpr にする",
    "runtime: 起動時の設定読み込みを 1 回にまとめる",
    "runtime: 終了時に JobSystem を先に止める",
    "build: PlatformToolset を Directory.Build.props へ寄せる",
    "fix: Master 構成で assert が残っていたのを外す",
    "fix: Archetype 移動でコンポーネントの破棄が 2 回走る",
    "refactor: nox::Vector の成長処理を関数に切り出す",
    "refactor: EntitySystem::Execute のテンプレートを整理する",
]

# 本物の変化 (時間の係数)。位置はコミット数に対する割合で決め、--commits を変えても入るようにする
# name, 位置 (-1 は最新), コンパイラごとの係数, そのコミットの件名
TIME_CHANGES = [
    ("ecs/query_iterate_frag16/10k", 0.53, {"MSVC": 1.11, "ClangCL": 1.085},
     "ecs: Archetype の列を Chunk ごとに再配置する"),
    ("string/format_heap/u8", 0.77, {"MSVC": 0.76, "ClangCL": 0.79},
     "util: Format の浮動小数点を std::to_chars 経由にする"),
    ("container/nox_vector_grow/256", -1, {"MSVC": 1.085, "ClangCL": 1.07}, None),
    ("delegate/move_only_invoke", -1, {"MSVC": 0.88, "ClangCL": 0.87}, None),
]
LATEST_SUBJECT = "Merge pull request #58 from noxitro/work/container-tuning"
# 確保回数の変化: name, 位置, 新しい allocs/op, 新しい bytes/op
ALLOC_CHANGES = [
    ("container/nox_vector_grow/256", -1, 12, 3068),
]
IMAGE_UPDATE_AT = 0.48
IMAGE_UPDATE_SUBJECT = "ci: windows-2025 イメージの更新に合わせてキャッシュを作り直す"
# Xeon に当たる区間 (割合)。バリアントごとに少しずらす
XEON_BLOCKS = [(0.25, 0.30), (0.62, 0.67)]

# --preview のブランチ
PREVIEW_BRANCH = "work/ecs-chunk-prefetch"
PREVIEW_SUBJECT = "ecs: Chunk の列挙で次の Chunk を先読みする"
PREVIEW_TIME = {
    "ecs/query_iterate/10k": 0.91,
    "ecs/query_iterate_frag16/10k": 0.87,
    "ecs/get_component_random/10k": 1.065,
    "container/nox_vector_reuse/256": 1.03,
}
# 予算 0 のベンチマークで確保が起きる (CI が失敗する例)
PREVIEW_ALLOC = {"container/nox_vector_reuse/256": (1, 1024)}
# ラウンドごとに確保回数が揃わなかった例 (alloc.stable = false)
PREVIEW_UNSTABLE = {"ecs/deferred_spawn/1k"}


def sig4(x):
    """有効数字 4 桁に丸める (history.js を小さく保つ。bench-report.py と同じ規則)。"""
    if x is None:
        return None
    if x == 0:
        return 0
    v = float(f"{x:.4g}")
    return int(v) if v.is_integer() and abs(v) < 1e15 else v


def dec4(x):
    return None if x is None else round(x, 4)


def quantile(xs, q):
    """線形補間の分位点 (numpy の "linear" と同じ)。"""
    s = sorted(xs)
    if not s:
        return None
    pos = (len(s) - 1) * q
    lo = math.floor(pos)
    hi = min(lo + 1, len(s) - 1)
    return s[lo] + (s[hi] - s[lo]) * (pos - lo)


def median(xs):
    return quantile(xs, 0.5)


def cv(xs):
    mean = sum(xs) / len(xs)
    if mean == 0:
        return 0.0
    var = sum((x - mean) ** 2 for x in xs) / len(xs)
    return math.sqrt(var) / mean


def verdict_of(ratio, lo, hi, threads):
    """bench_common.paired_analysis と同じ判定。

    悪化 / 改善: 区間が 1 をまたがず、比の中央値がしきい値以上動いた。
    変化なし: 区間の全体が 1 ± しきい値 の帯に収まる (差が無いと言える)。
    ばらつき大: どちらでもない (区間が帯からはみ出していて、変化が無いとは言い切れない)。
    """
    thr = THRESHOLD if threads == 1 else 2 * THRESHOLD
    if lo > 1 and ratio - 1 >= thr:
        return "regressed"
    if hi < 1 and 1 - ratio >= thr:
        return "improved"
    if lo > 1 - thr and hi < 1 + thr:
        return "unchanged"
    return "noisy"


VERDICT_CODE = {"regressed": 1, "improved": -1, "unchanged": 0, "noisy": 2}


def paired(name, head_rounds, base_rounds, threads):
    """DESIGN §5 の対の比較 (ブートストラップで中央値の 95 % 区間)。"""
    lr = [math.log(h / b) for h, b in zip(head_rounds, base_rounds) if h > 0 and b > 0]
    if len(lr) < 3:
        return {"ratio": None, "ci_low": None, "ci_high": None, "change": None, "verdict": "insufficient"}
    rng = random.Random(zlib.crc32(name.encode()))
    n = len(lr)
    stats = sorted(median([lr[rng.randrange(n)] for _ in range(n)]) for _ in range(BOOTSTRAP))
    ratio = math.exp(median(lr))
    lo = math.exp(quantile(stats, 0.025))
    hi = math.exp(quantile(stats, 0.975))
    return {
        "ratio": dec4(ratio), "ci_low": dec4(lo), "ci_high": dec4(hi), "change": dec4(ratio - 1),
        "verdict": verdict_of(ratio, lo, hi, threads),
    }


def fake_sha(seed):
    return hashlib.sha1(("nox-bench-demo:" + seed).encode()).hexdigest()


class Model:
    """ベンチマークごとの「本当の速さ」と、計測のばらつきを決める。"""

    def __init__(self, n_commits, seed):
        self.n = n_commits
        self.rng = random.Random(seed)
        r = self.rng
        self.bench = {}
        for (name, group, per, th, budget, title, base_ns, allocs, nbytes) in CATALOG:
            threads = THREADS_W if th == "W" else 1
            self.bench[name] = {
                "group": group, "per": per, "threads": threads, "budget": budget, "title": title,
                "base": base_ns, "allocs": allocs, "bytes": nbytes,
                # 計測ごとのばらつき (VM の当たり外れを含む)
                "run_cv": r.uniform(0.02, 0.045) if threads == 1 else r.uniform(0.06, 0.1),
                # 同じ VM で交互に測ったときのラウンドごとのばらつき
                "round_sd": r.uniform(0.008, 0.02) if threads == 1 else r.uniform(0.04, 0.07),
                "iqr": r.uniform(0.006, 0.02) if threads == 1 else r.uniform(0.03, 0.06),
                "xeon": r.uniform(1.1, 1.3),
                "image": r.choice([1.0, 1.0, 1.0, 0.98, 1.03]),
                "vfac": {v[0]: v[4] * r.uniform(0.9, 1.1) for v in VARIANTS},
            }
        self.image_at = max(1, int(n_commits * IMAGE_UPDATE_AT))
        self.time_changes = [(name, self.pos(p), fac, subj) for name, p, fac, subj in TIME_CHANGES]
        self.alloc_changes = [(name, self.pos(p), a, b) for name, p, a, b in ALLOC_CHANGES]
        # ゆっくりしたうねり (OS やイメージ側の影響)。平均へ戻るランダムウォーク
        self.drift = {}
        for name in self.bench:
            for v in VARIANTS:
                x, arr = 0.0, []
                for _ in range(n_commits):
                    x = 0.9 * x + r.gauss(0, 0.004)
                    arr.append(math.exp(x))
                self.drift[(name, v[0])] = arr
        # バリアントごとの CPU の割り当て
        self.cpu = {}
        for vi, v in enumerate(VARIANTS):
            arr = []
            for k in range(n_commits):
                xeon = any(int(a * n_commits) + vi * 2 <= k < int(b * n_commits) + vi * 2 for a, b in XEON_BLOCKS)
                if not xeon and 0 < k < n_commits - 1 and r.random() < 0.02:
                    xeon = True
                arr.append(CPU_XEON if xeon else CPU_EPYC)
            self.cpu[v[0]] = arr

    def pos(self, p):
        return self.n - 1 if p == -1 else max(1, min(self.n - 2, int(self.n * p)))

    def image(self, k):
        return IMAGE_OLD if k < self.image_at else IMAGE_NEW

    def env(self, variant, compiler, k):
        img = self.image(k)
        return {
            "cpu": self.cpu[variant][k], "logical_cpus": LOGICAL_CPUS, "image": img,
            "runner": "github-hosted", "compiler": COMPILER[(compiler, img)],
        }

    def step(self, name, compiler, k, extra=None):
        f = 1.0
        for (n, at, fac, _) in self.time_changes:
            if n == name and k >= at:
                f *= fac[compiler]
        if extra:
            f *= extra.get(name, 1.0)
        return f

    def true_ns(self, name, variant, compiler, k, cpu, extra=None):
        b = self.bench[name]
        f = b["base"] * b["vfac"][variant] * self.step(name, compiler, k, extra)
        if cpu == CPU_XEON:
            f *= b["xeon"]
        if k >= self.image_at:
            f *= b["image"]
        return f * self.drift[(name, variant)][min(k, self.n - 1)]

    def allocs(self, name, k, extra=None):
        b = self.bench[name]
        a, nb = b["allocs"], b["bytes"]
        for (n, at, na, nbytes) in self.alloc_changes:
            if n == name and k >= at:
                a, nb = na, nbytes
        if extra and name in extra:
            a, nb = extra[name]
        return a, nb


def make_commits(model):
    r = random.Random(7)
    n = model.n
    special = {model.image_at: IMAGE_UPDATE_SUBJECT}
    for (_, at, _, subj) in model.time_changes:
        if subj:
            special[at] = subj
    special[n - 1] = LATEST_SUBJECT
    # 新しい方から日時をさかのぼる (3〜30 時間おき)
    dates = [ANCHOR_DATE]
    for _ in range(n - 1):
        dates.append(dates[-1] - dt.timedelta(minutes=r.randint(180, 1800)))
    dates.reverse()
    commits = []
    run_number = 412
    run_id = 17_300_000_000
    pr = 12
    for k in range(n):
        run_number += r.randint(1, 4)
        run_id += r.randint(2_000_000, 40_000_000)
        if k in special:
            subject = special[k]
        elif r.random() < 0.3:
            pr += 1
            subject = "Merge pull request #%d from noxitro/work/%s" % (pr, r.choice(
                ["ecs-query", "job-queue", "memory-stats", "editor-theme", "remote-sync", "format", "vector"]))
        else:
            subject = r.choice(SUBJECTS)
            if r.random() < 0.35:
                pr += 1
                subject += " (#%d)" % pr
        commits.append({
            "sha": fake_sha("commit-%d" % k), "subject": subject,
            "date": dates[k].isoformat(timespec="seconds"), "run_id": run_id, "run_number": run_number,
        })
    return commits


def simulate_run(model, variant, compiler, config, k, commit, ref, base_k, base_commit, base_kind,
                 extra_time=None, extra_alloc=None, unstable=(), run_attempt=1):
    """1 回の CI 実行ぶんの nox-bench-run/1 を、ラウンド単位のデータから作る。"""
    rng = random.Random(zlib.crc32(("run:%s:%s" % (variant, commit["sha"])).encode()))
    env = model.env(variant, compiler, min(k, model.n - 1))
    cpu = env["cpu"]
    benchmarks = []
    for name, b in model.bench.items():
        threads = b["threads"]
        t_head = model.true_ns(name, variant, compiler, k, cpu, extra_time)
        t_base = model.true_ns(name, variant, compiler, base_k, cpu) if base_commit else None
        # 今回の計測が当たった VM の癖 (head と base に共通でかかる)
        vm = math.exp(rng.gauss(0, b["run_cv"] * 0.6))
        sd = b["round_sd"]
        if name == "job/fanout/1024":
            sd *= 7.0  # ばらつき大の判定を 1 件出す
        heads, bases, head_samples, base_samples = [], [], [], []
        for _ in range(ROUNDS):
            common = math.exp(rng.gauss(0, 0.012))
            h_mid = t_head * vm * common * math.exp(rng.gauss(0, sd))
            hs = [h_mid * math.exp(rng.gauss(0, b["iqr"])) for _ in range(SAMPLES_PER_ROUND)]
            heads.append(median(hs))
            head_samples.extend(hs)
            if t_base:
                b_mid = t_base * vm * common * math.exp(rng.gauss(0, sd))
                bs = [b_mid * math.exp(rng.gauss(0, b["iqr"])) for _ in range(SAMPLES_PER_ROUND)]
                bases.append(median(bs))
                base_samples.extend(bs)

        def side(rounds, samples, threads=threads):
            cyc = None
            if threads == 1:
                cyc = sig4(median(samples) * CPU_GHZ[cpu] * rng.uniform(0.97, 1.03))
            return {
                "rounds": [sig4(x) for x in rounds], "median": sig4(median(samples)),
                "q1": sig4(quantile(samples, 0.25)), "q3": sig4(quantile(samples, 0.75)),
                "p10": sig4(quantile(samples, 0.10)), "p90": sig4(quantile(samples, 0.90)),
                "min": sig4(min(samples)), "cv": dec4(cv(rounds)), "cycles": cyc,
            }

        head = side(heads, head_samples)
        base = side(bases, base_samples) if t_base else None
        pr = paired(name, heads, bases, threads) if t_base else None
        allocs, nbytes = model.allocs(name, k, extra_alloc)
        budget = b["budget"]
        budget_ok = None if budget is None else allocs <= budget
        if base_commit:
            ba, bb = model.allocs(name, base_k)
            if round(allocs, 6) > round(ba, 6):
                av = "regressed"
            elif round(allocs, 6) < round(ba, 6):
                av = "improved"
            else:
                av = "unchanged"
        else:
            ba = bb = av = None
        benchmarks.append({
            "name": name, "group": b["group"], "title": b["title"], "per": b["per"], "threads": threads,
            "alloc_budget": budget, "head": head, "base": base, "paired": pr,
            "alloc": {
                "allocs": float(allocs), "bytes": float(nbytes), "frees": float(allocs), "stable": name not in unstable,
                "budget": budget, "budget_ok": budget_ok,
                "base_allocs": None if ba is None else float(ba), "base_bytes": None if bb is None else float(bb),
                "verdict": av,
            },
        })
    counts = {"regressed": 0, "improved": 0, "unchanged": 0, "noisy": 0, "insufficient": 0}
    logs = []
    for bm in benchmarks:
        p = bm["paired"]
        if p:
            counts[p["verdict"]] += 1
            if p["ratio"]:
                logs.append(math.log(p["ratio"]))
    summary = dict(counts)
    summary["geomean_change"] = dec4(math.exp(sum(logs) / len(logs)) - 1) if logs else None
    summary["alloc_regressed"] = sum(1 for bm in benchmarks if bm["alloc"]["verdict"] == "regressed")
    summary["budget_violations"] = sum(1 for bm in benchmarks if bm["alloc"]["budget_ok"] is False)
    return {
        "schema": RUN_SCHEMA, "variant": variant, "compiler": compiler, "config": config, "smoke": False,
        "repo": REPO,
        "commit": {
            "sha": commit["sha"], "subject": commit["subject"], "date": commit["date"], "ref": ref,
            "run_id": commit["run_id"], "run_number": commit["run_number"], "run_attempt": run_attempt,
        },
        "base": None if not base_commit else {
            "sha": base_commit["sha"], "run_id": base_commit["run_id"],
            "run_number": base_commit["run_number"], "kind": base_kind,
        },
        "env": {
            "cpu": env["cpu"], "logical_cpus": env["logical_cpus"], "hypervisor": True,
            "image": env["image"], "runner": env["runner"], "compiler": env["compiler"],
        },
        "rounds": ROUNDS, "threshold": THRESHOLD,
        "benchmarks": benchmarks,
        "summary": summary,
    }


def history_point_fast(model, rng, name, variant, compiler, k, cpu):
    """履歴の 1 点 (最新以外) を、ラウンドを作らずに近似で作る (全点でブートストラップすると遅い)。"""
    b = model.bench[name]
    t = model.true_ns(name, variant, compiler, k, cpu)
    m = t * math.exp(rng.gauss(0, b["run_cv"]))
    iqr = b["iqr"] * rng.uniform(0.7, 1.4)
    point = {"m": m, "q1": m * (1 - iqr), "q3": m * (1 + iqr)}
    if k == 0 or rng.random() < 0.04:
        # base の実行ファイルが見つからなかった回 (期限切れなど)
        point.update({"r": None, "rl": None, "rh": None, "v": None, "bs": -1})
    else:
        true_ratio = model.step(name, compiler, k) / model.step(name, compiler, k - 1)
        sd = b["round_sd"] * (5.0 if rng.random() < 0.03 else 1.0)
        se = 1.2533 * sd / math.sqrt(ROUNDS)
        center = math.log(true_ratio) + rng.gauss(0, se)
        half = 1.96 * se * rng.uniform(0.8, 1.25)
        ratio, lo, hi = math.exp(center), math.exp(center - half), math.exp(center + half)
        point.update({
            "r": ratio, "rl": lo, "rh": hi,
            "v": VERDICT_CODE[verdict_of(ratio, lo, hi, b["threads"])], "bs": k - 1,
        })
    a, ab = model.allocs(name, k)
    point.update({"a": a, "ab": ab})
    return point


def point_from_run(bm, base_index):
    """nox-bench-run/1 のベンチマーク 1 件から履歴の 1 点を作る (bench-report.py と同じ対応)。"""
    p = bm["paired"] or {}
    return {
        "m": bm["head"]["median"], "q1": bm["head"]["q1"], "q3": bm["head"]["q3"],
        "r": p.get("ratio"), "rl": p.get("ci_low"), "rh": p.get("ci_high"),
        "v": VERDICT_CODE.get(p.get("verdict")), "bs": base_index,
        "a": bm["alloc"]["allocs"], "ab": bm["alloc"]["bytes"],
    }


def build(n_commits, preview, seed):
    model = Model(n_commits, seed)
    commits = make_commits(model)
    last = n_commits - 1
    variants = {}
    latest_runs = {}
    for (vid, label, compiler, config, _) in VARIANTS:
        rng = random.Random(zlib.crc32(("hist:" + vid).encode()))
        envs, env_index = [], {}
        cols = {name: {k: [] for k in ("c", "e", "m", "q1", "q3", "r", "rl", "rh", "v", "bs", "a", "ab")}
                for name in model.bench}
        # 最新のコミットは本物と同じくラウンド単位で計測した結果から作る
        last_run = simulate_run(model, vid, compiler, config, last, commits[last], "main",
                                last - 1, commits[last - 1], "previous-main")
        latest_runs[vid] = last_run
        last_points = {bm["name"]: point_from_run(bm, last - 1) for bm in last_run["benchmarks"]}
        for k in range(n_commits):
            env = model.env(vid, compiler, k)
            key = json.dumps(env, sort_keys=True)
            if key not in env_index:
                env_index[key] = len(envs)
                envs.append(env)
            for name in model.bench:
                if k == last:
                    pt = last_points[name]
                else:
                    pt = history_point_fast(model, rng, name, vid, compiler, k, env["cpu"])
                col = cols[name]
                col["c"].append(k)
                col["e"].append(env_index[key])
                for f in ("m", "q1", "q3", "a", "ab"):
                    col[f].append(sig4(pt[f]))
                for f in ("r", "rl", "rh"):
                    col[f].append(dec4(pt[f]))
                col["v"].append(pt["v"])
                col["bs"].append(pt["bs"])
        benches = {}
        for name, b in model.bench.items():
            entry = {
                "title": b["title"], "group": b["group"], "per": b["per"], "threads": b["threads"],
                "alloc_budget": b["budget"],
            }
            entry.update(cols[name])
            benches[name] = entry
        variants[vid] = {"label": label, "envs": envs, "benchmarks": benches}

    history = {
        "schema": HISTORY_SCHEMA, "repo": REPO, "generated": GENERATED,
        "commits": commits, "variants": variants,
    }

    if not preview:
        head = commits[last]
        latest = {
            "schema": LATEST_SCHEMA, "candidate": False, "branch": "main",
            "commit": {k: head[k] for k in ("sha", "subject", "date", "run_id", "run_number")},
            "run_url": "https://github.com/%s/actions/runs/%d" % (REPO, head["run_id"]),
            "variants": latest_runs,
        }
    else:
        # 未マージのブランチ: 分岐元 (merge-base) は main の 2 つ前
        mb = max(0, last - 2)
        cand = {
            "sha": fake_sha("branch"), "subject": PREVIEW_SUBJECT,
            "date": (ANCHOR_DATE + dt.timedelta(hours=2, minutes=17)).isoformat(timespec="seconds"),
            "run_id": commits[last]["run_id"] + 3_100_000, "run_number": commits[last]["run_number"] + 2,
        }
        runs = {}
        for (vid, _label, compiler, config, _) in VARIANTS:
            # candidate はコミット一覧に無いので、モデル上は merge-base と同じ位置 (+ ブランチの変化) とみなす
            runs[vid] = simulate_run(model, vid, compiler, config, mb, cand, PREVIEW_BRANCH, mb, commits[mb],
                                     "merge-base", extra_time=PREVIEW_TIME, extra_alloc=PREVIEW_ALLOC,
                                     unstable=PREVIEW_UNSTABLE)
        latest = {
            "schema": LATEST_SCHEMA, "candidate": True, "branch": PREVIEW_BRANCH,
            "commit": cand,
            "run_url": "https://github.com/%s/actions/runs/%d" % (REPO, cand["run_id"]),
            "variants": runs,
        }
    return history, latest


def write_js(path, var, obj):
    text = json.dumps(obj, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("window.%s = %s;\n" % (var, text))
    return len(text)


def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8", errors="replace")
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--commits", type=int, default=120, help="main のコミット数 (既定 120)")
    ap.add_argument("--preview", action="store_true", help="最新の結果を未マージのブランチ (candidate) にする")
    ap.add_argument("--seed", type=int, default=20260927, help="乱数の種 (既定は固定値)")
    ap.add_argument("--out-dir", default=None, help="出力先 (既定はこのスクリプトの隣の data/)")
    args = ap.parse_args()
    if args.commits < 8:
        print("--commits は 8 以上にしてください", file=sys.stderr)
        return 2

    history, latest = build(args.commits, args.preview, args.seed)
    out_dir = args.out_dir or os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")
    try:
        os.makedirs(out_dir, exist_ok=True)
        n_hist = write_js(os.path.join(out_dir, "history.js"), "NOX_BENCH_HISTORY", history)
        n_latest = write_js(os.path.join(out_dir, "latest.js"), "NOX_BENCH_LATEST", latest)
        with open(os.path.join(out_dir, "history.json"), "w", encoding="utf-8", newline="\n") as f:
            json.dump(history, f, ensure_ascii=False, separators=(",", ":"), allow_nan=False)
            f.write("\n")
    except OSError as e:
        print("書き込めませんでした: %s" % e, file=sys.stderr)
        return 1
    n_bench = len(CATALOG)
    print("デモデータを書きました: %s" % os.path.relpath(out_dir))
    print("  history.js  %d コミット × %d バリアント × %d ベンチマーク (%.0f KB)"
          % (args.commits, len(VARIANTS), n_bench, n_hist / 1024))
    print("  latest.js   %s (%.0f KB)" % ("プレビュー: " + PREVIEW_BRANCH if args.preview else "main の最新", n_latest / 1024))
    return 0


if __name__ == "__main__":
    sys.exit(main())
