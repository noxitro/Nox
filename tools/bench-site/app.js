/*
 * Nox ベンチマーク結果ページ。
 *
 * ビルド手順も外部ライブラリも無い 1 ファイルの素の JavaScript。理由:
 *   - file:// のまま (CI のアーティファクトを展開してそのまま) 開けるようにする。
 *     ES モジュールや fetch は file:// で動かないので、データは <script> で読む
 *     (data/history.js と data/latest.js が window.NOX_BENCH_* を定義する)
 *   - 外部への通信をしない (CDN もフォントも読まない)
 *   - グラフは SVG を手で組む。点の数は多くても数千なので十分速い
 *
 * データ形式は nox-bench-history/1 (列指向の履歴) と nox-bench-latest/1 (直近の
 * 実行 1 回分の生データ)。詳しくは .github/scripts/bench-report.py を参照。
 *
 * 色はすべて CSS 変数 (style.css) で、SVG 側は class だけを付ける。テーマを
 * 切り替えても描き直さずに色が変わる。
 */
"use strict";

(function () {
	// ================================================================ 定数

	const VARIANT_ORDER = ["msvc-master", "msvc-release", "clangcl-master", "clangcl-release"];
	const VARIANT_LABELS = {
		"msvc-master": "MSVC / Master",
		"msvc-release": "MSVC / Release",
		"clangcl-master": "ClangCL / Master",
		"clangcl-release": "ClangCL / Release",
	};
	const VERDICT = {
		regressed: { glyph: "▲", label: "悪化" },
		improved: { glyph: "▼", label: "改善" },
		unchanged: { glyph: "≈", label: "変化なし" },
		noisy: { glyph: "？", label: "ばらつき大" },
		insufficient: { glyph: "—", label: "回数不足" },
		none: { glyph: "—", label: "比較なし" },
	};
	// 履歴の v 列。回数不足 (対が 3 組未満) は null で、比 (r) だけが入る
	const CODE_TO_VERDICT = { "1": "regressed", "-1": "improved", "0": "unchanged", "2": "noisy" };
	const BASE_KIND = {
		"previous-master": "直前の master",
		"merge-base": "分岐元",
		"latest-master": "最新の master",
		explicit: "指定した exe",
	};
	const MIN_PAIRS = 3;
	const PER_LABEL = {
		entity: "エンティティ 1 体",
		lookup: "検索 1 回",
		dispatch: "Dispatch + Wait 1 回",
		call: "呼び出し 1 回",
		job: "ジョブ 1 個",
		pair: "対になった操作 1 組 (確保と解放など)",
		op: "タイトルの処理 1 回",
		byte: "1 バイト",
	};
	const METRICS = {
		time: { label: "時間", axis: "1 op あたりの時間 (中央値)。小さいほど速い" },
		allocs: { label: "確保回数", axis: "1 op あたりのヒープ確保回数。0 が理想" },
		bytes: { label: "確保バイト", axis: "1 op あたりのヒープ確保量" },
	};
	const DEFAULT_THRESHOLD = 0.05;
	// 全体の変化 (比の幾何平均) に入れないグループ。bench_common.py の GEOMEAN_EXCLUDED_GROUPS と同じ。
	// 起動 (startup) は OS のローダやウィンドウの作成を含む壁時計なので、マイクロベンチマークと混ぜない
	const GEOMEAN_EXCLUDED_GROUPS = new Set(["startup"]);
	const FOREST_DOMAIN = 0.15;
	const SPARK_POINTS = 30;
	const THEME_KEY = "nox-bench-theme";
	const VARIANT_KEY = "nox-bench-variant";
	const MINUS = "−";
	const NNBSP = " ";
	const CPU_SLOTS = 5; // --cpu-0..4 と、それ以降は「その他」の灰色

	// ================================================================ データ

	const H = acceptSchema(window.NOX_BENCH_HISTORY, "nox-bench-history/");
	const L = acceptSchema(window.NOX_BENCH_LATEST, "nox-bench-latest/");

	function acceptSchema(obj, prefix) {
		if (!obj || typeof obj !== "object") {
			return null;
		}
		if (typeof obj.schema !== "string" || obj.schema.indexOf(prefix) !== 0) {
			console.warn("未知のデータ形式のため読み飛ばします:", obj.schema);
			return null;
		}
		return obj;
	}

	// ================================================================ 小道具

	const $ = (id) => document.getElementById(id);
	const root = document.documentElement;
	const drawer = $("drawer");

	function esc(s) {
		return String(s == null ? "" : s).replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
	}

	function clamp(v, lo, hi) {
		return v < lo ? lo : v > hi ? hi : v;
	}

	function isNum(v) {
		return typeof v === "number" && isFinite(v);
	}

	/// 有効数字 3 桁 (指数表記にしない)
	function sig3(v) {
		if (!isNum(v)) {
			return "—";
		}
		if (v === 0) {
			return "0";
		}
		const r = Number(v.toPrecision(3));
		const a = Math.abs(r);
		const d = a >= 100 ? 0 : a >= 10 ? 1 : a >= 1 ? 2 : Math.min(8, 2 - Math.floor(Math.log10(a)));
		return r.toFixed(d);
	}

	function trimZeros(s) {
		return s.indexOf(".") >= 0 ? s.replace(/\.?0+$/, "") : s;
	}

	const TIME_UNITS = [[1, "ns"], [1e3, "µs"], [1e6, "ms"], [1e9, "s"]];

	function timeUnit(ns) {
		const a = Math.abs(ns);
		let u = TIME_UNITS[0];
		for (const t of TIME_UNITS) {
			if (a >= t[0] * 0.9995) {
				u = t;
			}
		}
		return u;
	}

	function fmtTime(ns) {
		if (!isNum(ns)) {
			return "—";
		}
		const u = timeUnit(ns);
		return sig3(ns / u[0]) + " " + u[1];
	}

	/// 変化率の数値部分 ("+3.4" / "−1.2")。マイナスは U+2212
	function fmtPctNum(x, digits) {
		if (!isNum(x)) {
			return "—";
		}
		const d = digits == null ? 1 : digits;
		const p = x * 100;
		const a = Math.abs(p);
		const s = a.toFixed(a >= 100 ? 0 : d);
		if (Number(s) === 0) {
			return (0).toFixed(d);
		}
		return (p > 0 ? "+" : MINUS) + s;
	}

	function fmtPct(x, digits) {
		return isNum(x) ? fmtPctNum(x, digits) + NNBSP + "%" : "—";
	}

	/// 比 (head / base) の 95 % 区間を、変化率で "−1.1 〜 +1.4 %" の形にする (% は最後に 1 回だけ)。
	/// 比のまま受け取って中で 1 を引く (区間が無い null から −100 % を作らないため)
	function fmtCi(lo, hi) {
		return isNum(lo) && isNum(hi) ? fmtPctNum(lo - 1) + " 〜 " + fmtPctNum(hi - 1) + NNBSP + "%" : "—";
	}

	function fmtCount(v) {
		if (!isNum(v)) {
			return "—";
		}
		if (Number.isInteger(v)) {
			return v.toLocaleString("en-US");
		}
		return trimZeros(sig3(v));
	}

	/**
	 * 1 op あたりの確保回数。0〜1 の端数は "1/512" (512 op に 1 回) の形にする。
	 * 逆数が整数から 2 % 以上ずれるときだけ有効数字 3 桁に戻す。
	 * title には「512 entity あたり 1 回」のように、その行の 1 op の単位で書いた説明を入れる。
	 */
	function allocFmt(v, per) {
		if (!isNum(v)) {
			return { text: "—", title: "" };
		}
		const unit = per || "op";
		const exact = trimZeros(sig3(v));
		if (v > 0 && v < 1) {
			const d = 1 / v;
			const n = Math.round(d);
			if (n >= 2 && Math.abs(d - n) <= n * 0.02) {
				const near = Math.abs(d - n) > n * 0.001;
				return {
					text: "1/" + n,
					title: (near ? "約 " : "") + n.toLocaleString("en-US") + " " + unit + " あたり 1 回" + (near ? " (1 " + unit + " あたり " + exact + " 回)" : ""),
				};
			}
			return { text: exact, title: "1 " + unit + " あたり " + exact + " 回" };
		}
		return { text: fmtCount(v), title: "" };
	}

	/// 確保回数の文字列だけ ("1/512" / "3")
	function fmtAllocs(v, per) {
		return allocFmt(v, per).text;
	}

	function fmtBytes(b) {
		if (!isNum(b)) {
			return "—";
		}
		if (Math.abs(b) < 1024) {
			return trimZeros(sig3(b)) + " B";
		}
		if (Math.abs(b) < 1024 * 1024) {
			return sig3(b / 1024) + " KiB";
		}
		return sig3(b / 1048576) + " MiB";
	}

	function fmtRatio(r) {
		return isNum(r) ? r.toFixed(3) + "×" : "—";
	}

	function parseDate(s) {
		const d = s ? new Date(s) : null;
		return d && !isNaN(d.getTime()) ? d : null;
	}

	function pad2(n) {
		return (n < 10 ? "0" : "") + n;
	}

	function fmtDate(s) {
		const d = parseDate(s);
		return d ? d.getMonth() + 1 + "/" + d.getDate() : "";
	}

	function fmtDateTime(s) {
		const d = parseDate(s);
		if (!d) {
			return "";
		}
		return d.getFullYear() + "/" + pad2(d.getMonth() + 1) + "/" + pad2(d.getDate()) + " " + pad2(d.getHours()) + ":" + pad2(d.getMinutes());
	}

	function fmtShortDateTime(s) {
		const d = parseDate(s);
		return d ? d.getMonth() + 1 + "/" + d.getDate() + " " + pad2(d.getHours()) + ":" + pad2(d.getMinutes()) : "";
	}

	function relTime(s) {
		const d = parseDate(s);
		if (!d) {
			return "";
		}
		const sec = (Date.now() - d.getTime()) / 1000;
		if (sec < 60) {
			return "たった今";
		}
		if (sec < 3600) {
			return Math.floor(sec / 60) + " 分前";
		}
		if (sec < 86400) {
			return Math.floor(sec / 3600) + " 時間前";
		}
		if (sec < 86400 * 30) {
			return Math.floor(sec / 86400) + " 日前";
		}
		if (sec < 86400 * 365) {
			return Math.floor(sec / (86400 * 30)) + " か月前";
		}
		return Math.floor(sec / (86400 * 365)) + " 年前";
	}

	function sha7(sha) {
		return sha ? String(sha).slice(0, 7) : "";
	}

	function truncate(s, n) {
		s = String(s || "");
		return s.length > n ? s.slice(0, n - 1) + "…" : s;
	}

	/// "AMD EPYC 7763 64-Core Processor" → "EPYC 7763" のように短くする
	function cpuShort(cpu) {
		if (!cpu) {
			return "不明な CPU";
		}
		let m = /EPYC\s+(\w+)/i.exec(cpu);
		if (m) {
			return "EPYC " + m[1];
		}
		m = /Xeon\(R\)\s+(?:(?:Platinum|Gold|Silver|Bronze)\s+)?(?:CPU\s+)?([\w-]+)/i.exec(cpu);
		if (m) {
			return "Xeon " + m[1];
		}
		m = /Core\(TM\)\s+(i\d-\w+)/i.exec(cpu);
		if (m) {
			return "Core " + m[1];
		}
		return cpu.replace(/\((R|TM)\)/g, "").replace(/\s+@.*$/, "").replace(/\s+\d+-Core Processor/i, "").replace(/\s+(CPU|Processor)\b/g, "").replace(/\s+/g, " ").trim();
	}

	function perLabel(per) {
		return PER_LABEL[per] || (per ? per + " 1 つ" : "1 op");
	}

	function commitUrl(repo, sha) {
		return "https://github.com/" + repo + "/commit/" + sha;
	}

	function runUrl(repo, runId) {
		return "https://github.com/" + repo + "/actions/runs/" + runId;
	}

	function hashName(name) {
		return encodeURIComponent(name).replace(/%2F/gi, "/");
	}

	// 小さなアイコン (装飾なので aria-hidden)
	const ICON = {
		commit: '<svg viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="2.6" class="i-stroke"/><path d="M1.5 8h3.9M10.6 8h3.9" class="i-stroke"/></svg>',
		clock: '<svg viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="6" class="i-stroke"/><path d="M8 4.8V8l2.2 1.4" class="i-stroke"/></svg>',
		play: '<svg viewBox="0 0 16 16" aria-hidden="true"><circle cx="8" cy="8" r="6" class="i-stroke"/><path d="M6.8 5.8v4.4L10.2 8z" class="i-fill"/></svg>',
		base: '<svg viewBox="0 0 16 16" aria-hidden="true"><path d="M4.5 2.5v11M11.5 5.5v2.2c0 1.5-1.2 2.8-2.8 2.8H4.5" class="i-stroke"/><circle cx="11.5" cy="4" r="1.6" class="i-stroke"/></svg>',
		close: '<svg viewBox="0 0 20 20" aria-hidden="true"><path d="M5 5l10 10M15 5 5 15" class="i-stroke"/></svg>',
		link: '<svg viewBox="0 0 20 20" aria-hidden="true"><path d="M8.5 11.5a3 3 0 0 0 4.2 0l2.6-2.6a3 3 0 0 0-4.2-4.2l-.9.9M11.5 8.5a3 3 0 0 0-4.2 0l-2.6 2.6a3 3 0 0 0 4.2 4.2l.9-.9" class="i-stroke"/></svg>',
		branch: '<svg viewBox="0 0 16 16" aria-hidden="true"><circle cx="4.5" cy="3.5" r="1.6" class="i-stroke"/><circle cx="4.5" cy="12.5" r="1.6" class="i-stroke"/><circle cx="11.5" cy="5" r="1.6" class="i-stroke"/><path d="M4.5 5.1v5.8M11.5 6.6c0 2.6-3 2.4-6.2 4.4" class="i-stroke"/></svg>',
	};

	// ================================================================ モデル

	/**
	 * 画面が使う形へ組み直す。history は列指向なので、ベンチマークごとの配列をそのまま持ち、
	 * 行 (最新値・判定など) だけを先に作っておく。
	 */
	function buildModel() {
		const firstRun = L && L.variants ? Object.values(L.variants)[0] : null;
		const repo = (H && H.repo) || (firstRun && firstRun.repo) || "noxitro/Nox";
		const commits = H && Array.isArray(H.commits) ? H.commits : [];
		const shaIndex = new Map();
		commits.forEach((c, i) => shaIndex.set(c.sha, i));

		const ids = new Set();
		if (H && H.variants) {
			Object.keys(H.variants).forEach((id) => ids.add(id));
		}
		if (L && L.variants) {
			Object.keys(L.variants).forEach((id) => ids.add(id));
		}
		const variantIds = Array.from(ids).sort((a, b) => {
			const ia = VARIANT_ORDER.indexOf(a);
			const ib = VARIANT_ORDER.indexOf(b);
			return (ia < 0 ? 99 : ia) - (ib < 0 ? 99 : ib) || a.localeCompare(b);
		});

		// CPU の色は全バリアントを通して、初めて現れた順に固定する (絞り込みで色が変わらないように)
		const cpuFirst = new Map();
		const noteCpu = (cpu, at) => {
			if (cpu && (!cpuFirst.has(cpu) || cpuFirst.get(cpu) > at)) {
				cpuFirst.set(cpu, at);
			}
		};
		if (H && H.variants) {
			// 系列ごとに点のあるコミットが違う (途中で足したベンチマーク、中央値が無く飛ばした点) ので
			// 全系列を見る。多くても 4 バリアント × 数十系列 × 1500 点で、コストは無視できる
			for (const hv of Object.values(H.variants)) {
				const envs = hv.envs || [];
				for (const s of Object.values(hv.benchmarks || {})) {
					const c = s.c || [];
					const e = s.e || [];
					for (let i = 0; i < c.length; i++) {
						const env = envs[e[i]];
						if (env) {
							noteCpu(env.cpu, c[i]);
						}
					}
				}
			}
		}
		if (L && L.variants) {
			for (const run of Object.values(L.variants)) {
				noteCpu(run && run.env && run.env.cpu, 1e9);
			}
		}
		const cpuList = Array.from(cpuFirst.entries()).sort((a, b) => a[1] - b[1]).map((x) => x[0]);
		const cpuIndex = new Map(cpuList.map((c, i) => [c, i]));

		const model = { repo, commits, shaIndex, variantIds, cpuList, cpuIndex, variants: new Map() };
		for (const id of variantIds) {
			model.variants.set(id, buildVariant(model, id));
		}
		noteVariantGaps(model);
		return model;
	}

	/// 構成の間で大きな差 (この倍率以上) があるときだけ、表の行に小さな印を出す
	const VARIANT_GAP = 1.5;

	/**
	 * 各行について、同じコミット・同じ CPU で計測したほかの構成のうち最も速いものと比べ、
	 * VARIANT_GAP 倍以上遅いときに row.gap を付ける (表の「ClangCL / Master より 3.3 倍遅い」)。
	 * 構成ごとに別々の VM なので、CPU が違う組み合わせや別のコミットどうしは比べない。
	 */
	function noteVariantGaps(model) {
		for (const vm of model.variants.values()) {
			const sha = vm.summary.commit && vm.summary.commit.sha;
			if (!sha) {
				continue;
			}
			for (const row of vm.rows) {
				if (!isNum(row.value) || row.value <= 0) {
					continue;
				}
				let best = null;
				for (const other of model.variants.values()) {
					if (other === vm || !other.summary.commit || other.summary.commit.sha !== sha) {
						continue;
					}
					const r = other.rowsByName.get(row.name);
					if (r && isNum(r.value) && r.value > 0 && r.cpu === row.cpu && (!best || r.value < best.value)) {
						best = { label: other.label, value: r.value };
					}
				}
				if (best && row.value / best.value >= VARIANT_GAP) {
					row.gap = { label: best.label, value: best.value, ratio: row.value / best.value };
				}
			}
		}
	}

	function buildVariant(model, id) {
		const hv = H && H.variants ? H.variants[id] : null;
		const run = L && L.variants ? L.variants[id] || null : null;
		const label = (hv && hv.label) || VARIANT_LABELS[id] || (run ? run.compiler + " / " + run.config : id);
		const envs = hv && Array.isArray(hv.envs) ? hv.envs : [];
		const series = hv && hv.benchmarks ? hv.benchmarks : {};

		let lastIdx = -1;
		for (const s of Object.values(series)) {
			if (s.c && s.c.length) {
				lastIdx = Math.max(lastIdx, s.c[s.c.length - 1]);
			}
		}
		const runIdx = run && run.commit && model.shaIndex.has(run.commit.sha) ? model.shaIndex.get(run.commit.sha) : -1;
		const candidate = !!(L && L.candidate && run);
		// latest.js が履歴より古い (結果の無い再生成で前回分が残った) ときは履歴の最新を使う
		const useRun = !!run && Array.isArray(run.benchmarks) && (candidate || runIdx < 0 || runIdx >= lastIdx);
		// 履歴に含まれない実行 (ブランチのプレビューなど) は、グラフの右端に別扱いの点として足す
		const extraRun = useRun && runIdx < 0;
		const runBench = new Map();
		if (useRun) {
			for (const b of run.benchmarks) {
				runBench.set(b.name, b);
			}
		}

		const vm = {
			id, label, envs, series, run: useRun ? run : null, runIdx, lastIdx, candidate, extraRun, runBench,
			extraX: model.commits.length, rows: [], rowsByName: new Map(), groups: [],
		};

		let names;
		if (useRun) {
			names = run.benchmarks.map((b) => b.name);
		}
		else {
			names = Object.keys(series).filter((n) => {
				const c = series[n].c;
				return c && c.length && c[c.length - 1] === lastIdx;
			});
			if (!names.length) {
				names = Object.keys(series);
			}
		}
		const seen = new Set();
		for (const name of names) {
			if (seen.has(name)) {
				continue;
			}
			seen.add(name);
			const row = makeRow(model, vm, name);
			if (row) {
				vm.rows.push(row);
				vm.rowsByName.set(name, row);
			}
		}
		const gmap = new Map();
		for (const r of vm.rows) {
			if (!gmap.has(r.group)) {
				gmap.set(r.group, { name: r.group, count: 0, regressed: 0, improved: 0, budget: 0 });
			}
			const g = gmap.get(r.group);
			g.count++;
			if (r.alloc && r.alloc.budgetOk === false) {
				g.budget++;
			}
			if (r.verdict === "regressed") {
				g.regressed++;
			}
			if (r.verdict === "improved") {
				g.improved++;
			}
		}
		vm.groups = Array.from(gmap.values());
		vm.summary = summarize(model, vm);
		return vm;
	}

	/// 履歴の 1 系列を、グラフで使う点の配列にする
	function seriesPoints(model, vm, s) {
		if (!s || !s.c) {
			return [];
		}
		const pts = [];
		const lastByCpu = new Map();
		for (let i = 0; i < s.c.length; i++) {
			const env = vm.envs[s.e ? s.e[i] : -1] || null;
			const cpu = env ? env.cpu : null;
			const v = s.v ? s.v[i] : null;
			const r = s.r ? s.r[i] : null;
			let verdict = v == null ? null : CODE_TO_VERDICT[String(v)] || null;
			if (!verdict && isNum(r)) {
				verdict = "insufficient"; // 比はあるが判定が無い = 対が 3 組未満
			}
			const p = {
				i, x: s.c[i], commit: model.commits[s.c[i]] || null, env, cpu,
				cpuSlot: cpuSlot(model, cpu),
				m: s.m ? s.m[i] : null, q1: s.q1 ? s.q1[i] : null, q3: s.q3 ? s.q3[i] : null,
				r, rl: s.rl ? s.rl[i] : null, rh: s.rh ? s.rh[i] : null,
				verdict,
				bs: s.bs ? s.bs[i] : -1,
				a: s.a ? s.a[i] : null, ab: s.ab ? s.ab[i] : null,
				prev: lastByCpu.has(cpu) ? lastByCpu.get(cpu) : null,
			};
			pts.push(p);
			lastByCpu.set(cpu, p);
		}
		return pts;
	}

	function cpuSlot(model, cpu) {
		const i = model.cpuIndex.has(cpu) ? model.cpuIndex.get(cpu) : CPU_SLOTS;
		return Math.min(i, CPU_SLOTS);
	}

	/// 対の比較が判定まで済んだ点か (回数不足の点は比だけあって区間が無い)
	function pairedOk(p) {
		return !!p && isNum(p.r) && isNum(p.rl) && isNum(p.rh) && !!p.verdict && p.verdict !== "insufficient";
	}

	/// 系列の有無。hash の b= などの外から来た名前で Object.prototype のキーを拾わないように
	function hasSeries(vm, name) {
		return Object.prototype.hasOwnProperty.call(vm.series, name);
	}

	/// latest の 1 ベンチマークを、履歴の点と同じ形にする (グラフの右端に置く)
	function runPoint(model, vm, rb, prevPts) {
		const env = vm.run.env || null;
		const cpu = env ? env.cpu : null;
		let prev = null;
		for (let i = prevPts.length - 1; i >= 0; i--) {
			if (prevPts[i].cpu === cpu) {
				prev = prevPts[i];
				break;
			}
		}
		const p = rb.paired || null;
		const c = vm.run.commit || {};
		return {
			i: -1, x: vm.extraX, extra: true, candidate: vm.candidate,
			commit: { sha: c.sha, subject: c.subject, date: c.date, run_id: c.run_id, run_number: c.run_number },
			env: env ? { cpu, logical_cpus: env.logical_cpus, image: env.image, runner: env.runner, compiler: env.compiler } : null,
			cpu, cpuSlot: cpuSlot(model, cpu),
			m: rb.head ? rb.head.median : null, q1: rb.head ? rb.head.q1 : null, q3: rb.head ? rb.head.q3 : null,
			r: p ? p.ratio : null, rl: p ? p.ci_low : null, rh: p ? p.ci_high : null,
			verdict: p ? (VERDICT[p.verdict] && p.verdict !== "none" ? p.verdict : isNum(p.ratio) ? "insufficient" : null) : null,
			bs: -1,
			a: rb.alloc ? rb.alloc.allocs : null, ab: rb.alloc ? rb.alloc.bytes : null,
			prev,
		};
	}

	function makeRow(model, vm, name) {
		const s = hasSeries(vm, name) ? vm.series[name] : null;
		const rb = vm.runBench.get(name) || null;
		const meta = rb || s;
		if (!meta) {
			return null;
		}
		const pts = seriesPoints(model, vm, s);
		const threads = meta.threads || 1;
		const threshold = (vm.run && vm.run.threshold) || DEFAULT_THRESHOLD;
		const row = {
			name, group: meta.group || name.split("/")[0], title: meta.title || name, per: meta.per || "op",
			threads, budget: meta.alloc_budget == null ? null : meta.alloc_budget,
			thr: threads > 1 ? threshold * 2 : threshold,
			pts, runBench: rb, value: null, q1: null, q3: null, paired: null, verdict: "none",
			fallback: null, alloc: null, cpu: null, extra: null, spark: [],
		};

		// 最新の点。latest があればそれ、無ければ履歴の末尾
		let histUntil = pts.length; // 「前の点」を探す範囲
		if (rb) {
			row.value = rb.head ? rb.head.median : null;
			row.q1 = rb.head ? rb.head.q1 : null;
			row.q3 = rb.head ? rb.head.q3 : null;
			row.cpu = vm.run.env ? vm.run.env.cpu : null;
			const pr = rb.paired;
			if (pr && pr.verdict !== "insufficient" && isNum(pr.ratio) && isNum(pr.ci_low) && isNum(pr.ci_high)) {
				row.paired = { ratio: pr.ratio, lo: pr.ci_low, hi: pr.ci_high, change: pr.ratio - 1 };
				row.verdict = VERDICT[pr.verdict] && pr.verdict !== "none" ? pr.verdict : "unchanged";
			}
			else if (pr && (pr.verdict === "insufficient" || isNum(pr.ratio))) {
				// 対が 3 組未満: 区間が無いので判定せず、集まった組だけの比を参考に出す
				row.verdict = "insufficient";
				if (isNum(pr.ratio) && pr.ratio > 0) {
					row.fallback = { kind: "pairs", change: pr.ratio - 1, pairs: isNum(pr.pairs) ? pr.pairs : null };
				}
			}
			if (rb.alloc && isNum(rb.alloc.allocs)) {
				const a = rb.alloc;
				row.alloc = {
					allocs: a.allocs, bytes: a.bytes, budget: a.budget == null ? row.budget : a.budget,
					budgetOk: a.budget_ok == null ? null : !!a.budget_ok,
					baseAllocs: isNum(a.base_allocs) ? a.base_allocs : null, baseBytes: isNum(a.base_bytes) ? a.base_bytes : null,
					verdict: a.verdict || null, stable: a.stable !== false,
				};
			}
			if (vm.extraRun) {
				row.extra = runPoint(model, vm, rb, pts);
			}
			else {
				histUntil = pts.findIndex((p) => p.x >= vm.runIdx);
				if (histUntil < 0) {
					histUntil = pts.length;
				}
			}
		}
		else if (pts.length) {
			const p = pts[pts.length - 1];
			histUntil = pts.length - 1;
			row.value = p.m;
			row.q1 = p.q1;
			row.q3 = p.q3;
			row.cpu = p.cpu;
			if (pairedOk(p)) {
				row.paired = { ratio: p.r, lo: p.rl, hi: p.rh, change: p.r - 1 };
				row.verdict = p.verdict;
			}
			else if (p.verdict === "insufficient") {
				row.verdict = "insufficient";
				if (isNum(p.r) && p.r > 0) {
					row.fallback = { kind: "pairs", change: p.r - 1, pairs: null };
				}
			}
			if (isNum(p.a)) {
				let baseA = null;
				let baseB = null;
				if (p.bs >= 0) {
					const bp = pts.find((q) => q.x === p.bs);
					baseA = bp && isNum(bp.a) ? bp.a : null;
					baseB = bp && isNum(bp.ab) ? bp.ab : null;
				}
				row.alloc = {
					allocs: p.a, bytes: p.ab, budget: row.budget,
					budgetOk: row.budget == null ? null : p.a <= row.budget + 1e-9,
					baseAllocs: baseA, baseBytes: baseB,
					verdict: baseA == null ? null : cmpAlloc(p.a, baseA), stable: true,
				};
			}
		}

		// 比較対象が無いときは、同じ CPU の直前の点との差を参考に出す
		if (!row.paired && !row.fallback && isNum(row.value)) {
			for (let i = Math.min(histUntil, pts.length) - 1; i >= 0; i--) {
				if (pts[i].cpu === row.cpu && isNum(pts[i].m) && pts[i].m > 0) {
					row.fallback = { kind: "prev", change: row.value / pts[i].m - 1, prev: pts[i] };
					break;
				}
			}
		}

		// 推移の小さなグラフ: 最新と同じ CPU の点だけを使う (CPU が違う点の絶対値は比べられず、
		// 混ぜると CPU の入れ替わりが性能の変化に見えてしまう)。イメージの更新では線を切る
		const upto = rb && !vm.extraRun ? Math.min(pts.length, histUntil + 1) : pts.length;
		const want = row.extra ? SPARK_POINTS - 1 : SPARK_POINTS;
		for (let i = upto - 1; i >= 0 && row.spark.length < want; i--) {
			const p = pts[i];
			if (isNum(p.m) && (!row.cpu || p.cpu === row.cpu)) {
				row.spark.push({ y: p.m, envKey: envKey(p.env) });
			}
		}
		row.spark.reverse();
		if (row.extra && isNum(row.extra.m)) {
			row.spark.push({ y: row.extra.m, envKey: envKey(row.extra.env), cand: true });
		}

		row.search = (name + " " + row.title).toLowerCase();
		row.change = row.paired ? row.paired.change : row.fallback ? row.fallback.change : null;
		return row;
	}

	function envKey(env) {
		return env ? [env.cpu, env.image, env.compiler].join("|") : "";
	}

	function cmpAlloc(a, b) {
		const x = Math.round(a * 1e6);
		const y = Math.round(b * 1e6);
		return x > y ? "regressed" : x < y ? "improved" : "unchanged";
	}

	/// 確保回数に知らせるべき変化があるか (予算違反、base からの増減)
	function allocChanged(r) {
		const a = r.alloc;
		return !!a && (a.budgetOk === false || a.verdict === "regressed" || a.verdict === "improved");
	}

	/// ヒーローと KPI 用の集計
	function summarize(model, vm) {
		const sm = {
			regressed: 0, improved: 0, unchanged: 0, noisy: 0, insufficient: 0, none: 0,
			geomean: null, allocRegressed: 0, allocImproved: 0, allocChanged: 0, budgetViolations: 0, budgeted: 0, paired: 0,
			fallbacks: 0, hasPrior: false,
			commit: null, base: null, env: null, rounds: null, threshold: DEFAULT_THRESHOLD, runUrl: null,
		};
		let logSum = 0;
		// 今回より前の点があるか (初回の計測かどうか)。latest が履歴の外なら履歴の点はすべて前の点
		const before = vm.run ? (vm.extraRun ? Infinity : vm.runIdx) : vm.lastIdx;
		for (const r of vm.rows) {
			sm[r.verdict] = (sm[r.verdict] || 0) + 1;
			if (r.paired && r.paired.ratio > 0 && !GEOMEAN_EXCLUDED_GROUPS.has(r.group)) {
				logSum += Math.log(r.paired.ratio);
				sm.paired++;
			}
			if (r.fallback) {
				sm.fallbacks++;
			}
			if (!sm.hasPrior && r.pts.length && r.pts[0].x < before) {
				sm.hasPrior = true;
			}
			if (r.alloc) {
				if (r.alloc.verdict === "regressed") {
					sm.allocRegressed++;
				}
				if (r.alloc.verdict === "improved") {
					sm.allocImproved++;
				}
				if (allocChanged(r)) {
					sm.allocChanged++;
				}
				if (r.alloc.budget != null) {
					sm.budgeted++;
				}
				if (r.alloc.budgetOk === false) {
					sm.budgetViolations++;
				}
			}
		}
		if (sm.paired) {
			sm.geomean = Math.exp(logSum / sm.paired) - 1;
		}
		if (vm.run) {
			const run = vm.run;
			// bench-run.py の集計を優先する (回数不足は含まない)。判定できた行が無いときは出さない
			if (sm.paired && run.summary && isNum(run.summary.geomean_change)) {
				sm.geomean = run.summary.geomean_change;
			}
			sm.commit = run.commit || null;
			sm.base = run.base || null;
			sm.env = run.env || null;
			sm.rounds = run.rounds || null;
			sm.threshold = run.threshold || DEFAULT_THRESHOLD;
			const id = run.commit && run.commit.run_id;
			sm.runUrl = (L && L.run_url) || (id ? runUrl(model.repo, id) : null);
		}
		else if (vm.lastIdx >= 0) {
			const c = model.commits[vm.lastIdx];
			sm.commit = c || null;
			sm.runUrl = c && c.run_id ? runUrl(model.repo, c.run_id) : null;
			// 最新の点の base と環境 (どのベンチマークでも同じ実行なので先頭を見る)
			const r0 = vm.rows.find((r) => r.pts.length);
			if (r0) {
				const p = r0.pts[r0.pts.length - 1];
				sm.env = p.env;
				if (p.bs >= 0 && model.commits[p.bs]) {
					const bc = model.commits[p.bs];
					sm.base = { sha: bc.sha, run_id: bc.run_id, run_number: bc.run_number, kind: p.bs === vm.lastIdx - 1 ? "previous-master" : null };
				}
			}
		}
		return sm;
	}

	// ================================================================ 状態

	const model = buildModel();
	const hasData = model.variantIds.length > 0;
	const state = {
		view: "overview",
		variant: null,
		query: "",
		group: "all",
		changedOnly: false,
		verdictFilter: null, // KPI カードから: "regressed" | "improved" | "alloc"
		sort: "group",
		drawer: null,
		metric: "time",
		scale: "abs",
		rangePreset: null,
		range: null,
		cmpA: null,
		cmpB: null,
	};
	let visibleRows = [];
	let lastRowFocus = null;

	function vmNow() {
		return model.variants.get(state.variant);
	}

	function storageGet(key) {
		try {
			return localStorage.getItem(key);
		}
		catch {
			return null;
		}
	}

	function storageSet(key, value) {
		try {
			localStorage.setItem(key, value);
		}
		catch {
			// プライベートウィンドウなどでは保存できないが、表示には影響しない
		}
	}

	// ================================================================ URL (#v=...&b=...)

	function readHash() {
		const params = new URLSearchParams(location.hash.replace(/^#/, ""));
		return {
			view: params.get("view") === "compare" ? "compare" : "overview",
			v: params.get("v"),
			b: params.get("b"),
			a: params.get("a"),
		};
	}

	function writeHash() {
		const parts = [];
		if (state.view === "compare") {
			parts.push("view=compare");
		}
		if (state.variant) {
			parts.push("v=" + encodeURIComponent(state.variant));
		}
		if (state.view === "compare") {
			if (state.cmpA) {
				parts.push("a=" + encodeURIComponent(state.cmpA));
			}
			if (state.cmpB) {
				parts.push("b=" + encodeURIComponent(state.cmpB));
			}
		}
		else if (state.drawer) {
			parts.push("b=" + hashName(state.drawer));
		}
		const h = "#" + parts.join("&");
		if (location.hash !== h) {
			history.replaceState(null, "", h);
		}
	}

	function applyHash() {
		const h = readHash();
		if (h.v && model.variants.has(h.v)) {
			state.variant = h.v;
		}
		state.view = h.view;
		if (h.view === "compare") {
			state.cmpA = h.a || null;
			state.cmpB = h.b || null;
		}
		renderAll();
		if (h.view === "overview" && h.b) {
			const vm = vmNow();
			if (vm && (vm.rowsByName.has(h.b) || hasSeries(vm, h.b))) {
				openDrawer(h.b, { fromHash: true });
			}
		}
		else if (drawer.open) {
			drawer.close();
		}
	}

	// ================================================================ 全体の描画

	function renderAll() {
		const empty = !hasData;
		$("empty-state").hidden = !empty;
		$("view-overview").hidden = empty || state.view !== "overview";
		$("view-compare").hidden = empty || state.view !== "compare";
		$("guide").hidden = empty;
		$("tab-overview").toggleAttribute("aria-current", state.view === "overview");
		$("tab-compare").toggleAttribute("aria-current", state.view === "compare");
		if (state.view === "overview") {
			$("tab-overview").setAttribute("aria-current", "page");
		}
		else {
			$("tab-compare").setAttribute("aria-current", "page");
		}
		renderVariantPicker();
		renderBanner();
		renderFooter();
		if (empty) {
			document.querySelector(".view-tabs").hidden = true;
			return;
		}
		updateTabLinks();
		if (state.view === "overview") {
			renderHero();
			renderKpis();
			renderChips();
			renderTable();
		}
		else {
			renderCompare();
		}
	}

	function updateTabLinks() {
		$("tab-overview").href = "#v=" + encodeURIComponent(state.variant);
		$("tab-compare").href = "#view=compare&v=" + encodeURIComponent(state.variant);
		$("brand-link").href = "#v=" + encodeURIComponent(state.variant);
	}

	// ---------------------------------------------------------------- バリアント

	function renderVariantPicker() {
		const box = $("variant-picker");
		let sel = document.querySelector(".variant-select");
		if (!sel) {
			sel = document.createElement("select");
			sel.className = "variant-select select-inline";
			sel.setAttribute("aria-label", "コンパイラと構成");
			sel.addEventListener("change", () => setVariant(sel.value));
			box.insertAdjacentElement("afterend", sel);
		}
		if (!hasData) {
			box.hidden = true;
			sel.hidden = true;
			return;
		}
		// 矢印キーで選ぶと、ボタンを作り直す間にフォーカスが body へ落ちるので戻す
		const hadFocus = box.contains(document.activeElement);
		box.innerHTML = model.variantIds.map((id) => {
			const vm = model.variants.get(id);
			const on = id === state.variant;
			const reg = vm.summary.regressed;
			// CI を落とす予算超過は時間の悪化より先に知らせる (予算超過だけの構成にも印を付ける)
			const bad = vm.summary.budgetViolations;
			const flag = (bad ? '<span class="seg-flag" aria-hidden="true">✕</span>' : "")
				+ (reg ? '<span class="seg-flag" aria-hidden="true">▲</span>' : "");
			const notes = (bad ? ["予算超過 " + bad + " 件"] : []).concat(reg ? ["悪化 " + reg + " 件"] : []);
			const sr = notes.length ? '<span class="sr-only"> (' + notes.join("、") + ")</span>" : "";
			const title = notes.length ? ' title="最新の計測で' + notes.join("、") + '"' : "";
			return '<button type="button" role="radio" data-variant="' + esc(id) + '" aria-checked="' + on + '" tabindex="' + (on ? 0 : -1) + '"' + title + ">" + esc(vm.label) + flag + sr + "</button>";
		}).join("");
		box.classList.add("seg");
		if (hadFocus) {
			const on = box.querySelector('[aria-checked="true"]');
			if (on) {
				on.focus();
			}
		}
		sel.innerHTML = model.variantIds.map((id) => {
			const vm = model.variants.get(id);
			const reg = vm.summary.regressed;
			const bad = vm.summary.budgetViolations;
			return '<option value="' + esc(id) + '"' + (id === state.variant ? " selected" : "") + ">" + esc(vm.label)
				+ (bad ? " ✕" + bad : "") + (reg ? " ▲" + reg : "") + "</option>";
		}).join("");
	}

	function setVariant(id) {
		if (!model.variants.has(id) || id === state.variant) {
			return;
		}
		state.variant = id;
		storageSet(VARIANT_KEY, id);
		if (state.group !== "all" && !vmNow().groups.some((g) => g.name === state.group)) {
			state.group = "all";
		}
		const openName = state.drawer;
		renderAll();
		if (openName) {
			const vm = vmNow();
			if (vm.rowsByName.has(openName) || hasSeries(vm, openName)) {
				openDrawer(openName, { keepView: true });
			}
			else {
				drawer.close();
			}
		}
		writeHash();
	}

	// ---------------------------------------------------------------- プレビューの帯

	function renderBanner() {
		const el = $("preview-banner");
		if (!L || !L.candidate || !hasData) {
			el.hidden = true;
			return;
		}
		const c = L.commit || {};
		const url = c.sha ? commitUrl(model.repo, c.sha) : null;
		el.innerHTML = '<div class="preview-banner-inner"><span class="pb-icon" aria-hidden="true">!</span><span>'
			+ "<strong>プレビュー:</strong> ブランチ <code>" + esc(L.branch || "?") + "</code> の "
			+ (url ? '<a class="sha" href="' + esc(url) + '" target="_blank" rel="noopener">' + esc(sha7(c.sha)) + "</a>" : "")
			+ " (未マージ)。master の履歴と比較しています。</span></div>";
		el.hidden = false;
	}

	// ---------------------------------------------------------------- ヒーロー

	/// 日本語の後に続けるときの区切り。英数字で終わるときだけ空白を挟む ("master と" / "分岐元と")
	function jaSep(s) {
		return /[A-Za-z0-9)]$/.test(s) ? " " : "";
	}

	/// base の種類を言葉にする ("直前の master" など)。種類が分からないときは "base"
	function baseText(base) {
		if (!base) {
			return null;
		}
		return BASE_KIND[base.kind] || "base";
	}

	function renderHero() {
		const vm = vmNow();
		const sm = vm.summary;
		const c = sm.commit || {};
		const candidate = vm.candidate;

		$("hero-eyebrow").innerHTML = "<span>" + (candidate ? "このブランチの計測" : "最新の計測") + '</span><span class="pill">' + esc(vm.label) + "</span>"
			+ (candidate && L.branch ? '<span class="pill warn mono">' + esc(L.branch) + "</span>" : "");
		$("hero-subject").textContent = c.subject || "(件名なし)";

		const meta = [];
		if (c.sha) {
			meta.push("<span>" + ICON.commit + '<a class="sha-chip" href="' + esc(commitUrl(model.repo, c.sha)) + '" target="_blank" rel="noopener" title="GitHub でコミットを開く">' + esc(sha7(c.sha)) + "</a></span>");
		}
		if (c.date) {
			meta.push('<span title="' + esc(fmtDateTime(c.date)) + '">' + ICON.clock + esc(relTime(c.date)) + "</span>");
		}
		if (sm.runUrl) {
			meta.push("<span>" + ICON.play + '<a href="' + esc(sm.runUrl) + '" target="_blank" rel="noopener">CI 実行' + (c.run_number ? " #" + esc(c.run_number) : "") + ' <span class="ext" aria-hidden="true">↗</span></a></span>');
		}
		if (sm.base) {
			meta.push("<span>" + ICON.base + "比較対象: " + esc(baseText(sm.base))
				+ (sm.base.sha ? ' <a class="sha" href="' + esc(commitUrl(model.repo, sm.base.sha)) + '" target="_blank" rel="noopener">' + esc(sha7(sm.base.sha)) + "</a>" : "") + "</span>");
		}
		$("hero-meta").innerHTML = meta.join("");

		// 1 文の要約
		const total = vm.rows.length;
		const bt = baseText(sm.base) || "比較対象";
		const rounds = isNum(sm.rounds) ? esc(sm.rounds) + " 回ずつ" : "";
		const strong = (n) => "<strong>" + esc(n) + "</strong>";
		let sentence;
		if (sm.paired) {
			const parts = ["悪化 " + strong(sm.regressed), "改善 " + strong(sm.improved), "変化なし " + strong(sm.unchanged)];
			if (sm.noisy) {
				parts.push("ばらつき大 " + strong(sm.noisy));
			}
			if (sm.insufficient) {
				parts.push("回数不足 " + strong(sm.insufficient));
			}
			if (sm.none) {
				parts.push("比較なし " + strong(sm.none));
			}
			sentence = esc(bt) + jaSep(bt) + "と同じ VM で交互に" + (rounds ? " " + rounds : "") + "計測: " + parts.join(" / ")
				+ ' <span class="muted nowrap">(' + esc(total) + " 件、しきい値 ±" + esc(Math.round(sm.threshold * 100)) + NNBSP + "%)</span>";
		}
		else if (sm.insufficient) {
			sentence = esc(bt) + jaSep(bt) + "と同じ VM で交互に計測しましたが、対になったラウンドが " + MIN_PAIRS + " 組に満たないため判定していません。"
				+ "変化の列の ~ 付きの値は、集まった組だけから求めた参考値です。";
		}
		else if (sm.fallbacks) {
			sentence = "今回は比較対象 (base) の実行ファイルが無かったため、対の比較をしていません。"
				+ "変化の列には、同じ CPU の直前の点との差を参考として出しています。";
		}
		else if (!sm.hasPrior) {
			sentence = candidate
				? "比較対象 (base) も master の計測もまだ無いため、比較と推移はありません。今回の値だけを表示しています。"
				: "初回の計測のため、比較と推移はまだありません。次に master へ push したときから出ます。";
		}
		else {
			sentence = "今回は比較対象 (base) の実行ファイルが無く、同じ CPU の過去の点も無いため、変化は出していません。";
		}
		$("hero-sentence").innerHTML = sentence;

		// 大きな数字: 全ベンチマークの幾何平均。比較が無いときは枠ごと出さない
		const fig = $("hero-figure");
		const num = $("hero-number");
		const cap = $("hero-caption");
		num.classList.remove("is-regress", "is-improve", "is-muted");
		if (!isNum(sm.geomean)) {
			fig.hidden = true;
			num.textContent = "—";
			num.removeAttribute("aria-label");
			cap.textContent = "";
			return;
		}
		fig.hidden = false;
		const g = sm.geomean;
		num.innerHTML = esc(fmtPctNum(g)) + '<span class="unit">%</span>';
		// 色を付けるのは、しきい値の半分以上動き、同じ向きに判定の付いたベンチマークがあるときだけ。
		// そうでなければ数字は目立たせない (ばらつき大の 1 件に引っ張られた値などを強調しない)
		const dirCount = g > 0 ? sm.regressed : sm.improved;
		const meaningful = Math.abs(g) >= sm.threshold / 2 && dirCount > 0;
		let glyph;
		let text;
		if (Math.abs(g) < sm.threshold / 2) {
			glyph = "≈";
			text = "ほぼ変化なし";
		}
		else {
			const big = Math.abs(g) >= sm.threshold;
			glyph = g > 0 ? "▲" : "▼";
			text = g > 0 ? (big ? "全体に遅くなった" : "全体にやや遅い") : big ? "全体に速くなった" : "全体にやや速い";
			if (!dirCount) {
				text += g > 0 ? " (個別の悪化なし)" : " (個別の改善なし)";
			}
		}
		num.classList.add(meaningful ? (g > 0 ? "is-regress" : "is-improve") : "is-muted");
		const cls = meaningful ? (g > 0 ? "c-regressed" : "c-improved") : "";
		const bs = baseText(sm.base) || "base";
		cap.innerHTML = '<span class="' + cls + '" aria-hidden="true">' + glyph + "</span>" + esc(text) + ' <span class="muted">· ' + esc(bs) + jaSep(bs) + "比、" + esc(sm.paired) + " 件</span>";
		num.setAttribute("aria-label", "全体の変化 " + fmtPct(g));
	}

	// ---------------------------------------------------------------- KPI

	function shortName(name) {
		const i = name.indexOf("/");
		return i >= 0 ? name.slice(i + 1) : name;
	}

	function renderKpis() {
		const vm = vmNow();
		const sm = vm.summary;
		const pct = Math.round(sm.threshold * 100);

		const worst = (verdict, dir) => vm.rows.filter((r) => r.verdict === verdict && r.paired).sort((a, b) => dir * (b.paired.change - a.paired.change))[0];

		// 何も比べられなかった (初回、base が無い、回数不足) ときは、悪化と改善を 1 枚にまとめる
		const compared = sm.paired > 0;
		$("kpis").classList.toggle("is-3", !compared);
		$("kpi-regressed").hidden = !compared;
		$("kpi-improved").hidden = !compared;
		$("kpi-nocmp").hidden = compared;
		if (!compared) {
			$("kpi-nocmp-sub").textContent = sm.insufficient
				? "対が " + MIN_PAIRS + " 組未満のため判定なし"
				: sm.fallbacks
					? "比較対象 (base) が無いため判定なし"
					: !sm.hasPrior && !vm.candidate ? "初回の計測のため比較なし" : "比較対象 (base) がありません";
		}

		const kr = $("kpi-regressed");
		kr.classList.toggle("is-bad", sm.regressed > 0);
		kr.classList.toggle("is-zero", !sm.regressed);
		kr.disabled = !sm.regressed;
		kr.setAttribute("aria-pressed", String(state.verdictFilter === "regressed"));
		$("kpi-regressed-value").textContent = String(sm.regressed);
		const wr = worst("regressed", 1);
		$("kpi-regressed-sub").textContent = wr ? "最大 " + fmtPct(wr.paired.change) + " · " + shortName(wr.name) : "±" + pct + " % を超える悪化なし";
		kr.title = wr ? "悪化したベンチマークだけを表示 (悪化の大きい順)" : "";

		const ki = $("kpi-improved");
		ki.classList.toggle("is-improve", sm.improved > 0);
		ki.classList.toggle("is-zero", !sm.improved);
		ki.disabled = !sm.improved;
		ki.setAttribute("aria-pressed", String(state.verdictFilter === "improved"));
		$("kpi-improved-value").textContent = String(sm.improved);
		const wi = worst("improved", -1);
		$("kpi-improved-sub").textContent = wi ? "最大 " + fmtPct(wi.paired.change) + " · " + shortName(wi.name) : "±" + pct + " % を超える改善なし";
		ki.title = wi ? "改善したベンチマークだけを表示 (改善の大きい順)" : "";

		// ヒープ確保: 予算違反 (CI が落ちる) > 確保回数の増加 (予算内でも決定的な悪化) > 予算内 の順に知らせる
		const kb = $("kpi-budget");
		const bad = sm.budgetViolations > 0;
		const up = sm.allocRegressed > 0;
		kb.classList.toggle("is-bad", bad);
		kb.classList.toggle("is-warn", !bad && up);
		kb.classList.toggle("is-good", !bad && !up && sm.budgeted > 0);
		kb.disabled = !sm.allocChanged;
		kb.setAttribute("aria-pressed", String(state.verdictFilter === "alloc"));
		kb.title = sm.allocChanged ? "確保回数に変化があったもの (予算違反・増加・減少) だけを表示" : "";
		let value;
		const subs = [];
		if (bad) {
			value = '<span class="kpi-mark" aria-hidden="true">✕</span>' + esc(sm.budgetViolations) + ' <span class="kpi-mark">件の予算違反</span>';
			subs.push("CI が失敗します");
			if (up) {
				subs.push("増加 " + sm.allocRegressed + " 件");
			}
		}
		else if (up) {
			value = '<span class="kpi-mark warn" aria-hidden="true">▲</span>' + esc(sm.allocRegressed) + ' <span class="kpi-mark">件で増加</span>';
			subs.push(sm.budgeted ? "予算違反はなし (予算つき " + sm.budgeted + " 件)" : "予算の設定なし");
		}
		else if (sm.budgeted) {
			value = '<span class="kpi-mark ok" aria-hidden="true">✓</span>予算内';
			subs.push("予算つき " + sm.budgeted + " 件すべて");
		}
		else {
			value = "—";
			subs.push("予算の設定なし");
		}
		if (sm.allocImproved) {
			subs.push("減少 " + sm.allocImproved + " 件");
		}
		$("kpi-budget-value").innerHTML = value;
		$("kpi-budget-sub").textContent = subs.join(" · ");

		const env = sm.env;
		const kv = $("kpi-env-value");
		const ks = $("kpi-env-sub");
		if (env) {
			kv.textContent = cpuShort(env.cpu);
			const bits = [];
			if (env.logical_cpus) {
				bits.push(env.logical_cpus + " 論理コア");
			}
			if (env.image) {
				bits.push(env.image);
			}
			if (env.compiler) {
				bits.push(env.compiler);
			}
			ks.textContent = bits.join(" · ");
			$("kpi-env").title = [env.cpu, env.runner, env.image, env.compiler].filter(Boolean).join("\n");
		}
		else {
			kv.textContent = "—";
			ks.textContent = "";
		}
	}

	// ---------------------------------------------------------------- ツールバー

	function renderChips() {
		const vm = vmNow();
		const chip = (key, label, count, g) => {
			const on = state.group === key;
			const flag = !g ? ""
				: g.budget ? '<span class="chip-flag" aria-hidden="true">✕</span><span class="sr-only">予算超過あり</span>'
				: g.regressed ? '<span class="chip-flag" aria-hidden="true">▲</span><span class="sr-only">悪化あり</span>'
				: "";
			return '<button type="button" class="chip" data-group="' + esc(key) + '" aria-pressed="' + on + '">'
				+ esc(label) + '<span class="chip-count">' + count + "</span>" + flag + "</button>";
		};
		$("group-chips").innerHTML = chip("all", "すべて", vm.rows.length, null)
			+ vm.groups.map((g) => chip(g.name, g.name, g.count, g)).join("");
		$("search").value = state.query;
		$("changed-only").checked = state.changedOnly;
		$("sort").value = state.sort;
	}

	function isChanged(r) {
		if (r.verdict === "regressed" || r.verdict === "improved" || r.verdict === "noisy") {
			return true;
		}
		if (allocChanged(r)) {
			return true;
		}
		return !r.paired && !!r.fallback && Math.abs(r.fallback.change) >= r.thr;
	}

	const VERDICT_FILTER = {
		regressed: { glyph: "▲", cls: "c-regressed", label: "悪化のみ", test: (r) => r.verdict === "regressed" },
		improved: { glyph: "▼", cls: "c-improved", label: "改善のみ", test: (r) => r.verdict === "improved" },
		alloc: { glyph: "", cls: "", label: "確保の変化のみ", test: allocChanged },
	};

	function filterRows(vm) {
		const tokens = state.query.toLowerCase().split(/\s+/).filter(Boolean);
		const inc = tokens.filter((t) => t[0] !== "-" || t.length === 1);
		const exc = tokens.filter((t) => t[0] === "-" && t.length > 1).map((t) => t.slice(1));
		const vf = state.verdictFilter ? VERDICT_FILTER[state.verdictFilter] : null;
		let rows = vm.rows.filter((r) => {
			if (state.group !== "all" && r.group !== state.group) {
				return false;
			}
			if (state.changedOnly && !isChanged(r)) {
				return false;
			}
			if (vf && !vf.test(r)) {
				return false;
			}
			for (const t of inc) {
				if (r.search.indexOf(t) < 0) {
					return false;
				}
			}
			for (const t of exc) {
				if (r.search.indexOf(t) >= 0) {
					return false;
				}
			}
			return true;
		});
		const chg = (r) => (isNum(r.change) ? r.change : null);
		switch (state.sort) {
			case "regressed":
				rows = rows.slice().sort((a, b) => (a.verdict === "regressed" ? 0 : 1) - (b.verdict === "regressed" ? 0 : 1)
					|| (chg(b) == null ? -1e9 : chg(b)) - (chg(a) == null ? -1e9 : chg(a)));
				break;
			case "improved":
				rows = rows.slice().sort((a, b) => (a.verdict === "improved" ? 0 : 1) - (b.verdict === "improved" ? 0 : 1)
					|| (chg(a) == null ? 1e9 : chg(a)) - (chg(b) == null ? 1e9 : chg(b)));
				break;
			case "name":
				rows = rows.slice().sort((a, b) => a.name.localeCompare(b.name));
				break;
			case "fast":
				rows = rows.slice().sort((a, b) => (isNum(a.value) ? a.value : 1e30) - (isNum(b.value) ? b.value : 1e30));
				break;
			default:
				break; // グループ順 (データの並び。グループは初出順)
		}
		return rows;
	}

	// ---------------------------------------------------------------- 表

	function renderTable() {
		const vm = vmNow();
		const rows = filterRows(vm);
		visibleRows = rows;
		const grouped = state.sort === "group";
		const html = [];
		if (grouped) {
			const byGroup = new Map();
			for (const r of rows) {
				if (!byGroup.has(r.group)) {
					byGroup.set(r.group, []);
				}
				byGroup.get(r.group).push(r);
			}
			for (const g of vm.groups) {
				const list = byGroup.get(g.name);
				if (!list) {
					continue;
				}
				const flags = [];
				if (g.budget) {
					flags.push('<span><span class="c-regressed" aria-hidden="true">✕</span> 予算超過 ' + g.budget + "</span>");
				}
				if (g.regressed) {
					flags.push('<span><span class="c-regressed" aria-hidden="true">▲</span> 悪化 ' + g.regressed + "</span>");
				}
				if (g.improved) {
					flags.push('<span><span class="c-improved" aria-hidden="true">▼</span> 改善 ' + g.improved + "</span>");
				}
				html.push('<tr class="group-row"><th colspan="6" scope="colgroup"><div class="g-inner"><span class="group-name">' + esc(g.name)
					+ '</span><span class="group-count">' + list.length + (list.length !== g.count ? " / " + g.count : "") + ' 件</span><span class="group-flags">' + flags.join("") + "</span></div></th></tr>");
				for (const r of list) {
					html.push(rowHtml(r, false));
				}
			}
		}
		else {
			for (const r of rows) {
				html.push(rowHtml(r, true));
			}
		}
		if (!rows.length) {
			html.push('<tr class="empty-row"><td colspan="6"><div>条件に合うベンチマークはありません。</div><button type="button" class="btn" data-action="clear-filters">条件をクリア</button></td></tr>');
		}
		$("bench-body").innerHTML = html.join("");
		const filtered = rows.length !== vm.rows.length;
		const vf = state.verdictFilter ? VERDICT_FILTER[state.verdictFilter] : null;
		const chip = vf
			? '<button type="button" class="filter-chip" data-action="clear-verdict" title="この絞り込みを外す">'
				+ (vf.glyph ? '<span class="' + vf.cls + '" aria-hidden="true">' + vf.glyph + "</span>" : "") + esc(vf.label)
				+ '<span class="x" aria-hidden="true">×</span><span class="sr-only"> (解除)</span></button>'
			: "";
		$("result-count").innerHTML = chip + (filtered
			? esc(vm.rows.length) + " 件中 <strong>" + esc(rows.length) + '</strong> 件を表示 · <button type="button" class="linklike" data-action="clear-filters">条件をクリア</button>'
			: esc(vm.rows.length) + " 件");
		$("dl-csv-sub").textContent = "表示中の " + rows.length + " 行";
		if (state.drawer) {
			markOpenRow(state.drawer);
		}
	}

	function rowHtml(r, showGroup) {
		let cls = "row";
		if (r.verdict === "regressed" || r.verdict === "improved") {
			cls += " is-" + r.verdict;
		}
		else if (r.alloc && r.alloc.budgetOk === false) {
			cls += " is-budget-bad";
		}
		const hasChange = !!(r.paired || r.fallback);
		return '<tr class="' + cls + '" tabindex="0" data-name="' + esc(r.name) + '">'
			+ '<td class="cell-name"><span class="b-title" title="' + esc(r.title) + '">' + esc(r.title) + "</span>"
			+ '<span class="b-name">' + (showGroup ? '<span class="tag">' + esc(r.group) + "</span>" : "") + '<span class="nm">' + esc(r.name) + "</span>" + gapHtml(r) + "</span></td>"
			+ '<td class="cell-value num"><span class="val">' + esc(fmtTime(r.value)) + '</span><span class="per">/ ' + esc(r.per) + "</span></td>"
			+ '<td class="cell-change"><div class="change' + (hasChange ? "" : " is-none") + '">' + pctHtml(r) + (hasChange ? forestSvg(r) : "") + "</div></td>"
			+ '<td class="cell-verdict">' + badgeHtml(r.verdict, "", r.thr) + "</td>"
			+ '<td class="cell-alloc num">' + allocHtml(r) + "</td>"
			+ '<td class="cell-trend">' + sparkSvg(r) + "</td>"
			+ "</tr>";
	}

	/// 構成の間の大きな差の印 (詳細の「構成ごとの比較」への入口)
	function gapHtml(r) {
		if (!r.gap) {
			return "";
		}
		const t = "同じコミットで " + r.gap.label + " は " + fmtTime(r.gap.value) + " (この構成の 1/" + fmtTimes(r.gap.ratio) + ")。構成ごとに別の VM で計測した値です";
		return '<span class="vgap" title="' + esc(t) + '">' + esc(r.gap.label) + " より " + esc(fmtTimes(r.gap.ratio)) + " 倍遅い</span>";
	}

	/// 判定の説明 (バッジの title)。bench_common.py の paired_analysis と同じ規則
	function verdictDesc(verdict, thr) {
		const t = "±" + Math.round((thr || DEFAULT_THRESHOLD) * 100) + " %";
		switch (verdict) {
			case "regressed":
				return "悪化: 95% 区間が 0 をまたがず、変化 (比の中央値) がしきい値 " + t + " 以上";
			case "improved":
				return "改善: 95% 区間が 0 をまたがず、変化 (比の中央値) がしきい値 " + t + " 以上";
			case "unchanged":
				return "変化なし: 95% 区間の全体がしきい値 " + t + " の帯に収まっている";
			case "noisy":
				return "ばらつき大: 95% 区間がしきい値 " + t + " の帯からはみ出していて、悪化とも変化なしとも言えない";
			case "insufficient":
				return "回数不足: 対になったラウンドが " + MIN_PAIRS + " 組未満のため判定していない";
			default:
				return "比較なし: 比較対象 (base) が無い";
		}
	}

	function badgeHtml(verdict, extraCls, thr) {
		const key = VERDICT[verdict] ? verdict : "none";
		const v = VERDICT[key];
		return '<span class="badge v-' + key + (extraCls ? " " + extraCls : "") + '" title="' + esc(verdictDesc(key, thr)) + '"><span class="glyph" aria-hidden="true">' + v.glyph + "</span>" + v.label + "</span>";
	}

	function pctHtml(r) {
		if (r.paired) {
			const sig = r.verdict === "regressed" || r.verdict === "improved";
			return '<span class="pct' + (sig ? " is-sig" : "") + '">' + esc(fmtPct(r.paired.change))
				+ '<span class="ci">' + esc(fmtCi(r.paired.lo, r.paired.hi)) + "</span></span>";
		}
		if (r.fallback && r.fallback.kind === "pairs") {
			const n = r.fallback.pairs;
			const t = "対になったラウンドが " + (isNum(n) ? n + " 組" : MIN_PAIRS + " 組未満") + "しかないため、区間を出さずにその組だけの比を参考に表示";
			return '<span class="pct is-fallback" title="' + esc(t) + '">~' + esc(fmtPct(r.fallback.change)) + '<span class="ci">' + (isNum(n) ? "対 " + esc(n) + " 組のみ" : "回数不足") + "</span></span>";
		}
		if (r.fallback) {
			const prev = r.fallback.prev;
			const t = "比較対象が無いため、同じ CPU の直前の点" + (prev && prev.commit ? " (" + sha7(prev.commit.sha) + ")" : "") + " との差 (参考)";
			return '<span class="pct is-fallback" title="' + esc(t) + '">~' + esc(fmtPct(r.fallback.change)) + '<span class="ci">直前の点比</span></span>';
		}
		return '<span class="pct muted" title="比較対象がありません">—</span>';
	}

	function allocHtml(r) {
		const a = r.alloc;
		if (!a) {
			return '<span class="muted">—</span>';
		}
		const now = allocFmt(a.allocs, r.per);
		const changed = (a.verdict === "regressed" || a.verdict === "improved") && isNum(a.baseAllocs);
		// ラウンドごとに回数が違った: 増減の印は信用できないので出さず、2 行目に「不安定」を出す
		const unstable = !a.stable ? '<span class="alloc-unstable" title="ラウンドごとに確保回数が違いました。増減の判定は参考になりません">不安定</span>' : "";
		let mark = "";
		if (!unstable && a.verdict === "regressed") {
			mark = '<span class="alloc-up" aria-hidden="true">▲</span><span class="sr-only">増加 </span>';
		}
		else if (!unstable && a.verdict === "improved") {
			mark = '<span class="alloc-down" aria-hidden="true">▼</span><span class="sr-only">減少 </span>';
		}
		const tips = [now.text + " 回 / " + r.per];
		if (now.title) {
			tips.push(now.title);
		}
		if (isNum(a.bytes)) {
			tips.push(fmtBytes(a.bytes) + " / " + r.per);
		}
		if (isNum(a.baseAllocs)) {
			tips.push("base " + fmtAllocs(a.baseAllocs, r.per) + " 回" + (isNum(a.baseBytes) ? " · " + fmtBytes(a.baseBytes) : ""));
		}
		let budget = "";
		if (a.budget != null) {
			budget = a.budgetOk === false
				? '<span class="budget bad"><span aria-hidden="true">✕</span> 予算 ' + esc(a.budget) + " 超過</span>"
				: '<span class="budget ok"><span class="ck" aria-hidden="true">✓</span> 予算 ' + esc(a.budget) + "</span>";
		}
		// 増減したときは base の値も並べる ("9 → 12")
		const from = changed ? '<span class="alloc-from">' + esc(fmtAllocs(a.baseAllocs, r.per)) + '<span aria-hidden="true"> →</span><span class="sr-only"> から </span></span>' : "";
		const second = unstable || budget ? '<span class="alloc-sub">' + unstable + budget + "</span>" : "";
		return '<div class="alloc"><span class="alloc-val' + (a.allocs === 0 && !changed ? " is-zero" : "") + '" title="' + esc(tips.join("\n")) + '">' + mark + from + esc(now.text) + "</span>" + second + "</div>";
	}

	/// 行内の変化: しきい値の帯、0 の線、95 % 区間、推定値
	function forestSvg(r) {
		const W = 116;
		const Hh = 24;
		const cx = W / 2;
		const half = W / 2 - 7;
		const x = (c) => cx + (clamp(c, -FOREST_DOMAIN, FOREST_DOMAIN) / FOREST_DOMAIN) * half;
		const parts = [];
		parts.push('<rect class="fp-band" x="' + x(-r.thr).toFixed(1) + '" y="4" width="' + (x(r.thr) - x(-r.thr)).toFixed(1) + '" height="16" rx="2"/>');
		parts.push('<line class="fp-zero" x1="' + cx + '" x2="' + cx + '" y1="2" y2="22"/>');
		let label;
		if (r.paired) {
			const cls = "c-" + (r.verdict || "unchanged");
			const lo = r.paired.lo - 1;
			const hi = r.paired.hi - 1;
			if (isNum(lo) && isNum(hi)) {
				parts.push('<line class="fp-ci ' + cls + '" x1="' + x(lo).toFixed(1) + '" x2="' + x(hi).toFixed(1) + '" y1="12" y2="12"/>');
				if (lo < -FOREST_DOMAIN) {
					parts.push('<path class="fp-arrow ' + cls + '" d="M' + (cx - half - 6) + " 12l5-4v8z" + '"/>');
				}
				if (hi > FOREST_DOMAIN) {
					parts.push('<path class="fp-arrow ' + cls + '" d="M' + (cx + half + 6) + " 12l-5-4v8z" + '"/>');
				}
			}
			parts.push('<circle class="fp-pt ' + cls + '" cx="' + x(r.paired.change).toFixed(1) + '" cy="12" r="4.5"/>');
			label = "変化 " + fmtPct(r.paired.change) + "、95% 信頼区間 " + fmtCi(r.paired.lo, r.paired.hi) + "。灰色の帯はしきい値 ±" + Math.round(r.thr * 100) + " %";
		}
		else if (r.fallback) {
			const c = r.fallback.change;
			parts.push('<circle class="fp-fallback" cx="' + x(c).toFixed(1) + '" cy="12" r="3.5"/>');
			if (Math.abs(c) > FOREST_DOMAIN) {
				parts.push('<path class="fp-arrow c-none" d="M' + (c < 0 ? cx - half - 6 + " 12l5-4v8z" : cx + half + 6 + " 12l-5-4v8z") + '"/>');
			}
			label = r.fallback.kind === "pairs"
				? "参考: 集まった組だけの比 " + fmtPct(c) + " (回数不足のため区間なし)"
				: "参考: 直前の点との差 " + fmtPct(c) + " (対の比較なし)";
		}
		else {
			label = "比較なし";
		}
		return '<svg class="forest" width="' + W + '" height="' + Hh + '" viewBox="0 0 ' + W + " " + Hh + '" role="img" aria-label="' + esc(label) + '">' + parts.join("") + "</svg>";
	}

	/// 行内の推移: 直近 30 点。環境 (CPU・イメージ) が変わった所で線を切る
	function sparkSvg(r) {
		const pts = r.spark;
		if (pts.length < 2) {
			return '<span class="spark-empty">' + (pts.length ? "履歴 1 点" : "履歴なし") + "</span>";
		}
		const W = 120;
		const Hh = 30;
		let lo = Infinity;
		let hi = -Infinity;
		for (const p of pts) {
			lo = Math.min(lo, p.y);
			hi = Math.max(hi, p.y);
		}
		if (hi - lo < hi * 0.02) {
			const mid = (hi + lo) / 2;
			lo = mid * 0.99;
			hi = mid * 1.01;
		}
		const n = pts.length;
		const x = (i) => 4 + (i * (W - 8)) / (n - 1);
		const y = (v) => Hh - 4 - ((v - lo) / (hi - lo)) * (Hh - 8);
		const segs = [];
		let cur = [];
		for (let i = 0; i < n; i++) {
			if (i > 0 && pts[i].envKey !== pts[i - 1].envKey) {
				segs.push(cur);
				cur = [];
			}
			cur.push(i);
		}
		segs.push(cur);
		const parts = [];
		for (const s of segs) {
			if (s.length === 1) {
				const i = s[0];
				if (i !== n - 1) {
					parts.push('<circle class="spark-dot" cx="' + x(i).toFixed(1) + '" cy="' + y(pts[i].y).toFixed(1) + '" r="1.4"/>');
				}
				continue;
			}
			const d = s.map((i, k) => (k ? "L" : "M") + x(i).toFixed(1) + " " + y(pts[i].y).toFixed(1)).join("");
			parts.push('<path class="spark-line" d="' + d + '"/>');
		}
		const last = pts[n - 1];
		const lx = x(n - 1);
		const ly = y(last.y);
		if (last.cand) {
			parts.push('<path class="spark-cand" d="M' + lx.toFixed(1) + " " + (ly - 4).toFixed(1) + "l4 4-4 4-4-4z" + '"/>');
		}
		else {
			parts.push('<circle class="spark-last" cx="' + lx.toFixed(1) + '" cy="' + ly.toFixed(1) + '" r="3"/>');
		}
		let mn = Infinity;
		let mx = -Infinity;
		for (const p of pts) {
			mn = Math.min(mn, p.y);
			mx = Math.max(mx, p.y);
		}
		const label = "同じ CPU での直近 " + n + " 回の推移: " + fmtTime(pts[0].y) + " から " + fmtTime(last.y) + "。最小 " + fmtTime(mn) + "、最大 " + fmtTime(mx);
		return '<svg class="spark" width="' + W + '" height="' + Hh + '" viewBox="0 0 ' + W + " " + Hh + '" role="img" aria-label="' + esc(label) + '">' + parts.join("") + "</svg>";
	}

	function markOpenRow(name) {
		for (const tr of document.querySelectorAll("tr.row.is-open")) {
			tr.classList.remove("is-open");
		}
		if (!name) {
			return;
		}
		for (const tr of document.querySelectorAll("tr.row")) {
			if (tr.dataset.name === name) {
				tr.classList.add("is-open");
			}
		}
	}

	// ================================================================ 詳細 (ドロワー)

	let chartCtl = null;
	let drawerRow = null;
	let resizeObs = null;

	function rowFor(name) {
		const vm = vmNow();
		return vm.rowsByName.get(name) || makeRow(model, vm, name);
	}

	function defaultPreset(n) {
		return n > 150 ? "100" : "all";
	}

	function openDrawer(name, opts) {
		opts = opts || {};
		const row = rowFor(name);
		if (!row) {
			return;
		}
		const same = state.drawer === name && drawer.open;
		state.drawer = name;
		drawerRow = row;
		if (!same) {
			if (!opts.keepView) {
				state.metric = "time";
				state.scale = "abs";
			}
			state.range = null;
			state.rangePreset = defaultPreset(row.pts.length + (row.extra ? 1 : 0));
		}
		if (state.metric !== "time" && !row.pts.some((p) => isNum(p.a))) {
			state.metric = "time";
		}
		// 同じベンチマークを描き直す (構成の切り替えなど) ときは、読んでいた位置を保つ
		const scrollers = same ? ["drawer-body", "drawer-inner"].map((id) => [id, $(id) ? $(id).scrollTop : 0]) : [];
		renderDrawer(row);
		for (const [id, top] of scrollers) {
			if ($(id)) {
				$(id).scrollTop = top;
			}
		}
		if (!drawer.open) {
			const active = document.activeElement;
			if (active && active.closest && active.closest("tr.row")) {
				lastRowFocus = active.closest("tr.row").dataset.name;
			}
			drawer.showModal();
			const t = $("drawer-title");
			if (t) {
				t.focus({ preventScroll: true });
			}
		}
		markOpenRow(name);
		writeHash();
	}

	function renderDrawer(row) {
		const vm = vmNow();
		const budgetPill = row.budget == null
			? '<span class="pill">予算なし</span>'
			: row.alloc && row.alloc.budgetOk === false
				? '<span class="pill bad"><span aria-hidden="true">✕</span> 予算 ' + esc(row.budget) + " 回/op を超過</span>"
				: '<span class="pill good"><span aria-hidden="true">✓</span> 予算 ' + esc(row.budget) + " 回/op</span>";
		const hasAlloc = row.pts.some((p) => isNum(p.a)) || (row.extra && isNum(row.extra.a));
		const metricSeg = segHtml("metric", [["time", "時間"], ["allocs", "確保回数", !hasAlloc], ["bytes", "確保バイト", !hasAlloc]], state.metric, "指標");
		const scaleSeg = segHtml("scale", [["abs", "絶対値"], ["rel", "変化率", state.metric !== "time"]], state.metric === "time" ? state.scale : "abs", "縦軸");
		const rangeSeg = segHtml("range", [["30", "直近 30"], ["100", "直近 100"], ["all", "全期間"]], state.range ? null : state.rangePreset, "表示範囲");

		$("drawer-inner").innerHTML = ''
			+ '<header class="drawer-head">'
			+ '<div class="drawer-head-top"><h2 class="drawer-title" id="drawer-title" tabindex="-1">' + esc(row.title) + "</h2>"
			+ '<div class="drawer-actions">'
			+ '<button type="button" class="icon-btn" data-action="copy-link" title="このベンチマークへのリンクをコピー" aria-label="リンクをコピー">' + ICON.link + "</button>"
			+ '<button type="button" class="icon-btn" data-action="close-drawer" title="閉じる (Esc)" aria-label="閉じる">' + ICON.close + "</button>"
			+ "</div></div>"
			+ '<div class="drawer-name">' + esc(row.name) + "</div>"
			+ '<div class="drawer-chips"><span class="pill mono">' + esc(row.group) + '</span><span class="pill">1 op = ' + esc(perLabel(row.per)) + "</span>"
			+ '<span class="pill">' + esc(row.threads) + " スレッド</span>" + budgetPill
			+ '<span class="pill accent">' + esc(vm.label) + "</span></div>"
			+ "</header>"
			+ '<div class="drawer-body" id="drawer-body">'
			+ '<section class="d-section" aria-labelledby="d-hist-title">'
			+ '<div class="d-section-head"><h3 id="d-hist-title">推移</h3>' + metricSeg + "</div>"
			+ '<p class="d-sub" id="d-axis-note">' + esc(METRICS[state.metric].axis) + "</p>"
			+ '<div class="chart-controls">' + scaleSeg + rangeSeg + '<span class="spacer"></span><span class="pill accent" id="d-zoom-pill" hidden></span></div>'
			+ '<div class="chart" id="d-chart" tabindex="0" aria-label="推移のグラフ。左右の矢印キーで点を移動し、Enter で GitHub のコミットを開きます"></div>'
			+ '<p class="sr-only" id="d-live" aria-live="polite"></p>'
			+ '<div class="legend" id="d-legend"></div>'
			+ '<p class="chart-hint">ドラッグで範囲を拡大 · 点をクリックすると GitHub のコミットを開きます · 横軸はコミットの並び (古い → 新しい)</p>'
			+ '<details class="data-table-wrap" id="d-table-wrap"><summary>表で見る</summary><div class="data-scroll" id="d-table"></div></details>'
			+ "</section>"
			+ variantsSectionHtml(row)
			+ runSectionHtml(row)
			+ aboutHtml(row)
			+ "</div>";

		const chartEl = $("d-chart");
		chartCtl = createHistoryChart(chartEl, row);
		chartCtl.render();
		renderRunCharts(row);

		if (resizeObs) {
			resizeObs.disconnect();
		}
		if (typeof ResizeObserver === "function") {
			let lastW = chartEl.clientWidth;
			let raf = 0;
			resizeObs = new ResizeObserver(() => {
				const w = chartEl.clientWidth;
				if (w === lastW) {
					return;
				}
				lastW = w;
				cancelAnimationFrame(raf);
				raf = requestAnimationFrame(() => {
					if (chartCtl) {
						chartCtl.render();
					}
					renderRunCharts(drawerRow);
				});
			});
			resizeObs.observe(chartEl);
		}
		$("d-table-wrap").addEventListener("toggle", () => {
			if ($("d-table-wrap").open && chartCtl) {
				chartCtl.renderDataTable();
			}
		});
	}

	function segHtml(name, options, value, label) {
		return '<div class="seg" role="radiogroup" aria-label="' + esc(label) + '" data-seg="' + esc(name) + '">'
			+ options.map((o) => {
				const on = o[0] === value;
				return '<button type="button" role="radio" data-value="' + esc(o[0]) + '" aria-checked="' + on + '"' + (o[2] ? " disabled" : "") + ' tabindex="' + (on || (value == null && o === options[options.length - 1]) ? 0 : -1) + '">' + esc(o[1]) + "</button>";
			}).join("") + "</div>";
	}

	function setSegValue(name, value) {
		const box = document.querySelector('[data-seg="' + name + '"]');
		if (!box) {
			return;
		}
		// 一致するボタンが無い (ドラッグで拡大した) ときも Tab で辿れるよう、segHtml と同じく
		// 最後のボタンを Tab の止まり先にする
		const btns = Array.from(box.querySelectorAll("button"));
		const hit = btns.some((b) => b.dataset.value === value);
		btns.forEach((b, i) => {
			const on = b.dataset.value === value;
			b.setAttribute("aria-checked", on);
			b.tabIndex = on || (!hit && i === btns.length - 1) ? 0 : -1;
		});
	}

	function onSeg(name, value) {
		if (name === "metric") {
			state.metric = value;
			if (value !== "time") {
				state.scale = "abs";
			}
			setSegValue("metric", value);
			setSegValue("scale", state.scale);
			const rel = document.querySelector('[data-seg="scale"] [data-value="rel"]');
			if (rel) {
				rel.disabled = value !== "time";
			}
			$("d-axis-note").textContent = METRICS[value].axis;
		}
		else if (name === "scale") {
			state.scale = value;
			setSegValue("scale", value);
		}
		else if (name === "range") {
			state.range = null;
			state.rangePreset = value;
			setSegValue("range", value);
		}
		if (chartCtl) {
			chartCtl.render();
			if ($("d-table-wrap").open) {
				chartCtl.renderDataTable();
			}
		}
	}

	// ---------------------------------------------------------------- 推移のグラフ

	/// 目盛りを「きりのいい」数で刻む
	function niceStep(range, count) {
		const raw = range / Math.max(1, count);
		const mag = Math.pow(10, Math.floor(Math.log10(raw)));
		const f = raw / mag;
		const nf = f <= 1 ? 1 : f <= 2 ? 2 : f <= 2.5 ? 2.5 : f <= 5 ? 5 : 10;
		return nf * mag;
	}

	function niceTicks(lo, hi, count) {
		if (!(hi > lo)) {
			return { ticks: [lo], step: 1 };
		}
		const step = niceStep(hi - lo, count);
		const out = [];
		for (let v = Math.ceil(lo / step - 1e-9) * step; v <= hi + step * 1e-9; v += step) {
			out.push(Math.abs(v) < step * 1e-9 ? 0 : v);
		}
		return { ticks: out, step };
	}

	function decimalsFor(step) {
		if (step >= 1) {
			return Number.isInteger(Math.round(step * 1000) / 1000) ? 0 : 1;
		}
		const d = -Math.floor(Math.log10(step) + 1e-9);
		return Math.round(step * Math.pow(10, d)) % 1 === 0 && String(step).indexOf("25") < 0 ? d : d + 1;
	}

	/// y 軸の目盛りの書式 (目盛り全体で単位をそろえる)
	function tickFormatter(metric, scale, maxAbs, step) {
		if (scale === "rel") {
			const d = Math.max(0, decimalsFor(step * 100));
			return (v) => (v > 1e-12 ? "+" : v < -1e-12 ? MINUS : "") + Math.abs(v * 100).toFixed(d) + NNBSP + "%";
		}
		if (metric === "time") {
			const u = timeUnit(maxAbs || 1);
			const d = Math.max(0, decimalsFor(step / u[0]));
			return (v) => (v / u[0]).toFixed(d) + " " + u[1];
		}
		if (metric === "bytes") {
			const unit = maxAbs >= 1048576 ? [1048576, "MiB"] : maxAbs >= 1024 ? [1024, "KiB"] : [1, "B"];
			const d = Math.max(0, decimalsFor(step / unit[0]));
			return (v) => (v / unit[0]).toFixed(d) + " " + unit[1];
		}
		const d = Math.max(0, decimalsFor(step));
		return (v) => v.toFixed(d);
	}

	function metricValue(p, metric) {
		return metric === "time" ? p.m : metric === "allocs" ? p.a : p.ab;
	}

	function fmtMetric(v, metric, per) {
		return metric === "time" ? fmtTime(v) : metric === "allocs" ? fmtAllocs(v, per) + " 回" : fmtBytes(v);
	}

	function markerSvg(slot, x, y, r, cls) {
		const X = x.toFixed(1);
		const Y = y.toFixed(1);
		switch (slot) {
			case 1:
				return '<rect class="' + cls + '" x="' + (x - r * 0.88).toFixed(1) + '" y="' + (y - r * 0.88).toFixed(1) + '" width="' + (r * 1.76).toFixed(1) + '" height="' + (r * 1.76).toFixed(1) + '" rx="1"/>';
			case 2: {
				const k = r * 1.25;
				return '<path class="' + cls + '" d="M' + X + " " + (y - k).toFixed(1) + "L" + (x + k).toFixed(1) + " " + Y + "L" + X + " " + (y + k).toFixed(1) + "L" + (x - k).toFixed(1) + " " + Y + 'Z"/>';
			}
			case 3: {
				const k = r * 1.2;
				return '<path class="' + cls + '" d="M' + X + " " + (y - k).toFixed(1) + "L" + (x + k).toFixed(1) + " " + (y + k * 0.8).toFixed(1) + "L" + (x - k).toFixed(1) + " " + (y + k * 0.8).toFixed(1) + 'Z"/>';
			}
			default:
				return '<circle class="' + cls + '" cx="' + X + '" cy="' + Y + '" r="' + r.toFixed(1) + '"/>';
		}
	}

	const SHAPE_CLASS = ["", "sq", "dia", "tri", "", ""];

	function createHistoryChart(el, row) {
		const all = row.pts.slice();
		const extra = row.extra;
		let view = null; // 直近の描画の座標系
		let active = -1; // フォーカス中の点 (view.pts の添字)
		let drag = null;

		function visible() {
			const metric = state.metric;
			let pts = all.concat(extra ? [extra] : []).filter((p) => isNum(metricValue(p, metric)));
			if (state.range) {
				pts = pts.filter((p) => p.x >= state.range[0] && p.x <= state.range[1]);
			}
			else if (state.rangePreset && state.rangePreset !== "all") {
				const n = Number(state.rangePreset);
				pts = pts.slice(Math.max(0, pts.length - n));
			}
			return pts;
		}

		function render() {
			const width = Math.floor(el.clientWidth);
			if (width < 40) {
				return;
			}
			const metric = state.metric;
			const scale = metric === "time" ? state.scale : "abs";
			const pts = visible();
			// 点が 1 つだけ (初回など) のときは線も帯も描けないので低くする
			const height = pts.length <= 1 ? 168 : width < 520 ? 232 : 272;
			const m = { top: 30, right: extra ? 18 : 14, bottom: 30, left: 64 };
			const iw = width - m.left - m.right;
			const ih = height - m.top - m.bottom;

			if (!pts.length) {
				el.innerHTML = '<div class="spark-empty" style="padding:40px 0;text-align:center">この範囲には点がありません</div>';
				view = null;
				return;
			}

			// 変化率は表示範囲の最初の点を基準にする
			const base = scale === "rel" ? metricValue(pts[0], metric) : 1;
			const tr = (v) => (scale === "rel" ? v / base - 1 : v);
			let lo = Infinity;
			let hi = -Infinity;
			for (const p of pts) {
				const v = metricValue(p, metric);
				lo = Math.min(lo, tr(v));
				hi = Math.max(hi, tr(v));
				if (metric === "time" && isNum(p.q1) && isNum(p.q3)) {
					lo = Math.min(lo, tr(p.q1));
					hi = Math.max(hi, tr(p.q3));
				}
			}
			const budget = metric === "allocs" && row.budget != null ? Number(row.budget) : null;
			if (metric !== "time") {
				lo = 0;
				if (budget != null) {
					hi = Math.max(hi, budget);
				}
				hi = hi > 0 ? hi * 1.12 : 1;
			}
			else {
				if (scale === "rel") {
					lo = Math.min(lo, 0);
					hi = Math.max(hi, 0);
				}
				let span = hi - lo;
				if (span <= Math.abs(hi) * 1e-6) {
					span = Math.abs(hi) * 0.1 || 0.1;
					lo -= span / 2;
					hi += span / 2;
				}
				lo -= span * 0.08;
				hi += span * 0.08;
				if (scale !== "rel" && lo < 0) {
					lo = 0;
				}
			}
			const yt = niceTicks(lo, hi, height < 250 ? 4 : 5);
			const fmtY = tickFormatter(metric, scale, Math.max(Math.abs(lo), Math.abs(hi)), yt.step);

			const x0 = pts[0].x;
			const x1 = pts[pts.length - 1].x;
			const padX = Math.max(0.5, (x1 - x0) * 0.01);
			const dx0 = x0 - padX;
			const dx1 = x1 + padX;
			const X = (x) => m.left + ((x - dx0) / (dx1 - dx0 || 1)) * iw;
			const Y = (v) => m.top + ih - ((v - lo) / (hi - lo)) * ih;
			view = { pts, X, Y, tr, m, iw, ih, width, height, dx0, dx1, metric, scale };

			const out = [];
			// 横の目盛り線と y 軸の目盛り
			for (const t of yt.ticks) {
				const y = Y(t);
				out.push('<line class="grid-line" x1="' + m.left + '" x2="' + (m.left + iw) + '" y1="' + y.toFixed(1) + '" y2="' + y.toFixed(1) + '"/>');
				out.push('<text class="tick" x="' + (m.left - 8) + '" y="' + (y + 3.5).toFixed(1) + '" text-anchor="end">' + esc(fmtY(t)) + "</text>");
			}
			if (scale === "rel" || metric !== "time") {
				const y0 = Y(0);
				if (y0 >= m.top - 0.5 && y0 <= m.top + ih + 0.5) {
					out.push('<line class="axis-line" x1="' + m.left + '" x2="' + (m.left + iw) + '" y1="' + y0.toFixed(1) + '" y2="' + y0.toFixed(1) + '"/>');
				}
			}
			out.push('<line class="axis-line" x1="' + m.left + '" x2="' + (m.left + iw) + '" y1="' + (m.top + ih) + '" y2="' + (m.top + ih) + '"/>');

			// x 軸 (コミットの並び)。ラベルはそのコミットの日付
			const hist = pts.filter((p) => !p.extra);
			if (hist.length) {
				const target = Math.max(2, Math.floor(iw / 84));
				const span = hist[hist.length - 1].x - hist[0].x;
				const step = Math.max(1, niceStep(span || 1, target));
				const firstTick = Math.ceil(hist[0].x / step) * step;
				// 半年を超える範囲では月/日だけだと年がまぎれるので「年/月」にする
				const d0 = parseDate(hist[0].commit && hist[0].commit.date);
				const d1 = parseDate(hist[hist.length - 1].commit && hist[hist.length - 1].commit.date);
				const longSpan = d0 && d1 && d1 - d0 > 183 * 86400 * 1000;
				const tickLabel = (c) => {
					const d = c ? parseDate(c.date) : null;
					return d ? (longSpan ? d.getFullYear() + "/" + (d.getMonth() + 1) : d.getMonth() + 1 + "/" + d.getDate()) : "";
				};
				let lastLabelX = -Infinity;
				for (let t = firstTick; t <= hist[hist.length - 1].x; t += step) {
					const c = model.commits[Math.round(t)];
					const x = X(t);
					if (x - lastLabelX < 56) {
						continue;
					}
					lastLabelX = x;
					out.push('<line class="axis-line" x1="' + x.toFixed(1) + '" x2="' + x.toFixed(1) + '" y1="' + (m.top + ih) + '" y2="' + (m.top + ih + 4) + '"/>');
					out.push('<text class="tick" x="' + x.toFixed(1) + '" y="' + (m.top + ih + 17) + '" text-anchor="middle">' + esc(c ? tickLabel(c) : "#" + t) + "</text>");
				}
			}

			// 環境の変化 (破線)。CPU は 2 点以下の短い入れ替わりには引かない。ランナーの CPU は
			// 実行ごとに当たり外れがあり、全部に線を引くと埋もれてしまう (点の色と形で分かる)
			const runLen = new Array(pts.length);
			for (let i = 0; i < pts.length;) {
				let j = i;
				while (j < pts.length && pts[j].cpu === pts[i].cpu) {
					j++;
				}
				for (let k = i; k < j; k++) {
					runLen[k] = j - i;
				}
				i = j;
			}
			const MIN_RUN = 3;
			const envMarks = [];
			for (let i = 1; i < pts.length; i++) {
				const a = pts[i - 1].env;
				const b = pts[i].env;
				if (!a || !b) {
					continue;
				}
				const labels = [];
				if (metric === "time" && a.cpu !== b.cpu && runLen[i - 1] >= MIN_RUN && runLen[i] >= MIN_RUN) {
					labels.push("CPU → " + cpuShort(b.cpu));
				}
				if (a.image !== b.image) {
					labels.push("イメージ更新");
				}
				else if (a.compiler !== b.compiler) {
					labels.push("コンパイラ更新");
				}
				if (labels.length) {
					envMarks.push({ x: (X(pts[i - 1].x) + X(pts[i].x)) / 2, label: labels.join(" · "), from: a, to: b });
				}
			}
			// 線を先に全部引いてから文字を載せる (文字の背景で後ろの線を隠すため)
			for (const e of envMarks) {
				out.push('<line class="env-line" x1="' + e.x.toFixed(1) + '" x2="' + e.x.toFixed(1) + '" y1="' + m.top + '" y2="' + (m.top + ih) + '"/>');
			}
			let lastEnvLabelEnd = -Infinity;
			for (const e of envMarks) {
				const w = e.label.length * 6.4 + 8;
				let lx = e.x + 4;
				let anchor = "start";
				if (lx + w > m.left + iw) {
					lx = e.x - 4;
					anchor = "end";
				}
				const left = anchor === "start" ? lx : lx - w;
				if (left > lastEnvLabelEnd + 6 && left >= m.left) {
					out.push('<rect class="env-label-bg" x="' + (left - 2).toFixed(1) + '" y="' + (m.top + 2) + '" width="' + (w - 2).toFixed(1) + '" height="15" rx="3"/>');
					out.push('<text class="env-label" x="' + (anchor === "start" ? lx + 2 : lx - 2).toFixed(1) + '" y="' + (m.top + 13) + '" text-anchor="' + anchor + '">' + esc(e.label) + "</text>");
					lastEnvLabelEnd = left + w;
				}
			}

			// q1–q3 の帯と中央値の線。CPU が変わる所では切る (絶対値を比べられないため)
			const segs = [];
			let cur = [];
			for (const p of hist) {
				if (cur.length && cur[cur.length - 1].cpu !== p.cpu) {
					segs.push(cur);
					cur = [];
				}
				cur.push(p);
			}
			if (cur.length) {
				segs.push(cur);
			}
			for (const s of segs) {
				if (metric === "time" && s.length > 1 && s.every((p) => isNum(p.q1) && isNum(p.q3))) {
					const top = s.map((p, k) => (k ? "L" : "M") + X(p.x).toFixed(1) + " " + Y(tr(p.q3)).toFixed(1)).join("");
					const bot = s.slice().reverse().map((p) => "L" + X(p.x).toFixed(1) + " " + Y(tr(p.q1)).toFixed(1)).join("");
					out.push('<path class="band" d="' + top + bot + 'Z"/>');
				}
				if (s.length > 1) {
					const d = s.map((p, k) => (k ? "L" : "M") + X(p.x).toFixed(1) + " " + Y(tr(metricValue(p, metric))).toFixed(1)).join("");
					out.push('<path class="median" d="' + d + '"/>');
				}
			}

			// 確保回数の予算 (これを超えると CI が失敗する)
			if (budget != null) {
				const yb = Y(budget);
				out.push('<line class="budget-line" x1="' + m.left + '" x2="' + (m.left + iw) + '" y1="' + yb.toFixed(1) + '" y2="' + yb.toFixed(1) + '"/>');
				out.push('<text class="budget-label" x="' + (m.left + 6) + '" y="' + (yb - 5).toFixed(1) + '">予算 ' + esc(budget) + " 回 / op</text>");
			}

			// 判定の列 (対の比較で悪化・改善と出た回)。判定は時間についてのものなので時間のときだけ。
			// 対の比較が 1 回も無い範囲 (初回など) では列ごと出さない
			const hasPairs = pts.some((p) => p.verdict);
			if (metric === "time" && hasPairs) {
				out.push('<text class="lane-label" x="' + (m.left - 8) + '" y="' + (m.top - 12) + '" text-anchor="end">判定</text>');
				for (const p of pts) {
					if (p.verdict === "regressed" || p.verdict === "improved") {
						out.push('<text class="lane-glyph c-' + p.verdict + '" x="' + X(p.x).toFixed(1) + '" y="' + (m.top - 12) + '">' + (p.verdict === "regressed" ? "▲" : "▼") + "</text>");
					}
				}
			}

			// 点 (色と形で CPU を表す)
			const r = pts.length > 160 ? 2.4 : pts.length > 70 ? 3 : 3.8;
			for (const p of hist) {
				out.push(markerSvg(p.cpuSlot, X(p.x), Y(tr(metricValue(p, metric))), r, "pt cpu-f" + p.cpuSlot));
			}

			// 履歴に無い点 (ブランチのプレビューなど) は白抜きのひし形で右端に
			if (pts.length && pts[pts.length - 1].extra) {
				const p = pts[pts.length - 1];
				const cx = X(p.x);
				const cy = Y(tr(metricValue(p, metric)));
				if (hist.length) {
					const q = hist[hist.length - 1];
					out.push('<path class="cand-link" d="M' + X(q.x).toFixed(1) + " " + Y(tr(metricValue(q, metric))).toFixed(1) + "L" + cx.toFixed(1) + " " + cy.toFixed(1) + '"/>');
				}
				out.push('<path class="cand" d="M' + cx.toFixed(1) + " " + (cy - 7).toFixed(1) + "l7 7-7 7-7-7z" + '"/>');
				const labelY = cy - 13 < m.top + 10 ? cy + 24 : cy - 13;
				out.push('<text class="cand-label" x="' + (cx + 6).toFixed(1) + '" y="' + labelY.toFixed(1) + '" text-anchor="end">' + (p.candidate ? "このブランチ" : "最新") + "</text>");
			}

			if (!hist.length) {
				out.push('<text class="axis-title" x="' + (m.left + iw / 2).toFixed(1) + '" y="' + (m.top + ih - 14) + '" text-anchor="middle">まだ履歴がありません。master で計測が積み重なると推移が出ます</text>');
			}
			else if (pts.length === 1) {
				// 判定の列は空なので、その高さに案内を置く (点と重ならない)
				out.push('<text class="axis-title" x="' + (m.left + iw / 2).toFixed(1) + '" y="' + (m.top - 12) + '" text-anchor="middle">まだ 1 点だけです。master で計測が積み重なると推移が出ます</text>');
			}

			// ホバー用の層 (十字線と強調)
			out.push('<g class="hover-layer"></g>');
			out.push('<rect class="brush" x="0" y="' + m.top + '" width="0" height="' + ih + '" visibility="hidden"/>');
			out.push('<rect class="overlay" x="' + m.left + '" y="' + (m.top - 22) + '" width="' + iw + '" height="' + (ih + 22) + '"/>');

			const vals = pts.map((p) => metricValue(p, metric));
			const nReg = pts.filter((p) => p.verdict === "regressed").length;
			const nImp = pts.filter((p) => p.verdict === "improved").length;
			const summary = METRICS[metric].label + "の推移、" + pts.length + " 点 (" + fmtDate(pts[0].commit && pts[0].commit.date) + " から " + fmtDate(pts[pts.length - 1].commit && pts[pts.length - 1].commit.date) + ")。"
				+ "最新 " + fmtMetric(vals[vals.length - 1], metric, row.per) + "、最小 " + fmtMetric(Math.min.apply(null, vals), metric, row.per) + "、最大 " + fmtMetric(Math.max.apply(null, vals), metric, row.per) + "。"
				+ "悪化 " + nReg + " 回、改善 " + nImp + " 回、環境の変化 " + envMarks.length + " 回。";

			el.innerHTML = '<svg width="' + width + '" height="' + height + '" viewBox="0 0 ' + width + " " + height + '" role="img" aria-label="' + esc(summary) + '">' + out.join("") + '</svg><div class="tip" hidden></div>';
			view.envMarks = envMarks;
			view.svg = el.querySelector("svg");
			view.tip = el.querySelector(".tip");
			view.hover = el.querySelector(".hover-layer");
			view.brush = el.querySelector(".brush");
			view.overlay = el.querySelector(".overlay");
			view.r = r;
			if (active >= pts.length) {
				active = pts.length - 1;
			}
			renderLegend(pts, envMarks);
			updateZoomPill(pts);
			if (active >= 0 && document.activeElement === el) {
				showPoint(active, false);
			}
		}

		function renderLegend(pts, envMarks) {
			const counts = new Map();
			for (const p of pts) {
				if (!p.extra) {
					counts.set(p.cpu, (counts.get(p.cpu) || 0) + 1);
				}
			}
			const items = [];
			const cpus = Array.from(counts.keys()).sort((a, b) => cpuSlot(model, a) - cpuSlot(model, b));
			for (const cpu of cpus) {
				const slot = cpuSlot(model, cpu);
				items.push('<span class="legend-item" title="' + esc(cpu || "") + '"><span class="key-dot cpu-b' + slot + " " + SHAPE_CLASS[slot] + '" style="color:var(--cpu-' + (slot < CPU_SLOTS ? slot : "other") + ')"></span>' + esc(cpuShort(cpu)) + ' <span class="n">' + counts.get(cpu) + "</span></span>");
			}
			if (state.metric === "time" && pts.filter((p) => !p.extra).length > 1) {
				items.push('<span class="legend-item"><span class="key-band"></span>中央値と四分位 (q1–q3)</span>');
			}
			if (state.metric === "time" && pts.some((p) => p.verdict === "regressed" || p.verdict === "improved")) {
				items.push('<span class="legend-item"><span class="g-reg" aria-hidden="true">▲</span><span class="g-imp" aria-hidden="true">▼</span>対の比較で悪化 / 改善</span>');
			}
			if (envMarks.length) {
				items.push('<span class="legend-item"><span class="key-line"></span>' + (state.metric === "time" ? "CPU・イメージの変化" : "イメージの変化") + "</span>");
			}
			if (state.metric === "allocs" && row.budget != null) {
				items.push('<span class="legend-item"><span class="key-line key-budget"></span>予算 (超えると CI が失敗)</span>');
			}
			if (extra && pts.length && pts[pts.length - 1].extra) {
				items.push('<span class="legend-item"><span class="key-cand"></span>' + (extra.candidate ? "このブランチ (未マージ)" : "最新 (履歴に未反映)") + "</span>");
			}
			$("d-legend").innerHTML = items.join("");
		}

		function updateZoomPill(pts) {
			const pill = $("d-zoom-pill");
			if (!pill) {
				return;
			}
			if (state.range && pts.length) {
				pill.hidden = false;
				pill.textContent = "拡大中: " + pts.length + " 点";
			}
			else {
				pill.hidden = true;
			}
		}

		function nearest(px) {
			const pts = view.pts;
			let lo = 0;
			let hi = pts.length - 1;
			while (hi - lo > 1) {
				const mid = (lo + hi) >> 1;
				if (view.X(pts[mid].x) < px) {
					lo = mid;
				}
				else {
					hi = mid;
				}
			}
			return Math.abs(view.X(pts[lo].x) - px) <= Math.abs(view.X(pts[hi].x) - px) ? lo : hi;
		}

		function showPoint(idx, fromPointer, px) {
			if (!view || idx < 0 || idx >= view.pts.length) {
				return;
			}
			active = idx;
			const p = view.pts[idx];
			const metric = view.metric;
			const cx = view.X(p.x);
			const cy = view.Y(view.tr(metricValue(p, metric)));
			const hl = p.extra
				? '<path class="cand" d="M' + cx.toFixed(1) + " " + (cy - 9).toFixed(1) + "l9 9-9 9-9-9z" + '"/>'
				: markerSvg(p.cpuSlot, cx, cy, view.r + 2.2, "pt is-hover cpu-f" + p.cpuSlot);
			view.hover.innerHTML = '<line class="crosshair" x1="' + cx.toFixed(1) + '" x2="' + cx.toFixed(1) + '" y1="' + (view.m.top - 4) + '" y2="' + (view.m.top + view.ih) + '"/>' + hl;
			view.overlay.classList.toggle("is-clickable", !!(p.commit && p.commit.sha));

			const tip = view.tip;
			tip.innerHTML = tipHtml(p, metric, fromPointer, row);
			tip.hidden = false;
			const tw = tip.offsetWidth;
			const th = tip.offsetHeight;
			let left = cx + 16;
			if (left + tw > view.width) {
				left = cx - 16 - tw;
			}
			if (left < 0) {
				left = Math.max(0, Math.min(view.width - tw, (px == null ? cx : px) - tw / 2));
			}
			const top = clamp(cy - th / 2, 0, Math.max(0, view.height - th));
			tip.style.transform = "translate(" + Math.round(left) + "px," + Math.round(top) + "px)";
			if (!fromPointer) {
				$("d-live").textContent = tipText(p, metric, row);
			}
		}

		function hide() {
			if (!view) {
				return;
			}
			view.hover.innerHTML = "";
			view.tip.hidden = true;
		}

		function localX(ev) {
			const rect = view.svg.getBoundingClientRect();
			return ev.clientX - rect.left;
		}

		function openCommit(p) {
			if (p && p.commit && p.commit.sha) {
				window.open(commitUrl(model.repo, p.commit.sha), "_blank", "noopener");
			}
		}

		el.addEventListener("pointermove", (ev) => {
			if (!view || !view.overlay) {
				return;
			}
			const px = localX(ev);
			if (drag) {
				if (Math.abs(px - drag.x) > 5) {
					drag.moved = true;
				}
				if (drag.moved) {
					const a = clamp(Math.min(px, drag.x), view.m.left, view.m.left + view.iw);
					const b = clamp(Math.max(px, drag.x), view.m.left, view.m.left + view.iw);
					view.brush.setAttribute("x", a.toFixed(1));
					view.brush.setAttribute("width", (b - a).toFixed(1));
					view.brush.setAttribute("visibility", "visible");
					hide();
					return;
				}
			}
			if (ev.target !== view.overlay && !drag) {
				hide();
				return;
			}
			showPoint(nearest(px), true, px);
		});
		el.addEventListener("pointerleave", () => {
			if (!drag) {
				hide();
			}
		});
		el.addEventListener("pointerdown", (ev) => {
			if (!view || ev.target !== view.overlay || ev.button !== 0) {
				return;
			}
			drag = { x: localX(ev), moved: false, id: ev.pointerId, type: ev.pointerType };
			try {
				el.setPointerCapture(ev.pointerId);
			}
			catch {
				// 取れなくてもドラッグ中の移動は拾える
			}
			showPoint(nearest(drag.x), true, drag.x);
		});
		const endDrag = (ev, cancelled) => {
			if (!drag) {
				return;
			}
			const d = drag;
			drag = null;
			if (view && view.brush) {
				view.brush.setAttribute("visibility", "hidden");
			}
			if (cancelled || !view) {
				return;
			}
			const px = localX(ev);
			if (d.moved) {
				const a = Math.min(px, d.x);
				const b = Math.max(px, d.x);
				const inv = (sx) => view.dx0 + ((sx - view.m.left) / view.iw) * (view.dx1 - view.dx0);
				const sel = view.pts.filter((p) => view.X(p.x) >= a && view.X(p.x) <= b);
				if (sel.length >= 2) {
					state.range = [sel[0].x, sel[sel.length - 1].x];
					setSegValue("range", null);
					render();
				}
				else if (inv(b) - inv(a) > 0) {
					showToast("2 点以上を含む範囲をドラッグしてください");
				}
			}
			else if (d.type === "mouse") {
				openCommit(view.pts[nearest(px)]);
			}
		};
		el.addEventListener("pointerup", (ev) => endDrag(ev, false));
		el.addEventListener("pointercancel", (ev) => endDrag(ev, true));

		el.addEventListener("focus", () => {
			if (view && view.pts.length) {
				showPoint(active >= 0 ? active : view.pts.length - 1, false);
			}
		});
		el.addEventListener("blur", hide);
		el.addEventListener("keydown", (ev) => {
			if (!view || !view.pts.length) {
				return;
			}
			let i = active < 0 ? view.pts.length - 1 : active;
			if (ev.key === "ArrowLeft") {
				i = Math.max(0, i - 1);
			}
			else if (ev.key === "ArrowRight") {
				i = Math.min(view.pts.length - 1, i + 1);
			}
			else if (ev.key === "Home") {
				i = 0;
			}
			else if (ev.key === "End") {
				i = view.pts.length - 1;
			}
			else if (ev.key === "Enter") {
				openCommit(view.pts[i]);
				ev.preventDefault();
				return;
			}
			else {
				return;
			}
			ev.preventDefault();
			showPoint(i, false);
		});

		function renderDataTable() {
			const box = $("d-table");
			if (!box) {
				return;
			}
			const metric = state.metric;
			const pts = visible().slice().reverse().slice(0, 400);
			const rows = pts.map((p) => {
				const c = p.commit || {};
				const v = metricValue(p, metric);
				const paired = pairedOk(p) ? fmtPct(p.r - 1) + " (" + fmtCi(p.rl, p.rh) + ")" : isNum(p.r) ? "~" + fmtPct(p.r - 1) + " (区間なし)" : "—";
				const verdict = p.verdict && VERDICT[p.verdict] ? VERDICT[p.verdict].glyph + " " + VERDICT[p.verdict].label : "—";
				return "<tr><td class=\"l\">" + (c.sha ? '<a class="sha" href="' + esc(commitUrl(model.repo, c.sha)) + '" target="_blank" rel="noopener">' + esc(sha7(c.sha)) + "</a>" : "") + (p.extra ? " (" + (p.candidate ? "ブランチ" : "最新") + ")" : "") + "</td>"
					+ '<td class="l">' + esc(fmtShortDateTime(c.date)) + "</td>"
					+ "<td>" + esc(fmtMetric(v, metric, row.per)) + "</td>"
					+ "<td>" + (metric === "time" && isNum(p.q1) ? esc(fmtTime(p.q1) + " – " + fmtTime(p.q3)) : "—") + "</td>"
					+ "<td>" + esc(paired) + "</td>"
					+ '<td class="l">' + esc(verdict) + "</td>"
					+ '<td class="l">' + esc(cpuShort(p.cpu)) + "</td></tr>";
			});
			box.innerHTML = '<table class="data-table"><thead><tr><th class="l">コミット</th><th class="l">日時</th><th>' + esc(METRICS[metric].label) + "</th><th>q1 – q3</th><th>対の比較 (95% CI)</th><th class=\"l\">判定</th><th class=\"l\">CPU</th></tr></thead><tbody>" + rows.join("") + "</tbody></table>";
		}

		return { render, renderDataTable };
	}

	function tipHtml(p, metric, fromPointer, row) {
		const c = p.commit || {};
		const v = metricValue(p, metric);
		const per = row ? row.per : "op";
		const out = [];
		out.push('<div class="tip-head"><span class="sha">' + esc(sha7(c.sha)) + "</span><span>" + esc(fmtDateTime(c.date)) + "</span>"
			+ (p.extra ? '<span class="pill accent" style="height:18px;font-size:11px">' + (p.candidate ? "このブランチ" : "最新") + "</span>" : "") + "</div>");
		if (c.subject) {
			out.push('<div class="tip-subject">' + esc(truncate(c.subject, 90)) + "</div>");
		}
		let range = "";
		if (metric === "time" && isNum(p.q1) && isNum(p.q3)) {
			range = '<span class="muted">q1–q3 ' + esc(fmtTime(p.q1)) + " – " + esc(fmtTime(p.q3)) + "</span>";
		}
		else if (metric === "allocs") {
			const f = allocFmt(v, per);
			range = f.title ? '<span class="muted">' + esc(f.title) + "</span>" : "";
		}
		out.push('<div class="tip-value">' + esc(fmtMetric(v, metric, per)) + range + "</div>");
		const grid = [];
		const thr = row ? row.thr : DEFAULT_THRESHOLD;
		// 対の比較は時間についての判定なので、確保の指標では出さない
		if (metric === "time" && pairedOk(p)) {
			grid.push(["判定", badgeHtml(p.verdict, "", thr)]);
			grid.push(["対の比較", "<strong>" + esc(fmtPct(p.r - 1)) + '</strong> <span class="muted">' + esc(fmtCi(p.rl, p.rh)) + "</span>"]);
		}
		else if (metric === "time" && p.verdict === "insufficient") {
			grid.push(["判定", badgeHtml("insufficient", "", thr)]);
			if (isNum(p.r)) {
				grid.push(["対の比較", "<strong>~" + esc(fmtPct(p.r - 1)) + '</strong> <span class="muted">区間なし (対が ' + MIN_PAIRS + " 組未満)</span>"]);
			}
		}
		else if (metric === "time") {
			grid.push(["判定", '<span class="muted">対の比較なし (base が無い回)</span>']);
		}
		if (p.prev && metric === "time") {
			const pv = metricValue(p.prev, metric);
			if (isNum(pv) && pv > 0 && isNum(v)) {
				grid.push(["直前の点比", "<strong>" + esc(fmtPct(v / pv - 1)) + '</strong> <span class="muted">同じ CPU</span>']);
			}
		}
		if (p.cpu) {
			const slot = p.cpuSlot;
			grid.push(["CPU", '<span class="key-dot cpu-b' + slot + " " + SHAPE_CLASS[slot] + '" style="color:var(--cpu-' + (slot < CPU_SLOTS ? slot : "other") + ')"></span> ' + esc(cpuShort(p.cpu)) + (p.env && p.env.image ? ' <span class="muted">· ' + esc(p.env.image) + "</span>" : "")]);
		}
		out.push('<div class="tip-grid">' + grid.map((g) => '<span class="tip-k">' + g[0] + '</span><span class="tip-v">' + g[1] + "</span>").join("") + "</div>");
		if (fromPointer && c.sha) {
			out.push('<div class="tip-hint">クリックでコミットを開く · ドラッグで拡大</div>');
		}
		return out.join("");
	}

	function tipText(p, metric, row) {
		const c = p.commit || {};
		const v = metricValue(p, metric);
		const per = row ? row.per : "op";
		let s = sha7(c.sha) + "、" + fmtDateTime(c.date) + "、" + fmtMetric(v, metric, per);
		if (metric === "allocs") {
			const f = allocFmt(v, per);
			if (f.title) {
				s += " (" + f.title + ")";
			}
		}
		if (pairedOk(p)) {
			s += "、対の比較 " + fmtPct(p.r - 1) + " " + VERDICT[p.verdict].label;
		}
		else if (p.verdict === "insufficient") {
			s += "、" + VERDICT.insufficient.label;
		}
		if (p.cpu) {
			s += "、" + cpuShort(p.cpu);
		}
		return s;
	}

	// ---------------------------------------------------------------- 最新の計測 (ラウンドごとの値)

	/// 比 (×3.3) の書式。1 に近いときは桁を増やす
	function fmtTimes(r) {
		return r < 1.095 ? r.toFixed(2) : r < 9.95 ? r.toFixed(1) : r.toFixed(0);
	}

	/**
	 * 構成ごとの比較: 同じベンチマークの最新の中央値を、構成 (コンパイラ × 構成) ごとに
	 * 共通の横軸の棒で並べる。構成ごとに別々の VM で動くので、対の比較のような精度は無い
	 * (CPU が違えば参考値)。それでも MSVC と ClangCL の数倍の差などはここで一目で分かる。
	 */
	function variantsSectionHtml(row) {
		const cur = vmNow();
		const items = [];
		for (const id of model.variantIds) {
			const v = model.variants.get(id);
			const r = v.rowsByName.get(row.name);
			if (!r || !isNum(r.value) || r.value <= 0) {
				continue;
			}
			const c = v.summary.commit;
			items.push({ id, label: v.label, value: r.value, cpu: r.cpu, alloc: r.alloc, sha: c && c.sha, current: id === cur.id });
		}
		if (items.length < 2) {
			return "";
		}
		let min = Infinity;
		let max = 0;
		for (const it of items) {
			min = Math.min(min, it.value);
			max = Math.max(max, it.value);
		}
		const cpus = Array.from(new Set(items.map((it) => it.cpu || "")));
		const mixedCpu = cpus.length > 1;
		const curSha = cur.summary.commit && cur.summary.commit.sha;
		const mixedSha = items.some((it) => it.sha && curSha && it.sha !== curSha);
		const pct = Math.round(row.thr * 100);
		const list = items.map((it) => {
			const ratio = it.value / min;
			const fastest = it.value === min;
			const w = Math.max(1, (it.value / max) * 100);
			const name = it.current
				? '<span class="vc-name">' + esc(it.label) + '<span class="vc-cur">表示中</span></span>'
				: '<button type="button" class="vc-name vc-switch" data-variant-switch="' + esc(it.id) + '" title="' + esc(it.label) + ' に切り替える">' + esc(it.label) + "</button>";
			let rel;
			if (fastest) {
				rel = '<span class="vc-rel is-fastest">最速</span>';
			}
			else {
				const near = ratio < 1 + row.thr;
				rel = '<span class="vc-rel' + (near ? " is-near" : "") + '" title="' + esc("最速の " + ratio.toFixed(2) + " 倍の時間" + (near ? " (差はしきい値 ±" + pct + " % 未満)" : "")) + '">×' + esc(fmtTimes(ratio)) + '<span class="sr-only"> (最速の ' + esc(fmtTimes(ratio)) + " 倍)</span></span>";
			}
			const af = it.alloc ? allocFmt(it.alloc.allocs, row.per) : null;
			const alloc = af
				? '<span class="vc-alloc' + (it.alloc.allocs === 0 ? " is-zero" : "") + '"' + (af.title ? ' title="' + esc(af.title) + '"' : "") + '><span class="vc-k">確保</span> ' + esc(af.text) + "</span>"
				: '<span class="vc-alloc is-zero"><span class="vc-k">確保</span> —</span>';
			const notes = [];
			if (mixedCpu) {
				notes.push(cpuShort(it.cpu));
			}
			if (it.sha && curSha && it.sha !== curSha) {
				notes.push(sha7(it.sha) + " の計測");
			}
			return '<li class="vc-row' + (it.current ? " is-current" : "") + (fastest ? " is-fastest" : "") + '">'
				+ '<span class="vc-head">' + name + (notes.length ? '<span class="vc-note">' + esc(notes.join(" · ")) + "</span>" : "") + "</span>"
				+ '<span class="vc-track" aria-hidden="true"><span class="vc-fill" style="width:' + w.toFixed(1) + '%"></span></span>'
				+ '<span class="vc-val">' + esc(fmtTime(it.value)) + "</span>" + rel + alloc + "</li>";
		}).join("");
		const cpuNote = mixedCpu
			? '<span class="pill warn" title="' + esc(cpus.map(cpuShort).join(" / ")) + '"><span aria-hidden="true">!</span> CPU が異なるため参考</span>'
			: '<span class="pill" title="' + esc(cpus[0] || "") + '">' + esc(cpuShort(cpus[0])) + "</span>";
		return '<section class="d-section" aria-labelledby="d-var-title">'
			+ '<div class="d-section-head"><h3 id="d-var-title" tabindex="-1">構成ごとの比較</h3>' + cpuNote + "</div>"
			+ '<p class="d-sub">' + (mixedSha ? "各構成の最新の計測" : "同じコミットを構成ごとに計測した値") + " (1 op あたりの中央値、短いほど速い)。"
			+ "構成ごとに別々の VM で動かしているので、数 % の差は VM の当たり外れの範囲です"
			+ (mixedCpu ? "。CPU が違う構成どうしの差には CPU の違いも含まれます" : "") + "。</p>"
			+ '<ul class="vc-list" role="list">' + list + "</ul></section>";
	}

	function runSectionHtml(row) {
		const vm = vmNow();
		const rb = row.runBench;
		const title = vm.candidate ? "このブランチの計測" : "最新の計測";
		if (!rb || !rb.head) {
			const p = row.pts[row.pts.length - 1];
			return '<section class="d-section" aria-labelledby="d-run-title"><div class="d-section-head"><h3 id="d-run-title">' + title + "</h3></div>"
				+ '<p class="d-sub">ラウンドごとの値は直近の実行 (latest.js) にだけ含まれます。この点は履歴から表示しています。</p>'
				+ (p ? statsTableHtml({ median: p.m, q1: p.q1, q3: p.q3 }, null, row) : "") + "</section>";
		}
		const run = vm.run;
		const base = run.base;
		let baseNote;
		if (base) {
			// kind が explicit (手元で --base-exe を渡した) のときは sha も CI 番号も無い
			const bits = ["比較対象 (base): " + esc(baseText(base))];
			if (base.sha) {
				bits.push('<a class="sha" href="' + esc(commitUrl(model.repo, base.sha)) + '" target="_blank" rel="noopener">' + esc(sha7(base.sha)) + "</a>");
			}
			if (base.run_number) {
				bits.push("CI #" + esc(base.run_number));
			}
			baseNote = '<p class="base-note">' + bits.join(" · ") + (base.sha ? " のビルド済み実行ファイル" : "") + "。head と同じ VM で交互に実行しました</p>";
		}
		else {
			baseNote = '<p class="base-note">比較対象 (base) の実行ファイルが無かったため、head だけを ' + esc(run.rounds || "?") + " ラウンド計測しました。</p>";
		}
		const p = rb.paired;
		let ratio = "";
		if (row.paired) {
			ratio = '<div class="ratio-card"><div class="ratio-top"><span class="ratio-big">' + esc(fmtPct(row.paired.change)) + "</span>"
				+ '<span class="ratio-ci">比 ' + esc(fmtRatio(row.paired.ratio)) + " · 95% CI " + esc(fmtCi(row.paired.lo, row.paired.hi)) + "</span>"
				+ badgeHtml(row.verdict, "", row.thr) + "</div>"
				+ '<div id="d-ratio"></div></div>';
		}
		else if (p && row.verdict === "insufficient") {
			const n = isNum(p.pairs) ? p.pairs + " 組" : MIN_PAIRS + " 組未満";
			ratio = '<p class="d-sub insufficient-note">' + badgeHtml("insufficient", "", row.thr) + " 対になったラウンドが " + esc(n) + "しかないため、区間を出さず判定もしていません"
				+ (isNum(p.ratio) ? " (その組だけの比は " + esc(fmtPct(p.ratio - 1)) + ")" : "") + "。</p>";
		}
		return '<section class="d-section" aria-labelledby="d-run-title">'
			+ '<div class="d-section-head"><h3 id="d-run-title">' + title + "</h3>"
			+ '<span class="pill mono">' + esc(sha7(run.commit && run.commit.sha)) + "</span></div>"
			+ '<p class="d-sub">ラウンドごとの中央値 (1 ラウンド = 実行ファイル 1 回)。縦線は全サンプルの中央値</p>'
			+ '<div class="run-grid"><div class="chart strip-wrap" id="d-strip"></div>' + ratio
			+ statsTableHtml(rb.head, rb.base, row) + "</div>" + baseNote + "</section>";
	}

	function statsTableHtml(h, b, row) {
		const cell = (v) => (v && typeof v === "object" ? "<td" + (v.title ? ' title="' + esc(v.title) + '"' : "") + ">" + esc(v.text) + "</td>" : "<td>" + esc(v) + "</td>");
		const line = (label, f, fb) => "<tr><th scope=\"row\">" + label + "</th>" + cell(f(h)) + (b ? cell((fb || f)(b)) : "") + "</tr>";
		const range = (x, lo, hi) => (x && isNum(x[lo]) && isNum(x[hi]) ? fmtTime(x[lo]) + " – " + fmtTime(x[hi]) : "—");
		const rows = [
			line("中央値", (x) => fmtTime(x && x.median)),
			line("四分位 (q1 – q3)", (x) => range(x, "q1", "q3")),
		];
		if (h && isNum(h.p10)) {
			rows.push(line("p10 – p90", (x) => range(x, "p10", "p90")));
			rows.push(line("最小", (x) => fmtTime(x && x.min)));
		}
		if (h && isNum(h.cv)) {
			rows.push(line("ラウンド間の CV", (x) => (x && isNum(x.cv) ? (x.cv * 100).toFixed(1) + NNBSP + "%" : "—")));
		}
		// cycles (QueryThreadCycleTime) は不変 TSC の基準ティックで、ns に定数を掛けただけなので出さない
		if (h && Array.isArray(h.rounds)) {
			rows.push(line("ラウンド数", (x) => (x && Array.isArray(x.rounds) ? String(x.rounds.length) : "—")));
		}
		const a = row.alloc;
		if (a) {
			rows.push(line("確保回数 / " + esc(row.per), () => allocFmt(a.allocs, row.per), () => allocFmt(a.baseAllocs, row.per)));
			rows.push(line("確保バイト / " + esc(row.per), () => fmtBytes(a.bytes), () => fmtBytes(a.baseBytes)));
		}
		return '<table class="stats"><thead><tr><th></th><th scope="col"><span class="col-head">head</span></th>' + (b ? '<th scope="col"><span class="col-base">base</span></th>' : "") + "</tr></thead><tbody>" + rows.join("") + "</tbody></table>";
	}

	/// ラウンドごとの値の分布 (base は灰、head はアクセント色) と、比の 95 % 区間
	function renderRunCharts(row) {
		const rb = row && row.runBench;
		const strip = $("d-strip");
		if (strip && rb && rb.head && Array.isArray(rb.head.rounds)) {
			const width = Math.floor(strip.clientWidth);
			if (width > 40) {
				const rowsDef = [];
				if (rb.base && Array.isArray(rb.base.rounds)) {
					rowsDef.push({ key: "base", label: "base", sub: baseText(vmNow().run.base) || "", vals: rb.base.rounds, med: rb.base.median, dot: "dot-base", medCls: "med-base" });
				}
				rowsDef.push({ key: "head", label: "head", sub: vmNow().candidate ? "このブランチ" : "このコミット", vals: rb.head.rounds, med: rb.head.median, dot: "dot-head", medCls: "med-head" });
				let lo = Infinity;
				let hi = -Infinity;
				for (const r of rowsDef) {
					for (const v of r.vals.concat([r.med])) {
						if (isNum(v)) {
							lo = Math.min(lo, v);
							hi = Math.max(hi, v);
						}
					}
				}
				let span = hi - lo;
				if (span < hi * 0.004) {
					span = hi * 0.004 || 1;
				}
				lo -= span * 0.12;
				hi += span * 0.12;
				const m = { left: 96, right: 16, top: 8, bottom: 26 };
				const rowH = 42;
				const height = m.top + rowsDef.length * rowH + m.bottom;
				const iw = width - m.left - m.right;
				const X = (v) => m.left + ((v - lo) / (hi - lo)) * iw;
				const ticks = niceTicks(lo, hi, Math.max(3, Math.floor(iw / 90)));
				const fmt = tickFormatter("time", "abs", hi, ticks.step);
				const out = [];
				for (const t of ticks.ticks) {
					const x = X(t);
					out.push('<line class="grid-line" x1="' + x.toFixed(1) + '" x2="' + x.toFixed(1) + '" y1="' + m.top + '" y2="' + (height - m.bottom) + '"/>');
					out.push('<text class="tick" x="' + x.toFixed(1) + '" y="' + (height - m.bottom + 16) + '" text-anchor="middle">' + esc(fmt(t)) + "</text>");
				}
				rowsDef.forEach((r, k) => {
					const cy = m.top + k * rowH + rowH / 2;
					out.push('<text class="strip-label" x="0" y="' + (cy - 1) + '">' + esc(r.label) + "</text>");
					out.push('<text class="strip-sub" x="0" y="' + (cy + 12) + '">' + esc(r.sub) + "</text>");
					r.vals.forEach((v, i) => {
						if (isNum(v)) {
							// 重なりが見えるよう、縦方向に少しだけずらす (決まった順なので毎回同じ位置)
							const jitter = ((i * 7) % 5 - 2) * 2.2;
							out.push('<circle class="' + r.dot + '" cx="' + X(v).toFixed(1) + '" cy="' + (cy + jitter).toFixed(1) + '" r="4.2"/>');
						}
					});
					if (isNum(r.med)) {
						const x = X(r.med);
						out.push('<line class="' + r.medCls + '" x1="' + x.toFixed(1) + '" x2="' + x.toFixed(1) + '" y1="' + (cy - 11) + '" y2="' + (cy + 11) + '"/>');
					}
				});
				const label = rowsDef.map((r) => r.label + " の中央値 " + fmtTime(r.med) + " (" + r.vals.length + " ラウンド)").join("、");
				strip.innerHTML = '<svg class="strip" width="' + width + '" height="' + height + '" viewBox="0 0 ' + width + " " + height + '" role="img" aria-label="' + esc(label) + '">' + out.join("") + "</svg>";
			}
		}
		const box = $("d-ratio");
		if (box && row.paired) {
			const width = Math.floor(box.clientWidth);
			if (width > 40) {
				const lo = row.paired.lo - 1;
				const hi = row.paired.hi - 1;
				const c = row.paired.change;
				const dom = Math.max(row.thr * 2, Math.abs(lo), Math.abs(hi), Math.abs(c)) * 1.15;
				const m = { left: 12, right: 12 };
				const iw = width - m.left - m.right;
				const X = (v) => m.left + ((v + dom) / (2 * dom)) * iw;
				const cls = "c-" + row.verdict;
				const t = niceTicks(-dom, dom, Math.max(2, Math.floor(iw / 80)));
				const out = [];
				out.push('<rect class="fp-band" x="' + X(-row.thr).toFixed(1) + '" y="6" width="' + (X(row.thr) - X(-row.thr)).toFixed(1) + '" height="24" rx="3"/>');
				for (const v of t.ticks) {
					out.push('<text class="tick" x="' + X(v).toFixed(1) + '" y="46" text-anchor="middle">' + esc(fmtPct(v, t.step * 100 < 1 ? 1 : 0)) + "</text>");
				}
				out.push('<line class="fp-zero" x1="' + X(0).toFixed(1) + '" x2="' + X(0).toFixed(1) + '" y1="2" y2="34"/>');
				out.push('<line class="fp-ci ' + cls + '" x1="' + X(lo).toFixed(1) + '" x2="' + X(hi).toFixed(1) + '" y1="18" y2="18"/>');
				out.push('<line class="fp-ci ' + cls + '" x1="' + X(lo).toFixed(1) + '" x2="' + X(lo).toFixed(1) + '" y1="12" y2="24"/>');
				out.push('<line class="fp-ci ' + cls + '" x1="' + X(hi).toFixed(1) + '" x2="' + X(hi).toFixed(1) + '" y1="12" y2="24"/>');
				out.push('<circle class="fp-pt ' + cls + '" cx="' + X(c).toFixed(1) + '" cy="18" r="6"/>');
				const label = "対の比較: " + fmtPct(c) + "、95% 信頼区間 " + fmtCi(row.paired.lo, row.paired.hi) + "。灰色の帯はしきい値 ±" + Math.round(row.thr * 100) + " %";
				// 判定の規則は bench_common.py の paired_analysis と同じ
				box.innerHTML = '<svg class="ratio-svg" width="' + width + '" height="52" viewBox="0 0 ' + width + ' 52" role="img" aria-label="' + esc(label) + '">' + out.join("") + "</svg>"
					+ '<p class="d-sub rule-note">灰色の帯はしきい値 ±' + Math.round(row.thr * 100) + NNBSP + "%" + (row.threads > 1 ? " (複数スレッドのため 2 倍)" : "") + "。"
					+ "点 (比の中央値) が帯の外にあり、区間が 0 をまたがないとき悪化 / 改善、"
					+ "区間の全体が帯の中に収まるとき変化なし、それ以外 (区間が帯からはみ出す) はばらつき大と判定します。</p>";
			}
		}
	}

	function aboutHtml(row) {
		let budget;
		if (row.budget == null) {
			budget = "予算なし。回数は記録しますが、増えても CI は止めません。";
		}
		else if (Number(row.budget) === 0) {
			budget = "1 op あたり 0 回 (ヒープ確保をしてはいけない)。1 回でも確保すると CI が失敗します。";
		}
		else {
			budget = "1 op あたり " + row.budget + " 回まで。上回ると CI が失敗します。";
		}
		const threads = row.threads > 1 ? row.threads + " スレッド (JobSystem のワーカーと呼び出し側)。VM の混み具合の影響を受けやすいので、しきい値を 2 倍にしています。" : "1 スレッド";
		return '<section class="d-section about" aria-labelledby="d-about-title"><div class="d-section-head"><h3 id="d-about-title">このベンチマークについて</h3></div><dl>'
			+ "<dt>内容</dt><dd>" + esc(row.title) + "</dd>"
			+ "<dt>1 op</dt><dd>" + esc(perLabel(row.per)) + " <span class=\"muted\">(<code>" + esc(row.per) + "</code>)</span></dd>"
			+ "<dt>スレッド</dt><dd>" + esc(threads) + "</dd>"
			+ "<dt>確保の予算</dt><dd>" + esc(budget) + "</dd>"
			+ '<dt>名前</dt><dd><code>' + esc(row.name) + "</code></dd>"
			+ "</dl></section>";
	}

	// ================================================================ 2 コミット比較

	function compareOptions(vm) {
		const idx = new Set();
		for (const s of Object.values(vm.series)) {
			for (const c of s.c || []) {
				idx.add(c);
			}
		}
		const list = Array.from(idx).sort((a, b) => b - a).map((i) => ({ key: sha7(model.commits[i].sha), idx: i, commit: model.commits[i] }));
		if (vm.extraRun && vm.run && vm.run.commit) {
			list.unshift({ key: sha7(vm.run.commit.sha), idx: -1, commit: vm.run.commit, extra: true });
		}
		return list;
	}

	function comparePoint(vm, opt, name) {
		if (opt.extra) {
			const rb = vm.runBench.get(name);
			return rb && rb.head && isNum(rb.head.median)
				? { m: rb.head.median, cpu: vm.run.env ? vm.run.env.cpu : null, a: rb.alloc && isNum(rb.alloc.allocs) ? rb.alloc.allocs : null }
				: null;
		}
		const s = hasSeries(vm, name) ? vm.series[name] : null;
		if (!s || !s.c || !s.m) {
			return null;
		}
		// c は昇順なので二分探索
		let lo = 0;
		let hi = s.c.length - 1;
		while (lo <= hi) {
			const mid = (lo + hi) >> 1;
			if (s.c[mid] === opt.idx) {
				const env = vm.envs[s.e ? s.e[mid] : -1];
				const a = s.a ? s.a[mid] : null;
				return isNum(s.m[mid]) ? { m: s.m[mid], cpu: env ? env.cpu : null, a: isNum(a) ? a : null } : null;
			}
			if (s.c[mid] < opt.idx) {
				lo = mid + 1;
			}
			else {
				hi = mid - 1;
			}
		}
		return null;
	}

	/// その選択肢 (1 回の CI 実行) の CPU。CPU は実行ごとに決まるので、点のあるどのベンチマークで見てもよい
	function compareCpu(vm, opt, names) {
		for (const name of names) {
			const p = comparePoint(vm, opt, name);
			if (p) {
				return { cpu: p.cpu };
			}
		}
		return null;
	}

	let compareRows = [];

	function renderCompare() {
		const vm = vmNow();
		const opts = compareOptions(vm);
		const selA = $("compare-a");
		const selB = $("compare-b");
		const find = (k) => opts.find((o) => o.key === k || (o.commit && o.commit.sha === k));
		const names = new Set(vm.rows.map((r) => r.name));
		Object.keys(vm.series).forEach((n) => names.add(n));
		let b = state.cmpB ? find(state.cmpB) : null;
		let a = state.cmpA ? find(state.cmpA) : null;
		let noSameCpu = false;
		if (!b) {
			b = opts[0] || null;
		}
		if (!a && b) {
			// 既定: B が最新の計測 (ブランチのプレビューか master の最新) なら、その計測の比較対象 (base)。
			// 概要の判定は head と base を同じ VM で交互に測った比なので、A を base にそろえないと
			// 概要では「変化なし」の行がここでは「遅くなった」と出るような食い違いが起きる。
			// それ以外は、B より古い点のうち B と同じ CPU の最も新しいもの
			// (ランナーの CPU は実行ごとに入れ替わるので、1 つ前をそのまま選ぶと CPU の差を比べてしまう)
			const bIsRun = b.extra || (b.commit && vm.run && vm.run.commit && b.commit.sha === vm.run.commit.sha);
			const baseSha = bIsRun && vm.run && vm.run.base ? vm.run.base.sha : null;
			a = baseSha ? find(baseSha) : null;
			if (!a) {
				const i = opts.indexOf(b);
				const cb = compareCpu(vm, b, names);
				for (let j = i + 1; j < opts.length && cb; j++) {
					const ca = compareCpu(vm, opts[j], names);
					if (ca && ca.cpu === cb.cpu) {
						a = opts[j];
						break;
					}
				}
				if (!a) {
					a = opts[i + 1] || opts[0] || null;
					noSameCpu = !!(a && a !== b);
				}
			}
		}
		state.cmpA = a ? a.key : null;
		state.cmpB = b ? b.key : null;
		const optHtml = (sel) => opts.map((o) => {
			const c = o.commit || {};
			const label = o.key + " · " + truncate(c.subject, 48) + " · " + fmtDate(c.date) + (o.extra ? (vm.candidate ? " (このブランチ)" : " (最新)") : "");
			return '<option value="' + esc(o.key) + '"' + (sel && o.key === sel.key ? " selected" : "") + ">" + esc(label) + "</option>";
		}).join("");
		selA.innerHTML = optHtml(a);
		selB.innerHTML = optHtml(b);
		writeHash();

		const rows = [];
		if (a && b) {
			for (const name of names) {
				const pa = comparePoint(vm, a, name);
				const pb = comparePoint(vm, b, name);
				const meta = vm.rowsByName.get(name) || (hasSeries(vm, name) ? vm.series[name] : null) || {};
				const threads = meta.threads || 1;
				const thr = (threads > 1 ? 2 : 1) * DEFAULT_THRESHOLD;
				if (!pa && !pb) {
					continue;
				}
				const change = pa && pb && pa.m > 0 ? pb.m / pa.m - 1 : null;
				rows.push({
					name, title: meta.title || name, group: meta.group || name.split("/")[0], per: meta.per || "op", thr,
					a: pa, b: pb, change, mismatch: !!(pa && pb && pa.cpu !== pb.cpu),
				});
			}
		}
		// CPU が違う行は参考値なので、同じ CPU の行の後ろに回す
		const mag = (r) => (isNum(r.change) ? Math.abs(r.change) : -1);
		rows.sort((x, y) => (x.mismatch ? 1 : 0) - (y.mismatch ? 1 : 0) || mag(y) - mag(x) || x.name.localeCompare(y.name));
		compareRows = rows;

		// 要約
		const both = rows.filter((r) => isNum(r.change));
		const same = both.filter((r) => !r.mismatch);
		const geo = (list) => (list.length ? Math.exp(list.reduce((sum, r) => sum + Math.log(1 + r.change), 0) / list.length) - 1 : null);
		const g = geo(same.length ? same : both);
		// CPU が違う行の差は CPU の差を含むので、「しきい値を超えた変化」には数えない
		const big = same.filter((r) => Math.abs(r.change) >= r.thr).length;
		const mism = both.length - same.length;
		const allMismatch = both.length > 0 && mism === both.length;
		const allocDiff = rows.filter((r) => r.a && r.b && isNum(r.a.a) && isNum(r.b.a) && cmpAlloc(r.b.a, r.a.a) !== "unchanged").length;
		const sum = [];
		if (a && b && a.key === b.key) {
			sum.push('<span class="cs-item">A と B が同じコミットです</span>');
		}
		sum.push('<span class="cs-item">幾何平均 <strong' + (allMismatch ? ' class="is-ref"' : "") + ">" + esc(fmtPct(g)) + "</strong>"
			+ (allMismatch ? '<span class="muted">(参考)</span>' : same.length && mism ? '<span class="muted">(同じ CPU の ' + esc(same.length) + " 件)</span>" : "") + "</span>");
		sum.push('<span class="cs-item">しきい値を超えた変化 <strong>' + esc(big) + "</strong> 件" + (mism && !allMismatch ? '<span class="muted">(CPU 違いの ' + esc(mism) + " 件を除く)</span>" : "") + "</span>");
		if (allocDiff) {
			sum.push('<span class="cs-item">確保回数の変化 <strong>' + esc(allocDiff) + "</strong> 件</span>");
		}
		if (allMismatch) {
			const ra = both[0].a.cpu;
			const rbc = both[0].b.cpu;
			sum.push('<span class="cs-item cs-warn"><span class="cpu-warn" style="margin:0"><span aria-hidden="true">!</span> CPU が異なるため参考値</span>'
				+ '<span class="muted">A は ' + esc(cpuShort(ra)) + "、B は " + esc(cpuShort(rbc)) + " で計測。差には CPU の違いが含まれます"
				+ (noSameCpu ? " (B と同じ CPU で計測した、より古い点がありません)" : "") + "</span></span>");
		}
		else if (mism) {
			sum.push('<span class="cs-item"><span class="cpu-warn" style="margin:0"><span aria-hidden="true">!</span> CPU が異なる ' + esc(mism) + " 件 (参考値として末尾に表示)</span></span>");
		}
		if (a && b && a.commit && b.commit && a.commit.sha && b.commit.sha) {
			sum.push('<a href="https://github.com/' + esc(model.repo) + "/compare/" + esc(a.commit.sha) + "..." + esc(b.commit.sha) + '" target="_blank" rel="noopener">GitHub で差分を見る <span class="ext" aria-hidden="true">↗</span></a>');
		}
		$("compare-summary").innerHTML = sum.join("");

		$("compare-body").innerHTML = rows.map((r) => {
			let verdict;
			if (!isNum(r.change)) {
				verdict = '<span class="badge v-none"><span class="glyph" aria-hidden="true">—</span>片方のみ</span>';
			}
			else if (r.mismatch) {
				// CPU が違う 2 点の差は CPU の差を含むので、速くなった / 遅くなったとは言わない
				verdict = '<span class="badge v-none" title="' + esc("A は " + cpuShort(r.a.cpu) + "、B は " + cpuShort(r.b.cpu) + " で計測。差には CPU の違いが含まれます") + '"><span class="glyph" aria-hidden="true">!</span>CPU 違い (参考)</span>'
					+ '<span class="cmp-cpu">' + esc(cpuShort(r.a.cpu) + " → " + cpuShort(r.b.cpu)) + "</span>";
			}
			else if (r.change >= r.thr) {
				verdict = '<span class="badge v-regressed est"><span class="glyph" aria-hidden="true">▲</span>遅くなった</span>';
			}
			else if (r.change <= -r.thr) {
				verdict = '<span class="badge v-improved est"><span class="glyph" aria-hidden="true">▼</span>速くなった</span>';
			}
			else {
				verdict = '<span class="badge v-unchanged"><span class="glyph" aria-hidden="true">≈</span>差が小さい</span>';
			}
			return '<tr class="row' + (r.mismatch ? " is-mismatch" : "") + '" tabindex="0" data-name="' + esc(r.name) + '">'
				+ '<td class="cell-name"><span class="b-title" title="' + esc(r.title) + '">' + esc(r.title) + '</span><span class="b-name"><span class="tag">' + esc(r.group) + '</span><span class="nm">' + esc(r.name) + "</span></span></td>"
				+ '<td class="cell-a num"><span class="val">' + esc(fmtTime(r.a && r.a.m)) + '</span><span class="per">' + esc(r.a ? cpuShort(r.a.cpu) : "") + "</span></td>"
				+ '<td class="cell-b num"><span class="val">' + esc(fmtTime(r.b && r.b.m)) + '</span><span class="per">' + esc(r.b ? cpuShort(r.b.cpu) : "") + "</span></td>"
				+ '<td class="cell-change"><div class="change"><span class="pct' + (isNum(r.change) && !r.mismatch && Math.abs(r.change) >= r.thr ? " is-sig" : "") + (r.mismatch ? " is-fallback" : "") + '">' + esc(fmtPct(r.change)) + "</span>" + compareBar(r) + "</div></td>"
				+ '<td class="cell-alloc num"><div class="alloc">' + compareAllocHtml(r) + "</div></td>"
				+ '<td class="cell-verdict"><div class="cmp-verdict">' + verdict + "</div></td></tr>";
		}).join("") || '<tr class="empty-row"><td colspan="6">比べられるデータがありません。</td></tr>';
		$("dl-csv-sub").textContent = "比較表の " + rows.length + " 行";
	}

	/// 比較表の確保回数: 同じなら 1 つ、違えば "A → B" と増減の印
	function compareAllocHtml(r) {
		const va = r.a && isNum(r.a.a) ? r.a.a : null;
		const vb = r.b && isNum(r.b.a) ? r.b.a : null;
		if (va == null && vb == null) {
			return '<span class="muted">—</span>';
		}
		const fa = allocFmt(va, r.per);
		const fb = allocFmt(vb, r.per);
		const title = "A " + fa.text + " 回 / " + r.per + (fa.title ? " (" + fa.title + ")" : "") + "\nB " + fb.text + " 回 / " + r.per + (fb.title ? " (" + fb.title + ")" : "");
		if (va == null || vb == null || cmpAlloc(vb, va) === "unchanged") {
			const v = vb != null ? vb : va;
			return '<span class="alloc-val' + (v === 0 ? " is-zero" : "") + '" title="' + esc(title) + '">' + esc((vb != null ? fb : fa).text) + "</span>";
		}
		const up = vb > va;
		return '<span class="alloc-val" title="' + esc(title) + '">'
			+ (up ? '<span class="alloc-up" aria-hidden="true">▲</span><span class="sr-only">増加 </span>' : '<span class="alloc-down" aria-hidden="true">▼</span><span class="sr-only">減少 </span>')
			+ '<span class="alloc-from">' + esc(fa.text) + '<span aria-hidden="true"> →</span><span class="sr-only"> から </span></span>' + esc(fb.text) + "</span>";
	}

	function compareBar(r) {
		const W = 116;
		const cx = W / 2;
		const half = W / 2 - 7;
		const x = (c) => cx + (clamp(c, -FOREST_DOMAIN, FOREST_DOMAIN) / FOREST_DOMAIN) * half;
		const parts = ['<rect class="fp-band" x="' + x(-r.thr).toFixed(1) + '" y="4" width="' + (x(r.thr) - x(-r.thr)).toFixed(1) + '" height="16" rx="2"/>', '<line class="fp-zero" x1="' + cx + '" x2="' + cx + '" y1="2" y2="22"/>'];
		if (isNum(r.change)) {
			// CPU が違う行は判定の色を付けず、中身の無い点 (参考値) で描く
			const cls = r.mismatch ? "c-none" : r.change >= r.thr ? "c-regressed" : r.change <= -r.thr ? "c-improved" : "c-unchanged";
			parts.push('<line class="fp-ci ' + cls + '" x1="' + cx + '" x2="' + x(r.change).toFixed(1) + '" y1="12" y2="12" style="opacity:.45"/>');
			parts.push(r.mismatch
				? '<circle class="fp-fallback" cx="' + x(r.change).toFixed(1) + '" cy="12" r="3.5"/>'
				: '<circle class="fp-pt ' + cls + '" cx="' + x(r.change).toFixed(1) + '" cy="12" r="4.5"/>');
			if (Math.abs(r.change) > FOREST_DOMAIN) {
				parts.push('<path class="fp-arrow ' + cls + '" d="M' + (r.change < 0 ? cx - half - 6 + " 12l5-4v8z" : cx + half + 6 + " 12l-5-4v8z") + '"/>');
			}
		}
		const label = "B / A の変化 " + fmtPct(r.change) + (r.mismatch ? " (CPU が違うため参考値)" : "");
		return '<svg class="forest" width="' + W + '" height="24" viewBox="0 0 ' + W + ' 24" role="img" aria-label="' + esc(label) + '">' + parts.join("") + "</svg>";
	}

	// ================================================================ フッター・読み方

	function renderFooter() {
		const repo = model.repo;
		const bits = [];
		if (H && H.generated) {
			bits.push("<span>生成 " + esc(fmtDateTime(H.generated)) + "</span>");
		}
		const schemas = [H && H.schema, L && L.schema].filter(Boolean);
		if (schemas.length) {
			bits.push("<span>データ形式 " + schemas.map((s) => "<code>" + esc(s) + "</code>").join(" · ") + "</span>");
		}
		if (H && H.commits) {
			bits.push("<span>" + H.commits.length + " コミット分の履歴</span>");
		}
		bits.push('<a href="https://github.com/' + esc(repo) + '/actions/workflows/ci.yml" target="_blank" rel="noopener">CI ワークフロー</a>');
		if (H) {
			bits.push('<a href="data/history.json">history.json</a>');
		}
		if (L) {
			bits.push('<a href="data/latest.js">latest.js</a>');
		}
		$("data-menu").hidden = !hasData;
		$("dl-history").hidden = !H;
		$("dl-latest").hidden = !L;
		$("footer-inner").innerHTML = bits.join("");
		$("guide-links").innerHTML = '<a href="https://github.com/' + esc(repo) + '/tree/master/runtime/bench" target="_blank" rel="noopener">ベンチマークのソース</a>'
			+ '<a href="https://github.com/' + esc(repo) + '/actions/workflows/ci.yml" target="_blank" rel="noopener">CI の実行一覧</a>';
		const thr = vmNow() ? vmNow().summary.threshold : DEFAULT_THRESHOLD;
		for (const el of document.querySelectorAll(".js-thr")) {
			el.textContent = String(Math.round(thr * 100));
		}
		$("repo-link").href = "https://github.com/" + repo;
		$("repo-name").textContent = repo;
	}

	// ================================================================ CSV・トースト

	function csvCell(v) {
		if (v == null) {
			return "";
		}
		const s = String(v);
		return /[",\n]/.test(s) ? '"' + s.replace(/"/g, '""') + '"' : s;
	}

	function downloadCsv() {
		const vm = vmNow();
		if (!vm) {
			return;
		}
		let header;
		let lines;
		let name;
		const sha = vm.summary.commit ? sha7(vm.summary.commit.sha) : "";
		if (state.view === "compare") {
			header = ["variant", "name", "group", "title", "a_sha", "b_sha", "a_median_ns", "b_median_ns", "change", "cpu_mismatch", "a_allocs_per_op", "b_allocs_per_op"];
			lines = compareRows.map((r) => [vm.id, r.name, r.group, r.title, state.cmpA, state.cmpB, r.a && r.a.m, r.b && r.b.m, isNum(r.change) ? r.change.toFixed(5) : "", r.mismatch, r.a ? r.a.a : "", r.b ? r.b.a : ""]);
			name = "nox-bench-" + vm.id + "-" + state.cmpA + "-" + state.cmpB + ".csv";
		}
		else {
			header = ["variant", "name", "group", "title", "per", "threads", "median_ns", "change", "ci_low", "ci_high", "verdict", "allocs_per_op", "bytes_per_op", "alloc_budget", "budget_ok"];
			lines = visibleRows.map((r) => [
				vm.id, r.name, r.group, r.title, r.per, r.threads, r.value,
				r.paired ? r.paired.change.toFixed(5) : "", r.paired ? r.paired.lo : "", r.paired ? r.paired.hi : "",
				r.verdict !== "none" ? r.verdict : "", r.alloc ? r.alloc.allocs : "", r.alloc ? r.alloc.bytes : "", r.budget, r.alloc ? r.alloc.budgetOk : "",
			]);
			name = "nox-bench-" + vm.id + (sha ? "-" + sha : "") + ".csv";
		}
		// Excel が UTF-8 と判断できるよう BOM を付ける
		const text = "﻿" + [header].concat(lines).map((l) => l.map(csvCell).join(",")).join("\r\n") + "\r\n";
		const url = URL.createObjectURL(new Blob([text], { type: "text/csv;charset=utf-8" }));
		const a = document.createElement("a");
		a.href = url;
		a.download = name;
		document.body.appendChild(a);
		a.click();
		a.remove();
		setTimeout(() => URL.revokeObjectURL(url), 1000);
		showToast("CSV を保存しました (" + lines.length + " 行)");
	}

	let toastTimer = 0;

	function showToast(msg) {
		const t = $("toast");
		t.textContent = msg;
		t.hidden = false;
		clearTimeout(toastTimer);
		toastTimer = setTimeout(() => {
			t.hidden = true;
		}, 2400);
	}

	function copyLink() {
		const url = location.href;
		const done = () => showToast("リンクをコピーしました");
		if (navigator.clipboard && navigator.clipboard.writeText) {
			navigator.clipboard.writeText(url).then(done, () => fallbackCopy(url, done));
		}
		else {
			fallbackCopy(url, done);
		}
	}

	function fallbackCopy(text, done) {
		const ta = document.createElement("textarea");
		ta.value = text;
		ta.setAttribute("readonly", "");
		ta.style.position = "fixed";
		ta.style.opacity = "0";
		(drawer.open ? drawer : document.body).appendChild(ta);
		ta.select();
		let ok = false;
		try {
			ok = document.execCommand("copy");
		}
		catch {
			ok = false;
		}
		ta.remove();
		if (ok) {
			done();
		}
		else {
			showToast("コピーできませんでした。アドレスバーの URL を使ってください");
		}
	}

	// ================================================================ テーマ

	function currentThemeChoice() {
		const t = root.dataset.theme;
		return t === "light" || t === "dark" ? t : "auto";
	}

	function applyTheme(choice) {
		if (choice === "light" || choice === "dark") {
			root.dataset.theme = choice;
		}
		else {
			delete root.dataset.theme;
		}
		storageSet(THEME_KEY, choice);
		syncThemeButtons();
		rerenderCharts();
	}

	function syncThemeButtons() {
		const choice = currentThemeChoice();
		for (const b of document.querySelectorAll("[data-theme-choice]")) {
			const on = b.dataset.themeChoice === choice;
			b.setAttribute("aria-checked", on);
			b.tabIndex = on ? 0 : -1;
		}
	}

	function rerenderCharts() {
		// 色は CSS 変数なので描き直しは必須ではないが、文字幅などが変わる場合に備えて描き直す
		if (drawer.open && chartCtl) {
			chartCtl.render();
			renderRunCharts(drawerRow);
		}
	}

	// ================================================================ イベント

	function moveRowFocus(delta) {
		const rows = Array.from(document.querySelectorAll((state.view === "compare" ? "#compare-body" : "#bench-body") + " tr.row"));
		if (!rows.length) {
			return;
		}
		const cur = rows.indexOf(document.activeElement);
		const next = cur < 0 ? (delta > 0 ? 0 : rows.length - 1) : clamp(cur + delta, 0, rows.length - 1);
		rows[next].focus();
		rows[next].scrollIntoView({ block: "nearest" });
	}

	function clearFilters() {
		state.query = "";
		state.group = "all";
		state.changedOnly = false;
		state.verdictFilter = null;
		renderChips();
		renderKpis();
		renderTable();
	}

	function clearVerdictFilter() {
		state.verdictFilter = null;
		renderKpis();
		renderTable();
	}

	function onRowActivate(tr) {
		const name = tr.dataset.name;
		if (state.view === "compare") {
			// 比較表の行は概要へ移ってから詳細を開く
			state.view = "overview";
			renderAll();
		}
		openDrawer(name);
	}

	function bindEvents() {
		// 分割ボタン (radiogroup) の矢印キー移動
		document.addEventListener("keydown", (ev) => {
			const btn = ev.target.closest && ev.target.closest('[role="radiogroup"] [role="radio"]');
			if (!btn || (ev.key !== "ArrowRight" && ev.key !== "ArrowLeft")) {
				return;
			}
			const group = btn.parentElement;
			const items = Array.from(group.querySelectorAll('[role="radio"]:not([disabled])'));
			const i = items.indexOf(btn);
			const next = items[(i + (ev.key === "ArrowRight" ? 1 : items.length - 1)) % items.length];
			if (next) {
				ev.preventDefault();
				next.focus();
				next.click();
			}
		});

		$("variant-picker").addEventListener("click", (ev) => {
			const b = ev.target.closest("[data-variant]");
			if (b) {
				setVariant(b.dataset.variant);
			}
		});

		for (const b of document.querySelectorAll("[data-theme-choice]")) {
			b.addEventListener("click", () => applyTheme(b.dataset.themeChoice));
		}
		if (window.matchMedia) {
			const mq = window.matchMedia("(prefers-color-scheme: dark)");
			const onChange = () => rerenderCharts();
			if (mq.addEventListener) {
				mq.addEventListener("change", onChange);
			}
		}

		// データメニュー
		const menuBtn = $("data-menu-button");
		const menuList = $("data-menu-list");
		const closeMenu = (focusBtn) => {
			menuList.hidden = true;
			menuBtn.setAttribute("aria-expanded", "false");
			if (focusBtn) {
				menuBtn.focus();
			}
		};
		menuBtn.addEventListener("click", () => {
			const open = menuList.hidden;
			menuList.hidden = !open;
			menuBtn.setAttribute("aria-expanded", String(open));
			if (open) {
				const first = menuList.querySelector('[role="menuitem"]');
				if (first) {
					first.focus();
				}
			}
		});
		menuList.addEventListener("keydown", (ev) => {
			const items = Array.from(menuList.querySelectorAll('[role="menuitem"]'));
			const i = items.indexOf(document.activeElement);
			if (ev.key === "ArrowDown" || ev.key === "ArrowUp") {
				ev.preventDefault();
				items[(i + (ev.key === "ArrowDown" ? 1 : items.length - 1)) % items.length].focus();
			}
			else if (ev.key === "Escape") {
				ev.preventDefault();
				closeMenu(true);
			}
			else if (ev.key === "Tab") {
				closeMenu(false);
			}
		});
		document.addEventListener("click", (ev) => {
			if (!menuList.hidden && !ev.target.closest("#data-menu")) {
				closeMenu(false);
			}
		});
		$("dl-csv").addEventListener("click", () => {
			closeMenu(true);
			downloadCsv();
		});
		for (const a of menuList.querySelectorAll("a")) {
			a.addEventListener("click", () => closeMenu(false));
		}

		// ツールバー
		const search = $("search");
		let searchTimer = 0;
		search.addEventListener("input", () => {
			clearTimeout(searchTimer);
			searchTimer = setTimeout(() => {
				state.query = search.value;
				renderTable();
			}, 60);
		});
		search.addEventListener("keydown", (ev) => {
			if (ev.key === "Escape" && search.value) {
				ev.preventDefault();
				search.value = "";
				state.query = "";
				renderTable();
			}
			else if (ev.key === "Enter" || ev.key === "ArrowDown") {
				ev.preventDefault();
				const first = document.querySelector("#bench-body tr.row");
				if (first) {
					first.focus();
				}
			}
		});
		$("group-chips").addEventListener("click", (ev) => {
			const c = ev.target.closest("[data-group]");
			if (!c) {
				return;
			}
			state.group = state.group === c.dataset.group && c.dataset.group !== "all" ? "all" : c.dataset.group;
			for (const b of document.querySelectorAll("#group-chips [data-group]")) {
				b.setAttribute("aria-pressed", String(b.dataset.group === state.group));
			}
			renderTable();
		});
		$("changed-only").addEventListener("change", (ev) => {
			state.changedOnly = ev.target.checked;
			renderTable();
		});
		$("sort").addEventListener("change", (ev) => {
			state.sort = ev.target.value;
			renderTable();
		});

		// KPI をクリックすると、その判定の行だけに絞り込む (もう一度押すと外す)。
		// 絞り込みは表の上のチップ (「悪化のみ ×」) にも出して、そこからも外せるようにする
		for (const id of ["kpi-regressed", "kpi-improved", "kpi-budget"]) {
			$(id).addEventListener("click", () => {
				const k = $(id).dataset.kpi === "budget" ? "alloc" : $(id).dataset.kpi;
				if (state.verdictFilter === k) {
					clearVerdictFilter();
					return;
				}
				state.verdictFilter = k;
				state.changedOnly = false;
				state.group = "all";
				state.query = "";
				if (k !== "alloc") {
					state.sort = k;
				}
				renderChips();
				renderKpis();
				renderTable();
				$("bench-heading").scrollIntoView({ behavior: matchMedia("(prefers-reduced-motion: reduce)").matches ? "auto" : "smooth", block: "start" });
			});
		}

		// 表の行
		for (const bodyId of ["bench-body", "compare-body"]) {
			const body = $(bodyId);
			body.addEventListener("click", (ev) => {
				if (ev.target.closest("a, button")) {
					const act = ev.target.closest("[data-action]");
					if (act && act.dataset.action === "clear-filters") {
						clearFilters();
					}
					return;
				}
				const tr = ev.target.closest("tr.row");
				if (tr) {
					onRowActivate(tr);
				}
			});
			body.addEventListener("keydown", (ev) => {
				const tr = ev.target.closest && ev.target.closest("tr.row");
				if (!tr || ev.target !== tr) {
					return;
				}
				if (ev.key === "Enter" || ev.key === " ") {
					ev.preventDefault();
					onRowActivate(tr);
				}
				else if (ev.key === "ArrowDown" || ev.key === "ArrowUp") {
					ev.preventDefault();
					moveRowFocus(ev.key === "ArrowDown" ? 1 : -1);
				}
			});
		}
		$("result-count").addEventListener("click", (ev) => {
			if (ev.target.closest('[data-action="clear-filters"]')) {
				clearFilters();
			}
			else if (ev.target.closest('[data-action="clear-verdict"]')) {
				clearVerdictFilter();
				// 押したチップは消えるので、フォーカスを検索欄へ移す (body に落とさない)
				$("search").focus({ preventScroll: true });
			}
		});

		// 全体のショートカット: / で検索、j / k で行を移動
		document.addEventListener("keydown", (ev) => {
			if (ev.defaultPrevented || ev.ctrlKey || ev.metaKey || ev.altKey || drawer.open || !hasData) {
				return;
			}
			const t = ev.target;
			const typing = t && t.matches && t.matches("input, select, textarea, [contenteditable]");
			if (typing) {
				return;
			}
			if (ev.key === "/" && state.view === "overview") {
				ev.preventDefault();
				search.focus();
				search.select();
			}
			else if (ev.key === "j" || ev.key === "k") {
				ev.preventDefault();
				moveRowFocus(ev.key === "j" ? 1 : -1);
			}
		});

		// 比較
		$("compare-a").addEventListener("change", (ev) => {
			state.cmpA = ev.target.value;
			renderCompare();
		});
		$("compare-b").addEventListener("change", (ev) => {
			state.cmpB = ev.target.value;
			renderCompare();
		});
		$("compare-swap").addEventListener("click", () => {
			const a = state.cmpA;
			state.cmpA = state.cmpB;
			state.cmpB = a;
			renderCompare();
		});

		// タブ・ロゴはハッシュを書き換えるだけ (hashchange で描き直す)
		window.addEventListener("hashchange", applyHash);

		// ドロワー
		drawer.addEventListener("click", (ev) => {
			if (ev.target === drawer) {
				drawer.close(); // 背景 (::backdrop) のクリック
				return;
			}
			const sw = ev.target.closest("[data-variant-switch]");
			if (sw) {
				setVariant(sw.dataset.variantSwitch);
				const h = $("d-var-title");
				if (h) {
					h.focus({ preventScroll: true });
				}
				return;
			}
			const act = ev.target.closest("[data-action]");
			if (act) {
				if (act.dataset.action === "close-drawer") {
					drawer.close();
				}
				else if (act.dataset.action === "copy-link") {
					copyLink();
				}
				return;
			}
			const seg = ev.target.closest("[data-seg] [data-value]");
			if (seg && !seg.disabled) {
				onSeg(seg.closest("[data-seg]").dataset.seg, seg.dataset.value);
			}
		});
		drawer.addEventListener("close", () => {
			const name = state.drawer;
			state.drawer = null;
			chartCtl = null;
			drawerRow = null;
			if (resizeObs) {
				resizeObs.disconnect();
				resizeObs = null;
			}
			markOpenRow(null);
			writeHash();
			const focusName = lastRowFocus || name;
			lastRowFocus = null;
			if (focusName) {
				for (const tr of document.querySelectorAll("#bench-body tr.row")) {
					if (tr.dataset.name === focusName) {
						tr.focus({ preventScroll: false });
						break;
					}
				}
			}
		});
	}

	// ================================================================ 起動

	function init() {
		syncThemeButtons();
		if (hasData) {
			const saved = storageGet(VARIANT_KEY);
			state.variant = model.variants.has(saved) ? saved : model.variants.has("msvc-master") ? "msvc-master" : model.variantIds[0];
		}
		bindEvents();
		if (!hasData) {
			renderAll();
			return;
		}
		applyHash();
		if (!location.hash) {
			writeHash();
		}
	}

	init();
})();
