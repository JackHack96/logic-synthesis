.sz +2
.ce 3
\fBBerkeley PLA Test Set Results\fP
\fBJune 5, 1986\fP
\fB(revised January 30, 1988)\fP
.sz -2
.PP
These tables present the results of Boolean minimization
for the 145 PLA's in the Berkeley PLA test set.  The cost
function is assumed to be minimum number of terms with only
a secondary concern given to the number of literals.
Each example is classified as one of 3 types:
.TS
center box;
c|c
l|l.
type	description
_
\fIindust\fP	example donated from actual chip designs
\fImath\fP	mathematical function
\fIrandom\fP	randomly generated example
.TE
.PP
.sp
Each example also belongs to one of 5 categories, which
measures the relative difficulty of the problem:
.TS
center box;
c|c
l|l.
class	description
_
\fItrivial\fP	minimum solution consists of essential prime implicants
\fInoncyclic\fP	the covering problem contains no cyclic constraints
\fIcyclic-s\fP	the covering problem contains cyclic constraints,
	and the covering problem has been solved
\fIcyclic-us\fP	the covering problem contains cyclic constraints,
	and the covering problem as not been solved
\fIprimes\fP	unable to enumerate all prime implicants
.TE
.sp
These classifications were determined by using the exact minimization
algorithms of Espresso-MV as well as the exact minimization algorithm
of McBoole.  The classifications of \fIcyclic-us\fP
and \fIprimes\fP are dependent on the exact
minimization algorithms which were used.
For example,
although we know the minimum solution for \fIZ9sym\fP and \fIibm\fP (by methods
not involving the use of an exact minimization algorithm)
these examples are still classified as \fIcyclic-us\fP
and \fIprimes\fP respectively
because the exact minimization algorithm was unable
to determine the minimum solution.
(Note 1/28/88: \fIZ9SYM\fP has been moved to \fIcyclic-s\fP because
the new covering algorithm has directly derived the minimum solution.)
.PP
For each example, we first give the number of inputs, the number of
outputs, and the number of terms in the initial representation of
the function.  If the number of terms is marked by *, then there
is a don't-care set specified for the function (which is not counted
in the initial number of terms).
.PP
We then present the number of prime implicants (when known),
the number of essential primes, and the minimum solution (when known).
When the minimum solution is not known for the class \fIcyclic-us\fP
a lower bound (as determined by the covering algorithm of
Espresso-MV) and an upper bound (the best solution we've seen)
are given.  For the class \fIprimes\fP, the lower bound is merely
the number of essential prime implicants,
and the upper bound is the best solution we've seen.
For the examples \fIex1010\fP and \fIexam\fP the best results have
been reported by the authors Prestol-II, and we have not seen or
verified the results.  In fact, I am beginning to have my doubts as
to the validity of these results.
.PP
These tables also present the results for Espresso-MV in both its normal
mode (Esp.) and its \fIstrong\fP mode (Esp. (s)).
.PP
.bp
.TS H
center box;
c2|c2 c2|c2 c2|c2 c2|c2|c2 c2
^ |^  ^ |^  ^ |c  c |c |c  c
l |r  r |l  l |n  n |n |n  n.
name	in/out	terms	type	class	#	#	minimum	Esp.	Esp.
					primes	essen.	solution		(s)
_
.TH
alu1	12/8	 19	indust	trivial	780	19	19	19	19
bcd.div3	4/4	* 9	math	trivial	13	9	9	9	9
clpl	11/5	 20	indust	trivial	143	20	20	20	20
co14	14/1	 14	math	trivial	14	14	14	14	14
max46	9/1	 46	indust	trivial	49	46	46	46	46
newapla2	6/7	 7	indust	trivial	7	7	7	7	7
newbyte	5/8	 8	indust	trivial	8	8	8	8	8
newtag	8/1	 8	indust	trivial	8	8	8	8	8
ryy6	16/1	 112	indust	trivial	112	112	112	112	112
_
add6	12/7	 1092	math	noncyclic	8568	153	355	355	355
adr4	8/5	 255	math	noncyclic	397	35	75	75	75
al2	16/47	 103	indust	noncyclic	9179	16	66	66	66
alcom	15/38	 47	indust	noncyclic	4657	16	40	40	40
alu2	10/8	* 87	indust	noncyclic	434	36	68	68	68
alu3	10/8	* 68	indust	noncyclic	540	27	64	66	64
apla	10/12	* 112	indust	noncyclic	201	0	25	25	25
b4	33/23	* 54	indust	noncyclic	6455	40	54	54	54
b11	8/31	* 74	indust	noncyclic	44	22	27	27	27
b2	16/17	 110	indust	noncyclic	928	54	104	106	104
b7	8/31	* 74	indust	noncyclic	44	22	27	27	27
b9	16/5	 123	indust	noncyclic	3002	48	119	119	119
bc0	26/11	 419	indust	noncyclic	6596	37	177	178	177
bca	26/46	* 301	indust	noncyclic	305	144	180	180	180
bcb	26/39	* 299	indust	noncyclic	255	137	155	156	155
bcd	26/38	* 243	indust	noncyclic	172	100	117	117	117
br1	12/8	 34	indust	noncyclic	29	17	19	19	19
br2	12/8	 35	indust	noncyclic	27	9	13	13	13
dc1	4/7	 15	indust	noncyclic	22	3	9	9	9
dc2	8/7	 58	indust	noncyclic	173	18	39	39	39
dk17	10/11	* 57	indust	noncyclic	111	0	18	18	18
ex7	16/5	 123	indust	noncyclic	3002	48	119	119	119
exep	30/63	* 149	indust	noncyclic	558	82	108	108	108
exp	8/18	* 89	indust	noncyclic	238	30	56	59	56
in1	16/17	 110	indust	noncyclic	928	54	104	106	104
in3	35/29	 75	indust	noncyclic	1114	44	74	74	74
in5	24/14	 62	indust	noncyclic	1067	53	62	62	62
in6	33/23	 54	indust	noncyclic	6174	40	54	54	54
in7	26/10	 84	indust	noncyclic	2112	31	54	54	54
life	9/1	 140	math	noncyclic	224	56	84	84	84
luc	8/27	 27	indust	noncyclic	190	14	26	26	26
m1	6/12	 32	indust	noncyclic	59	6	19	19	19
newapla	12/10	 17	indust	noncyclic	113	9	17	17	17
newapla1	12/7	 10	indust	noncyclic	31	9	10	10	10
newcond	11/2	 31	indust	noncyclic	72	18	31	31	31
newcpla2	7/10	 19	indust	noncyclic	38	14	19	19	19
newcwp	4/5	 11	indust	noncyclic	23	7	11	11	11
newtpla	15/5	 23	indust	noncyclic	40	16	23	23	23
newtpla1	10/2	 4	indust	noncyclic	6	3	4	4	4
newtpla2	10/4	 9	indust	noncyclic	23	4	9	9	9
newxcpla1	9/23	 40	indust	noncyclic	191	18	39	39	39
p1	8/18	* 89	random	noncyclic	287	25	54	55	54
p3	8/14	* 66	random	noncyclic	185	22	39	39	39
p82	5/14	 24	indust	noncyclic	48	16	21	21	21
prom1	9/40	 502	indust	noncyclic	9326	182	472	472	472
radd	8/5	 120	math	noncyclic	397	35	75	75	75
rckl	32/7	 96	math	noncyclic	302	6	32	32	32
rd53	5/3	 31	math	noncyclic	51	21	31	31	31
rd73	7/3	 147	math	noncyclic	211	106	127	127	127
risc	8/31	 74	indust	noncyclic	46	22	28	28	28
sex	9/14	 23	indust	noncyclic	99	13	21	21	21
sqn	7/3	 84	indust	noncyclic	75	23	38	38	38
t2	17/16	* 128	indust	noncyclic	233	25	52	53	53
t3	12/8	 148	indust	noncyclic	42	30	33	33	33
t4	12/8	* 38	indust	noncyclic	174	0	16	16	16
vg2	25/8	 110	indust	noncyclic	1188	100	110	110	110
vtx1	27/6	 110	indust	noncyclic	1220	100	110	110	110
x1dn	27/6	 112	indust	noncyclic	1220	100	110	110	110
x9dn	27/7	 120	indust	noncyclic	1272	110	120	120	120
z4	7/4	 127	math	noncyclic	167	35	59	59	59
_
Z5xp1	7/10	 128	math	cyclic-s	390	8	63	63	64
Z9sym	9/1	 420	math	cyclic-s	1680	0	84	85	84
addm4	9/8	 480	math	cyclic-s	1122	24	189	200	192
amd	14/24	 171	indust	cyclic-s	457	32	66	66	66
b10	15/11	* 135	indust	cyclic-s	938	51	100	100	100
b12	15/9	 431	indust	cyclic-s	1490	2	41	42	41
b3	32/20	* 234	indust	cyclic-s	3056	123	210	211	211
bcc	26/45	* 245	indust	cyclic-s	237	119	137	137	137
bench	6/8	* 31	random	cyclic-s	391	0	16	18	17
chkn	29/7	 153	indust	cyclic-s	671	86	140	140	140
cps	24/109	 654	indust	cyclic-s	2487	57	157	163	159
dekoder	4/7	* 10	indust	cyclic-s	26	3	9	9	9
dist	8/5	 255	math	cyclic-s	401	23	120	121	121
dk27	9/9	* 20	indust	cyclic-s	82	0	10	10	10
dk48	15/17	* 42	indust	cyclic-s	157	0	21	22	22
exps	8/38	* 196	indust	cyclic-s	852	56	132	134	133
f51m	8/8	 255	math	cyclic-s	561	13	76	77	76
fout	6/10	* 61	random	cyclic-s	436	2	40	44	42
gary	15/11	 214	indust	cyclic-s	706	60	107	107	107
in0	15/11	 135	indust	cyclic-s	706	60	107	107	107
in2	19/10	 137	indust	cyclic-s	666	85	134	136	134
in4	32/20	 234	indust	cyclic-s	3076	118	211	212	212
inc	7/9	* 34	indust	cyclic-s	124	12	29	30	29
intb	15/7	 664	indust	cyclic-s	6522	186	629	629	629
l8err	8/8	* 253	math	cyclic-s	142	15	50	51	51
lin.rom	7/36	 128	indust	cyclic-s	1087	8	128	128	128
log8mod	8/5	 46	math	cyclic-s	105	13	38	38	38
m181	15/9	 430	math	cyclic-s	1636	2	41	42	41
m2	8/16	 96	indust	cyclic-s	243	7	47	47	47
m3	8/16	 128	indust	cyclic-s	344	4	62	65	63
m4	8/16	 256	indust	cyclic-s	670	11	101	107	104
mark1	20/31	* 23	indust	cyclic-s	208	1	19	19	19
max128	7/24	 128	indust	cyclic-s	469	6	78	82	79
max512	9/6	 512	indust	cyclic-s	535	20	133	142	137
mlp4	8/8	 225	math	cyclic-s	606	12	121	128	127
mp2d	14/14	 123	indust	cyclic-s	469	13	30	31	31
newcpla1	9/16	 38	indust	cyclic-s	170	22	38	38	38
newill	8/1	 8	indust	cyclic-s	11	5	8	8	8
opa	17/69	 342	indust	cyclic-s	477	22	77	79	79
pope.rom	6/48	 64	indust	cyclic-s	593	12	59	62	59
root	8/5	 255	math	cyclic-s	152	9	57	57	57
spla	16/46	* 2296	indust	cyclic-s	4972	33	248	262	260
sqr6	6/12	 63	math	cyclic-s	205	3	47	49	49
sym10	10/1	 837	math	cyclic-s	3150	0	210	210	210
t1	21/23	 796	indust	cyclic-s	15135	7	100	102	102
test1	8/10	* 209	random	cyclic-s	2407	0	110	123	115
tial	14/8	 640	math	cyclic-s	7145	220	575	579	579
tms	8/16	 30	indust	cyclic-s	162	13	30	30	30
wim	4/7	* 10	indust	cyclic-s	25	3	9	9	9
x6dn	39/5	 121	indust	cyclic-s	916	60	81	81	81
_
bench1	9/9	* 285	random	cyclic-us	5972	0	111/125	136	128
ex5	8/63	 256	indust	cyclic-us	2532	28	62/67	74	72
exam	10/10	* 410	random	cyclic-us	4955	0	54/63	67	66
max1024	10/6	 1024	indust	cyclic-us	1278	14	249/261	274	267
prom2	9/21	 287	indust	cyclic-us	2635	9	276/287	287	287
test4	8/30	* 256	random	cyclic-us	6139	0	0/104	122	104
_
accpla	50/69	 183	indust	primes	?	97	97/175	175	175
ex1010	10/10	* 810	random	primes	?	0	0/246	283	264
ex4	128/28	 620	indust	primes	?	138	138/279	279	279
ibm	48/17	 173	indust	primes	?	172	173/173	173	173
jbp	36/57	 166	indust	primes	?	0	0/122	122	122
mainpla	27/54	 181	indust	primes	?	29	29/172	172	172
misg	56/23	 75	indust	primes	?	3	3/69	69	69
mish	94/43	 91	indust	primes	?	3	3/82	82	82
misj	35/14	 48	indust	primes	?	13	13/35	35	35
pdc	16/40	* 2406	indust	primes	?	2	2/100	125	121
shift	19/16	 100	indust	primes	?	100	100/100	100	100
signet	39/8	 124	indust	primes	?	104	104/119	119	119
soar.pla	83/94	 529	indust	primes	?	2	2/352	352	352
test2	11/35	* 1999	random	primes	?	0	0/995	1105
test3	10/35	* 1003	random	primes	?	0	0/491	543	491
ti	47/72	 241	indust	primes	?	46	46/213	213	213
ts10	22/16	 128	indust	primes	?	128	128/128	128	128
x2dn	82/56	 112	indust	primes	?	2	2/104	104	104
x7dn	66/15	 622	indust	primes	?	378	378/538	538	538
xparc	41/73	 551	indust	primes	?	140	140/254	254	254
.TE
.bp
New results: 8 of the previous 14 cyclic-unsolved examples have been solved
using the new covering algorithm in Espresso Version 2.3.
For reference, here are the results as
given in the previous report dated June 5, 1986:
.TS
center box;
c2|c2 c2|c2 c2|c2 c2|c2|c2 c2
^ |^  ^ |^  ^ |c  c |c |c  c
l |r  r |l  l |n  n |n |n  n.
name	in/out	terms	type	class	#	#	minimum	Esp.	Esp.
					primes	essen.	solution		(s)
_
Z9sym	9/1	 420	math	cyclic-us	1680	0	84/84	85	84
b4	33/23	* 54	indust	cyclic-us	6455	40	40/54	54	54
bc0	26/11	 419	indust	cyclic-us	6596	37	37/177	178	177
lin.rom	7/36	 128	indust	cyclic-us	1087	8	125/128	128	128
spla	16/46	* 2296	indust	cyclic-us	4972	33	33/251	262	260
t1	21/23	 796	indust	cyclic-us	15135	7	7/102	102	102
test1	8/10	* 209	random	cyclic-us	2407	0	103/111	123	115
tial	14/8	 640	math	cyclic-us	7145	220	220/575	579	579
.TE
.PP
Also, several of the lower-bounds for the \fBcyclic-us\fP examples have
been improved.  
The new results for examples ex5, exam, max1024, and prom2 are due
to Kong-Ngai Wong, MIMOS, Malaysia.
I have discredited the upper-bound result for exam given by Marc
Bartholomeus because I was unable to duplicate his result using
his program.
For reference, here are the previously reported results:
.TS
center box;
c2|c2 c2|c2 c2|c2 c2|c2|c2 c2
^ |^  ^ |^  ^ |c  c |c |c  c
l |r  r |l  l |n  n |n |n  n.
name	in/out	terms	type	class	#	#	minimum	Esp.	Esp.
					primes	essen.	solution		(s)
_
bench1	9/9	* 285	random	cyclic-us	5972	0	111/126	136	128
ex5	8/63	 256	indust	cyclic-us	2532	28	59/67	74	72
exam	10/10	* 410	random	cyclic-us	4955	0	52/59	67	66
max1024	10/6	 1024	indust	cyclic-us	1278	14	239/267	274	267
prom2	9/21	 287	indust	cyclic-us	2635	9	274/287	287	287
t1	21/23	 796	indust	cyclic-us	15135	7	7/102	102	102
test4	8/30	* 256	random	cyclic-us	6139	0	0/104	122	104
.TE
