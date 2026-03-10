# tools/ui_agent/ui_agent.py
# -*- coding: utf-8 -*-
import argparse
import json
import os
from pywinauto import Desktop
from pywinauto.application import Application

def ensure_dir(path):
    d = os.path.dirname(path)
    if d and not os.path.exists(d):
        os.makedirs(d)

def list_windows():
    windows = []
    for w in Desktop(backend="uia").windows():
        try:
            title = w.window_text()
            if title:
                windows.append(title)
        except:
            pass
    print(json.dumps(windows, ensure_ascii=False, indent=2))

def find_window(title):
    return Desktop(backend="uia").window(title_re=".*%s.*" % title)

def focus_window(title):
    w = find_window(title)
    w.set_focus()
    print("focused")

def capture_window(title, out):
    from PIL import ImageGrab
    w = find_window(title)
    rect = w.rectangle()
    ensure_dir(out)
    img = ImageGrab.grab(bbox=(rect.left, rect.top, rect.right, rect.bottom))
    img.save(out)
    print(out)

def click_text(title, text):
    w = find_window(title)
    ctrl = w.child_window(title=text)
    ctrl.click_input()
    print("clicked")

def dump_tree(title, out):
    w = find_window(title)
    lines = []
    def walk(ctrl, depth=0):
        try:
            lines.append({
                "depth": depth,
                "title": ctrl.window_text(),
                "class": ctrl.friendly_class_name()
            })
            for c in ctrl.children():
                walk(c, depth + 1)
        except:
            pass
    walk(w)
    ensure_dir(out)
    with open(out, "w") as f:
        json.dump(lines, f, ensure_ascii=False, indent=2)
    print(out)

def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd")

    sub.add_parser("list-windows")

    p = sub.add_parser("focus")
    p.add_argument("--title", required=True)

    p = sub.add_parser("capture")
    p.add_argument("--title", required=True)
    p.add_argument("--out", required=True)

    p = sub.add_parser("click-text")
    p.add_argument("--title", required=True)
    p.add_argument("--text", required=True)

    p = sub.add_parser("dump-tree")
    p.add_argument("--title", required=True)
    p.add_argument("--out", required=True)

    args = ap.parse_args()

    if args.cmd == "list-windows":
        list_windows()
    elif args.cmd == "focus":
        focus_window(args.title)
    elif args.cmd == "capture":
        capture_window(args.title, args.out)
    elif args.cmd == "click-text":
        click_text(args.title, args.text)
    elif args.cmd == "dump-tree":
        dump_tree(args.title, args.out)
    else:
        ap.print_help()

if __name__ == "__main__":
    main()