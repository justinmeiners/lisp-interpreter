
(let ((data (read)))
    (gc-flip)
    (print-gc-statistics)
    (newline)
    (display "records: ")
    (display (length data))
    (newline)
    (let ((record (car data)))
        (assert (= (cdr (vector-assq 'index record)) 0))
        (assert (eq? (cdr (vector-assq 'isActive record)) 'False))
        (assert (= (cdr (vector-assq 'age record)) 21))))
   
(display "done")
(newline)


