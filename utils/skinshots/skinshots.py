#!/usr/bin/env python3
"""Screenshot the built-in theme on many simulators at several commits and
lay the shots out side by side.

For each commit, in a private worktree patched by simhack.py, each target's
simulator is configured, built, run once per screen (WPS, recording, FM
radio; the remote LCD along with them) and deleted before the next one is
built, so only one build exists at a time. A target already shot at a commit
is skipped on the next run unless --force is given.

  skinshots.py --work DIR [options] COMMIT [COMMIT...]

Output in DIR/out: shots/<sha>/<target>/*.png, collage/*.png, index.html.
"""
import argparse
import concurrent.futures
import math
import os
import re
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import threading
import time
import wave

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.abspath(os.path.join(HERE, "..", ".."))
SCREENS = ["wps", "recording", "radio"]

def log(*a):
    print(*a, flush=True)

def run(cmd, cwd=None, logfile=None, timeout=None, env=None, check=True):
    out = open(logfile, "w") if logfile else subprocess.DEVNULL
    try:
        r = subprocess.run(cmd, cwd=cwd, stdout=out, stderr=subprocess.STDOUT,
                           stdin=subprocess.DEVNULL, timeout=timeout, env=env)
    finally:
        if logfile:
            out.close()
    if check and r.returncode:
        raise RuntimeError(f"{cmd[0]} failed ({r.returncode})"
                           + (f", see {logfile}" if logfile else ""))
    return r.returncode

def git(*a, cwd=SRC):
    return subprocess.run(["git", "-C", cwd] + list(a), check=True,
                          capture_output=True, text=True).stdout.strip()

def free_gb(path):
    return shutil.disk_usage(path).free / 2**30

def read_targets(path):
    out = []
    for line in open(path):
        line = line.split("#", 1)[0].strip()
        if line:
            out.append(line)
    return out

# --- test media -------------------------------------------------------------

def make_media(d):
    os.makedirs(d, exist_ok=True)
    wav = os.path.join(d, "test.wav")
    if not os.path.exists(wav):
        rate = 22050
        w = wave.open(wav, "wb")
        w.setnchannels(2)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(b"".join(
            struct.pack("<hh", int(8000 * math.sin(2 * math.pi * 440 * i / rate)),
                        int(8000 * math.sin(2 * math.pi * 660 * i / rate)))
            for i in range(rate * 120)))
        w.close()
    open(os.path.join(d, "test.m3u"), "w").write("test.wav\n")
    open(os.path.join(d, "test.fmr"), "w").write(
        "87500000:Rockbox FM\n101500000:Test Station\n")

# --- one target at one commit ------------------------------------------------

def features(build):
    p = os.path.join(build, "apps", "features")
    return set(open(p).read().split()) if os.path.exists(p) else set()

def lcd_label(build, tree):
    """WxHxD from the target's config header, as the theme files name it."""
    model = None
    for line in open(os.path.join(build, "Makefile")):
        m = re.match(r"export MODELNAME=(\S+)", line)
        if m:
            model = m.group(1)
    vals = {}
    hdr = os.path.join(tree, "firmware", "export", "config", f"{model}.h")
    if model and os.path.exists(hdr):
        for line in open(hdr):
            m = re.match(r"#define\s+(LCD_WIDTH|LCD_HEIGHT|LCD_DEPTH)\s+(\d+)", line)
            if m and m.group(1) not in vals:
                vals[m.group(1)] = m.group(2)
    if len(vals) == 3:
        return f"{vals['LCD_WIDTH']}x{vals['LCD_HEIGHT']}x{vals['LCD_DEPTH']}"
    return None

