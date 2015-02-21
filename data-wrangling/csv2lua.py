# Convert a CSV file to a Lua table script that can be imported using `dofile`.

import csv

def csv2lua(in_file, out_file, global_name):
    fp_in = open(in_file, 'r')
    rows = list(csv.reader(fp_in))
    fp_in.close()

    headers = rows[0]
    lua_rows = []
    for row in rows[1:]:
        cells = []
        print row
        for i, cell in enumerate(row):
            key = headers[i]
            try:
                cell = int(cell)
                cells.append('%s=%s' % (key, cell))
            except ValueError:
                cells.append('%s="%s"' % (key, cell))
        lua_rows.append('  {' + ', '.join(cells) + '}')



        #start_freq, end_freq, allocation, applications = row
        #s = '  {start_freq=%s, end_freq=%s, allocation="%s", applications="%s"}' % (start_freq, end_freq, allocation, applications)
        #lua_rows.append(s)
    s = '%s = {\n%s\n}\n' % (global_name, ',\n'.join(lua_rows))
    print s
    fp_out = open(out_file, 'w')
    fp_out.write(s)
    fp_out.close()

def usage():
    print "python csv2lua.py <table.csv> <table.lua> <global>"

if __name__ == '__main__':
    import sys
    if len(sys.argv) != 4:
        usage()
    else:
        csv2lua(sys.argv[1], sys.argv[2], sys.argv[3])
