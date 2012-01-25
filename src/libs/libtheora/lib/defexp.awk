# awk script to convert symbol export table formats

# converts an msvc .def file to an darwin ld export-symbols-list file
# we only support the most basic module definition syntax

# skip comments
/^\w*#.*/ {next}
/^\w*;.*/ {next}

# remember and propagate the library name
/LIBRARY/ {name = $2; print "# export list for", name; next}

# skip various other lines
/^\w*NAME/ ||
/^\w*VERSION/ ||
/^\w*EXPORTS/ ||
/^\w*HEAPSIZE/ ||
/^\w*STACKSIZE/ ||
/^\w*STUB/ {next}

# todo: handle SECTIONS

# for symbols, strip the semicolon and mangle the name
/[a-zA-Z]+/ {sub(/\;/, ""); print "_" $1}

# todo: warn if we see publicname=privatename mappings
#       which other linkers don't support
