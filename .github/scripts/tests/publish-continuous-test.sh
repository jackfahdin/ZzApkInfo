#!/usr/bin/env bash
# GitHub 是唯一替身；实际运行发布脚本并校验文件、摘要和远端写入边界。
set -euo pipefail
script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
test_root=$(mktemp -d)
trap 'rm -rf "$test_root"' EXIT

gh() {
    printf '%s\n' "$*" >> "$MOCK_LOG"
    case "$*" in
        'api repos/test/repo/git/ref/heads/master --jq .object.sha')
            [[ "$TEST_CASE" != api-error ]] || return 1
            if [[ "$TEST_CASE" == stale ]]; then
                echo newer
            elif [[ "$TEST_CASE" == late-stale && -f "$MOCK_CHECKED" ]]; then
                echo newer
            else
                touch "$MOCK_CHECKED"
                echo abc123
            fi ;;
        'release view continuous-build')
            if [[ "$TEST_CASE" == fresh || "$TEST_CASE" == race ]]; then
                test -f "$MOCK_STATE"
            fi ;;
        'release create continuous-build '*)
            touch "$MOCK_STATE"
            [[ "$TEST_CASE" != race ]] ;;
        'release view continuous-build --json assets --jq .assets[].name')
            for item in "$MOCK_REMOTE"/*; do
                [[ -f "$item" ]] && basename "$item"
            done
            return 0 ;;
        'release download continuous-build '*)
            # 模拟只下载明确指定的附件。
            cp "$MOCK_REMOTE/$5" "$7/" ;;
        'release delete-asset continuous-build '*)
            rm -f "$MOCK_REMOTE/$4" ;;
        'release upload continuous-build '*)
            shift 3
            local item
            for item in "$@"; do
                [[ "$item" != --clobber ]] || continue
                if [[ "$item" == release-output/* && "$TEST_CASE" == upload-error ]]; then return 1; fi
                cp "$item" "$MOCK_REMOTE/"
                if [[ "$item" == release-output/*.zip && "$TEST_CASE" == partial-upload ]]; then return 1; fi
            done ;;
        'api --method PATCH repos/test/repo/git/refs/tags/continuous-build '*|'release edit continuous-build '*) ;;
        *) echo "未预期的 gh 调用：$*" >&2; return 1 ;;
    esac
}
export -f gh
export GH_REPO=test/repo GITHUB_SHA=abc123 GITHUB_REF_NAME=master
export GITHUB_REF_TYPE=branch GITHUB_EVENT_NAME=push RELEASE_VERSION=1.0.0

for scenario in normal crlf fresh race stale late-stale api-error missing corrupt invalid-version pr manual feature-branch upload-error partial-upload; do
    (
        export TEST_CASE="$scenario"
        export MOCK_LOG="$test_root/$scenario.log" MOCK_STATE="$test_root/$scenario.state"
        export MOCK_CHECKED="$test_root/$scenario.checked"
        export MOCK_REMOTE="$test_root/$scenario.remote"
        mkdir -p "$MOCK_REMOTE"
        if [[ "$scenario" != fresh && "$scenario" != race ]]; then
            printf 'old package' > "$MOCK_REMOTE/ZzApkInfo-continuous-windows-x64.zip"
            (cd "$MOCK_REMOTE" && sha256sum ZzApkInfo-continuous-windows-x64.zip > ZzApkInfo-continuous-windows-x64.zip.sha256)
        fi
        touch "$MOCK_LOG"
        mkdir -p "$test_root/$scenario/release-input"
        cd "$test_root/$scenario"
        package=release-input/ZzApkInfo-1.0.0-windows-x64.zip
        printf 'package contents\n' > "$package"
        (cd release-input && sha256sum ZzApkInfo-1.0.0-windows-x64.zip > ZzApkInfo-1.0.0-windows-x64.zip.sha256)
        case "$scenario" in
            crlf) sed -i 's/$/\r/' "$package.sha256" ;;
            missing) rm "$package" ;;
            corrupt) printf 'changed' >> "$package" ;;
            invalid-version) export RELEASE_VERSION=invalid ;;
            pr) export GITHUB_EVENT_NAME=pull_request ;;
            manual) export GITHUB_EVENT_NAME=workflow_dispatch ;;
            feature-branch) export GITHUB_REF_NAME=feature ;;
        esac
        result=0
        bash "$script_dir/publish-continuous.sh" > output.log 2>&1 || result=$?
        case "$scenario" in
            normal|crlf|fresh|race)
                test "$result" -eq 0
                grep -q '^release upload continuous-build .*ZzApkInfo-continuous-windows-x64.zip' "$MOCK_LOG"
                grep -q '^api --method PATCH .*continuous-build' "$MOCK_LOG"
                cmp "$package" release-output/ZzApkInfo-continuous-windows-x64.zip
                (cd release-output && sha256sum -c ZzApkInfo-continuous-windows-x64.zip.sha256)
                if [[ "$scenario" == fresh || "$scenario" == race ]]; then
                    grep -q '^release create continuous-build .*--prerelease --latest=false' "$MOCK_LOG"
                fi ;;
            stale|late-stale)
                test "$result" -eq 0
                if grep -Eq '^release (create|edit|upload)|^api --method' "$MOCK_LOG"; then exit 1; fi ;;
            *)
                test "$result" -ne 0
                if [[ "$scenario" == upload-error || "$scenario" == partial-upload ]]; then
                    if grep -Eq '^release edit|^api --method PATCH' "$MOCK_LOG"; then exit 1; fi
                    [[ "$(cat "$MOCK_REMOTE/ZzApkInfo-continuous-windows-x64.zip")" == 'old package' ]]
                    (cd "$MOCK_REMOTE" && sha256sum -c ZzApkInfo-continuous-windows-x64.zip.sha256)
                fi
                if [[ "$scenario" != upload-error && "$scenario" != partial-upload ]]; then
                    if grep -Eq '^release (create|edit|upload)|^api --method' "$MOCK_LOG"; then exit 1; fi
                fi ;;
        esac
        echo "通过：$scenario"
    )
done
