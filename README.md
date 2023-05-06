# 2-Level-Branch-Predictor

the predictor supports Global Table and Local table,
also support Locat History, Global History.

if Global Table used, there is an option of using three diffrent ways to access the needed infromation from the table,
the first is regular a basic index based on history, 
the second way is to make XOR bitwise between the histoy and lsb bits of the PC
the third way is to make XOR bitwise between the histoy and upper bits of the PC
that way we could get less intersection with different branches.


