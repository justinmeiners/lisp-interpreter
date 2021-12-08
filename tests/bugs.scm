
; add a test for every bug that is incountered
; to avoid recreating it in the future

(define-macro => 
   (lambda (test expected)
      `(assert (equal? ,test (quote ,expected))) ))

; test basic vector creation and operations
(define v #(1 2 3 4 5 6 7 8 9 10))

(define (sum-to-n n) (/ (* n (+ n 1)) 2))

(define (sum-vector v)
  (define (iter sum i)
    (if (= i (vector-length v))
        sum
        (iter (+ sum (vector-ref v i)) (+ i 1))))
  (iter 0 0))

(display "Sum to 10: ")
(display (sum-vector v))
(newline)
(assert (= (sum-to-n 10) (sum-vector v)))

(define big-v #(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200))

(display (vector-length big-v))

(=> (vector-length big-v) 200)

; procedures with no arguments don't expand properly

(define (hello-world) (display "hello world") (newline))
(hello-world)

; vector and list assoc
(define vec-map #((bob . 1) (john . 2) (dan . 3) (alice . 4)))
(define list-map '((bob . 1) (john . 2) (dan . 3) (alice . 4)))

(assert (= (cdr (vector-assq 'john vec-map)) 2))
(assert (= (cdr (vector-assq 'alice vec-map)) 4))
(assert (null? (vector-assq 'bad-key vec-map)))

(assert (= (cdr (assoc 'john list-map)) 2))
(assert (= (cdr (assoc 'alice list-map)) 4))
(assert (null? (assoc 'bad-key list-map)))

(assert
  (= (do ((i 1 (+ i 1)) (n 0 n))
        ((> i 10) n)
        (set! n  (+ i n)))
      (* 5 11)))

(=> (subvector #(1 2 3 4) 1 4) #(2 3 4))
(=> (subvector #(1 2 3 4) 0 2) #(1 2))
(=> (subvector #(A 1 A 1 A 1 A 1) 1 3) #(1 A))

(let ((sym (gensym)))
   (assert (eq? sym sym)))
