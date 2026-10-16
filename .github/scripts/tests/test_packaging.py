"""使用真实 CMake/CPack 验证发布结构；仅用替身提供 Windows DLL 导入表。"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import zipfile

REPOSITORY = Path(__file__).resolve().parents[3]


class PackagingTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="apk packaging ")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / "source"
        self.build = self.root / "build"
        self.qt = self.root / "Qt kit"
        self.crt = self.root / "redist/x64/Microsoft.VC143.CRT"
        self.system = self.root / "Windows/System32"
        self.components = self.root / "components"
        for name in ("LICENSE", "docs/third-party/THIRD_PARTY_NOTICES.md", "ZzThirdParty/ZzLog/LICENSE",
                     "ZzThirdParty/ZzLog/licenses/dependency.txt", "ZzThirdParty/qwindowkit/LICENSE",
                     "ZzThirdParty/qwindowkit/docs/framelesshelper-related.md"):
            notice = self.components / name
            notice.parent.mkdir(parents=True, exist_ok=True)
            notice.write_text("license fixture")
        for directory in (self.source / "tools", self.source / "licenses", self.qt / "bin",
                          self.qt / "plugins/platforms", self.qt / "plugins/imageformats", self.crt, self.system):
            directory.mkdir(parents=True)
        shutil.copytree(REPOSITORY / "cmake", self.source / "cmake")
        for name in ("aapt.exe", "aapt2.exe", "NOTICE.txt"):
            (self.source / "tools" / name).write_text(name)
        (self.source / "README.md").write_text("APK information")
        (self.system / "KERNEL32.dll").write_text("system library")
        for name in ("Qt6Core.dll", "Qt6Gui.dll", "Qt6Widgets.dll", "Qt6Svg.dll", "Qt6Quick.dll", "Qt6Network.dll"):
            (self.qt / "bin" / name).write_text(name)
        for name in ("msvcp140.dll", "vcruntime140.dll", "vcruntime140_1.dll", "msvcp140_atomic_wait.dll", "unused.dll"):
            (self.crt / name).write_text(name)
        for name in ("platforms/qwindows.dll", "imageformats/qico.dll", "imageformats/qjpeg.dll"):
            (self.qt / "plugins" / name).write_text(name)
        self.imports = {
            "ZzApkInfo.exe": ["Qt6Widgets.dll", "Qt6Svg.dll", "msvcp140.dll", "vcruntime140.dll", "KERNEL32.dll"],
            "Qt6Widgets.dll": ["Qt6Gui.dll"], "Qt6Gui.dll": ["Qt6Core.dll"],
            "Qt6Svg.dll": ["Qt6Gui.dll"], "Qt6Core.dll": ["msvcp140_atomic_wait.dll", "api-ms-win-core-file-l1-1-0.dll"],
            "msvcp140.dll": ["vcruntime140_1.dll"], "vcruntime140_1.dll": ["vcruntime140.dll"],
            "qwindows.dll": ["Qt6Gui.dll"], "qico.dll": ["Qt6Gui.dll"],
        }
        self.manifest = self.root / "imports.json"
        self.manifest.write_text(json.dumps(self.imports))
        self.dumpbin = self.root / "dumpbin"
        self.dumpbin.write_text("#!/usr/bin/env python3\nimport json, pathlib, sys\n"
                               f"imports = json.loads(pathlib.Path({str(self.manifest)!r}).read_text())\n"
                               "for name in imports.get(pathlib.Path(sys.argv[-1]).name, []): print('    ' + name)\n")
        self.dumpbin.chmod(0o755)
        qmake = self.qt / "bin/qmake"
        qmake.write_text(f"#!/bin/sh\nprintf '%s\\n' '{self.qt / 'plugins'}'\n")
        qmake.chmod(0o755)
        # 测试真实项目的 CPack 配置，避免测试里复制出另一套部署逻辑。
        project = (REPOSITORY / "CMakeLists.txt").read_text()
        start = project.index("        get_target_property(qmakePath")
        finish = project.index("\n    endif()\n    if(BUILD_TESTING)", start)
        cmake = f'''cmake_minimum_required(VERSION 3.25)
project(ZzApkInfo VERSION 1.2.3 LANGUAGES CXX)
add_executable(ZzApkInfo main.cpp)
set_target_properties(ZzApkInfo PROPERTIES SUFFIX .exe)
add_executable(Qt6::qmake IMPORTED)
set_target_properties(Qt6::qmake PROPERTIES IMPORTED_LOCATION "{qmake}")
set(ZZAPK_DUMPBIN "{self.dumpbin}" CACHE FILEPATH "")
set(ZZAPK_ZZPURETOOLS_SOURCE_DIR "{self.components}")
set(ZZAPK_SOURCE_REVISION test-revision)
install(FILES main.cpp DESTINATION include COMPONENT Development)
'''
        (self.source / "CMakeLists.txt").write_text(cmake + project[start:finish])
        (self.source / "main.cpp").write_text("int main() { return 0; }\n")
        self.environment = dict(os.environ, VCToolsRedistDir=str(self.root / "redist"), SystemRoot=str(self.root / "Windows"))

    def command(self, *arguments, success=True):
        result = subprocess.run(arguments, env=self.environment, capture_output=True, text=True)
        if success:
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout + result.stderr

    def configure(self, configuration="Release"):
        self.command("cmake", "-S", str(self.source), "-B", str(self.build), f"-DCMAKE_BUILD_TYPE={configuration}")

    def test_cpack_output_and_dependency_closure(self):
        self.configure()
        self.command("cmake", "--build", str(self.build), "--target", "package-windows")
        name = "ZzApkInfo-1.2.3-windows-x64"
        output = self.source / "dist"
        archive = output / f"{name}.zip"
        with zipfile.ZipFile(archive) as package:
            files = set(package.namelist())
        for file in ("ZzApkInfo.exe", "platforms/qwindows.dll", "imageformats/qico.dll", "Qt6Core.dll", "Qt6Svg.dll",
                     "msvcp140.dll", "msvcp140_atomic_wait.dll", "vcruntime140_1.dll", "vcruntime140.dll", "qt.conf",
                     "licenses/QWindowKit/LICENSE", "licenses/ZzPureTools/LICENSE"):
            self.assertIn(f"{name}/{file}", files)
            self.assertTrue((output / name / file).is_file())
        for unused in ("Qt6Quick.dll", "Qt6Network.dll", "qjpeg.dll", "unused.dll", "KERNEL32.dll", "main.cpp"):
            self.assertFalse(any(path.endswith('/' + unused) for path in files), unused)
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        self.assertEqual((output / f"{name}.zip.sha256").read_text(), f"{digest}  {name}.zip\n")
        # 单独调用 cpack 同样可用，不依赖 PowerShell 包装脚本。
        self.command("cpack", "--config", str(self.build / "CPackConfig.cmake"), "-C", "Release")

    def test_missing_runtime_stops_packaging(self):
        self.configure()
        (self.crt / "vcruntime140_1.dll").unlink()
        log = self.command("cmake", "--build", str(self.build), "--target", "package-windows", success=False)
        self.assertIn("Required MSVC runtime is missing", log)
        self.assertFalse(list((self.source / "dist").glob("*.zip")))

    def test_scanner_failure_stops_packaging(self):
        self.configure()
        self.dumpbin.write_text("#!/bin/sh\nexit 1\n")
        log = self.command("cmake", "--build", str(self.build), "--target", "package-windows", success=False)
        self.assertIn("Cannot inspect", log)

    def test_debug_binary_is_rejected(self):
        self.configure("Debug")
        log = self.command("cmake", "--build", str(self.build), "--target", "package-windows", success=False)
        self.assertIn("requires a Release build", log)


if __name__ == "__main__":
    unittest.main()
