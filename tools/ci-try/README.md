# ci-try: 作業中の変更を CI で確かめる

手元でビルドやテストを走らせずに、いまの作業ツリーが CI に通るかを確かめるツールです。
未コミットの変更も含めてそのまま GitHub に上げ、CI (`.github/workflows/ci.yml`) の結果を待ちます。

## 何をするか

1. 作業ツリーのスナップショットを 1 コミットにする (未コミットの変更・削除・未追跡のファイルを含む。`.gitignore` で除外されたものは含まない)。
2. そのコミットを試し用のブランチ `user/ci-try` へ force-push する。**master には push しない。**
3. CI の実行を見つけて URL を表示し、終わるまで待って合否を出す。

手元の HEAD・index・作業ツリー・stash は変えません (一時的な index で組み立てるため)。
コミットしたり stash したりする必要はなく、ブランチも切り替えません。

CI では master の push と同じく、6 構成のビルド・GoogleTest・`runtime.exe` の起動確認・Editor・ベンチマークが走ります。
ベンチマークの結果は master の履歴には積まれず、実行ページのアーティファクト `bench-site` にプレビューとして置かれます。

## 使い方

ダブルクリックで使うときは `tools/ci-try/ci-try.bat` を開きます。
そのバッチが置かれたチェックアウトの作業ツリーが対象です。

コマンドで使うときは、対象のチェックアウト (ワークツリーでもよい) の中で実行します。

```powershell
pwsh tools/ci-try/ci-try.ps1
```

| オプション | 意味 |
|---|---|
| `-NoWatch` | push して URL を表示したら終わる (結果は待たない) |
| `-NoUntracked` | 未追跡のファイルを含めない |
| `-Branch <名前>` | push 先を変える (既定 `user/ci-try`。master / main は拒否) |

いつもは 11〜14 分かかります。待っている途中で Ctrl+C で抜けても、CI は GitHub 上で続きます。
不合格なら、表示される `gh run view <id> --log-failed` で失敗したジョブのログを読めます。

## 前提

- `git` と、結果を待つなら [GitHub CLI](https://cli.github.com/) (`gh`、`gh auth login` 済み)。
- `origin` へ push できること。

## 注意

- 続けて実行すると `user/ci-try` を上書きし、走っている前の実行は打ち切られます (CI の concurrency)。
- 文書だけの変更 (`*.md` や `docs/` など) では CI のビルドは走らず、「実行が見つからない」で終わります。
- 失敗すると、master と同じく Discord に通知が流れます。
- pre-push フック (外部資料名・秘密情報の検査) を入れていれば、push の前にそれも走ります。
- リポジトリは公開なので、push した内容は誰でも見られます。
