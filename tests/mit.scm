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

; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_7.html
(assert (string? "Hi"))
(assert (not (string? 'Hi)))

(assert (= (string-length "") 0))
(assert (= (string-length "The length") 10))
(assert (string=? "PIE" "PIE"))
(assert (not (string=? "PIE" "pie")))

; https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Symbols.html
(assert (symbol? 'foo))
(assert (symbol? (car '(a b))))
(assert (not (symbol? "bar")))

(assert (string=? "FLYING-FISH" (symbol->string 'flying-fish)))


; Universl Time https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Universal-Time.html
(assert (integer? (get-universal-time)))

; https://www.gnu.org/software/mit-scheme/documentation/mit-scheme-ref/Procedure-Operations.html#Procedure-Operations
(assert (procedure? (lambda (x) x)))


