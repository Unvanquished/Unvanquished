#! /usr/bin/python

# generate the linzer-feig multiply-add idct for ia64
# (c) 2002 Christian Schwarz <schwarz@ira.uka.de>,
#          Haiko Gaisser <haiko@gaisser.de>,
#          Sebastian Hack <mail@s-hack.de>


import math

pre_shuffle = [ 0, 4, 2, 6, 1, 7, 3, 5 ]
post_shuffle = [ 0, 1, 6, 3, 7, 2, 5, 4 ]

constants = 16
float_scratch = range(32, 32+constants)
regbase = max(float_scratch)+1
intregbase = 33

def print_matrix(matrix,s=''):
    if s != '':
        print "\n\t// %s" % s
    for i in range(0, 8):
        print "\t// ",
        for j in range(0, 4):
            print "%2d" % matrix[i*4+j],
        print ""

def exchange_elements(list, a, b):
    """ Exchange two list elements
    """
    (list[a], list[b]) = (list[b], list[a])

def alloc_regs(matrix, n):
    """ get the smallest register not used by the matrix
    """
    
    regs = [ ]
    for i in range(0, n):
        m = regbase
        while m in matrix or m in regs:
            m = m + 1
        regs.append(m)
    return regs

def transpose_2x2_submatrix(matrix, i, j):
    """ transpose a 2x2 submatrix in the 8x8 matrix
    """
    a = j
    b = i

    tmp = matrix[i*8+j]
    matrix[i*8+j] = matrix[a*8+b]
    matrix[a*8+b] = tmp

    tmp = matrix[i*8+j+4]
    matrix[i*8+j+4] = matrix[a*8+b+4]
    matrix[a*8+b+4] = tmp


def transpose(matrix):
    """ register renaming for transpose
    """
    regs = alloc_regs(matrix, 16)
    save_regs = regs[:]

    # emit code ...
    for i in range(1,8,2):
        for j in range(0,4):
            r1 = matrix[(i-1)*4+j]
            r2 = matrix[i*4+j]
            print '\tfmix.r  f%d = f%d, f%d' % (save_regs.pop(0), r1, r2)

    print '\t;;'

    for i in range(0,8,2):
        for j in range(0,4):
            r1 = matrix[i*4+j]
            r2 = matrix[(i+1)*4+j]
            print '\tfmix.l  f%d = f%d, f%d' % (r1, r1, r2)

    print '\t;;'

    # first stage, transpose the 2x2 matrices
    for i in range(1,8,2):
        for j in range(0,4):
            r = matrix[i*4+j]
            matrix[i*4+j] = regs.pop(0)

#    print_matrix(matrix)

    # exchange the 2x2 matrices by renaming the registers
    for i in range(0, 4):
        for j in range(i+1, 4):
            transpose_2x2_submatrix(matrix, i, j)

#    print ''
#    print_matrix(matrix)
#    print "transpose"
#    print_matrix(matrix)

# register renaming for 8 regs containing a column
def shuffle_column(matrix, col, permutation):
    l = [ ]
    for i in range(0,8):
        l.append(matrix[i*4+col])
    for i in range(0,8):
        matrix[i*4+col] = l[permutation[i]]

def butterfly(matrix, col, i, j, c1, c2):
    """ register renaming for a butterfly operation in a column
    """
    ri = matrix[i*4+col]
    rj = matrix[j*4+col]
    regs = alloc_regs(matrix, 1)

    print '\t// (f%d, f%d) = (f%d, f%d) $ (%s, %s), (line %d, %d)' % \
          (regs[0], rj, ri, rj, c1, c2, i, j)
    print '\tfpma    f%d = f%d, %s, f%d' % (regs[0], rj, c1, ri)
    print '\tfpnma   f%d = f%d, %s, f%d' % (rj, rj, c2, ri)
    print '\t;;'
    
    matrix[i*4+col] = regs[0]


def column_idct(matrix, col):

    print_matrix(matrix, "before pre shuffle")
    shuffle_column(matrix, col, pre_shuffle)
    print_matrix(matrix, "after pre shuffle")

    butterfly(matrix, col, 0, 1, 'c0', 'c0')
    butterfly(matrix, col, 2, 3, 'c1', 'c2')
    butterfly(matrix, col, 4, 5, 'c3', 'c4')
    butterfly(matrix, col, 6, 7, 'c5', 'c6')
    print '\t;;'
    butterfly(matrix, col, 0, 3, 'c7', 'c7')
    butterfly(matrix, col, 1, 2, 'c8', 'c8')
    butterfly(matrix, col, 4, 6, 'c9', 'c9')
    butterfly(matrix, col, 5, 7, 'c10', 'c10')
    print '\t;;'
    butterfly(matrix, col, 5, 6, 'c11', 'c11')
    butterfly(matrix, col, 0, 4, 'c12', 'c12')
    butterfly(matrix, col, 3, 7, 'c14', 'c14')
    print '\t;;'
    butterfly(matrix, col, 1, 5, 'c13', 'c13')
    butterfly(matrix, col, 2, 6, 'c13', 'c13')
    
    print_matrix(matrix, "before post shuffle")
    shuffle_column(matrix, col, post_shuffle)
    print_matrix(matrix, "after post shuffle")

def gen_idct(matrix):

    for j in range(0, 2):
        for i in range(0, 4):
           print '\tfpma    f%d = f%d, c0, f0' \
                 % (2 * (matrix[i],))
        print '\t;;'
        for i in range(0,4):
            column_idct(matrix, i)
        print '\t;;'
        transpose(matrix)

