/*
 * Nox ビルド計測レポート。
 *
 * データは window.BI_DATA (.github/scripts/build-insights-report.py が埋め込む)。
 * 外部のライブラリは使わず、1 ファイルの HTML だけで開けるようにしてある。
 *
 * 表示の状態 (タブ・構成・検索語・選んだ翻訳単位) は URL の # に持つので、
 * リンクを共有すれば同じ画面が開く。
 */
(function () {
	"use strict";

	const DATA = window.BI_DATA || { configs: [], diffs: {} };
	const THEME_KEY = "nox-bi-theme";
	const PAGE = 200;

	const CATS = [
		["project", "プロジェクト"],
		["generated", "生成コード"],
		["msvc", "MSVC"],
		["sdk", "WinSDK"],
		["vcpkg", "vcpkg"],
		["other", "その他"],
	];
	const CAT_LABEL = Object.fromEntries(CATS);
	const WHERE_LABEL = { cl: "cl", ltcg: "リンク時 (LTCG)", both: "cl + LTCG" };

	// ------------------------------------------------------------ 小道具

	function esc(s) {
		return String(s == null ? "" : s)
			.replace(/&/g, "&amp;")
			.replace(/</g, "&lt;")
			.replace(/>/g, "&gt;")
			.replace(/"/g, "&quot;");
	}

	function ms(us) {
		if (us == null || us < 0) return "-";
		const v = us / 1000;
		if (v >= 60000) return (v / 60000).toFixed(1) + " 分";
		if (v >= 10000) return (v / 1000).toFixed(1) + " s";
		if (v >= 100) return Math.round(v).toLocaleString("ja-JP") + " ms";
		if (v >= 10) return v.toFixed(1) + " ms";
		return v.toFixed(2) + " ms";
	}

	function int(n) {
		return (n || 0).toLocaleString("ja-JP");
	}

	function pct(part, whole, digits) {
		if (!whole) return "-";
		return ((part / whole) * 100).toFixed(digits == null ? 1 : digits) + "%";
	}

	function change(base, cur) {
		if (!base) return cur ? "新規" : "-";
		const r = ((cur - base) / base) * 100;
		return (r >= 0 ? "+" : "") + r.toFixed(1) + "%";
	}

	function changeClass(base, cur) {
		if (!base) return "";
		const r = (cur - base) / base;
		if (r > 0.05) return "delta-up";
		if (r < -0.05) return "delta-down";
		return "";
	}

	function h(html) {
		const t = document.createElement("template");
		t.innerHTML = html.trim();
		return t.content;
	}

	function matches(text, q) {
		return !q || String(text).toLowerCase().indexOf(q) >= 0;
	}

	// ------------------------------------------------------------ 状態 (URL の #)

	const state = { tab: "overview", cfg: null, q: "", unit: null, hl: null };
	// タブごとの一時的な状態 (並び順・開いている行・表示件数・絞り込み)
	const local = {};

	function readHash() {
		const p = new URLSearchParams(location.hash.slice(1));
		state.tab = p.get("tab") || "overview";
		state.cfg = p.get("cfg") || state.cfg;
		state.q = p.get("q") || "";
		state.unit = p.get("unit");
		state.hl = p.get("hl");
		const names = DATA.configs.map((c) => c.name);
		if (!state.cfg || names.indexOf(state.cfg) < 0) state.cfg = names[0] || null;
	}

	function go(changes) {
		const next = Object.assign({}, state, changes);
		const p = new URLSearchParams();
		p.set("tab", next.tab);
		if (next.cfg) p.set("cfg", next.cfg);
		if (next.q) p.set("q", next.q);
		if (next.unit) p.set("unit", next.unit);
		if (next.hl) p.set("hl", next.hl);
		const hash = "#" + p.toString();
		if (location.hash === hash) render();
		else location.hash = hash;
	}

	function tabState(key, init) {
		const k = key + "@" + state.cfg;
		if (!local[k]) local[k] = init();
		return local[k];
	}

	function cfg() {
		return DATA.configs.find((c) => c.name === state.cfg) || null;
	}

	function diff() {
		return (DATA.diffs || {})[state.cfg] || null;
	}

	// ------------------------------------------------------------ 共通の表

	/*
	 * columns: [{key, label, num, sort (行 → 値), cell (行 → HTML), bar (行 → 0..1)}]
	 * detail: 行 → HTML (開いたときに出す)
	 */
	function renderTable(host, key, rows, columns, opts) {
		const ts = tabState(key, () => ({ sort: opts.sort, desc: true, open: new Set(), limit: PAGE }));
		if (!ts.sort) ts.sort = opts.sort;
		const col = columns.find((c) => c.key === ts.sort) || columns[1];
		const sorted = rows.slice().sort((a, b) => {
			const va = col.sort(a);
			const vb = col.sort(b);
			if (typeof va === "string") return ts.desc ? vb.localeCompare(va) : va.localeCompare(vb);
			return ts.desc ? vb - va : va - vb;
		});
		const shown = sorted.slice(0, ts.limit);
		const out = [];
		out.push('<div class="table-wrap"><table class="data"><thead><tr>');
		for (const c of columns) {
			const arrow = c.key === ts.sort ? '<span class="arrow">' + (ts.desc ? "▼" : "▲") + "</span>" : "";
			out.push('<th class="sortable' + (c.num ? " num" : "") + '" data-sort="' + c.key + '"' +
				(c.title ? ' title="' + esc(c.title) + '"' : "") + ">" + esc(c.label) + arrow + "</th>");
		}
		out.push("</tr></thead><tbody>");
		shown.forEach((r) => {
			const id = opts.id(r);
			const open = ts.open.has(id);
			out.push('<tr class="row' + (open ? " open" : "") + '" data-id="' + esc(id) + '">');
			columns.forEach((c, i) => {
				let cls = c.num ? "num" : "";
				if (i === 0) cls += " name";
				let inner = c.cell(r);
				if (i === 0 && opts.detail) inner = '<span class="caret">' + (open ? "▾" : "▸") + "</span>" + inner;
				if (c.bar) {
					const w = Math.max(0, Math.min(1, c.bar(r))) * 100;
					out.push('<td class="' + cls + ' bar-cell"><span class="bar" style="width:' + w.toFixed(1) + '%"></span><span class="val">' + inner + "</span></td>");
				} else {
					out.push('<td class="' + cls.trim() + '">' + inner + "</td>");
				}
			});
			out.push("</tr>");
			if (open && opts.detail) {
				out.push('<tr class="detail"><td colspan="' + columns.length + '">' + opts.detail(r) + "</td></tr>");
			}
		});
		out.push("</tbody></table></div>");
		if (!rows.length) out.push('<div class="empty">該当するものが無い</div>');
		if (sorted.length > shown.length) {
			out.push('<button type="button" class="show-more">さらに表示 (残り ' + int(sorted.length - shown.length) + " 件)</button>");
		}
		host.innerHTML = out.join("");

		host.querySelectorAll("th.sortable").forEach((th) =>
			th.addEventListener("click", () => {
				const k = th.dataset.sort;
				if (ts.sort === k) ts.desc = !ts.desc;
				else {
					ts.sort = k;
					const c = columns.find((x) => x.key === k);
					ts.desc = !!c.num;
				}
				renderTable(host, key, rows, columns, opts);
			})
		);
		host.querySelectorAll("tr.row").forEach((tr) =>
			tr.addEventListener("click", (e) => {
				if (e.target.closest("a")) return;
				if (!opts.detail) {
					if (opts.onClick) opts.onClick(tr.dataset.id);
					return;
				}
				const id = tr.dataset.id;
				if (ts.open.has(id)) ts.open.delete(id);
				else ts.open.add(id);
				renderTable(host, key, rows, columns, opts);
			})
		);
		const more = host.querySelector(".show-more");
		if (more)
			more.addEventListener("click", () => {
				ts.limit += PAGE * 2;
				renderTable(host, key, rows, columns, opts);
			});
		if (opts.after) opts.after(host);
	}

	function toolbar(opts) {
		const parts = ['<div class="toolbar">'];
		parts.push('<input type="search" class="search" placeholder="' + esc(opts.placeholder || "絞り込み") + '" value="' + esc(state.q) + '" aria-label="絞り込み">');
		if (opts.extra) parts.push(opts.extra);
		parts.push('<span class="info"></span></div>');
		return parts.join("");
	}

	function bindSearch(root, onChange) {
		const input = root.querySelector(".search");
		if (!input) return;
		let timer = 0;
		input.addEventListener("input", () => {
			clearTimeout(timer);
			timer = setTimeout(() => {
				state.q = input.value;
				// 入力中は # を書き換えるだけにし、フォーカスを失わないよう再描画は呼び出し側で行う
				history.replaceState(null, "", "#" + hashFor(state));
				onChange();
			}, 120);
		});
	}

	function hashFor(s) {
		const p = new URLSearchParams();
		p.set("tab", s.tab);
		if (s.cfg) p.set("cfg", s.cfg);
		if (s.q) p.set("q", s.q);
		if (s.unit) p.set("unit", s.unit);
		if (s.hl) p.set("hl", s.hl);
		return p.toString();
	}

	function setInfo(root, text) {
		const el = root.querySelector(".toolbar .info");
		if (el) el.textContent = text;
	}

	function catBadge(c, cat) {
		return '<span class="cat cat-' + cat + '" title="' + esc(CAT_LABEL[cat] || cat) + '"></span>';
	}

	// ------------------------------------------------------------ 概要

	function headerIsPchOnly(hd) {
		return hd[5] > 0 && hd[5] === hd[4];
	}

	function pchCandidates(c) {
		// PCH の外で、3 つ以上の翻訳単位が取り込んでいるヘッダ。PCH に入れれば解析が 1 回で済む
		return c.headers.filter((hd) => hd[5] === 0 && hd[4] >= 3).sort((a, b) => b[1] - a[1]);
	}

	function topList(items, label, value, onClick) {
		if (!items.length) return '<div class="empty">データが無い</div>';
		const max = Math.max.apply(null, items.map(value)) || 1;
		const out = ['<ul class="toplist">'];
		items.forEach((it, i) => {
			out.push('<li data-i="' + i + '" title="' + esc(label(it, true)) + '"><span class="bar" style="width:' + ((value(it) / max) * 100).toFixed(1) + '%"></span>' +
				'<span class="n">' + label(it) + '</span><span class="v">' + ms(value(it)) + "</span></li>");
		});
		out.push("</ul>");
		return out.join("");
	}

	function bindTopList(root, sel, items, onClick) {
		root.querySelectorAll(sel + " li").forEach((li) =>
			li.addEventListener("click", () => onClick(items[+li.dataset.i]))
		);
	}

	function tile(label, value, sub) {
		return '<div class="tile"><div class="label">' + esc(label) + '</div><div class="value">' + value + "</div>" +
			(sub ? '<div class="sub">' + sub + "</div>" : "") + "</div>";
	}

	function deltaSub(d, key) {
		if (!d || !d.reliable || !d.totals[key]) return "";
		const b = d.totals[key][0];
		const c = d.totals[key][1];
		return '前回 master 比 <span class="' + changeClass(b, c) + '">' + change(b, c) + "</span>";
	}

	function viewOverview(view, c) {
		const d = diff();
		const t = c.totals || {};
		const ltcg = c.functions.filter((f) => f.at !== "cl").reduce((s, f) => s + f.t, 0);
		const tiles = [
			tile("ビルドの実時間", ms(c.wall_us), (c.build_seconds ? "ビルドステップ全体 " + c.build_seconds.toFixed(0) + " 秒 · " : "") + deltaSub(d, "wall_us")),
			tile("フロントエンド合計", ms(t.fe_us), deltaSub(d, "fe_us") || "解析とテンプレート展開 (全翻訳単位の和)"),
			tile("バックエンド合計", ms(t.be_us), ltcg ? "リンク時のコード生成 " + ms(ltcg) + " は別" : deltaSub(d, "be_us")),
			tile("翻訳単位", int(t.passes), "解析したファイル " + int(t.file_parses)),
			tile("テンプレート展開", c.templates ? int(t.template_instantiations) : "—", c.templates ? deltaSub(d, "template_instantiations") || "回数" : "この構成では取っていない"),
			tile("コード生成した関数", int(t.functions), c.processors ? "論理 CPU " + c.processors : ""),
		];
		const cands = pchCandidates(c).slice(0, 10);
		const heavyUnits = c.units.slice().sort((a, b) => b.fe + Math.max(0, b.be) - (a.fe + Math.max(0, a.be))).slice(0, 10);
		view.innerHTML =
			'<div class="tiles">' + tiles.join("") + "</div>" +
			'<div class="grid-2">' +
			card("重いヘッダ", "全翻訳単位での解析時間の合計", "headers", topList(c.headers.slice(0, 10), (hd) => catBadge(c, c.cats[hd[0]]) + esc(c.paths[hd[0]]), (hd) => hd[1]), "ov-headers") +
			card("PCH に入れる候補", "PCH の外で 3 つ以上の翻訳単位が取り込んでいるヘッダ", "headers", topList(cands, (hd) => catBadge(c, c.cats[hd[0]]) + esc(c.paths[hd[0]]) + ' <span class="muted">×' + hd[4] + "</span>", (hd) => hd[1]), "ov-cands") +
			(c.templates
				? card("重いテンプレート", "primary template ごと (再帰の二重計上は除く)", "templates", topList(c.templates_list.slice(0, 10), (tp) => esc(tp.n), (tp) => tp.i), "ov-templates")
				: card("テンプレート", "", "templates", '<div class="empty">この構成ではテンプレートの展開を記録していない</div>', "ov-templates")) +
			card("コード生成の重い関数", "", "functions", topList(c.functions.slice(0, 10), (f) => esc(f.n), (f) => f.t), "ov-functions") +
			card("重い翻訳単位", "フロントエンド + バックエンド", "units", topList(heavyUnits, (u) => esc(u.src), (u) => u.fe + Math.max(0, u.be)), "ov-units") +
			"</div>";

		bindTopList(view, "#ov-headers", c.headers.slice(0, 10), (hd) => openRow("headers", c.paths[hd[0]]));
		bindTopList(view, "#ov-cands", cands, (hd) => openRow("headers", c.paths[hd[0]]));
		bindTopList(view, "#ov-templates", c.templates_list.slice(0, 10), (tp) => openRow("templates", tp.n));
		bindTopList(view, "#ov-functions", c.functions.slice(0, 10), (f) => openRow("functions", f.n));
		bindTopList(view, "#ov-units", heavyUnits, (u) => go({ tab: "tree", unit: u.src, q: "", hl: null }));
	}

	function card(title, hint, tab, body, id) {
		return '<div class="card"><div class="card-head"><h3>' + esc(title) + "</h3>" +
			(hint ? '<span class="hint">' + esc(hint) + "</span>" : "") +
			'<a class="more" href="#' + hashFor(Object.assign({}, state, { tab: tab, q: "", unit: null, hl: null })) + '">すべて見る →</a></div>' +
			'<div class="card-body" id="' + id + '">' + body + "</div></div>";
	}

	function openRow(tab, id) {
		const ts = tabState(tab, () => ({ sort: null, desc: true, open: new Set(), limit: PAGE }));
		ts.open.add(id);
		go({ tab: tab, q: id, unit: null, hl: null });
	}

	// ------------------------------------------------------------ ヘッダ

	function viewHeaders(view, c) {
		const fs = tabState("headers-filter", () => ({ cats: new Set(), hidePch: false }));
		const fe = (c.totals || {}).fe_us || 1;
		const chips = CATS.map(([k, label]) =>
			'<button type="button" class="chip" data-cat="' + k + '" aria-pressed="' + fs.cats.has(k) + '"><span class="swatch cat-' + k + '"></span>' + esc(label) + "</button>"
		).join("");
		view.innerHTML = '<div class="card">' +
			toolbar({
				placeholder: "ヘッダのパスで絞り込み",
				extra: '<div class="chips">' + chips + '</div><label class="check"><input type="checkbox" id="hide-pch"' + (fs.hidePch ? " checked" : "") + "> PCH 内を隠す</label>",
			}) + '<div id="tbl"></div></div>';
		const host = view.querySelector("#tbl");
		const columns = [
			{ key: "path", label: "ヘッダ", sort: (r) => c.paths[r[0]], cell: (r) => catBadge(c, c.cats[r[0]]) + esc(c.paths[r[0]]) + (headerIsPchOnly(r) ? '<span class="badge pch">PCH 内</span>' : (r[5] === 0 && r[4] >= 3 ? '<span class="badge cand" title="PCH の外で 3 つ以上の翻訳単位が取り込んでいる">PCH 候補</span>' : "")) },
			{ key: "incl", label: "合計", num: true, title: "全翻訳単位での解析時間 (中で取り込んだヘッダを含む) の合計", sort: (r) => r[1], cell: (r) => ms(r[1]), bar: (r) => r[1] / (maxIncl || 1) },
			{ key: "share", label: "FE 比", num: true, title: "フロントエンド合計に占める割合", sort: (r) => r[1], cell: (r) => pct(r[1], fe) },
			{ key: "excl", label: "自身", num: true, title: "このファイル自身の解析時間 (中で取り込んだヘッダを除く)", sort: (r) => r[2], cell: (r) => ms(r[2]) },
			{ key: "wctr", label: "按分", num: true, title: "並走していたもの同士で実時間を分けた値。ビルドの実時間への寄与", sort: (r) => r[3], cell: (r) => ms(r[3]) },
			{ key: "passes", label: "翻訳単位", num: true, title: "このヘッダを取り込んだ翻訳単位の数", sort: (r) => r[4], cell: (r) => int(r[4]) },
			{ key: "parses", label: "解析回数", num: true, title: "解析された回数 (#pragma once が無いと 1 つの翻訳単位で複数回)", sort: (r) => r[6], cell: (r) => int(r[6]) },
			{ key: "avg", label: "平均", num: true, sort: (r) => r[1] / Math.max(1, r[6]), cell: (r) => ms(r[1] / Math.max(1, r[6])) },
			{ key: "max", label: "最大", num: true, sort: (r) => r[7], cell: (r) => ms(r[7]) },
		];
		let maxIncl = 1;
		const detail = (r) => {
			const rows = (r[8] || []).map((p) => '<tr><td class="name">' + catBadge(c, c.cats[p[0]]) + esc(c.paths[p[0]]) + '</td><td class="num">' + int(p[1]) + ' 回</td><td class="num">' + ms(p[2]) + "</td></tr>").join("");
			return '<div class="detail-grid"><div><h4>取り込み元 (直接 #include しているファイル)</h4><table class="mini">' + (rows || '<tr><td class="muted">無し</td></tr>') + "</table></div>" +
				'<div><h4>この先</h4><p class="muted">' + (headerIsPchOnly(r) ? "PCH を作るときにだけ解析されている。各翻訳単位では再解析されない。" : "PCH の外で " + int(r[4]) + " 個の翻訳単位が取り込んでいる。1 回あたり平均 " + ms(r[1] / Math.max(1, r[6])) + "。") + "</p>" +
				'<a class="btn" href="#' + hashFor({ tab: "tree", cfg: state.cfg, q: "", unit: null, hl: c.paths[r[0]] }) + '">インクルードツリーで経路を見る →</a></div></div>';
		};
		const refresh = () => {
			const q = state.q.toLowerCase();
			const rows = c.headers.filter((r) =>
				matches(c.paths[r[0]], q) && (!fs.cats.size || fs.cats.has(c.cats[r[0]])) && !(fs.hidePch && headerIsPchOnly(r))
			);
			maxIncl = rows.reduce((m, r) => Math.max(m, r[1]), 1);
			renderTable(host, "headers", rows, columns, { sort: "incl", id: (r) => c.paths[r[0]], detail: detail });
			setInfo(view, int(rows.length) + " / " + int(c.headers.length) + " 件");
		};
		view.querySelectorAll(".chip").forEach((b) =>
			b.addEventListener("click", () => {
				const k = b.dataset.cat;
				if (fs.cats.has(k)) fs.cats.delete(k);
				else fs.cats.add(k);
				b.setAttribute("aria-pressed", fs.cats.has(k));
				refresh();
			})
		);
		view.querySelector("#hide-pch").addEventListener("change", (e) => {
			fs.hidePch = e.target.checked;
			refresh();
		});
		bindSearch(view, refresh);
		refresh();
	}

	// ------------------------------------------------------------ インクルードツリー

	/* ノード: [パスの番号, inclusive, exclusive, wctr, [子...], 畳んだ数, 畳んだ時間] */

	function treeContains(node, target) {
		if (!node) return false;
		if (node[0] === target) return true;
		const kids = node[4] || [];
		for (let i = 0; i < kids.length; i++) if (treeContains(kids[i], target)) return true;
		return false;
	}

	function collectHitIds(node, target, id, out) {
		let hit = node[0] === target;
		const kids = node[4] || [];
		for (let i = 0; i < kids.length; i++) {
			if (collectHitIds(kids[i], target, id + "." + i, out)) hit = true;
		}
		if (hit) out.add(id);
		return hit;
	}

	function viewTree(view, c) {
		const target = state.hl ? c.paths.findIndex((p) => p === state.hl) : -1;
		let units = c.units.filter((u) => u.tree).slice().sort((a, b) => b.fe - a.fe);
		if (target >= 0) units = units.filter((u) => treeContains(u.tree, target));
		let unit = units.find((u) => u.src === state.unit) || units[0] || null;
		const maxFe = units.reduce((m, u) => Math.max(m, u.fe), 1);

		view.innerHTML = '<div class="card">' +
			(state.hl ? '<div class="notice info" style="margin:12px 12px 0">' + (target >= 0 ? "<code>" + esc(state.hl) + "</code> を取り込んでいる翻訳単位 " + int(units.length) + " 件だけを出し、そこへの経路を開いている。" : "<code>" + esc(state.hl) + "</code> はツリーに無い (0.5 ms 未満で畳まれた可能性がある)。") + ' <a href="#' + hashFor({ tab: "tree", cfg: state.cfg }) + '">解除</a></div>' : "") +
			'<div class="split"><div class="side"><input type="search" class="search" placeholder="翻訳単位で絞り込み" aria-label="翻訳単位で絞り込み" value="' + esc(state.q) + '"><ul class="unit-list" role="listbox" aria-label="翻訳単位"></ul></div>' +
			'<div class="tree-pane"><div class="tree-head"></div><div class="tree" role="tree"></div></div></div></div>';
		const list = view.querySelector(".unit-list");
		const renderList = () => {
			const q = state.q.toLowerCase();
			const shown = units.filter((u) => matches(u.src, q));
			list.innerHTML = shown.map((u) =>
				'<li role="option" data-src="' + esc(u.src) + '" aria-selected="' + (unit && u.src === unit.src) + '" title="' + esc(u.src) + '">' +
				'<span class="bar" style="width:' + ((u.fe / maxFe) * 100).toFixed(1) + '%"></span><span class="n">' + esc(u.src) + "</span>" +
				(u.pch ? '<span class="badge pch">PCH</span>' : "") + '<span class="v">' + ms(u.fe) + "</span></li>"
			).join("") || '<li class="muted">該当なし</li>';
			list.querySelectorAll("li[data-src]").forEach((li) =>
				li.addEventListener("click", () => {
					state.unit = li.dataset.src;
					history.replaceState(null, "", "#" + hashFor(state));
					unit = units.find((u) => u.src === state.unit);
					list.querySelectorAll("li").forEach((x) => x.setAttribute("aria-selected", x === li));
					renderTreePane();
				})
			);
		};
		const input = view.querySelector(".side .search");
		let timer = 0;
		input.addEventListener("input", () => {
			clearTimeout(timer);
			timer = setTimeout(() => {
				state.q = input.value;
				history.replaceState(null, "", "#" + hashFor(state));
				renderList();
			}, 120);
		});

		const renderTreePane = () => {
			const head = view.querySelector(".tree-head");
			const pane = view.querySelector(".tree");
			if (!unit) {
				head.innerHTML = "";
				pane.innerHTML = '<div class="empty">翻訳単位が無い</div>';
				return;
			}
			const ts = tabState("tree:" + unit.src, () => {
				const open = new Set(["0"]);
				if (target >= 0) collectHitIds(unit.tree, target, "0", open);
				return { open: open };
			});
			head.innerHTML = '<span class="title">' + esc(unit.src) + "</span>" + (unit.pch ? '<span class="badge pch">PCH を作る翻訳単位</span>' : "") +
				'<span class="muted">フロントエンド ' + ms(unit.fe) + " · バックエンド " + ms(unit.be) + " · 解析したファイル " + int(unit.parses) + "</span>" +
				'<span class="btns"><button type="button" class="btn" data-act="expand">すべて開く</button><button type="button" class="btn" data-act="collapse">閉じる</button></span>';
			const rootIncl = unit.tree[1] || 1;
			const rows = ['<div class="tree-row head"><span>ファイル</span><span class="num">合計</span><span class="num">自身</span><span class="num wctr">按分</span></div>'];
			const walk = (node, id, depth) => {
				const kids = node[4] || [];
				const hasKids = kids.length > 0 || node[5] > 0;
				const open = ts.open.has(id);
				const hit = target >= 0 && node[0] === target;
				rows.push('<div class="tree-row' + (hit ? " hit" : "") + '" role="treeitem" aria-level="' + (depth + 1) + '"' + (hasKids ? ' aria-expanded="' + open + '"' : "") + ' data-id="' + id + '">' +
					'<span class="p" style="padding-left:' + (8 + depth * 16) + 'px" title="' + esc(c.paths[node[0]]) + '"><span class="tog">' + (hasKids ? (open ? "▾" : "▸") : "") + "</span>" + catBadge(c, c.cats[node[0]]) + esc(c.paths[node[0]]) + "</span>" +
					'<span class="num incl"><span class="bar" style="width:' + ((node[1] / rootIncl) * 100).toFixed(1) + '%"></span><span>' + ms(node[1]) + "</span></span>" +
					'<span class="num"><span>' + ms(node[2]) + '</span></span><span class="num wctr"><span>' + ms(node[3]) + "</span></span></div>");
				if (!open) return;
				kids.forEach((k, i) => walk(k, id + "." + i, depth + 1));
				if (node[5] > 0) {
					rows.push('<div class="tree-row folded"><span class="p" style="padding-left:' + (24 + (depth + 1) * 16) + 'px">ほか ' + int(node[5]) + " 件 (それぞれ 0.5 ms 未満)</span>" +
						'<span class="num"><span>' + ms(node[6]) + '</span></span><span class="num"></span><span class="num wctr"></span></div>');
				}
			};
			walk(unit.tree, "0", 0);
			pane.innerHTML = rows.join("");
			pane.querySelectorAll(".tree-row[data-id]").forEach((row) =>
				row.addEventListener("click", () => {
					if (!row.hasAttribute("aria-expanded")) return;
					const id = row.dataset.id;
					if (ts.open.has(id)) ts.open.delete(id);
					else ts.open.add(id);
					renderTreePane();
				})
			);
			head.querySelector('[data-act="expand"]').addEventListener("click", () => {
				const all = (node, id) => {
					ts.open.add(id);
					(node[4] || []).forEach((k, i) => all(k, id + "." + i));
				};
				all(unit.tree, "0");
				renderTreePane();
			});
			head.querySelector('[data-act="collapse"]').addEventListener("click", () => {
				ts.open = new Set(["0"]);
				renderTreePane();
			});
		};
		renderList();
		renderTreePane();
	}

	// ------------------------------------------------------------ テンプレート

	function viewTemplates(view, c) {
		if (!c.templates) {
			view.innerHTML = '<div class="card"><div class="empty">この構成ではテンプレートの展開を記録していない (イベントが多く、計測の負荷を抑えるため)。ほかの構成を選ぶ。</div></div>';
			return;
		}
		const fe = (c.totals || {}).fe_us || 1;
		view.innerHTML = '<div class="card">' + toolbar({ placeholder: "テンプレート名で絞り込み" }) + '<div id="tbl"></div></div>';
		const host = view.querySelector("#tbl");
		let maxI = 1;
		const columns = [
			{ key: "name", label: "テンプレート", sort: (r) => r.n, cell: (r) => esc(r.n) },
			{ key: "kind", label: "種類", sort: (r) => r.k, cell: (r) => esc(r.k) },
			{ key: "incl", label: "合計", num: true, title: "展開にかかった時間の合計 (同じテンプレートの中で再帰した分は数えない)", sort: (r) => r.i, cell: (r) => ms(r.i), bar: (r) => r.i / maxI },
			{ key: "share", label: "FE 比", num: true, sort: (r) => r.i, cell: (r) => pct(r.i, fe) },
			{ key: "excl", label: "自身", num: true, title: "中で展開した別のテンプレートを除いた時間", sort: (r) => r.e, cell: (r) => ms(r.e) },
			{ key: "count", label: "回数", num: true, sort: (r) => r.c, cell: (r) => int(r.c) },
			{ key: "units", label: "翻訳単位", num: true, sort: (r) => r.u, cell: (r) => int(r.u) },
			{ key: "specs", label: "特殊化", num: true, title: "型引数の組み合わせの数", sort: (r) => r.sc, cell: (r) => int(r.sc) },
		];
		const detail = (r) => {
			const specs = r.s.map((s) => '<tr><td class="name">' + esc(s[0]) + '</td><td class="num">' + ms(s[1]) + '</td><td class="num">' + int(s[2]) + " 回</td></tr>").join("");
			const files = r.f.map((f) => '<tr><td class="name">' + catBadge(c, c.cats[f[0]]) + esc(c.paths[f[0]]) + '</td><td class="num">' + ms(f[1]) + "</td></tr>").join("");
			return '<div class="detail-grid"><div><h4>重い特殊化 (上位 ' + r.s.length + " / " + int(r.sc) + ')</h4><table class="mini">' + (specs || '<tr><td class="muted">無し</td></tr>') + "</table></div>" +
				'<div><h4>展開した場所 (解析中だったファイル)</h4><table class="mini">' + (files || '<tr><td class="muted">無し</td></tr>') + "</table>" +
				'<p class="muted">関数テンプレートの本体は翻訳単位の最後にまとめて展開されることが多く、そのときは翻訳単位のソースが場所になる。</p></div></div>';
		};
		const refresh = () => {
			const q = state.q.toLowerCase();
			const rows = c.templates_list.filter((r) => matches(r.n, q));
			maxI = rows.reduce((m, r) => Math.max(m, r.i), 1);
			renderTable(host, "templates", rows, columns, { sort: "incl", id: (r) => r.n, detail: detail });
			setInfo(view, int(rows.length) + " / " + int(c.templates_list.length) + " 件 · 展開 " + int((c.totals || {}).template_instantiations) + " 回");
		};
		bindSearch(view, refresh);
		refresh();
	}

	// ------------------------------------------------------------ 関数

	function viewFunctions(view, c) {
		view.innerHTML = '<div class="card">' + toolbar({ placeholder: "関数名で絞り込み" }) + '<div id="tbl"></div></div>';
		const host = view.querySelector("#tbl");
		let maxT = 1;
		const columns = [
			{ key: "name", label: "関数", sort: (r) => r.n, cell: (r) => esc(r.n) },
			{ key: "total", label: "合計", num: true, title: "コード生成 (最適化を含む) の時間の合計。同じ関数が複数の翻訳単位で生成されればその和", sort: (r) => r.t, cell: (r) => ms(r.t), bar: (r) => r.t / maxT },
			{ key: "wctr", label: "按分", num: true, sort: (r) => r.w, cell: (r) => ms(r.w) },
			{ key: "count", label: "回数", num: true, sort: (r) => r.c, cell: (r) => int(r.c) },
			{ key: "max", label: "最大", num: true, sort: (r) => r.m, cell: (r) => ms(r.m) },
			{ key: "where", label: "場所", sort: (r) => r.at, cell: (r) => esc(WHERE_LABEL[r.at] || r.at) },
			{ key: "inl", label: "強制インライン", num: true, title: "__forceinline で展開された関数の数と、その大きさの合計", sort: (r) => r.ic, cell: (r) => (r.ic ? int(r.ic) + " 件 / " + int(r.is) : "-") },
		];
		const detail = (r) => {
			const inl = r.ti.map((x) => '<tr><td class="name">' + esc(x[0]) + '</td><td class="num">' + int(x[1]) + "</td></tr>").join("");
			return '<div class="detail-grid"><div><h4>装飾名</h4><code>' + esc(r.d) + "</code></div>" +
				'<div><h4>大きい強制インライン (上位)</h4><table class="mini">' + (inl || '<tr><td class="muted">無し</td></tr>') + "</table></div></div>";
		};
		const refresh = () => {
			const q = state.q.toLowerCase();
			const rows = c.functions.filter((r) => matches(r.n, q) || matches(r.d, q));
			maxT = rows.reduce((m, r) => Math.max(m, r.t), 1);
			renderTable(host, "functions", rows, columns, { sort: "total", id: (r) => r.n, detail: detail });
			setInfo(view, int(rows.length) + " / " + int(c.functions.length) + " 件 (上位だけ記録)");
		};
		bindSearch(view, refresh);
		refresh();
	}

	// ------------------------------------------------------------ 翻訳単位

	function viewUnits(view, c) {
		view.innerHTML = '<div class="card">' + toolbar({ placeholder: "ソースのパスで絞り込み" }) + '<div id="tbl"></div></div>';
		const host = view.querySelector("#tbl");
		let maxT = 1;
		const total = (u) => u.fe + Math.max(0, u.be);
		const columns = [
			{ key: "src", label: "翻訳単位", sort: (r) => r.src, cell: (r) => esc(r.src) + (r.pch ? '<span class="badge pch">PCH を作る</span>' : "") },
			{ key: "total", label: "合計", num: true, sort: total, cell: (r) => ms(total(r)), bar: (r) => total(r) / maxT },
			{ key: "fe", label: "フロントエンド", num: true, sort: (r) => r.fe, cell: (r) => ms(r.fe) },
			{ key: "be", label: "バックエンド", num: true, title: "/GL の構成では中間表現を書くだけで、コード生成はリンク時に行う", sort: (r) => r.be, cell: (r) => ms(r.be) },
			{ key: "few", label: "按分 (FE)", num: true, sort: (r) => r.fe_w, cell: (r) => ms(r.fe_w) },
			{ key: "parses", label: "解析ファイル", num: true, sort: (r) => r.parses, cell: (r) => int(r.parses) },
		];
		const refresh = () => {
			const q = state.q.toLowerCase();
			const rows = c.units.filter((r) => matches(r.src, q));
			maxT = rows.reduce((m, r) => Math.max(m, total(r)), 1);
			renderTable(host, "units", rows, columns, { sort: "total", id: (r) => r.src, onClick: (id) => go({ tab: "tree", unit: id, q: "", hl: null }) });
			setInfo(view, int(rows.length) + " 件 · 行を押すとインクルードツリーを開く");
		};
		bindSearch(view, refresh);
		refresh();
	}

	// ------------------------------------------------------------ タイムライン

	function viewTimeline(view, c) {
		const ts = tabState("timeline", () => ({ zoom: 1 }));
		const t0 = c.build_start_us || 0;
		const items = [];
		c.units.forEach((u) => {
			const be = Math.max(0, u.be);
			items.push({ kind: "unit", start: u.start - t0, fe: u.fe, be: be, end: u.start - t0 + u.fe + be, label: u.src, pch: u.pch });
		});
		c.invocations.filter((inv) => inv[0] === "link").forEach((inv) => {
			items.push({ kind: "link", start: inv[2] - t0, end: inv[2] - t0 + inv[3], dur: inv[3], label: (inv[5] && inv[5][0]) || "link #" + inv[1] });
		});
		items.sort((a, b) => a.start - b.start);
		// 重ならないように段へ詰める
		const lanes = [];
		items.forEach((it) => {
			let lane = lanes.findIndex((end) => end <= it.start);
			if (lane < 0) {
				lane = lanes.length;
				lanes.push(0);
			}
			lanes[lane] = it.end;
			it.lane = lane;
		});
		const span = Math.max(1, items.reduce((m, it) => Math.max(m, it.end), 0));
		const width = Math.max(600, (view.clientWidth || 1100) - 40) * ts.zoom;
		const laneH = 16;
		const top = 22;
		const height = top + lanes.length * laneH + 8;
		const x = (us) => (us / span) * (width - 8);
		const svg = [];
		svg.push('<svg width="' + width + '" height="' + height + '" role="img" aria-label="ビルドのタイムライン">');
		svg.push('<g class="tl-axis">');
		const step = niceStep(span / 8);
		for (let v = 0; v <= span; v += step) {
			svg.push('<line x1="' + x(v) + '" x2="' + x(v) + '" y1="16" y2="' + height + '"/><text x="' + (x(v) + 3) + '" y="12">' + esc(ms(v)) + "</text>");
		}
		svg.push("</g>");
		items.forEach((it, i) => {
			const y = top + it.lane * laneH;
			if (it.kind === "unit") {
				svg.push('<rect class="tl-bar tl-fe" data-i="' + i + '" x="' + x(it.start) + '" y="' + y + '" width="' + Math.max(1, x(it.fe)) + '" height="' + (laneH - 3) + '" rx="2"/>');
				if (it.be > 0) svg.push('<rect class="tl-bar tl-be" data-i="' + i + '" x="' + x(it.start + it.fe) + '" y="' + y + '" width="' + Math.max(1, x(it.be)) + '" height="' + (laneH - 3) + '" rx="2"/>');
			} else {
				svg.push('<rect class="tl-bar tl-link" data-i="' + i + '" x="' + x(it.start) + '" y="' + y + '" width="' + Math.max(1, x(it.dur)) + '" height="' + (laneH - 3) + '" rx="2"/>');
			}
		});
		svg.push("</svg>");
		view.innerHTML = '<div class="card"><div class="toolbar"><div class="legend"><span style="--c:var(--fe)">フロントエンド</span><span style="--c:var(--be)">バックエンド</span><span style="--c:var(--link)">リンク</span></div>' +
			'<div class="seg" role="group" aria-label="拡大">' + [1, 2, 4, 8].map((z) => '<button type="button" data-z="' + z + '" aria-pressed="' + (ts.zoom === z) + '">×' + z + "</button>").join("") + "</div>" +
			'<span class="info">並列 ' + lanes.length + " 段 · 実時間 " + ms(span) + " · バックエンドはフロントエンドの直後に並べて描いている</span></div>" +
			'<div class="timeline">' + (items.length ? svg.join("") : '<div class="empty">データが無い</div>') + "</div></div>";
		view.querySelectorAll(".seg button").forEach((b) =>
			b.addEventListener("click", () => {
				ts.zoom = +b.dataset.z;
				viewTimeline(view, c);
			})
		);
		const tip = document.getElementById("tooltip");
		view.querySelectorAll(".tl-bar").forEach((r) => {
			r.addEventListener("mousemove", (e) => {
				const it = items[+r.dataset.i];
				tip.innerHTML = '<div class="mono">' + esc(it.label) + "</div>" +
					(it.kind === "unit" ? "フロントエンド " + ms(it.fe) + " · バックエンド " + ms(it.be) + (it.pch ? " · PCH を作る" : "") : "リンク " + ms(it.dur)) +
					'<div class="muted">開始 ' + ms(it.start) + "</div>";
				tip.hidden = false;
				tip.style.left = Math.min(window.innerWidth - tip.offsetWidth - 8, e.clientX + 14) + "px";
				tip.style.top = e.clientY + 14 + "px";
			});
			r.addEventListener("mouseleave", () => (tip.hidden = true));
			r.addEventListener("click", () => {
				const it = items[+r.dataset.i];
				if (it.kind === "unit") go({ tab: "tree", unit: it.label, q: "", hl: null });
			});
		});
	}

	function niceStep(raw) {
		const pow = Math.pow(10, Math.floor(Math.log10(Math.max(1, raw))));
		const n = raw / pow;
		return (n <= 1 ? 1 : n <= 2 ? 2 : n <= 5 ? 5 : 10) * pow;
	}

	// ------------------------------------------------------------ 差分

	function viewDiff(view, c) {
		const d = diff();
		if (!d) {
			view.innerHTML = '<div class="card"><div class="empty">比較元 (前回の master の計測) が無い。master に計測が溜まると、ここに前回との違いが出る。</div></div>';
			return;
		}
		const base = DATA.base || {};
		const labels = {
			wall_us: ["ビルドの実時間", ms],
			fe_us: ["フロントエンド合計", ms],
			be_us: ["バックエンド合計", ms],
			template_instantiations: ["テンプレート展開", int],
			functions: ["コード生成した関数", int],
			file_parses: ["解析したファイル", int],
			passes: ["翻訳単位", int],
		};
		const totalRows = Object.keys(labels).filter((k) => d.totals[k]).map((k) => {
			const [b, cur] = d.totals[k];
			return "<tr><td>" + labels[k][0] + '</td><td class="num">' + labels[k][1](b) + '</td><td class="num">' + labels[k][1](cur) + '</td><td class="num ' + changeClass(b, cur) + '">' + change(b, cur) + "</td></tr>";
		}).join("");
		const section = (title, rows, fmtName) => {
			if (!rows.length) return '<div class="card"><div class="card-head"><h3>' + esc(title) + '</h3><span class="hint">目立った変化なし</span></div></div>';
			const body = rows.map((r) =>
				'<tr><td class="name">' + fmtName(r[0]) + '<span class="badge ' + r[5] + '">' + { up: "増", down: "減", new: "新規", gone: "消えた" }[r[5]] + "</span></td>" +
				'<td class="num">' + ms(r[1]) + '</td><td class="num">' + ms(r[2]) + '</td><td class="num ' + changeClass(r[1], r[2]) + '">' + change(r[1], r[2]) + "</td>" +
				'<td class="num">' + r[3].toFixed(1) + "% → " + r[4].toFixed(1) + '%</td><td class="num ' + (r[4] > r[3] ? "delta-up" : "delta-down") + '">' + (r[4] - r[3] >= 0 ? "+" : "") + (r[4] - r[3]).toFixed(2) + " pt</td></tr>"
			).join("");
			return '<div class="card"><div class="card-head"><h3>' + esc(title) + '</h3></div><div class="table-wrap"><table class="data"><thead><tr><th>名前</th><th class="num">前回</th><th class="num">今回</th><th class="num">変化</th><th class="num">全体に占める割合</th><th class="num">割合の変化</th></tr></thead><tbody>' + body + "</tbody></table></div></div>";
		};
		const baseLink = base.sha ? (base.run_url ? '<a href="' + esc(base.run_url) + '">' + esc(base.sha.slice(0, 7)) + "</a>" : esc(base.sha.slice(0, 7))) : "前回の master";
		view.innerHTML =
			(d.reliable ? "" : '<div class="notice">どちらかの計測でイベントが欠けているので、下の比較は当てにならない。</div>') +
			'<div class="notice info">比較元: ' + baseLink + "。共有ランナーの実行時間は ±10% 程度揺れるので、時間だけでなく「全体に占める割合」の変化も見る。割合は 0.3 pt 以上、時間は 20 ms かつ 15% 以上動いたものだけを出している。</div>" +
			'<div class="card"><div class="card-head"><h3>全体</h3></div><div class="table-wrap"><table class="data"><thead><tr><th>項目</th><th class="num">前回</th><th class="num">今回</th><th class="num">変化</th></tr></thead><tbody>' + totalRows + "</tbody></table></div></div>" +
			section("ヘッダ", d.headers, (n) => esc(n)) +
			(c.templates ? section("テンプレート", d.templates, (n) => esc(n)) : "") +
			section("関数のコード生成", d.functions, (n) => esc(n)) +
			section("翻訳単位", d.units, (n) => esc(n));
	}

	// ------------------------------------------------------------ 枠

	function renderChrome() {
		const cm = DATA.commit || {};
		const info = document.getElementById("commit-info");
		const parts = [];
		if (cm.sha) parts.push(cm.commit_url ? '<a class="mono" href="' + esc(cm.commit_url) + '">' + esc(cm.sha.slice(0, 7)) + "</a>" : '<span class="mono">' + esc(cm.sha.slice(0, 7)) + "</span>");
		if (cm.ref) parts.push("<span>" + esc(cm.ref) + "</span>");
		if (cm.subject) parts.push('<span class="subject" title="' + esc(cm.subject) + '">' + esc(cm.subject) + "</span>");
		if (cm.run_url) parts.push('<a href="' + esc(cm.run_url) + '">CI の実行 ↗</a>');
		info.innerHTML = parts.join("");

		const picker = document.getElementById("config-picker");
		picker.innerHTML = DATA.configs.map((c) =>
			'<button type="button" role="radio" data-cfg="' + esc(c.name) + '" aria-checked="' + (c.name === state.cfg) + '"' +
			(c.partial ? ' title="イベントが欠けている"' : "") + ">" + esc(c.name) + (c.partial ? '<span class="dot"></span>' : "") + "</button>"
		).join("");
		picker.querySelectorAll("button").forEach((b) => b.addEventListener("click", () => go({ cfg: b.dataset.cfg })));

		const c = cfg();
		document.querySelectorAll(".tab").forEach((a) => {
			const tab = a.dataset.tab;
			a.href = "#" + hashFor({ tab: tab, cfg: state.cfg });
			if (tab === state.tab) a.setAttribute("aria-current", "page");
			else a.removeAttribute("aria-current");
		});

		const notices = [];
		(DATA.missing || []).forEach((m) => notices.push('<div class="notice">' + esc(m) + " の計測結果が無い (ビルドジョブのログを参照)。</div>"));
		if (c && c.partial) notices.push('<div class="notice">この構成ではイベントが ' + int(c.lost) + " 件欠けた。欠けた箇所では時間が実際より長く出ることがある。</div>");
		document.getElementById("notices").innerHTML = notices.join("");

		document.getElementById("footer").innerHTML =
			"<p><b>合計</b>: 全翻訳単位での時間の和。並列でビルドしているので実時間より大きくなる。<b>按分</b>: 同時に走っていたもの同士で実時間を分けた値で、ビルドの実時間への寄与を表す。" +
			"<b>自身</b>: 中で取り込んだヘッダや、中で展開した別のテンプレートを除いた時間。</p>" +
			"<p>PCH に入っているヘッダは PCH を作る翻訳単位でだけ解析され、ほかの翻訳単位では数えない。インクルードツリーは 0.5 ms 未満のファイルを「ほか N 件」に畳んでいる。</p>" +
			"<p>手元で同じレポートを作る方法は tools/build-insights/README.md。生成 " + esc(DATA.generated || "") + "</p>";
	}

	function render() {
		readHash();
		renderChrome();
		const view = document.getElementById("view");
		const c = cfg();
		document.getElementById("tooltip").hidden = true;
		if (!c) {
			view.innerHTML = '<div class="card"><div class="empty">計測結果が無い。</div></div>';
			return;
		}
		const views = {
			overview: viewOverview,
			headers: viewHeaders,
			tree: viewTree,
			templates: viewTemplates,
			functions: viewFunctions,
			units: viewUnits,
			timeline: viewTimeline,
			diff: viewDiff,
		};
		(views[state.tab] || viewOverview)(view, c);
	}

	// ------------------------------------------------------------ テーマ

	function applyTheme(choice) {
		const root = document.documentElement;
		try {
			if (choice === "light" || choice === "dark") {
				root.dataset.theme = choice;
				localStorage.setItem(THEME_KEY, choice);
			} else {
				delete root.dataset.theme;
				localStorage.removeItem(THEME_KEY);
			}
		} catch (e) {
			// 保存できなくても表示は切り替える
		}
		document.querySelectorAll("[data-theme-choice]").forEach((b) =>
			b.setAttribute("aria-checked", b.dataset.themeChoice === (root.dataset.theme || "auto"))
		);
	}

	document.querySelectorAll("[data-theme-choice]").forEach((b) => b.addEventListener("click", () => applyTheme(b.dataset.themeChoice)));
	applyTheme(document.documentElement.dataset.theme || "auto");
	window.addEventListener("hashchange", render);
	render();
})();
