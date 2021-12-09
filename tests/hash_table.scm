
(define h (make-hash-table))

(assert (hash-table? h))
(hash-table/put! h 2000 1)

(assert (equal? (list (cons 2000 1)) (hash-table->alist h)))

(hash-table/put! h 2001 2)

(assert (equal? -1 (hash-table/get h 3000 -1)))
(assert (equal? 1 (hash-table/get h 2000 -1)))
(assert (equal? 2 (hash-table/get h 2001 -1)))

(define h2 (alist->hash-table (hash-table->alist h)))

(assert (equal? -1 (hash-table/get h2 3000 -1)))
(assert (equal? 1 (hash-table/get h2 2000 -1)))
(assert (equal? 2 (hash-table/get h2 2001 -1)))


