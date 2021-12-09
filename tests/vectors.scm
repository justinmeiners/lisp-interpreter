(define v #(1 2 3))
(vector-swap! v 0 2)
(assert (= 3 (vector-ref v 0)))
(assert (= 1 (vector-ref v 2)))

(define (vec-sorted? v op)
 ; "if x and y are any two adjacent elements in the result,
 ;  where x precedes y, it is the case that (procedure y x) => #f"
 ; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_8.html#SEC72

 (or (< (vector-length v) 2)
  (and (not (op (vector-ref v 1) (vector-ref v 0)))
   (vec-sorted? (vector-tail v 1) op))))

; First make sure our sorted checker works
(assert (vec-sorted? #(1 2 2 4 5 6) <))
(assert (vec-sorted? #(1) <))
(assert (vec-sorted? #(1 2) <))
(assert (vec-sorted? #(7 6 5 4 3 2 1) >))
(assert (not (vec-sorted? #(2 1) <)))
(assert (not (vec-sorted? #(1 2 3 4 4 3) <)))
(assert (not (vec-sorted? #(1 2 3 2 4 5) <)))

; Now test the sort function
(assert (vec-sorted? (sort! #(1) <) <))
(assert (vec-sorted? (sort! #(2 1) <) <))
(assert (vec-sorted? (sort! #(1 2 3) <) <))
(assert (vec-sorted? (sort! #(3 8 1 7 2 9 4 5) <) <))
(assert (vec-sorted? (sort! #(1 2 3 4 5 6 7 8) <) <))
(assert (vec-sorted? (sort! #(3 8 1 7 2 9 4 5) >) >))
(assert (vec-sorted? (sort! #(1 2 3 4 5 6 7 8) >) >))
(assert (vec-sorted? (sort! #(92 59 30 57 74 78 43 33 77 10 78 83 76 49 42 94 82 70 15 11 90 86 44 70 39 64 69 30 59 95 15 79 13 54 98 82 42 96 79 17 56 93 20 1 84 72 75 19 74 43) >) >))
(assert (vec-sorted? (sort! #(92 59 30 57 74 78 43 33 77 10 78 83 76 49 42 94 82 70 15 11 90 86 44 70 39 64 69 30 59 95 15 79 13 54 98 82 42 96 79 17 56 93 20 1 84 72 75 19 74 43) <) <))
(assert (vec-sorted? (sort! #(3 8 1 7 2 9 4 5) <) <))

; Try other data types
(assert (vec-sorted? (sort! #(#\C #\B #\A #\D) char<?) char<?))

; Converting between lists and vectors
;https://www.gnu.org/software/mit-scheme/documentation/stable/mit-scheme-ref/Construction-of-Vectors.html
(=> (vector 'a 'b 'c) #(A B C))
(=> (list->vector '(dididit dah)) #(dididit dah))

; Binary serach
(assert (= (vector-binary-search #(1 2 3 4 5) < (lambda (x) x) 3) 3))
(assert (null? (vector-binary-search #(1 2 2 4 5) < (lambda (x) x) 3)))

(define v (vector 1 1 2))
(vector-fill! v 3)
(=> v #(3 3 3))

(=> (make-initialized-vector 5 (lambda (x) (* x x))) #(0 1 4 9 16))
 
(=> (vector-head #(1 2 3) 2) #(1 2))

; Issues parsing large vector
(define big-v #(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200))

(=> (vector-length big-v) 200)

; Subvector 
(=> (subvector #(1 2 3 4) 1 4) #(2 3 4))
(=> (subvector #(1 2 3 4) 0 2) #(1 2))
(=> (subvector #(A 1 A 1 A 1 A 1) 1 3) #(1 A))
 
