dobro:
1. t1 = n * 2
2. return t1
positivo:
3. if n > 0 goto 5
4. goto 7
5. t2 = 1
6. goto 8
7. t2 = 0
8. return t2
mostrar:
9. param "valor:"
10. param v
11. param p
12. call println, 3
13. return
main:
14. a = 5
15. t3 = a + 1
16. param t3
17. b = call dobro, 1
18. param b
19. p = call positivo, 1
20. if a < b goto 22
21. goto 24
22. t4 = 1
23. goto 25
24. t4 = 0
25. param b
26. param t4
27. call mostrar, 2
28. return 0
