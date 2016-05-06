#Chip-8 test applications

These test applications are used to test different aspects of the emulator.

##License

GPLv3

##Pseudocode for test applications

- flags1 (check flags on addition)

   ```
   r1 = 254 
   r2 = 1 
   DO 
      r1 = r1 + r2 
      IF flag = 0 THEN Print 0 
      IF flag = 1 THEN Print 1 
      WaitForKeyPress() 
   LOOP 
   ```

- flags2 (check flags on subtraction)

   ```
   r1 = 1
   r2 = 1 
   DO 
      r1 = r1 - r2 
      IF flag = 0 THEN Print 0 
      IF flag = 1 THEN Print 1 
      WaitForKeyPress() 
   LOOP
   ```

- flags3 (check flags on leftshift)

   ```
   r1 = 127
   DO 
      r1 = r1 << 1 
      IF flag = 0 THEN Print 0 
      IF flag = 1 THEN Print 1 
      WaitForKeyPress() 
   LOOP 
   ```

- flags4 (check flags on rightshift)

   ```
   r1 = 254
   DO 
      r1 = r1 >> 1 
      IF flag = 0 THEN Print 0 
      IF flag = 1 THEN Print 1 
      WaitForKeyPress() 
   LOOP 
   ```

- count (counts from 0 to 9 in an infinite loop)

   ```
   r0 = 0
   DO 
      ClearScreen()
      Print r0
      r0 = r0 + 1
      DelayTimer = 255
      DO
      LOOP UNTIL DelayTimer = 0
      IF r0 = 10 THEN r0 = 0
   LOOP 
   ```

- bsort (bubblesort)

   ```
      bubblesort algorithm
   ```