def shoot(build, screens, media, logdir, delay):
    """Run the simulator once per screen, all at once, each on its own copy
    of simdisk. Returns {screen: {"main"|"remote": bmp path}}."""
    env = dict(os.environ, RB_AUTODUMP=str(delay), RB_AUTOPLAY="1",
               SDL_VIDEODRIVER="offscreen", SDL_AUDIODRIVER="dummy")
    roots = {}
    for sc in screens:
        root = os.path.join(build, f"root-{sc}")
        shutil.rmtree(root, ignore_errors=True)
        shutil.copytree(os.path.join(build, "simdisk"), root, symlinks=True)
        for f in ("test.wav", "test.m3u"):
            shutil.copy(os.path.join(media, f), root)
        cfgdir = os.path.join(root, ".rockbox")
        os.makedirs(os.path.join(cfgdir, "fmpresets"), exist_ok=True)
        shutil.copy(os.path.join(media, "test.fmr"), os.path.join(cfgdir, "fmpresets"))
        open(os.path.join(cfgdir, "config.cfg"), "w").write(
            f"start in screen: {sc}\n"
            "fmr: /.rockbox/fmpresets/test.fmr\n")
        roots[sc] = root
    result = {}
    todo = list(screens)
    for attempt in range(2):
        procs = {}
        for sc in todo:
            out = open(os.path.join(logdir, f"run-{sc}.log"), "w")
            procs[sc] = (subprocess.Popen(
                ["timeout", "60", "./rockboxui", "--root", roots[sc]], cwd=build,
                stdin=subprocess.DEVNULL, stdout=out, stderr=subprocess.STDOUT,
                env=env), out)
        for sc, (pr, out) in procs.items():
            pr.wait()
            out.close()
            dumps = {}
            for f in sorted(os.listdir(roots[sc])):
                if re.match(r"dump[ _].*\.bmp$", f):
                    dumps.setdefault("main", os.path.join(roots[sc], f))
                elif re.match(r"rdump[ _].*\.bmp$", f):
                    dumps.setdefault("remote", os.path.join(roots[sc], f))
            if "main" in dumps:
                result[sc] = dumps
        todo = [sc for sc in todo if sc not in result]
        if not todo:
            break
    return result, todo

def fmt_time(sec):
    sec = int(sec)
    return f"{sec // 3600}:{sec // 60 % 60:02}:{sec % 60:02}" if sec >= 3600 \
        else f"{sec // 60}:{sec % 60:02}"

class Progress:
    def __init__(self, total):
        self.total, self.done, self.failed = total, 0, []
        self.start = time.monotonic()
        self.lock = threading.Lock()

    def finish(self, target, ok):
        with self.lock:
            self.done += 1
            if not ok:
                self.failed.append(target)
            el = time.monotonic() - self.start
            left = self.total - self.done
            eta = f", ~{fmt_time(el / self.done * left)} left" if left else ""
            fail = f", {len(self.failed)} failed" if self.failed else ""
            log(f"[{self.done}/{self.total} targets] elapsed {fmt_time(el)}{eta}{fail}")

def state(args, sha, target):
    d = os.path.join(args.out, "shots", sha, target)
    if args.force:
        return None
    if os.path.exists(os.path.join(d, "done")):
        return "done"
    if os.path.exists(os.path.join(d, "failed")) and not args.retry_failed:
        return "failed"
    return None

def do_target(args, trees, commits, short, target, media):
    """Build target once and shoot it at each commit, rebuilding incrementally
    in between. The build sees the source through short/src-<target>, a
    symlink repointed at each commit's worktree."""
    P = f"  {target}:"
    todo = [sha for sha, _ in commits if state(args, sha, target) is None]
    skipped = [sha for sha, _ in commits if sha not in todo]
    if skipped:
        log(f"{P} skipping {' '.join(skipped)} (done or failed before)")
    if not todo:
        return True
    src = os.path.join(short, f"src-{target}")
    build = os.path.join(short, "b", target)
    ok = True
    configured_at = None
    try:
        for sha in todo:
            if free_gb(args.work) < args.min_free_gb:
                raise SystemExit(f"only {free_gb(args.work):.1f} GB free, need "
                                 f"{args.min_free_gb} (--min-free-gb); stopping")
            shotdir = os.path.join(args.out, "shots", sha, target)
            logdir = os.path.join(args.out, "logs", sha, target)
            for d in (shotdir, logdir):
                shutil.rmtree(d, ignore_errors=True)
                os.makedirs(d)
            t0 = time.monotonic()
            try:
                if os.path.lexists(src):
                    os.remove(src)
                os.symlink(trees[sha], src)
                if configured_at and configure_changed(configured_at, sha):
                    configured_at = None
                if configured_at is None:
                    shutil.rmtree(build, ignore_errors=True)
                    os.makedirs(build)
                    run([os.path.join(src, "tools", "configure"), f"--target={target}",
                         "--type=s"], cwd=build,
                        logfile=os.path.join(logdir, "configure.log"))
                    configured_at = sha
                    how = "build"
                else:
                    touch_changed(trees[sha], prev_sha, sha)
                    how = "rebuild"
                run(["make", f"-j{args.jobs}", f"-l{os.cpu_count()}", "bin", "codecs"],
                    cwd=build, logfile=os.path.join(logdir, "make.log"))
                shutil.rmtree(os.path.join(build, "simdisk"), ignore_errors=True)
                run(["make", "install"], cwd=build,
                    logfile=os.path.join(logdir, "install.log"))
                tb = time.monotonic() - t0
                feat = features(build)
                label = lcd_label(build, src) or "unknown"
                screens = ["wps"] + [x for x in ("recording", "radio") if x in feat]
                dumps, missing = shoot(build, screens, media, logdir, args.delay)
                got = []
                for sc in screens:
                    for kind, bmp in dumps.get(sc, {}).items():
                        name = sc if kind == "main" else f"{sc}-remote"
                        Image.open(bmp).convert("RGB").save(
                            os.path.join(shotdir, name + ".png"))
                        got.append(name)
                open(os.path.join(shotdir, "lcd"), "w").write(label + "\n")
                log(f"{P} {sha} ({label}) {how} {fmt_time(tb)}, shots "
                    f"{fmt_time(time.monotonic() - t0 - tb)}: {' '.join(got)}"
                    + (f"  MISSING: {' '.join(missing)}" if missing else ""))
                if not got:
                    raise RuntimeError("no screenshots")
                open(os.path.join(shotdir, "done"), "w").write(" ".join(got) + "\n")
            except (RuntimeError, subprocess.TimeoutExpired) as e:
                log(f"{P} {sha} FAILED: {e}")
                open(os.path.join(shotdir, "failed"), "w").write(f"{e}\n")
                ok = False
                configured_at = None  # start the next commit from scratch
            prev_sha = sha
            for f in os.listdir(build) if os.path.isdir(build) else []:
                if f.startswith("root-"):
                    shutil.rmtree(os.path.join(build, f), ignore_errors=True)
        return ok
    finally:
        shutil.rmtree(build, ignore_errors=True)
        if os.path.lexists(src):
            os.remove(src)

