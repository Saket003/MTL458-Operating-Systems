n = 1000
with open('input-part1.txt', 'w') as f:
    for i in range(n):
        #write i+1 in file
        f.write(f'{i+1}\n')
    f.write('0\n')