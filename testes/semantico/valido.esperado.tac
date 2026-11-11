fatorial:
1. resultado = 1
2. if n > 1 goto 4
3. goto 9
4. t1 = n - 1
5. param t1
6. anterior = call fatorial, 1
7. t2 = n * anterior
8. resultado = t2
9. return resultado
media:
10. t3 = a + b
11. t4 = t3 / 2.0
12. resultado = t4
13. return resultado
soma:
14. t5 = valores[0]
15. t6 = valores[1]
16. t7 = t5 + t6
17. t8 = valores[2]
18. t9 = t7 + t8
19. return t9
maior:
20. if a > b goto 22
21. goto 24
22. t10 = 1
23. goto 25
24. t10 = 0
25. resultado = t10
26. return resultado
main:
27. ok = 1
28. t11 = call scanf, 0
29. valores[0] = t11
30. t12 = call scanf, 0
31. valores[1] = t12
32. t13 = call scanf, 0
33. valores[2] = t13
34. t14 = 1 * 2
35. t15 = t14 + 0
36. t16 = call scanf, 0
37. matriz[t15] = t16
38. param valores
39. total = call soma, 1
40. t17 = 1 * 2
41. t18 = t17 + 0
42. t19 = matriz[t18]
43. param total
44. param t19
45. m = call media, 2
46. param 5
47. fat = call fatorial, 1
48. param fat
49. param 100
50. resultado = call maior, 2
51. m = total
52. if resultado goto 54
53. goto 56
54. if ok goto 56
55. goto 58
56. if m >= 1.5 goto 58
57. goto 65
58. param "m:"
59. param m
60. param "total:"
61. param total
62. param valores
63. call println, 5
64. goto 68
65. param "fatorial:"
66. param fat
67. call println, 2
68. return 0
