"""End-to-end smoke test: launch OrcaSlicer with the automation server, load a
model, slice it, wait for completion, and save both a window PNG and a 3D PNG.

Run:
    python example_slice.py --orca /path/to/OrcaSlicer --model /path/to/cube.stl

On Linux CI, wrap with a virtual display, e.g.:
    xvfb-run -a python example_slice.py --orca ./OrcaSlicer --model cube.stl
"""
from __future__ import annotations
import argparse
import subprocess
import sys
import time

from orca_automation import OrcaClient, OrcaError


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--orca", required=True, help="path to the OrcaSlicer executable")
    ap.add_argument("--model", required=True, help="path to an STL/3MF to load")
    ap.add_argument("--port", type=int, default=13619)
    args = ap.parse_args()

    proc = subprocess.Popen([
        args.orca,
        "--automation-server",
        f"--automation-server-port={args.port}",
        args.model,
    ])
    try:
        orca = OrcaClient(port=args.port)

        # Wait for the server to come up.
        for _ in range(60):
            try:
                print("connected:", orca.version())
                break
            except OSError:
                time.sleep(0.5)
        else:
            print("ERROR: automation server did not start", file=sys.stderr)
            return 1

        # Wait until the project (model) is loaded.
        deadline = time.time() + 30
        while time.time() < deadline:
            if orca.app_state().get("project_loaded"):
                break
            time.sleep(0.5)

        # Click Slice and wait for the Export button to become enabled
        # (slicing complete) — wait_for replaces fragile fixed sleeps.
        orca.click({"id": "btn_slice"})
        orca.wait_for({"id": "btn_export"}, state="enabled", timeout_ms=180000,
                      poll_ms=500)

        with open("window.png", "wb") as f:
            f.write(orca.screenshot())
        with open("preview_3d.png", "wb") as f:
            f.write(orca.screenshot_3d(width=1920, height=1080))
        print("wrote window.png and preview_3d.png")
        return 0
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()


if __name__ == "__main__":
    raise SystemExit(main())
