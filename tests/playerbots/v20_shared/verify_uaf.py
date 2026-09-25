"""The V19 defect must fail for the expected real lifetime chain, not any crash."""
from pathlib import Path
import subprocess,sys
binary=Path(sys.argv[1]);output=Path(sys.argv[2])
r=subprocess.run([str(binary),'--repro'],capture_output=True,text=True,encoding='utf-8',errors='replace')
log=r.stdout+r.stderr;output.write_text(log,encoding='utf-8')
required=['AddressSanitizer: heap-use-after-free','PerformanceMonitor::start','SingleCalculatedValue','SharedObjectContext::GetUntypedValue','freed by thread','PlayerbotAI']
missing=[s for s in required if s not in log]
if r.returncode==0 or missing:
 print('Expected V19 UAF not reproduced; exit=',r.returncode,'missing=',missing)
 print(log[:6000]);raise SystemExit(1)
print('Confirmed expected V19 UAF: new PlayerbotAI -> factory/cache -> delete AI -> SingleCalculatedValue Get -> PMO start; full log:',output)