CONFIGURE_FILES = ("tools/configure", "tools/root.make")

HOST_TOOLS = ("rdf2binary", "convbdf", "codepages", "scramble", "bmp2rb", "uclpack",
              "mkboot", "iaudio_bl_flash.c", "iaudio_bl_flash.h")

def build_host_tools(tree, short, sha, target, logdir):
    """The host tools are built into the source tree, not the build dir:
    targets built at once from one worktree would race to build and run
    them. Build them once, before."""
    src = os.path.join(short, f"src-tools-{sha}")
    build = os.path.join(short, "b", f"tools-{sha}")
    os.symlink(tree, src)
    os.makedirs(build)
    try:
        run([os.path.join(src, "tools", "configure"), f"--target={target}", "--type=s"],
            cwd=build, logfile=os.path.join(logdir, "configure.log"))
        run(["make", "-j4"] + [os.path.join(src, "tools", t) for t in HOST_TOOLS],
            cwd=build, logfile=os.path.join(logdir, "make.log"))
    finally:
        shutil.rmtree(build, ignore_errors=True)
        os.remove(src)

def configure_changed(a, b):
    return bool(git("diff", "--name-only", a, b, "--", *CONFIGURE_FILES))

def touch_changed(tree, a, b):
    """The worktrees were checked out before the build: make would take the
    files that differ between a and b as older than its objects."""
    now = time.time()
    for f in git("diff", "--name-only", a, b).splitlines():
        p = os.path.join(tree, f)
        if os.path.exists(p):
            os.utime(p, (now, now))

# --- collage ----------------------------------------------------------------

def font(size):
    try:
        return ImageFont.load_default(size=size)
    except TypeError:
        return ImageFont.load_default()

def diff_image(a, b):
    """b dimmed, with every pixel that differs from a in magenta."""
    if a.size != b.size:
        return None, -1
    dim = Image.blend(b, Image.new("RGB", b.size, (255, 255, 255)), 0.65)
    pa, pb, pd = a.load(), b.load(), dim.load()
    n = 0
    for y in range(a.size[1]):
        for x in range(a.size[0]):
            if pa[x, y] != pb[x, y]:
                pd[x, y] = (255, 0, 255)
                n += 1
    return dim, n

