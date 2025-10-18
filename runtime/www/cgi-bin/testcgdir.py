#!/usr/bin/env python3
import os

print("Content-Type: text/plain\n")

print("=== CGI chdir test ===")
cwd = os.getcwd()
script_path = os.path.abspath(__file__)
print(f"Current working directory: {cwd}")
print(f"Script absolute path: {script_path}\n")

print("Listing local files:\n")
files = os.listdir(".")
if files:
    for f in files:
        print(f"- {f}")
else:
    print("No files found.")

# Optional: try reading relative_test.txt
filename = "relative_test.txt"
if os.path.exists(filename):
    print(f"\nContents of {filename}:")
    with open(filename) as f:
        print(f.read())
else:
    print(f"\n{filename} does not exist.")