def gen_consts():
    print 'addreg1 = r14'
    print 'addreg2 = r15'
    
    for i in range(0, constants):
        print 'c%d = f%d' % (i, float_scratch.pop(0))

    sqrt2 = math.sqrt(2.0)
    t = [ ]
    s = [ ]
    c = [ ]
    for i in range(0,5):
        t.append(math.tan(i * math.pi / 16))
        s.append(math.sin(i * math.pi / 16))
        c.append(math.cos(i * math.pi / 16))
        
    consts = [ ]
    consts.append(1.0 / (2.0 * sqrt2))
    consts.append(-1 / t[2])
    consts.append(-t[2])
    consts.append(t[1])
    consts.append(1 / t[1])
    consts.append(t[3])
    consts.append(1 / t[3])
    consts.append(0.5 * c[2])
    consts.append(0.5 * s[2])
    consts.append(c[3] / c[1])
    consts.append(s[3] / s[1])
    consts.append(c[1] / s[1])
    consts.append(0.5 * c[1])
    consts.append(0.5 * s[1] * c[4])
    consts.append(0.5 * s[1])
    consts.append(1.0)

    print '.sdata'
    for i in range(0, constants):
        if i % 2 == 0:
            print '.align 16'
        print '.data_c%d:' % i
        print '.single %.30f, %.30f' % (consts[i], consts[i])
    print ''

def gen_load(matrix):
    
    for i in range(0, 64, 2):
        print '\tld2  r%d = [addreg1], 4' % (intregbase+i)
        print '\tld2  r%d = [addreg2], 4' % (intregbase+i+1)
        print '\t;;'

    for i in range(0, 64, 2):
        print '\tsxt2  r%d = r%d' % (2*(intregbase+i,))
        print '\tsxt2  r%d = r%d' % (2*(intregbase+i+1,))
    print '\t;;'
        
    for i in range(0, 64, 2):
        print '\tsetf.sig  f%d = r%d' % (regbase+i, intregbase+i)
        print '\tsetf.sig  f%d = r%d' % (regbase+i+1, intregbase+i+1)
    print '\t;;'

    for i in range(0, 64, 2):
        print '\tfcvt.xf  f%d = f%d' % (2*(regbase+i,))
        print '\tfcvt.xf  f%d = f%d' % (2*(regbase+i+1,))
    print '\t;;'

    for i in range(0, 32):
        print '\tfpack    f%d = f%d, f%d' \
              % (regbase+i, regbase+2*i, regbase+2*i+1)
        print '\t;;'

    """
    for i in range(0, len(matrix)):
        print '\tld2  r18 = [addreg1], 4' 
        print '\tld2  r19 = [addreg2], 4'
        print '\t;;'
        print '\tsxt2 r18 = r18'
        print '\tsxt2 r19 = r19'
        print '\t;;'
        print '\tsetf.sig f18 = r18'
        print '\tsetf.sig f19 = r19'
        print '\t;;'
        print '\tfcvt.xf  f18 = f18'
        print '\tfcvt.xf  f19 = f19'
        print '\t;;'
        print '\tfpack      f%d = f18, f19' % (matrix[i])
        print '\t;;'
    """
        
def gen_store(matrix):
    print '\tmov   addreg1 = in0'
    print '\tadd   addreg2 = 4, in0'
    print '\t;;'

    for i in range(0, len(matrix)):
        print '\tfpcvt.fx f%d = f%d' % (2*(matrix[i],))
    print '\t;;'

    for i in range(0, len(matrix)):
        print '\tgetf.sig r%d = f%d' % (intregbase+i, matrix[i])
    print '\t;;'

    for i in range(0, len(matrix)):
        print '\tshl      r%d = r%d, 7' % (2*(intregbase+i,))
    print '\t;;'

    for i in range(0, len(matrix)):
        print '\tpack4.sss r%d = r%d, r0' % (2*(intregbase+i,))
    print '\t;;'

    for i in range(0, len(matrix)):
        print '\tpshr2    r%d = r%d, 7' % (2*(intregbase+i,))
    print '\t;;'

    for i in range(0, len(matrix)):
        print '\tmux2     r%d = r%d, 0xe1' % (2*(intregbase+i,))
    print '\t;;'

    for i in range(0, len(matrix), 2):
        print '\tst4   [addreg1] = r%d, 8' % (intregbase+i)
        print '\tst4   [addreg2] = r%d, 8' % (intregbase+i+1)
	print '\t;;'
    
def main():
    gen_consts()

    print '.text'
    print '.global idct_ia64'
    print '.global idct_ia64_init'
    print '.align 16'
    print '.proc idct_ia64_init'
    print 'idct_ia64_init:'
    print 'br.ret.sptk.few b0'
    print '.endp'
    print '.align 16'
    print '.proc idct_ia64'
    print 'idct_ia64:'

    f = open('idct_init.s')
    print f.read()
    f.close()
    
    matrix = [ ]
    for i in range(0,32):
        matrix.append(regbase + i)

    gen_load(matrix)
#    print_matrix(matrix)
    gen_idct(matrix)
#    transpose(matrix)
    print_matrix(matrix)
    gen_store(matrix)

    f = open('idct_fini.s')
    print f.read()
    f.close()
    
    print '.endp'


if __name__ == "__main__":
    main()