def collage(args, commits):
    shots = os.path.join(args.out, "shots")
    cdir = os.path.join(args.out, "collage")
    shutil.rmtree(cdir, ignore_errors=True)
    os.makedirs(cdir)
    shas = [c[0] for c in commits]
    targets = sorted({t for s in shas if os.path.isdir(os.path.join(shots, s))
                      for t in os.listdir(os.path.join(shots, s))})
    sheets, summary = [], []
    title_f, label_f = font(18), font(14)
    pad = 12

    for t in targets:
        label = "unknown"
        imgs = {}
        for s in shas:
            d = os.path.join(shots, s, t)
            if os.path.exists(os.path.join(d, "lcd")):
                label = open(os.path.join(d, "lcd")).read().strip()
            if os.path.isdir(d):
                for f in os.listdir(d):
                    if f.endswith(".png"):
                        imgs[(s, f[:-4])] = Image.open(os.path.join(d, f)).convert("RGB")
        names = [n for base in SCREENS for n in (base, base + "-remote")
                 if any((s, n) in imgs for s in shas)]
        if not names:
            continue
        # one integer scale for the whole sheet, so pixels compare 1:1
        minw = min(im.size[0] for im in imgs.values())
        scale = max(1, math.ceil(args.min_width / minw))
        cols = shas + (["diff"] if len(shas) >= 2 else [])
        colw = max(im.size[0] for im in imgs.values()) * scale
        rows = []
        for n in names:
            rowh = max(imgs[(s, n)].size[1] for s in shas if (s, n) in imgs) * scale
            rows.append((n, rowh))
        hdr_h = 30 + 20 * len(shas) + 24
        lab_w = 140
        W = lab_w + len(cols) * (colw + pad) + pad
        H = hdr_h + sum(h + pad for _, h in rows) + pad
        sheet = Image.new("RGB", (W, H), (40, 40, 40))
        dr = ImageDraw.Draw(sheet)
        dr.text((pad, 6), f"{label}  {t}", fill="white", font=title_f)
        for i, (s, subj) in enumerate(commits):
            dr.text((pad, 30 + 20 * i), f"[{i + 1}] {s}  {subj}", fill=(200, 200, 200),
                    font=label_f)
        for ci, c in enumerate(cols):
            dr.text((lab_w + pad + ci * (colw + pad), hdr_h - 20),
                    f"diff [1] -> [{len(shas)}]" if c == "diff"
                    else f"[{ci + 1}] {c}", fill="white", font=label_f)
        y = hdr_h
        for n, rowh in rows:
            dr.text((pad, y + 4), n, fill="white", font=label_f)
            for ci, c in enumerate(cols):
                x = lab_w + pad + ci * (colw + pad)
                if c == "diff":
                    a, b = imgs.get((shas[0], n)), imgs.get((shas[-1], n))
                    im, ndiff = diff_image(a, b) if a and b else (None, None)
                    if ndiff is None:
                        res = "missing in a commit"
                    elif ndiff < 0:
                        res = "size differs"
                    else:
                        res = "same" if ndiff == 0 else f"{ndiff} px differ"
                    summary.append((label, t, n, res))
                    dr.text((pad, y + 24), res, fill=(255, 120, 255), font=label_f)
                else:
                    im = imgs.get((c, n))
                if im is None:
                    dr.rectangle([x, y, x + colw - 1, y + rowh - 1], outline=(120, 120, 120))
                    dr.text((x + 8, y + 8), "n/a", fill=(160, 160, 160), font=label_f)
                else:
                    sheet.paste(im.resize((im.size[0] * scale, im.size[1] * scale),
                                          Image.NEAREST), (x, y))
            y += rowh + pad
        fn = f"{label}-{t}.png"
        sheet.save(os.path.join(cdir, fn))
        sheets.append((label, t, fn, sheet.size))

    def key(e):
        m = re.match(r"(\d+)x(\d+)", e[0])
        return (int(m.group(1)) * int(m.group(2)) if m else 0, e[1])
    sheets.sort(key=key)

    with open(os.path.join(args.out, "index.html"), "w") as h:
        h.write("<!doctype html><meta charset=utf-8><title>skinshots</title>"
                "<style>body{background:#222;color:#ddd;font:14px sans-serif}"
                "img{display:block;margin:4px 0 24px;image-rendering:pixelated}</style>\n")
        for i, (s, subj) in enumerate(commits):
            h.write(f"<div>[{i + 1}] {s} {subj}</div>\n")
        for label, t, fn, _ in sheets:
            h.write(f"<h3 id='{t}'>{label} {t}</h3><img src='collage/{fn}'>\n")

    if sheets and args.all_png:
        W = max(sz[0] for *_, sz in sheets)
        H = sum(sz[1] for *_, sz in sheets)
        allim = Image.new("RGB", (W, H), (40, 40, 40))
        y = 0
        for *_, fn, sz in sheets:
            allim.paste(Image.open(os.path.join(cdir, fn)), (0, y))
            y += sz[1]
        allim.save(os.path.join(cdir, "all.png"))

    log("\nsummary (first vs last commit):")
    for label, t, n, res in sorted(summary, key=lambda e: (key(e), e[2])):
        log(f"  {label:12} {t:20} {n:18} {res}")
    log(f"\n{len(sheets)} sheets in {cdir}\nopen {os.path.join(args.out, 'index.html')}")

