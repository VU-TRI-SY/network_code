# Open the source file in read mode ('r')
source_file_path = 'note.txt'
ls = []
count = 0
with open(source_file_path, 'r') as source_file:
    # Read the content of the source file
    for line in source_file:
        print(int(line.strip()), sep=' ', end=' ')
        count += 1
        if count==25: 
            print()
            count = 0
        
