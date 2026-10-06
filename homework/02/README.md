[English](README.md) | [简体中文](README.zh-CN.md)

# Homework 2: Expression Solvers

## problem1: The 24-point game

Enter four integers. The program prints expressions that evaluate to 24, or `no` if none are found.

```text
1 2 3 4
```

After running the repository's build script, use `build/console/TwentyfourSolver.exe`.

## problem2: Target-value expressions

Enter the number of values `n` and a target, followed by `n` nonnegative integers. The solver applies addition or multiplication in input order, evaluating each step from left to right. Output expressions omit parentheses, so read them according to this rule.

```text
3 9
1 2 3
```

If the target is reachable, an expression is printed. Otherwise, the program prints `No` followed by the smallest reachable value greater than the target, or `-1` if none exists.

Use `build/console/TargetSolver.exe`. The original assignment assumes valid input with `n >= 1`; input validation and overflow handling have not been added.
