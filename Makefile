test:
    g++ KaizoTrap.cpp -o KaizoTrap.out
    
prod:
    g++ KaizoTrap.cpp -O4 -o KaizoTrap.out
    
gprof:
    g++ KaizoTrap.cpp -O4 -pg -o KaizoTrap.out