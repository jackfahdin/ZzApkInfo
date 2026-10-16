#!/usr/bin/env bash
# 持续构建只更新固定预发布；不会创建或移动正式版本标签。
set -euo pipefail
: "${GH_REPO:?}"
: "${GITHUB_SHA:?}"
: "${GITHUB_REF_NAME:?}"
: "${GITHUB_REF_TYPE:?}"
: "${GITHUB_EVENT_NAME:?}"
: "${RELEASE_VERSION:?}"
[[ "$RELEASE_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
[[ "$GITHUB_EVENT_NAME" == push && "$GITHUB_REF_TYPE" == branch ]]
[[ "$GITHUB_REF_NAME" == master || "$GITHUB_REF_NAME" == main ]]

package_name="ZzApkInfo-$RELEASE_VERSION-windows-x64.zip"
package="release-input/$package_name"
[[ -s "$package" && -f "$package.sha256" ]]
read -r expected_digest recorded_name < "$package.sha256"
# PowerShell 生成的校验文件使用 CRLF，文件名不包含末尾回车。
recorded_name=${recorded_name%$'\r'}
[[ "$expected_digest" =~ ^[a-fA-F0-9]{64}$ && "$recorded_name" == "$package_name" ]]
actual_digest=$(sha256sum "$package")
actual_digest=${actual_digest%% *}
if [[ "${expected_digest,,}" != "$actual_digest" ]]; then
    echo '发布包校验失败，停止发布。' >&2
    exit 1
fi

is_current() {
    local branch_sha
    branch_sha=$(gh api "repos/$GH_REPO/git/ref/heads/$GITHUB_REF_NAME" --jq .object.sha) || return 2
    [[ "$branch_sha" == "$GITHUB_SHA" ]]
}
check_current() {
    local result=0
    is_current || result=$?
    if (( result == 1 )); then
        echo '主分支已有更新，本次仅保留 Actions artifact。'
        exit 0
    elif (( result != 0 )); then
        echo '无法核对主分支提交，停止发布。' >&2
        exit 1
    fi
}
check_current

mkdir -p release-output
asset=ZzApkInfo-continuous-windows-x64.zip
cp "$package" "release-output/$asset"
(cd release-output && sha256sum "$asset" > "$asset.sha256")

tag=continuous-build
exists=false
if gh release view "$tag" >/dev/null 2>&1; then exists=true; fi
# 等待发布队列或准备附件时可能已有新提交，写入前再次确认。
check_current
title='Continuous Build'
notes="Windows x64 Release 持续构建，来源 $GITHUB_REF_NAME，提交 $GITHUB_SHA，版本 $RELEASE_VERSION。附件包含便携 ZIP 和 SHA-256 校验文件。"
if [[ "$exists" == false ]]; then
    if ! gh release create "$tag" --target "$GITHUB_SHA" --title "$title" \
        --notes "$notes" --prerelease --latest=false; then
        # 首次创建竞争允许复用已建立的 release，其他 API 错误仍会失败。
        gh release view "$tag" >/dev/null
    fi
fi
# GitHub 分别上传附件，覆盖前备份旧文件；部分上传失败时恢复配套附件。
mkdir -p release-backup
remote_assets=$(gh release view "$tag" --json assets --jq '.assets[].name')
for name in "$asset" "$asset.sha256"; do
    if grep -Fxq "$name" <<< "$remote_assets"; then
        gh release download "$tag" --pattern "$name" --dir release-backup
    fi
done
if ! gh release upload "$tag" "release-output/$asset" "release-output/$asset.sha256" --clobber; then
    restore_failed=false
    for name in "$asset" "$asset.sha256"; do
        if [[ -f "release-backup/$name" ]]; then
            gh release upload "$tag" "release-backup/$name" --clobber || restore_failed=true
        else
            # 首次发布没有旧附件，仅删除本次可能上传的不完整文件。
            gh release delete-asset "$tag" "$name" --yes || true
        fi
    done
    if [[ "$restore_failed" == true ]]; then
        echo '附件上传失败，且旧附件恢复失败，请检查 continuous-build。' >&2
    else
        echo '附件上传失败，已恢复发布前的附件。' >&2
    fi
    exit 1
fi
gh api --method PATCH "repos/$GH_REPO/git/refs/tags/$tag" \
    -f sha="$GITHUB_SHA" -F force=true >/dev/null
gh release edit "$tag" --title "$title" --notes "$notes" --prerelease --latest=false
