
(define h (make-hash-table))

(assert (hash-table? h))
(hash-table-set! h 2000 1)

(assert (equal? (list (cons 2000 1)) (hash-table->alist h)))

(hash-table-set! h 2001 2)

(assert (equal? -1 (hash-table-ref h 3000 -1)))
(assert (equal? 1 (hash-table-ref h 2000 -1)))
(assert (equal? 2 (hash-table-ref h 2001 -1)))

(define h2 (alist->hash-table (hash-table->alist h)))

(assert (equal? -1 (hash-table-ref h2 3000 -1)))
(assert (equal? 1 (hash-table-ref h2 2000 -1)))
(assert (equal? 2 (hash-table-ref h2 2001 -1)))


(define h3 (alist->hash-table '((APPLE . "apple") (PEAR . "pear") (BANANA . "banana"))))


(assert (equal? "apple" (hash-table-ref h3 'APPLE)))
(assert (equal? "pear" (hash-table-ref h3 'PEAR)))
(assert (equal? '() (hash-table-ref h3 'HASH)))


