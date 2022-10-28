; try this one with read
(let* ((file (open-input-file "big_data_gen.sexpr"))
       (data (read file)))
    (gc-flip)
    (print-gc-statistics)
    (newline)
    (display "records: ")
    (display (length data))
    (newline)
    (let ((record (car data)))
        (assert (= (cdr (vector-assq 'index record)) 0))
        (assert (eq? (cdr (vector-assq 'isActive record)) 'False))
        (assert (= (cdr (vector-assq 'age record)) 21)))

    (close-input-port file))
   
(display "done")
(newline)


