import subprocess

def main():
    fens = ["8/8/8/8/2K2r2/2R5/8/k7 w - - 0 1", "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"]
    for i in fens:
        subprocess.run(["./bin/Chess", i])

main()