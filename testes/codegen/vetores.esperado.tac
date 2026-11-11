soma:
1. t1 = v[0]
2. t2 = v[1]
3. t3 = t1 + t2
4. t4 = v[2]
5. t5 = t3 + t4
6. return t5
main:
7. t6 = v[i]
8. x = t6
9. t7 = i * 4
10. t8 = t7 + j
11. t9 = m[t8]
12. x = t9
13. t10 = i + 1
14. t11 = v[t10]
15. t12 = t11 * 2
16. x = t12
17. t13 = call scanf, 0
18. v[2] = t13
19. t14 = 1 * 4
20. t15 = t14 + j
21. t16 = call scanf, 0
22. m[t15] = t16
23. x = call scanf, 0
24. t17 = v[0]
25. t18 = i * 4
26. t19 = t18 + 1
27. t20 = m[t19]
28. param t17
29. param t20
30. call println, 2
31. param v
32. x = call soma, 1
33. return
