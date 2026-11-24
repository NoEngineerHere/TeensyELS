Import("env")
import pathlib
import subprocess

root = env.subst("$PROJECT_DIR")
try:
    revision = subprocess.check_output(
        ["git", "rev-parse", "--short=12", "HEAD"], cwd=root, text=True,
        stderr=subprocess.DEVNULL).strip()
    dirty = subprocess.check_output(
        ["git", "status", "--porcelain", "--untracked-files=normal"], cwd=root, text=True,
        stderr=subprocess.DEVNULL).strip()
    if dirty:
        revision += "-dirty"
except (OSError, subprocess.CalledProcessError):
    revision = "unknown"
env.Append(CPPDEFINES=[("FIRMWARE_REVISION", env.StringifyMacro(revision))])
build_dir = pathlib.Path(env.subst("$BUILD_DIR"))
build_dir.mkdir(parents=True, exist_ok=True)
(build_dir / "firmware-identity.txt").write_text(revision + "\n", encoding="utf-8")
