#include "lib/kernel_crc8.h"

/*
 * crc8_populate_msb - fill crc table for given polynomial in reverse bit order.
 *
 * table:	table to be filled.
 * polynomial:	polynomial for which table is to be filled.
 */
void crc8_populate_msb(u8 table[CRC8_TABLE_SIZE], u8 polynomial)
{
	int i, j;
	const u8 msbit = 0x80;
	u8 t = msbit;

	table[0] = 0;

	for (i = 1; i < CRC8_TABLE_SIZE; i *= 2) {
		t = (t << 1) ^ (t & msbit ? polynomial : 0);
		for (j = 0; j < i; j++)
			table[i+j] = table[j] ^ t;
	}
}


/*
 * crc8_populate_lsb - fill crc table for given polynomial in regular bit order.
 *
 * table:	table to be filled.
 * polynomial:	polynomial for which table is to be filled.
 */
void crc8_populate_lsb(u8 table[CRC8_TABLE_SIZE], u8 polynomial)
{
	int i, j;
	u8 t = 1;

	table[0] = 0;

	for (i = (CRC8_TABLE_SIZE >> 1); i; i >>= 1) {
		t = (t >> 1) ^ (t & 1 ? polynomial : 0);
		for (j = 0; j < CRC8_TABLE_SIZE; j += 2*i)
			table[i+j] = table[j] ^ t;
	}
}


/**************************************************
*	poly = x^8 + x^7 + x^6 + x^4 + x^2 + 1
*
* For msb first direction x^7 maps to the msb. So the polynomial is as below.
*
* - msb first: poly = (1)11010101 = 0xD5
***************************************************/
const u8 defalut_msb_table[CRC8_TABLE_SIZE] = {
0	,
213 ,
127 ,
170 ,
254 ,
43  ,
129 ,
84  ,
41  ,
252 ,
86  ,
131 ,
215 ,
2   ,
168 ,
125 ,
82  ,
135 ,
45  ,
248 ,
172 ,
121 ,
211 ,
6   ,
123 ,
174 ,
4   ,
209 ,
133 ,
80  ,
250 ,
47  ,
164 ,
113 ,
219 ,
14 	,
90  ,
143 ,
37  ,
240 ,
141 ,
88  ,
242 ,
39  ,
115 ,
166 ,
12 	,
217 ,
246 ,
35  ,
137 ,
92  ,
8   ,
221 ,
119 ,
162 ,
223 ,
10  ,
160 ,
117 ,
33  ,
244 ,
94  ,
139 ,
157 ,
72  ,
226 ,
55  ,
99  ,
182 ,
28 	,
201 ,
180 ,
97  ,
203 ,
30 	,
74  ,
159 ,
53  ,
224 ,
207 ,
26 	,
176 ,
101 ,
49  ,
228 ,
78  ,
155 ,
230 ,
51  ,
153 ,
76  ,
24 	,
205 ,
103 ,
178 ,
57  ,
236 ,
70  ,
147 ,
199 ,
18 	,
184 ,
109 ,
16 	,
197 ,
111 ,
186 ,
238 ,
59  ,
145 ,
68  ,
107 ,
190 ,
20 	,
193 ,
149 ,
64  ,
234 ,
63  ,
66  ,
151 ,
61  ,
232 ,
188 ,
105 ,
195 ,
22 	,
239 ,
58  ,
144 ,
69  ,
17 	,
196 ,
110 ,
187 ,
198 ,
19 	,
185 ,
108 ,
56  ,
237 ,
71  ,
146 ,
189 ,
104 ,
194 ,
23 	,
67  ,
150	,
60  ,
233 ,
148	,
65  ,
235 ,
62  ,
106 ,
191 ,
21 	,
192 ,
75  ,
158	,
52  ,
225	,
181	,
96  ,
202	,
31 	,
98  ,
183	,
29 	,
200	,
156	,
73  ,
227	,
54  ,
25 	,
204	,
102	,
179	,
231	,
50  ,
152	,
77  ,
48  ,
229	,
79  ,
154	,
206	,
27 	,
177	,
100	,
114	,
167	,
13 	,
216	,
140	,
89  ,
243	,
38  ,
91  ,
142	,
36  ,
241	,
165	,
112	,
218	,
15 	,
32  ,
245	,
95  ,
138	,
222	,
11 	,
161	,
116	,
9  ,
220	,
118	,
163	,
247	,
34  ,
136	,
93  ,
214	,
3  ,
169	,
124	,
40  ,
253	,
87  ,
130	,
255	,
42  ,
128	,
85  ,
1  ,
212	,
126	,
171	,
132	,
81  ,
251	,
46  ,
122	,
175	,
5  ,
208	,
173	,
120	,
210	,
7  ,
83  ,
134	,
44  ,
249	
};
/*
 * crc8 - calculate a crc8 over the given input data.
 *
 * table: crc table used for calculation.
 * pdata: pointer to data buffer.
 * nbytes: number of bytes in data buffer.
 * crc:	previous returned crc8 value.
 */
u8 crc8(const u8 table[CRC8_TABLE_SIZE], u8 *pdata, size_t nbytes, u8 crc)
{
	/* loop over the buffer data */
	while (nbytes-- > 0)
		crc = table[(crc ^ *pdata++) & 0xff];

	return crc;
}

