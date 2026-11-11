soma:
1. t1 = a + b
2. resultado = t1
3. param "Resultado:"
4. param resultado
5. call println, 2
6. return resultado
main:
7. idade = 20
8. pi = 3.14
9. x = 1
10. ok = 0
11. idade = call scanf, 0
12. pi = call scanf, 0
13. param "Idade:"
14. param idade
15. call println, 2
16. param "Pi:"
17. param pi
18. call println, 2
19. param "Nome:"
20. param nome
21. call println, 2
22. if x <= 5 goto 24
23. goto 38
24. if idade >= 18 goto 26
25. goto 28
26. if ok goto 28
27. goto 30
28. if idade != 0 goto 30
29. goto 33
30. param "maior de idade"
31. call println, 1
32. goto 35
33. t2 = idade % 2
34. idade = t2
35. t3 = x + 1
36. x = t3
37. goto 22
38. param idade
39. param pi
40. pi = call soma, 2
41. return 0
