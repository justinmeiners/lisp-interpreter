; copy examples from MIT scheme documentation and add related ones.

; Conditionals
; https://www.gnu.org/software/mit-scheme/documentation/stable/mit-scheme-ref/Conditionals.html

(assert '())
(assert (and (= 2 2) (> 2 1)))
(assert (and))
 
(assert (equal? (reverse '(a b c)) '(c b a)))

; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_8.html
(assert (pair? '(a . b)))
(assert (pair? '(a b c)))

(assert (not (pair? '())))
(assert (not (pair? '#(a b))))

(assert (= (length '(a b c)) 3))
(assert (= (length '()) 0))

(assert (not (null? '(a b c))))
(assert (null? '()))

(assert (eq? (list-ref '(a b c d) 2) 'c))

(assert (equal? '(a b c d) (append '(a) '(b c d))))   

(assert (equal? '(1 2 3 4 6) (sort '(1 4 2 6 3) <)))

(assert (equal? '(1 1 1 1) (make-list 4 1)))

; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_7.html

; TODO: add characters to reader
(assert (string=? (make-string 10 #\x) "xxxxxxxxxx"))

(assert (string? "Hi"))
(assert (not (string? 'Hi)))

(assert (= (string-length "") 0))
(assert (= (string-length "The length") 10))
(assert (string=? "PIE" "PIE"))
(assert (not (string=? "PIE" "pie")))

(assert (string=? (list->string (string->list "hello")) "hello"))

; https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Symbols.html
(assert (symbol? 'foo))
(assert (symbol? (car '(a b))))
(assert (not (symbol? "bar")))

(assert (string=? "FLYING-FISH" (symbol->string 'flying-fish)))

; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_9.html

(assert (equal? (vector-head #(1 2 3) 2) #(1 2)))


; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_13.html
(assert (= (apply + (list 3 4 5 6)) 18))

; Universl Time https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Universal-Time.html
(assert (integer? (get-universal-time)))

; https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Procedure-Operations.html#Procedure-Operations
(assert (procedure? (lambda (x) x)))
(assert (compound-procedure? (lambda (x) x)))
(assert (not (compiled-procedure? (lambda (x) x))))
(assert (not (procedure? 3)))
(assert (= 18 (apply + (list 3 4 5 6))))
(assert (compiled-procedure? eval))

(assert (= (gcd 32 -36) 4))
(assert (= (gcd 4 3) 1))
(assert (= (gcd) 0))

(assert (= (abs -1) 1))

(assert (equal? '(3 3 3) (map + '(1 1 1) '(2 2 2))))

(assert (equal? '(1 2 3) (map abs '(-1 -2 3))))
(assert (equal? #(1 2 3) (vector-map abs #(-1 -2 3))))

; Numbers
(assert (integer? 3))
(assert (real? 3))

(assert (real? 3.5))
(assert (not (integer? 3.5)))

(assert (< 3 4))
(assert (> 4 3))
(assert (>= 4 3))
(assert (<= 3 4))
(assert (<= 1 1))
(assert (< -5 5))
(assert (not (> 3 4)))

(assert (= (modulo -13 4) 3))
(assert (= (remainder -13 4) -1))

(assert (= (remainder 13 -4) 1))

(assert (even? 2))
(assert (not (odd? 2)))
(assert (odd? 3))
(assert (odd? 7))
(assert (not (odd? 4)))



;https://www.gnu.org/software/mit-scheme/documentation/stable/mit-scheme-ref/Construction-of-Vectors.html

(assert (equal? (vector 'a 'b 'c) #(A B C)))
(assert (equal? (list->vector '(dididit dah)) #(dididit dah)))


(assert (= (vector-binary-search #(1 2 3 4 5) < (lambda (x) x) 3) 3))
(assert (null? (vector-binary-search #(1 2 2 4 5) < (lambda (x) x) 3)))


(assert (equal? (make-initialized-vector 5 (lambda (x) (* x x))) #(0 1 4 9 16)))