# --- main -------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("commits", nargs="+", help="commits to compare, in column order")
    ap.add_argument("--work", required=True, help="work dir (worktree, build, out)")
    ap.add_argument("--targets", default=os.path.join(HERE, "targets.txt"),
                    help="target list file (default: %(default)s)")
    ap.add_argument("--only", help="comma separated targets instead of --targets")
    ap.add_argument("-p", "--parallel", type=int, default=3,
                    help="targets built at once (default %(default)s)")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count(),
                    help="make jobs per build, capped by load (default %(default)s)")
    ap.add_argument("--delay", type=int, default=4,
                    help="seconds after boot to take the shot (default %(default)s)")
    ap.add_argument("--min-free-gb", type=float, default=5,
                    help="stop before a build when less is free (default %(default)s)")
    ap.add_argument("--force", action="store_true", help="reshoot targets already done")
    ap.add_argument("--retry-failed", action="store_true",
                    help="retry targets that failed on an earlier run")
    ap.add_argument("--collage-only", action="store_true", help="skip building")
    ap.add_argument("--min-width", type=int, default=320,
                    help="upscale so the narrowest shot is at least this wide")
    ap.add_argument("--no-all-png", dest="all_png", action="store_false",
                    help="don't write collage/all.png")
    args = ap.parse_args()
    args.work = os.path.abspath(args.work)
    args.out = os.path.join(args.work, "out")
    os.makedirs(args.out, exist_ok=True)

    commits = []
    for c in args.commits:
        sha = git("rev-parse", "--short=10", c + "^{commit}")
        commits.append((sha, git("log", "-1", "--format=%s", sha)))
    targets = args.only.split(",") if args.only else read_targets(args.targets)

    if not args.collage_only:
        media = os.path.join(args.work, "media")
        make_media(media)
        os.makedirs(os.path.join(args.work, "build"), exist_ok=True)
        # make's dependency step overflows the command line with long paths:
        # configure and build through short symlinks
        short = tempfile.mkdtemp(prefix="ss", dir="/tmp")
        os.symlink(os.path.join(args.work, "build"), os.path.join(short, "b"))
        trees = {sha: os.path.join(args.work, f"tree-{sha}") for sha, _ in commits}
        signal.signal(signal.SIGTERM, lambda *a: sys.exit(1))
        try:
            git("worktree", "prune")
            for sha, subj in commits:
                tree = trees[sha]
                if os.path.exists(tree):
                    subprocess.run(["git", "-C", SRC, "worktree", "remove", "--force",
                                    tree], capture_output=True)
                    shutil.rmtree(tree, ignore_errors=True)
                git("worktree", "add", "--detach", tree, sha)
                run([sys.executable, os.path.join(HERE, "simhack.py"), tree])
                logdir = os.path.join(args.out, "logs", sha, "host-tools")
                os.makedirs(logdir, exist_ok=True)
                build_host_tools(tree, short, sha, targets[0], logdir)
            log(f"{len(targets)} targets x {len(commits)} commits, "
                f"{args.parallel} at a time: "
                + " | ".join(f"{sha} {subj}" for sha, subj in commits))
            prog = Progress(len(targets))
            def job(t):
                try:
                    ok = do_target(args, trees, commits, short, t, media)
                except Exception as e:
                    log(f"  {t}: CRASHED: {e!r}")
                    ok = False
                prog.finish(t, ok)
            with concurrent.futures.ThreadPoolExecutor(args.parallel) as ex:
                list(ex.map(job, targets))
            if prog.failed:
                log("failed: " + " ".join(prog.failed) + f"\n(logs in {args.out}/logs)")
        finally:
            shutil.rmtree(short, ignore_errors=True)
            shutil.rmtree(os.path.join(args.work, "build"), ignore_errors=True)
            for tree in trees.values():
                if os.path.exists(tree):
                    subprocess.run(["git", "-C", SRC, "worktree", "remove", "--force",
                                    tree], capture_output=True)
                    shutil.rmtree(tree, ignore_errors=True)
            git("worktree", "prune")

    collage(args, commits)

if __name__ == "__main__":
    main()
