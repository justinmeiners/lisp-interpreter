(assert '())

(==> (cons 'a (cons 'b (cons 'c '()))) (a b c))
(==> (car (cons 1 2)) 1)
(==> (cdr (cons 1 2)) 2)
(==> (car (list 1 2)) 1)
(==> (cdr (list 1 2)) (2))

(define test-pair (cons 1 2))
(set-car! test-pair 3)
(set-cdr! test-pair 4)
(==> test-pair (3 . 4))

(==> (reverse '(a b c)) (c b a))

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

(==> (append '(a) '(b c d))  (a b c d))   
(==> (sort '(1 4 2 6 3) <) (1 2 3 4 6))
(==> (make-list 4 1) (1 1 1 1))

(==> (memq 'a '(a b c)) (a b c))
(==> (memq 'b '(a b c)) (b c))
(==> (memq 'a '(b c d)) #f)
 
(==> (member (list 'a) '(b (a) c)) ((a) c))
(==> (member 'a '(b (a) c)) #f)

(assert (= (apply + (list 3 4 5 6)) 18)) 

(==> (append-reverse! '("y" "x" "w") '("z")) ("w" "x" "y" "z"))

; Association lists
(define list-map '((bob . 1) (john . 2) (dan . 3) (alice . 4)))

(assert (= (cdr (assoc 'john list-map)) 2))
(assert (= (cdr (assoc 'alice list-map)) 4))
(assert (not (assoc 'bad-key list-map)))

(assert (= (cdr (assq 'john list-map)) 2))
(assert (= (cdr (assq 'alice list-map)) 4))
(assert (not (assq 'bad-key list-map)))

(assert (list? '(1 2)))
(assert (not (list? (cons 1 2))))
 
