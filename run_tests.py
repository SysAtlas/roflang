#!/usr/bin/python3

import os
import subprocess
import difflib
from pathlib import Path

PROJECT_ROOT_DIR = os.getcwd()
E2E_TL_DIR = f"{PROJECT_ROOT_DIR}/test/e2e"
COMPILER_DIR = f"{PROJECT_ROOT_DIR}/build/bin/roflc" 

TEST_DIRECTIVE: str = 'TEST'
RETURNCODE_DIRECTIVE: str = 'RETURNCODE'
STDOUT_DIRECTIVE: str = 'STDOUT'
STDERR_DIRECTIVE: str = 'STDERR'

passed_test_count = 0
failed_test_count = 0

def parse_directives(test_path: str) -> tuple[int | None, str, str]:
  test_file = open(test_path)
  test: list[str] = test_file.readlines()
  return_code_expected: int | None = None
  stdout_expected: str = ""
  stderr_expected: str = ""

  for line in test:
    if not line.startswith('//'):
      break
    line = line.strip('//')
    line = line.lstrip()
    if not line.startswith(f'{TEST_DIRECTIVE}:'):
      continue
    line = line.lstrip(f'{TEST_DIRECTIVE}:')
    line = line.lstrip()
    if line.startswith(RETURNCODE_DIRECTIVE):
      if return_code_expected is not None:
        print(f"Warning! Multiple {RETURNCODE_DIRECTIVE} directives detected in test {test_path}, skipping...")
        break
      l: list[str] = line.split()
      rc_str: str = l[1]
      if len(l) != 2 or not rc_str.isdigit():
        print(f"Warning: error parsing {RETURNCODE_DIRECTIVE} directive in test {test_path}, skipping...")
        break
      return_code_expected = int(rc_str)
    elif line.startswith(STDOUT_DIRECTIVE):
      l = line.split(STDOUT_DIRECTIVE)
      stdout_expected += l[1][1:]
    elif line.startswith(STDERR_DIRECTIVE):
      l = line.split(STDERR_DIRECTIVE)
      stderr_expected += l[1][1:]
    else:
      print(f"Warning: unknown test directive in test {test_path}, skipping...")
  return (return_code_expected, stdout_expected, stderr_expected)

def execute_test(test_path: str, rc_exp, stdout_expected, stderr_expected):
  compilation_result = subprocess.run(["bash", f"{PROJECT_ROOT_DIR}/compile_and_link.sh", test_path], capture_output=True, timeout=5)
  result = subprocess.run([f"./main.out"], capture_output=True, text=True, timeout=5)

  failure_msg: str = "" 

  if compilation_result.returncode != 0:
    failure_msg += f"COMPILATION FAILED: {test_path}\n"
    return

  if result.returncode != rc_exp:
    failure_msg += f"Expected return code {rc_exp} but got {result.returncode}\n\n"

  if stdout_expected != result.stdout:
    failure_msg += "".join(difflib.unified_diff(
        [repr(line) + "\n" for line in stdout_expected.splitlines(keepends=True)],
        [repr(line) + "\n" for line in result.stdout.splitlines(keepends=True)],
        fromfile="expected stdout",
        tofile="actual stdout",
    ))

  if stderr_expected != result.stderr:
    failure_msg += "".join(difflib.unified_diff(
        [repr(line) + "\n" for line in stderr_expected.splitlines(keepends=True)],
        [repr(line) + "\n" for line in result.stderr.splitlines(keepends=True)],
        fromfile="expected stderr",
        tofile="actual stderr",
    ))


  if failure_msg:
    failure_msg = f"FAILED: {test_path}\n{failure_msg}"
    print(failure_msg)
  else:
    print(f"PASSED: {test_path}")

def run_test(test_path: str):
  ret_code_opt, stdout_expected, stderr_expected = parse_directives(test_path)

  if ret_code_opt is None and len(stdout_expected) == 0 and len(stderr_expected) == 0:
    print(f"Warning: test {test_path} contains no test directives! Skipping...")
    return

  rc_exp: int = 0
  if ret_code_opt is not None:
    rc_exp = ret_code_opt

  execute_test(test_path, rc_exp, stdout_expected, stderr_expected)


if __name__ == '__main__':
  os.mkdir('sandbox')
  try:
    os.chdir('sandbox')
    for path in Path(f'{PROJECT_ROOT_DIR}/test/e2e').rglob("*"):
      if path.is_file():
        run_test(str(path))
  finally:
    try:
      os.remove("main.out")
    except FileNotFoundError:
      pass
    os.chdir('..')
    os.rmdir('sandbox')