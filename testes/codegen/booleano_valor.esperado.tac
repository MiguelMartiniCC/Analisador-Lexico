main:
1. if x < y goto 3
2. goto 5
3. t1 = 1
4. goto 6
5. t1 = 0
6. ok = t1
7. if ok goto 9
8. goto 13
9. if x != 0 goto 11
10. goto 13
11. t2 = 1
12. goto 14
13. t2 = 0
14. r = t2
15. if ok goto 19
16. goto 17
17. t3 = 1
18. goto 20
19. t3 = 0
20. ok = t3
21. r = ok
22. if ok goto 26
23. goto 24
24. if r goto 26
25. goto 27
26. x = 1
27. return
