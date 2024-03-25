def case1():
    print("Case 1")

def case2():
    print("Case 2")

def case3():
    print("Case 3")

def case4():
    print("Case 4")

cases = {
    0: case1,
    1: case2,
    2: case3
}

cases.get(0)()
