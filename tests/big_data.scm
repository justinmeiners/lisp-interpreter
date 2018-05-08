
; check that we can load an interact with this file

(let ((data (read-path "big_data_gen.sexpr")))
    (display "records: ")
    (display (length data))
    (newline)
    (let ((record (car data)))
        (assert (= (cdr (vector-assoc 'index record)) 0))
        (assert (eq? (cdr (vector-assoc 'isActive record)) 'False))
        (assert (= (cdr (vector-assoc 'age record)) 21))))
   
(display "done")
(newline)

(let ((data (read-path "big_data_canada.sexpr")))
    (display "records: ")
    (display (vector-length data)))

