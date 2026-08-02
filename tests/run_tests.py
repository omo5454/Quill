#!/usr/bin/env python3
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BIN_INTERPRETER = ROOT / "bin" / "quill-c"
BIN_TRANSPILER = ROOT / "bin" / "quill"

if not BIN_INTERPRETER.exists() or not BIN_TRANSPILER.exists():
    subprocess.run(["bash", str(ROOT / "build.sh")], check=True)

sys.path.insert(0, str(ROOT / "tests"))
from test_cases import CASES


def run_and_capture(command):
    result = subprocess.run(command, capture_output=True, text=True)
    return result.returncode, result.stdout.strip(), result.stderr.strip()


def run_interpreter(src_path):
    return run_and_capture([str(BIN_INTERPRETER), str(src_path)])


def run_transpiler(src_path, out_dir):
    c_file = out_dir / "generated.c"
    compile_cmd = [str(BIN_TRANSPILER), "--compile", "-o", str(c_file), str(src_path)]
    compile_rc, compile_out, compile_err = run_and_capture(compile_cmd)
    if compile_rc != 0:
        return compile_rc, compile_out, compile_err, None

    binary = c_file.with_suffix("")
    if binary.exists():
        run_rc, run_out, run_err = run_and_capture([str(binary)])
        return run_rc, run_out, run_err, str(binary)

    return compile_rc, compile_out, compile_err, None


def normalize_output(value):
    return value.replace("\r\n", "\n").strip()


def main():
    total = 0
    passed = 0
    failed = 0

    with tempfile.TemporaryDirectory(prefix="quill-tests-") as tmpdir:
        tmpdir = Path(tmpdir)
        for case in CASES:
            total += 1
            source_path = tmpdir / f"{case['id']}.qsc"
            source_path.write_text(case["source"], encoding="utf-8")

            interp_rc, interp_out, interp_err = run_interpreter(source_path)
            transp_rc, transp_out, transp_err, transp_binary = run_transpiler(source_path, tmpdir)

            interp_norm = normalize_output(interp_out)
            transp_norm = normalize_output(transp_out)
            expected_norm = normalize_output(case["expected"])

            mismatch = (
                interp_rc != 0
                or transp_rc != 0
                or interp_norm != expected_norm
                or transp_norm != expected_norm
                or interp_norm != transp_norm
            )

            if mismatch:
                failed += 1
                print(f"FAIL {case['id']}: {case['name']}")
                print(f"  expected: {expected_norm!r}")
                print(f"  interpreter: rc={interp_rc} out={interp_norm!r} err={interp_err!r}")
                print(f"  transpiler: rc={transp_rc} out={transp_norm!r} err={transp_err!r}")
                if transp_binary:
                    print(f"  binary: {transp_binary}")
                print()
            else:
                passed += 1
                print(f"PASS {case['id']}: {case['name']}")

    print(f"Summary: {passed}/{total} passed, {failed} failed")
    if failed:
        print("Any mismatch between interpreter and transpiler is treated as a failure.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
