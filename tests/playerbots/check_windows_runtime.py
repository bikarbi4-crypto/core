"""Validate the staged Windows loader closure using only --version (no world/DB)."""
from pathlib import Path
from hashlib import sha256
import ctypes
import json
import os
import shutil
import subprocess
import sys
import tempfile

assert sys.platform == 'win32'
artifact = Path(sys.argv[1]).resolve()
revision = sys.argv[2]
assert len(revision) == 40 and all(c in '0123456789abcdef' for c in revision)
required = {'libmysql.dll', 'libeay32.dll', 'libcurl.dll', 'libssl-3-x64.dll', 'libcrypto-3-x64.dll'}
dlls = list((artifact / 'runtime').glob('*.dll'))
assert required <= {p.name.lower() for p in dlls}
# Suppress Windows loader/error dialogs in this process and its child only.
ctypes.windll.kernel32.SetErrorMode(0x0001 | 0x0002 | 0x8000)
with tempfile.TemporaryDirectory(prefix='vmangos-version-only-', dir=artifact.parent) as directory:
    isolated = Path(directory)
    assert isolated.resolve().parent == artifact.parent
    shutil.copy2(artifact / 'mangosd.exe', isolated)
    for dll in dlls:
        shutil.copy2(dll, isolated)
    env = os.environ.copy()
    windows = Path(os.environ['SystemRoot'])
    env['PATH'] = os.pathsep.join([str(windows / 'System32'), str(windows)])
    # Main parses --version and returns before configuration/database startup.
    result = subprocess.run([str(isolated / 'mangosd.exe'), '--version'], cwd=isolated,
                            env=env, capture_output=True, text=True, timeout=20,
                            creationflags=subprocess.CREATE_NO_WINDOW)
    if result.returncode:
        raise RuntimeError(f'Packaged DLL loading failed: 0x{result.returncode & 0xffffffff:08x}; {result.stderr}')
    assert revision[:20] in result.stdout, result.stdout
    assert not list(isolated.glob('*.conf')) and not list(isolated.glob('*.log'))
    report = {'commit': revision, 'exit_code': result.returncode, 'version': result.stdout.strip(),
              'runtime_search_path': 'isolated executable directory and Windows system directories only',
              'files': [{'path': 'runtime/'+p.name, 'sha256': sha256(p.read_bytes()).hexdigest()} for p in dlls],
              'world_started': False, 'configuration_read': False, 'database_connected': False}
(artifact / 'validation/runtime-smoke.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print(result.stdout.strip())
print('PASS: --version loaded with all packaged application DLLs; no world or database started.')
