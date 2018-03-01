BEGIN {
    printf ".SUFFIXES: .1 .fmt\n";
    printf ".1.fmt:\n";
    printf "\tformat $*.1 >$*.fmt\n";
}
{
    file[nfile++] = $1;
}
END {
    len = 8;
    printf "make_fmt: ";
    for(j = 0; j < nfile; j++) {
	if (len + length(file[j])+5 > 75) {
	    printf " \\\n\t";
	    len = 8;
	}
	len += length(file[j]) + 5;
	printf "%s.fmt ", file[j];
    }
    printf "\n";

    for(j = 0; j < nfile; j++) {
	printf "%s.fmt: %s.1 header.me\n", file[j], file[j];\
    }

    len = 14;
    printf "sis.1: sis.man ";
    for(j = 0; j < nfile; j++) {
	if (len + length(file[j])+3 > 75) {
	    printf " \\\n\t";
	    len = 8;
	}
	len += length(file[j]) + 3;
	printf "%s.1 ", file[j];
    }
    printf "\n";
}
