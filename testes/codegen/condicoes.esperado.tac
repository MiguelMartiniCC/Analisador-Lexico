main:
1. if ok goto 3
2. goto 4
3. a = 1
4. if a < b goto 10
5. goto 6
6. if ok goto 8
7. goto 10
8. a = 2
9. goto 11
10. a = 3
11. if a >= 18 goto 13
12. goto 15
13. if ok goto 15
14. goto 17
15. if a != 0 goto 17
16. goto 20
17. t1 = a - 1
18. a = t1
19. goto 11
20. return
