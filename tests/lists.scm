(=> (reverse '(a b c)) (c b a))

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

(=> (append '(a) '(b c d))  (a b c d))   
(=> (sort '(1 4 2 6 3) <) (1 2 3 4 6))
(=> (make-list 4 1) (1 1 1 1))

(=> (memq 'a '(a b c)) (a b c))
(=> (memq 'b '(a b c)) (b c))
(=> (memq 'a '(b c d)) #f)
 
