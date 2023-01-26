import subprocess

def main():
    fens = ["8/8/8/8/2K2r2/2R5/8/k7 w - - 0 1"]
    for i in fens:
        subprocess.run(["./bin/Chess", i])


main()